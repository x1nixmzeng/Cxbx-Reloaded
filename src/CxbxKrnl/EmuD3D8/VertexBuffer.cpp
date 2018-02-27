// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
// ******************************************************************
// *
// *    .,-:::::    .,::      .::::::::.    .,::      .:
// *  ,;;;'````'    `;;;,  .,;;  ;;;'';;'   `;;;,  .,;;
// *  [[[             '[[,,[['   [[[__[[\.    '[[,,[['
// *  $$$              Y$$$P     $$""""Y$$     Y$$$P
// *  `88bo,__,o,    oP"``"Yo,  _88o,,od8P   oP"``"Yo,
// *    "YUMMMMMP",m"       "Mm,""YUMMMP" ,m"       "Mm,
// *
// *   Cxbx->Win32->CxbxKrnl->EmuD3D8->VertexBuffer.cpp
// *
// *  This file is part of the Cxbx project.
// *
// *  Cxbx and Cxbe are free software; you can redistribute them
// *  and/or modify them under the terms of the GNU General Public
// *  License as published by the Free Software Foundation; either
// *  version 2 of the license, or (at your option) any later version.
// *
// *  This program is distributed in the hope that it will be useful,
// *  but WITHOUT ANY WARRANTY; without even the implied warranty of
// *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// *  GNU General Public License for more details.
// *
// *  You should have recieved a copy of the GNU General Public License
// *  along with this program; see the file COPYING.
// *  If not, write to the Free Software Foundation, Inc.,
// *  59 Temple Place - Suite 330, Bostom, MA 02111-1307, USA.
// *
// *  (c) 2002-2004 Aaron Robinson <caustik@caustik.com>
// *                Kingofc <kingofc@freenet.de>
// *
// *  All rights reserved
// *
// ******************************************************************
#define _XBOXKRNL_DEFEXTRN_

#include "CxbxKrnl/VMManager.h"
#include "CxbxKrnl/xxhash32.h" // For XXHash32::hash()
#include "CxbxKrnl/Emu.h"
#include "CxbxKrnl/EmuXTL.h"
#include "CxbxKrnl/ResourceTracker.h"

// TODO: Find somewhere to put this that doesn't conflict with 
extern void EmuUpdateActiveTextureStages();

#include <ctime>

#define HASH_SEED 0

#define VERTEX_BUFFER_CACHE_SIZE 256
#define MAX_STREAM_NOT_USED_TIME (2 * CLOCKS_PER_SEC) // TODO: Trim the not used time

// inline vertex buffer emulation
DWORD                  *g_pIVBVertexBuffer = nullptr;
X_D3DPRIMITIVETYPE      g_IVBPrimitiveType = X_D3DPT_INVALID;
UINT                         g_IVBTblOffs = 0;
struct _D3DIVB         g_IVBTable[IVB_TABLE_SIZE];
extern DWORD                 g_IVBFVF = 0;
extern X_D3DVertexBuffer      *g_pVertexBuffer = NULL;

extern DWORD				g_dwPrimPerFrame = 0;
extern X_D3DVertexBuffer*g_D3DStreams[16];
extern UINT g_D3DStreamStrides[16];
void *GetDataFromXboxResource(X_D3DResource *pXboxResource);
extern IDirect3DVertexBuffer8 *GetHostVertexBuffer(X_D3DResource *pXboxResource, DWORD dwSize);

VertexPatcher::VertexPatcher()
{
    this->m_uiNbrStreams = 0;
    ZeroMemory(this->m_pStreams, sizeof(PATCHEDSTREAM) * MAX_NBR_STREAMS);
    this->m_bPatched = false;
    this->m_bAllocatedStreamZeroData = false;
    this->m_pNewVertexStreamZeroData = NULL;
    this->m_pDynamicPatch = NULL;
}

VertexPatcher::~VertexPatcher()
{
}

void VertexPatcher::DumpCache(void)
{
    printf("--- Dumping streams cache ---\n");
    RTNode *pNode = g_PatchedStreamsCache.getHead();
    while(pNode)
    {
        CACHEDSTREAM *pCachedStream = (CACHEDSTREAM *)pNode->pResource;
        if(pCachedStream)
        {
            // TODO: Write nicer dump presentation
            printf("Key: 0x%.08X Cache Hits: %d IsUP: %s OrigStride: %d NewStride: %d HashCount: %d HashFreq: %d Length: %d Hash: 0x%.08X\n",
                   pNode->uiKey, pCachedStream->uiCacheHit, pCachedStream->bIsUP ? "YES" : "NO",
                   pCachedStream->Stream.uiOrigStride, pCachedStream->Stream.uiNewStride,
                   pCachedStream->uiCount, pCachedStream->uiCheckFrequency,
                   pCachedStream->uiLength, pCachedStream->uiHash);
        }

        pNode = pNode->pNext;
    }
}

size_t GetVertexBufferSize(DWORD dwVertexCount, DWORD dwStride, PWORD pIndexData, DWORD dwOffset)
{	
	// If we are drawing from an offset, we know that the vertex count must have offset vertices
	// before the first drawn vertices
	dwVertexCount += dwOffset;

	// If this is not an indexed draw, the size is simply VertexCount * Stride
	if (pIndexData == nullptr) {
		return dwVertexCount * dwStride;
	}

	// We are an indexed draw, so we have to parse the index buffer
	// The highest index we see can be used to determine the vertex buffer size
	DWORD highestVertexIndex = 0;
	for (DWORD i = 0; i < dwVertexCount; i++) {
		if (pIndexData[i] > highestVertexIndex) {
			highestVertexIndex = pIndexData[i];
		}
	}

	return (highestVertexIndex + 1) * dwStride;		
}

void VertexPatcher::CacheStream(VertexPatchDesc *pPatchDesc,
                                     UINT             uiStream,
									 uint32_t	      uiHash)
{
    UINT                       uiStride;
    X_D3DVertexBuffer    *pOrigVertexBuffer = nullptr;
    void                      *pCalculateData = NULL;
    uint32                     uiKey;
    UINT                       uiLength;
    CACHEDSTREAM              *pCachedStream = (CACHEDSTREAM *)calloc(1, sizeof(CACHEDSTREAM));

    // Check if the cache is full, if so, throw away the least used stream
    if(g_PatchedStreamsCache.get_count() > VERTEX_BUFFER_CACHE_SIZE)
    {
        uint32 uiKey = 0;
        uint32 uiMinHit = 0xFFFFFFFF;

        RTNode *pNode = g_PatchedStreamsCache.getHead();
        while(pNode)
        {
            if(pNode->pResource)
            {
                // First, check if there is an "expired" stream in the cache (not recently used)
                if(((CACHEDSTREAM *)pNode->pResource)->lLastUsed < (clock() + MAX_STREAM_NOT_USED_TIME))
                {
                    printf("!!!Found an old stream, %2.2f\n", ((FLOAT)((clock() + MAX_STREAM_NOT_USED_TIME) - ((CACHEDSTREAM *)pNode->pResource)->lLastUsed)) / (FLOAT)CLOCKS_PER_SEC);
                    uiKey = pNode->uiKey;
                    break;
                }
                // Find the least used cached stream
                if((uint32)((CACHEDSTREAM *)pNode->pResource)->uiCacheHit < uiMinHit)
                {
                    uiMinHit = ((CACHEDSTREAM *)pNode->pResource)->uiCacheHit;
                    uiKey = pNode->uiKey;
                }
            }
            pNode = pNode->pNext;
        }
        if(uiKey != 0)
        {
            printf("!!!Removing stream\n\n");
            FreeCachedStream((void*)uiKey);
        }
    }

    // Start the actual stream caching
    if(!pPatchDesc->pVertexStreamZeroData)
    {
        pOrigVertexBuffer = m_pStreams[uiStream].pOriginalStream;
        m_pStreams[uiStream].pPatchedStream->AddRef();

		pCalculateData = (void*)GetDataFromXboxResource(pOrigVertexBuffer);

		uiLength = GetVertexBufferSize(pPatchDesc->dwVertexCount, g_D3DStreamStrides[uiStream], pPatchDesc->pIndexData, pPatchDesc->dwOffset);
        pCachedStream->bIsUP = false;
        uiKey = (uint32)pOrigVertexBuffer;
    }
    else
    {
        // There should only be one stream (stream zero) in this case
        if(uiStream != 0)
        {
            CxbxKrnlCleanup("Trying to patch a Draw..UP with more than stream zero!");
        }
        uiStride  = pPatchDesc->uiVertexStreamZeroStride;
        pCalculateData = (uint08 *)pPatchDesc->pVertexStreamZeroData;
        uiLength = GetVertexBufferSize(pPatchDesc->dwVertexCount, uiStride, pPatchDesc->pIndexData, pPatchDesc->dwOffset);
        pCachedStream->bIsUP = true;
        pCachedStream->pStreamUP = pCalculateData;
        uiKey = (uint32)pCalculateData;
    }

	// If we weren't given a known hash, calculate a new one
	if (uiHash == 0) {
		uiHash = XXHash32::hash((void *)pCalculateData, uiLength, HASH_SEED);
	}
    
    pCachedStream->uiHash = uiHash;
    pCachedStream->Stream = m_pStreams[uiStream];
	pCachedStream->uiCheckFrequency = 1; // Start with checking every 1th Draw..
    pCachedStream->uiCount = 0;
    pCachedStream->uiLength = uiLength;
    pCachedStream->uiCacheHit = 0;
    pCachedStream->dwPrimitiveCount = pPatchDesc->dwPrimitiveCount;
    pCachedStream->lLastUsed = clock();
    g_PatchedStreamsCache.insert(uiKey, pCachedStream);
}

void VertexPatcher::FreeCachedStream(void *pStream)
{
    g_PatchedStreamsCache.Lock();
    CACHEDSTREAM *pCachedStream = (CACHEDSTREAM *)g_PatchedStreamsCache.get(pStream);
    if(pCachedStream)
    {
        if(pCachedStream->bIsUP && pCachedStream->pStreamUP)
        {
            free(pCachedStream->pStreamUP);
        }

        if(pCachedStream->Stream.pPatchedStream)
        {
            pCachedStream->Stream.pPatchedStream->Release();
        }
        free(pCachedStream);
    }
    g_PatchedStreamsCache.Unlock();
    g_PatchedStreamsCache.remove(pStream);
}

bool VertexPatcher::ApplyCachedStream(VertexPatchDesc *pPatchDesc,
                                           UINT             uiStream,
										   bool			   *pbFatalError,
										   uint32_t		   *pHash)
{
    UINT                       uiStride;
    X_D3DVertexBuffer    *pOrigVertexBuffer = nullptr;
    void                      *pCalculateData = NULL;
    UINT                       uiLength;
    bool                       bApplied = false;
    uint32                     uiKey;
    //CACHEDSTREAM              *pCachedStream = (CACHEDSTREAM *)malloc(sizeof(CACHEDSTREAM));

    if(!pPatchDesc->pVertexStreamZeroData)
    {
		pOrigVertexBuffer = g_D3DStreams[uiStream];
		uiStride = g_D3DStreamStrides[uiStream];
        uiLength = GetVertexBufferSize(pPatchDesc->dwVertexCount, uiStride, pPatchDesc->pIndexData, pPatchDesc->dwOffset);
        uiKey = (uint32)pOrigVertexBuffer;
        //pCachedStream->bIsUP = false;
    }
    else
    {
        // There should only be one stream (stream zero) in this case
        if(uiStream != 0)
        {
            CxbxKrnlCleanup("Trying to find a cached Draw..UP with more than stream zero!");
        }

        uiStride  = pPatchDesc->uiVertexStreamZeroStride;
        pCalculateData = (uint08 *)pPatchDesc->pVertexStreamZeroData;
        uiLength = GetVertexBufferSize(pPatchDesc->dwVertexCount, uiStride, pPatchDesc->pIndexData, pPatchDesc->dwOffset);
        uiKey = (uint32)pCalculateData;
        //pCachedStream->bIsUP = true;
        //pCachedStream->pStreamUP = pCalculateData;
    }
    g_PatchedStreamsCache.Lock();

    CACHEDSTREAM *pCachedStream = (CACHEDSTREAM *)g_PatchedStreamsCache.get(uiKey);
    if(pCachedStream)
    {
        pCachedStream->lLastUsed = clock();
        pCachedStream->uiCacheHit++;
        bool bMismatch = false;
        if(pCachedStream->uiCount == (pCachedStream->uiCheckFrequency - 1))
        {
            if(pOrigVertexBuffer) {
				pCalculateData = (void*)GetDataFromXboxResource(pOrigVertexBuffer);
            }

            uint32_t uiHash = XXHash32::hash((void *)pCalculateData, uiLength, HASH_SEED);
			*pHash = uiHash;
			
            if(uiHash == pCachedStream->uiHash)
            {
                // Take a while longer to check
                if(pCachedStream->uiCheckFrequency < 32*1024)
                {
                    pCachedStream->uiCheckFrequency *= 2;
                }
                pCachedStream->uiCount = 0;
            }
            else
            {
                // TODO: Do something about this
                if(pCachedStream->bIsUP)
                {
                    FreeCachedStream(pCachedStream->pStreamUP);
                }
                else
                {
                    FreeCachedStream(pCachedStream->Stream.pOriginalStream);
                }
                pCachedStream = NULL;
                bMismatch = true;
            }
        }
        else
        {
            pCachedStream->uiCount++;
        }

        if(!bMismatch)
        {
            if(!pCachedStream->bIsUP)
            {
                m_pStreams[uiStream].pOriginalStream = pOrigVertexBuffer;
                m_pStreams[uiStream].uiOrigStride = uiStride;
                g_pD3DDevice8->SetStreamSource(uiStream, pCachedStream->Stream.pPatchedStream, pCachedStream->Stream.uiNewStride);
                pCachedStream->Stream.pPatchedStream->AddRef();
                m_pStreams[uiStream].pPatchedStream = pCachedStream->Stream.pPatchedStream;
                m_pStreams[uiStream].uiNewStride = pCachedStream->Stream.uiNewStride;
            }
            else
            {
                pPatchDesc->pVertexStreamZeroData = pCachedStream->pStreamUP;
                pPatchDesc->uiVertexStreamZeroStride = pCachedStream->Stream.uiNewStride;
            }

            if(pCachedStream->dwPrimitiveCount)
            {
                // The primitives were patched, draw with the correct number of primimtives from the cache
                pPatchDesc->dwPrimitiveCount = pCachedStream->dwPrimitiveCount;
            }

            bApplied = true;
            m_bPatched = true;
        }
    }

    g_PatchedStreamsCache.Unlock();

    return bApplied;
}

int CountActiveD3DStreams()
{
	int lastStreamIndex;
	for (int i = 0; i < 16; i++) {
		if (g_D3DStreams[i] != nullptr) {
			lastStreamIndex = i;
		}
	}

	return lastStreamIndex + 1;
}

UINT VertexPatcher::GetNbrStreams(VertexPatchDesc *pPatchDesc)
{
	// Draw..Up always have one stream
	if (pPatchDesc->pVertexStreamZeroData != nullptr) {
		return 1;
	}

    if(VshHandleIsVertexShader(pPatchDesc->hVertexShader)) {
        VERTEX_DYNAMIC_PATCH *pDynamicPatch = VshGetVertexDynamicPatch(pPatchDesc->hVertexShader);
		if (pDynamicPatch) {
			return pDynamicPatch->NbrStreams;
		}

		return CountActiveD3DStreams();
    } 
	
	if (pPatchDesc->hVertexShader) {
		return CountActiveD3DStreams();
    }

    return 0;
}

bool VertexPatcher::PatchStream(VertexPatchDesc *pPatchDesc,
                                     UINT             uiStream)
{
    // FVF buffers doesn't have Xbox extensions, but texture coordinates may
    // need normalization if used with linear textures.
    if(!VshHandleIsVertexShader(pPatchDesc->hVertexShader))
    {
        if(pPatchDesc->hVertexShader & D3DFVF_TEXCOUNT_MASK)
        {
            return NormalizeTexCoords(pPatchDesc, uiStream);
        }
        else
        {
            return false;
        }
    }

    if(!m_pDynamicPatch || m_pDynamicPatch->NbrStreams <= uiStream || !m_pDynamicPatch->pStreamPatches[uiStream].NeedPatch)
    {
        return false;
    }

    // Do some groovy patchin'
    
    X_D3DVertexBuffer    *pOrigVertexBuffer;
    IDirect3DVertexBuffer8    *pNewVertexBuffer;
    uint08                    *pOrigData;
    uint08                    *pNewData;
	UINT                       uiVertexCount;
    UINT                       uiStride;
	UINT					   uiLength;
    PATCHEDSTREAM             *pStream = &m_pStreams[uiStream];
    STREAM_DYNAMIC_PATCH      *pStreamPatch = &m_pDynamicPatch->pStreamPatches[uiStream];
    DWORD dwNewSize;

    if(!pPatchDesc->pVertexStreamZeroData)
    {
		pOrigVertexBuffer = g_D3DStreams[uiStream];
		uiStride = g_D3DStreamStrides[uiStream];
		uiLength = GetVertexBufferSize(pPatchDesc->dwVertexCount, uiStride, pPatchDesc->pIndexData, pPatchDesc->dwOffset);

        // Set a new (exact) vertex count
		uiVertexCount = uiLength / uiStride;

		// Dxbx addition : Don't update pPatchDesc.dwVertexCount because an indexed draw
		// can (and will) use less vertices than the supplied nr of indexes. Thix fixes
		// the missing parts in the CompressedVertices sample (in Vertex shader mode).
		pStreamPatch->ConvertedStride = max(pStreamPatch->ConvertedStride, uiStride); // ??
		dwNewSize = uiVertexCount * pStreamPatch->ConvertedStride;

        pOrigData = (uint08*)GetDataFromXboxResource(pOrigVertexBuffer);
        g_pD3DDevice8->CreateVertexBuffer(dwNewSize, 0, 0, D3DPOOL_MANAGED, &pNewVertexBuffer);
        if(FAILED(pNewVertexBuffer->Lock(0, 0, &pNewData, 0)))
        {
            CxbxKrnlCleanup("Couldn't lock the new buffer");
        }
        if(!pStream->pOriginalStream)
        {
            // The stream was not previously patched, we'll need this when restoring
            pStream->pOriginalStream = pOrigVertexBuffer;
        }
    }
    else
    {
        // There should only be one stream (stream zero) in this case
        if(uiStream != 0)
        {
            CxbxKrnlCleanup("Trying to patch a Draw..UP with more than stream zero!");
        }
        uiStride  = pPatchDesc->uiVertexStreamZeroStride;
		pStreamPatch->ConvertedStride = max(pStreamPatch->ConvertedStride, uiStride); // ??
		pOrigData = (uint08 *)pPatchDesc->pVertexStreamZeroData;
		uiLength = GetVertexBufferSize(pPatchDesc->dwVertexCount, uiStride, pPatchDesc->pIndexData, pPatchDesc->dwOffset);
		uiVertexCount = uiLength / uiStride;
		dwNewSize = uiVertexCount * pStreamPatch->ConvertedStride;
        pNewVertexBuffer = NULL;
        pNewData = (uint08*)g_VMManager.Allocate(dwNewSize);
        if(!pNewData)
        {
            CxbxKrnlCleanup("Couldn't allocate the new stream zero buffer");
        }
    }

	for (uint32 uiVertex = 0; uiVertex < uiVertexCount; uiVertex++)
	{
		uint08 *pOrigVertex = &pOrigData[uiVertex * uiStride];
		uint08 *pNewDataPos = &pNewData[uiVertex * pStreamPatch->ConvertedStride];
		for (UINT uiType = 0; uiType < pStreamPatch->NbrTypes; uiType++)
		{
			// Dxbx note : The following code handles only the D3DVSDT enums that need conversion;
			// All other cases are catched by the memcpy in the default-block.
			switch (pStreamPatch->pTypes[uiType])
			{
			case X_D3DVSDT_NORMPACKED3: { // 0x16: // Make it FLOAT3
				// Hit by Dashboard
				int32 iPacked = ((int32 *)pOrigVertex)[0];
				// Cxbx note : to make each component signed, two need to be shifted towards the sign-bit first :
				((FLOAT *)pNewDataPos)[0] = ((FLOAT)((iPacked << 21) >> 21)) / 1023.0f;
				((FLOAT *)pNewDataPos)[1] = ((FLOAT)((iPacked << 10) >> 21)) / 1023.0f;
				((FLOAT *)pNewDataPos)[2] = ((FLOAT)((iPacked      ) >> 22)) / 511.0f;
				pOrigVertex += 1 * sizeof(int32);
				break;
			}
			case X_D3DVSDT_SHORT1: { // 0x15: // Make it SHORT2 and set the second short to 0
				((SHORT *)pNewDataPos)[0] = ((SHORT*)pOrigVertex)[0];
				((SHORT *)pNewDataPos)[1] = 0x00;
				pOrigVertex += 1 * sizeof(SHORT);
				break;
			}
			case X_D3DVSDT_SHORT3: { // 0x35: // Make it a SHORT4 and set the fourth short to 1
				// Hit by Turok
				memcpy(pNewDataPos, pOrigVertex, 3 * sizeof(SHORT));
				((SHORT *)pNewDataPos)[3] = 0x01;
				pOrigVertex += 3 * sizeof(SHORT);
				break;
			}
			case X_D3DVSDT_PBYTE1: { // 0x14:  // Make it FLOAT1
				((FLOAT *)pNewDataPos)[0] = ((FLOAT)((BYTE*)pOrigVertex)[0]) / 255.0f;
				pOrigVertex += 1 * sizeof(BYTE);
				break;
			}
			case X_D3DVSDT_PBYTE2: { // 0x24:  // Make it FLOAT2
				((FLOAT *)pNewDataPos)[0] = ((FLOAT)((BYTE*)pOrigVertex)[0]) / 255.0f;
				((FLOAT *)pNewDataPos)[1] = ((FLOAT)((BYTE*)pOrigVertex)[1]) / 255.0f;
				pOrigVertex += 2 * sizeof(BYTE);
				break;
			}
			case X_D3DVSDT_PBYTE3: { // 0x34: // Make it FLOAT3
				// Hit by Turok
				((FLOAT *)pNewDataPos)[0] = ((FLOAT)((BYTE*)pOrigVertex)[0]) / 255.0f;
				((FLOAT *)pNewDataPos)[1] = ((FLOAT)((BYTE*)pOrigVertex)[1]) / 255.0f;
				((FLOAT *)pNewDataPos)[2] = ((FLOAT)((BYTE*)pOrigVertex)[2]) / 255.0f;
				pOrigVertex += 3 * sizeof(BYTE);
				break;
			}
			case X_D3DVSDT_PBYTE4: { // 0x44: // Make it FLOAT4
				// Hit by Jet Set Radio Future
				((FLOAT *)pNewDataPos)[0] = ((FLOAT)((BYTE*)pOrigVertex)[0]) / 255.0f;
				((FLOAT *)pNewDataPos)[1] = ((FLOAT)((BYTE*)pOrigVertex)[1]) / 255.0f;
				((FLOAT *)pNewDataPos)[2] = ((FLOAT)((BYTE*)pOrigVertex)[2]) / 255.0f;
				((FLOAT *)pNewDataPos)[3] = ((FLOAT)((BYTE*)pOrigVertex)[3]) / 255.0f;
				pOrigVertex += 4 * sizeof(BYTE);
				break;
			}
			case X_D3DVSDT_NORMSHORT1: { // 0x11: // Make it FLOAT1
				// UNTESTED - Need test-case!
				((FLOAT *)pNewDataPos)[0] = ((FLOAT)((SHORT*)pOrigVertex)[0]) / 32767.0f;
				//((FLOAT *)pNewDataPos)[1] = 0.0f; // Would be needed for FLOAT2
				pOrigVertex += 1 * sizeof(SHORT);
				break;
			}
#if !DXBX_USE_D3D9 // No need for patching in D3D9
			case X_D3DVSDT_NORMSHORT2: { // 0x21: // Make it FLOAT2
				// UNTESTED - Need test-case!
				((FLOAT *)pNewDataPos)[0] = ((FLOAT)((SHORT*)pOrigVertex)[0]) / 32767.0f;
				((FLOAT *)pNewDataPos)[1] = ((FLOAT)((SHORT*)pOrigVertex)[1]) / 32767.0f;
				pOrigVertex += 2 * sizeof(SHORT);
				break;
			}
#endif
			case X_D3DVSDT_NORMSHORT3: { // 0x31: // Make it FLOAT3
				// UNTESTED - Need test-case!
				((FLOAT *)pNewDataPos)[0] = ((FLOAT)((SHORT*)pOrigVertex)[0]) / 32767.0f;
				((FLOAT *)pNewDataPos)[1] = ((FLOAT)((SHORT*)pOrigVertex)[1]) / 32767.0f;
				((FLOAT *)pNewDataPos)[2] = ((FLOAT)((SHORT*)pOrigVertex)[2]) / 32767.0f;
				pOrigVertex += 3 * sizeof(SHORT);
				break;
			}
#if !DXBX_USE_D3D9 // No need for patching in D3D9
			case X_D3DVSDT_NORMSHORT4: { // 0x41: // Make it FLOAT4
				// UNTESTED - Need test-case!
				((FLOAT *)pNewDataPos)[0] = ((FLOAT)((SHORT*)pOrigVertex)[0]) / 32767.0f;
				((FLOAT *)pNewDataPos)[1] = ((FLOAT)((SHORT*)pOrigVertex)[1]) / 32767.0f;
				((FLOAT *)pNewDataPos)[2] = ((FLOAT)((SHORT*)pOrigVertex)[2]) / 32767.0f;
				((FLOAT *)pNewDataPos)[3] = ((FLOAT)((SHORT*)pOrigVertex)[3]) / 32767.0f;
				pOrigVertex += 4 * sizeof(SHORT);
				break;
			}
#endif
			case X_D3DVSDT_FLOAT2H: { // 0x72: // Make it FLOAT4 and set the third float to 0.0
				((FLOAT *)pNewDataPos)[0] = ((FLOAT*)pOrigVertex)[0];
				((FLOAT *)pNewDataPos)[1] = ((FLOAT*)pOrigVertex)[1];
				((FLOAT *)pNewDataPos)[2] = 0.0f;
				((FLOAT *)pNewDataPos)[3] = ((FLOAT*)pOrigVertex)[2];
				pOrigVertex += 3 * sizeof(FLOAT);
				break;
			}
			/*TODO
			case X_D3DVSDT_NONE: { // 0x02:
				printf("D3DVSDT_NONE / xbox ext. nsp /");
				dwNewDataType = 0xFF;
				break;
			}
			*/
			default: {
				// Generic 'conversion' - just make a copy :
				memcpy(pNewDataPos, pOrigVertex, pStreamPatch->pSizes[uiType]);
				pOrigVertex += pStreamPatch->pSizes[uiType];
				break;
			}
			} // switch

			// Increment the new pointer :
			pNewDataPos += pStreamPatch->pSizes[uiType];
		}
	}

    if(!pPatchDesc->pVertexStreamZeroData)
    {
        //if(pNewVertexBuffer != nullptr) // Dxbx addition
			pNewVertexBuffer->Unlock();

        if(FAILED(g_pD3DDevice8->SetStreamSource(uiStream, pNewVertexBuffer, pStreamPatch->ConvertedStride)))
        {
            CxbxKrnlCleanup("Failed to set the type patched buffer as the new stream source!\n");
        }
        if(pStream->pPatchedStream)
        {
            // The stream was already primitive patched, release the previous vertex buffer to avoid memory leaks
            pStream->pPatchedStream->Release();
        }
        pStream->pPatchedStream = pNewVertexBuffer;
    }
    else
    {
        pPatchDesc->pVertexStreamZeroData = pNewData;
        pPatchDesc->uiVertexStreamZeroStride = pStreamPatch->ConvertedStride;
        if(!m_bAllocatedStreamZeroData)
        {
            // The stream was not previously patched. We'll need this when restoring
            m_bAllocatedStreamZeroData = true;
            m_pNewVertexStreamZeroData = pNewData;
        }
    }

    pStream->uiOrigStride = uiStride;
    pStream->uiNewStride = pStreamPatch->ConvertedStride;
    m_bPatched = true;

    return true;
}

bool VertexPatcher::NormalizeTexCoords(VertexPatchDesc *pPatchDesc, UINT uiStream)
{
    // Check for active linear textures.
	bool bHasLinearTex = false, bTexIsLinear[4] = { false };
	X_D3DBaseTexture *pLinearBaseTexture[4];

    for(uint08 i = 0; i < 4; i++)
    {
		X_D3DBaseTexture *pBaseTexture = EmuD3DActiveTexture[i];
		if (pBaseTexture)
		{ 
			X_D3DFORMAT XBFormat = (X_D3DFORMAT)((pBaseTexture->Format & X_D3DFORMAT_FORMAT_MASK) >> X_D3DFORMAT_FORMAT_SHIFT);
			if (EmuXBFormatIsLinear(XBFormat))
			{
				bHasLinearTex = bTexIsLinear[i] = true;
				pLinearBaseTexture[i] = pBaseTexture;
			}
        }
    }

    if(!bHasLinearTex)
        return false;

    X_D3DVertexBuffer *pOrigVertexBuffer;
    IDirect3DVertexBuffer8 *pNewVertexBuffer;
    PATCHEDSTREAM *pStream;
    uint08 *pData, *pUVData;
    uint uiStride, uiVertexCount;

    if(pPatchDesc->pVertexStreamZeroData)
    {
        // In-place patching of inline buffer.
        pNewVertexBuffer = 0;
        pData = (uint08 *)pPatchDesc->pVertexStreamZeroData;
        uiStride = pPatchDesc->uiVertexStreamZeroStride;
		DWORD uiLength = GetVertexBufferSize(pPatchDesc->dwVertexCount, uiStride, pPatchDesc->pIndexData, pPatchDesc->dwOffset);
        uiVertexCount = uiLength / uiStride;
    }
    else
    {
        // Copy stream for patching and caching.
		pOrigVertexBuffer = g_D3DStreams[uiStream];
		uiStride = g_D3DStreamStrides[uiStream];
		UINT uiLength = GetVertexBufferSize(pPatchDesc->dwVertexCount, uiStride, pPatchDesc->pIndexData, pPatchDesc->dwOffset);

		uiVertexCount = uiLength / uiStride;

		uint08 *pOrigData = (uint08*)GetDataFromXboxResource(pOrigVertexBuffer);

        g_pD3DDevice8->CreateVertexBuffer(uiLength, 0, 0, D3DPOOL_MANAGED, &pNewVertexBuffer);
        if(FAILED(pNewVertexBuffer->Lock(0, 0, &pData, 0)))
        {
            CxbxKrnlCleanup("Couldn't lock new FVF buffer.");
        }
        memcpy(pData, pOrigData, uiLength);

        pStream = &m_pStreams[uiStream];
        if(!pStream->pOriginalStream)
        {
            pStream->pOriginalStream = pOrigVertexBuffer;
        }
    }

    // Locate texture coordinate offset in vertex structure.
    uint uiOffset = 0;
    if(pPatchDesc->hVertexShader & D3DFVF_XYZRHW)
        uiOffset += (sizeof(FLOAT) * 4);
    else
    {
        if(pPatchDesc->hVertexShader & D3DFVF_XYZ)
            uiOffset += (sizeof(FLOAT) * 3 );
        else if(pPatchDesc->hVertexShader & D3DFVF_XYZB1)
            uiOffset += (sizeof(FLOAT) *4 );
        else if(pPatchDesc->hVertexShader & D3DFVF_XYZB2)
            uiOffset += (sizeof(FLOAT) * 5);
        else if(pPatchDesc->hVertexShader & D3DFVF_XYZB3)
            uiOffset += (sizeof(FLOAT) * 6);
        else if(pPatchDesc->hVertexShader & D3DFVF_XYZB4)
            uiOffset += (sizeof(FLOAT) * 7);

        if(pPatchDesc->hVertexShader & D3DFVF_NORMAL)
            uiOffset += (sizeof(FLOAT) * 3);
    }

    if(pPatchDesc->hVertexShader & D3DFVF_DIFFUSE)
        uiOffset += sizeof(DWORD);
    if(pPatchDesc->hVertexShader & D3DFVF_SPECULAR)
        uiOffset += sizeof(DWORD);

    DWORD dwTexN = (pPatchDesc->hVertexShader & D3DFVF_TEXCOUNT_MASK) >> D3DFVF_TEXCOUNT_SHIFT;

    // Normalize texture coordinates.
    for(uint32 uiVertex = 0; uiVertex < uiVertexCount; uiVertex++)
    {
        pUVData = pData + (uiVertex * uiStride) + uiOffset;

        if(dwTexN >= 1)
        {
            if(bTexIsLinear[0])
            {
                ((FLOAT*)pUVData)[0] /= ( pLinearBaseTexture[0]->Size & X_D3DSIZE_WIDTH_MASK) + 1;
                ((FLOAT*)pUVData)[1] /= ((pLinearBaseTexture[0]->Size & X_D3DSIZE_HEIGHT_MASK) >> X_D3DSIZE_HEIGHT_SHIFT) + 1;
            }
            pUVData += sizeof(FLOAT) * 2;
        }

        if(dwTexN >= 2)
        {
            if(bTexIsLinear[1])
            {
                ((FLOAT*)pUVData)[0] /= ( pLinearBaseTexture[1]->Size & X_D3DSIZE_WIDTH_MASK) + 1;
                ((FLOAT*)pUVData)[1] /= ((pLinearBaseTexture[1]->Size & X_D3DSIZE_HEIGHT_MASK) >> X_D3DSIZE_HEIGHT_SHIFT) + 1;
            }
            pUVData += sizeof(FLOAT) * 2;
        }

        if(dwTexN >= 3)
        {
            if(bTexIsLinear[2])
            {
                ((FLOAT*)pUVData)[0] /= ( pLinearBaseTexture[2]->Size & X_D3DSIZE_WIDTH_MASK) + 1;
                ((FLOAT*)pUVData)[1] /= ((pLinearBaseTexture[2]->Size & X_D3DSIZE_HEIGHT_MASK) >> X_D3DSIZE_HEIGHT_SHIFT) + 1;
            }
            pUVData += sizeof(FLOAT) * 2;
        }

        if((dwTexN >= 4) && bTexIsLinear[3])
        {
            ((FLOAT*)pUVData)[0] /= ( pLinearBaseTexture[3]->Size & X_D3DSIZE_WIDTH_MASK) + 1;
            ((FLOAT*)pUVData)[1] /= ((pLinearBaseTexture[3]->Size & X_D3DSIZE_HEIGHT_MASK) >> X_D3DSIZE_HEIGHT_SHIFT) + 1;
        }
    }

    if(pNewVertexBuffer)
    {
        pNewVertexBuffer->Unlock();

        if(FAILED(g_pD3DDevice8->SetStreamSource(uiStream, pNewVertexBuffer, uiStride)))
        {
            CxbxKrnlCleanup("Failed to set the texcoord patched FVF buffer as the new stream source.");
        }
        if(pStream->pPatchedStream)
        {
            pStream->pPatchedStream->Release();
        }

        pStream->pPatchedStream = pNewVertexBuffer;
        pStream->uiOrigStride = uiStride;
        pStream->uiNewStride = uiStride;
        m_bPatched = true;
    }

    return m_bPatched;
}

bool VertexPatcher::PatchPrimitive(VertexPatchDesc *pPatchDesc,
                                        UINT             uiStream)
{
    PATCHEDSTREAM *pStream = &m_pStreams[uiStream];

    if((pPatchDesc->PrimitiveType < X_D3DPT_POINTLIST) || (pPatchDesc->PrimitiveType >= X_D3DPT_MAX))
    {
        EmuWarning("Unknown primitive type: 0x%.02X\n", pPatchDesc->PrimitiveType);
		return false;
    }

	if (pPatchDesc->pVertexStreamZeroData == nullptr)
	{
		pStream->pOriginalStream = g_D3DStreams[uiStream];
		pStream->uiOrigStride = g_D3DStreamStrides[uiStream];
		pStream->uiNewStride = pStream->uiOrigStride; // The stride is still the same
	}
	else
	{
		pStream->uiOrigStride = pPatchDesc->uiVertexStreamZeroStride;
	}

	DWORD uiLength = GetVertexBufferSize(pPatchDesc->dwVertexCount, pStream->uiOrigStride, pPatchDesc->pIndexData, pPatchDesc->dwOffset);
	DWORD uiVertexCount = uiLength / pStream->uiOrigStride;

    // Unsupported primitives that don't need deep patching.
    switch(pPatchDesc->PrimitiveType)
    {
        // Quad strip is just like a triangle strip, but requires two
        // vertices per primitive.
        case X_D3DPT_QUADSTRIP:
            pPatchDesc->dwVertexCount -= pPatchDesc->dwVertexCount % 2;
            pPatchDesc->PrimitiveType = X_D3DPT_TRIANGLESTRIP;
            break;

        // Convex polygon is the same as a triangle fan.
        case X_D3DPT_POLYGON:
            pPatchDesc->PrimitiveType = X_D3DPT_TRIANGLEFAN;
            break;
    }

	// Get render primitive count
    pPatchDesc->dwPrimitiveCount = EmuD3DVertex2PrimitiveCount(pPatchDesc->PrimitiveType, pPatchDesc->dwVertexCount);

	// Get total primitive count
	DWORD dwTotalPrimitiveCount = EmuD3DVertex2PrimitiveCount(pPatchDesc->PrimitiveType, uiVertexCount);

    // Skip primitives that don't need further patching.
    switch(pPatchDesc->PrimitiveType)
    {
        case X_D3DPT_QUADLIST:
			//EmuWarning("VertexPatcher::PatchPrimitive: Processing D3DPT_QUADLIST");
			break;
        case X_D3DPT_LINELOOP:
			//EmuWarning("VertexPatcher::PatchPrimitive: Processing D3DPT_LINELOOP");
            break;

        default:
            return false;
    }

    if(pPatchDesc->pVertexStreamZeroData && uiStream > 0)
    {
        CxbxKrnlCleanup("Draw..UP call with more than one stream!\n");
    }

    // sizes of our part in the vertex buffer
    DWORD dwOriginalSize    = 0;
    DWORD dwNewSize         = 0;

    // sizes with the rest of the buffer
    DWORD dwOriginalSizeWR  = 0;
    DWORD dwNewSizeWR       = 0;

    // vertex data arrays
    BYTE *pOrigVertexData = nullptr;
    BYTE *pPatchedVertexData = nullptr;

    // Quad list
    if(pPatchDesc->PrimitiveType == X_D3DPT_QUADLIST)
    {
        pPatchDesc->dwPrimitiveCount *= 2;
		dwTotalPrimitiveCount *= 2;

        // This is a list of sqares/rectangles, so we convert it to a list of triangles
        dwOriginalSize  = dwTotalPrimitiveCount * pStream->uiOrigStride * 2;
        dwNewSize       = dwTotalPrimitiveCount * pStream->uiOrigStride * 3;
    }
    // Line loop
    else if(pPatchDesc->PrimitiveType == X_D3DPT_LINELOOP)
    {
        pPatchDesc->dwPrimitiveCount += 1;
		dwTotalPrimitiveCount += 1;

        // We will add exactly one more line
        dwOriginalSize  = dwTotalPrimitiveCount * pStream->uiOrigStride;
        dwNewSize       = dwTotalPrimitiveCount * pStream->uiOrigStride + pStream->uiOrigStride;
    }

    if(pPatchDesc->pVertexStreamZeroData == nullptr)
    {
        // Retrieve the original buffer size
        {
            // Here we save the full buffer size
			dwOriginalSizeWR = GetVertexBufferSize(pPatchDesc->dwVertexCount, pStream->uiOrigStride, pPatchDesc->pIndexData, pPatchDesc->dwOffset);

            // So we can now calculate the size of the rest (dwOriginalSizeWR - dwOriginalSize) and
            // add it to our new calculated size of the patched buffer
            dwNewSizeWR = dwNewSize + dwOriginalSizeWR - dwOriginalSize;
        }

        HRESULT hRet = g_pD3DDevice8->CreateVertexBuffer(dwNewSizeWR, 0, 0, D3DPOOL_MANAGED, &pStream->pPatchedStream);
		if (FAILED(hRet)) {
			EmuWarning("CreateVertexBuffer Failed. Size: %d", dwNewSizeWR);
		}
		

        if(pStream->pOriginalStream != 0)
        {
			pOrigVertexData = (BYTE*)GetDataFromXboxResource(pStream->pOriginalStream);
        }

        if(pStream->pPatchedStream != 0)
        {
            pStream->pPatchedStream->Lock(0, 0, &pPatchedVertexData, 0);
        }
    }
    else
    {
        dwOriginalSizeWR = dwOriginalSize;
        dwNewSizeWR = dwNewSize;

        m_pNewVertexStreamZeroData = (uint08*)malloc(dwNewSizeWR);
        m_bAllocatedStreamZeroData = true;

        pPatchedVertexData = (uint08*)m_pNewVertexStreamZeroData;
        pOrigVertexData = (uint08*)pPatchDesc->pVertexStreamZeroData;

        pPatchDesc->pVertexStreamZeroData = pPatchedVertexData;
    }

    // Copy the nonmodified data
    memcpy(pPatchedVertexData, pOrigVertexData, pPatchDesc->dwOffset);
    memcpy(&pPatchedVertexData[pPatchDesc->dwOffset+dwNewSize],
           &pOrigVertexData[pPatchDesc->dwOffset+dwOriginalSize],
           dwOriginalSizeWR - pPatchDesc->dwOffset - dwOriginalSize);

    // Quad list
    if(pPatchDesc->PrimitiveType == X_D3DPT_QUADLIST)
    {
        uint08 *pPatch1 = &pPatchedVertexData[pPatchDesc->dwOffset     * pStream->uiOrigStride];
        uint08 *pPatch2 = &pPatchedVertexData[(pPatchDesc->dwOffset + 3) * pStream->uiOrigStride];
        uint08 *pPatch3 = &pPatchedVertexData[(pPatchDesc->dwOffset + 4) * pStream->uiOrigStride];
        uint08 *pPatch4 = &pPatchedVertexData[(pPatchDesc->dwOffset + 5) * pStream->uiOrigStride];

        uint08 *pOrig1 = &pOrigVertexData[pPatchDesc->dwOffset     * pStream->uiOrigStride];
        uint08 *pOrig2 = &pOrigVertexData[(pPatchDesc->dwOffset + 2) * pStream->uiOrigStride];
        uint08 *pOrig3 = &pOrigVertexData[(pPatchDesc->dwOffset + 3) * pStream->uiOrigStride];

        for(uint32 i = 0;i < dwTotalPrimitiveCount /2;i++)
        {
		//	__asm int 3;
		//	DbgPrintf( "pPatch1 = 0x%.08X pOrig1 = 0x%.08X pStream->uiOrigStride * 3 = 0x%.08X\n", pPatch1, pOrig1, pStream->uiOrigStride * 3 );
            memcpy(pPatch1, pOrig1, pStream->uiOrigStride * 3); // Vertex 0,1,2 := Vertex 0,1,2
		//	__asm int 3;
            memcpy(pPatch2, pOrig2, pStream->uiOrigStride);     // Vertex 3     := Vertex 2
		//	__asm int 3;
            memcpy(pPatch3, pOrig3, pStream->uiOrigStride);     // Vertex 4     := Vertex 3
		//	__asm int 3;
            memcpy(pPatch4, pOrig1, pStream->uiOrigStride);     // Vertex 5     := Vertex 0

            pPatch1 += pStream->uiOrigStride * 6;
            pPatch2 += pStream->uiOrigStride * 6;
            pPatch3 += pStream->uiOrigStride * 6;
            pPatch4 += pStream->uiOrigStride * 6;

            pOrig1 += pStream->uiOrigStride * 4;
            pOrig2 += pStream->uiOrigStride * 4;
            pOrig3 += pStream->uiOrigStride * 4;

            if(pPatchDesc->hVertexShader & D3DFVF_XYZRHW)
            {
                for(int z = 0; z < 6; z++)
                {
                    if(((FLOAT*)&pPatchedVertexData[pPatchDesc->dwOffset + i * pStream->uiOrigStride * 6 + z * pStream->uiOrigStride])[2] == 0.0f)
                        ((FLOAT*)&pPatchedVertexData[pPatchDesc->dwOffset + i * pStream->uiOrigStride * 6 + z * pStream->uiOrigStride])[2] = 1.0f;
                    if(((FLOAT*)&pPatchedVertexData[pPatchDesc->dwOffset + i * pStream->uiOrigStride * 6 + z * pStream->uiOrigStride])[3] == 0.0f)
                        ((FLOAT*)&pPatchedVertexData[pPatchDesc->dwOffset + i * pStream->uiOrigStride * 6 + z * pStream->uiOrigStride])[3] = 1.0f;
                }
            }
        }
    }
    // Line loop
    else if(pPatchDesc->PrimitiveType == X_D3DPT_LINELOOP)
    {
        memcpy(&pPatchedVertexData[pPatchDesc->dwOffset], &pOrigVertexData[pPatchDesc->dwOffset], dwOriginalSize);
	    memcpy(&pPatchedVertexData[pPatchDesc->dwOffset + dwOriginalSize], &pOrigVertexData[pPatchDesc->dwOffset], pStream->uiOrigStride);
    }

    if(pPatchDesc->pVertexStreamZeroData == nullptr)
    {
        pStream->pPatchedStream->Unlock();

        g_pD3DDevice8->SetStreamSource(uiStream, pStream->pPatchedStream, pStream->uiOrigStride);
    }

    m_bPatched = true;

    return true;
}

bool VertexPatcher::Apply(VertexPatchDesc *pPatchDesc, bool *pbFatalError)
{
    bool Patched = false;
    // Get the number of streams
    m_uiNbrStreams = GetNbrStreams(pPatchDesc);

    if(VshHandleIsVertexShader(pPatchDesc->hVertexShader))
    {
        m_pDynamicPatch = &((VERTEX_SHADER *)VshHandleGetVertexShader(pPatchDesc->hVertexShader)->Handle)->VertexDynamicPatch;
    }
    for(UINT uiStream = 0; uiStream < m_uiNbrStreams; uiStream++)
    {
        bool LocalPatched = false;

		uint32_t uiHash = 0;
        if(ApplyCachedStream(pPatchDesc, uiStream, pbFatalError, &uiHash))
        {
            m_pStreams[uiStream].bUsedCached = true;
            continue;
        }

        LocalPatched |= PatchPrimitive(pPatchDesc, uiStream);
        LocalPatched |= PatchStream(pPatchDesc, uiStream);
        if(LocalPatched && !pPatchDesc->pVertexStreamZeroData)
        {
            // Insert the patched stream in the cache
            CacheStream(pPatchDesc, uiStream, uiHash);
            m_pStreams[uiStream].bUsedCached = true;
        }
        Patched |= LocalPatched;

		// If we didn't patch the stream, use a non-patched stream
		// TODO: Update the converion/patching code to make a host copy even when no patching is required
		// Doing this will fully remove the need to call _Register on Vertex Buffers
		if (!Patched && pPatchDesc->pVertexStreamZeroData == nullptr) {
			DWORD dwSize = GetVertexBufferSize(pPatchDesc->dwVertexCount, g_D3DStreamStrides[uiStream], pPatchDesc->pIndexData, pPatchDesc->dwOffset);
			g_pD3DDevice8->SetStreamSource(uiStream, GetHostVertexBuffer(g_D3DStreams[uiStream], dwSize), g_D3DStreamStrides[uiStream]);
		}
    }

    return Patched;
}

bool VertexPatcher::Restore()
{
    if(!this->m_bPatched)
        return false;

    for(UINT uiStream = 0; uiStream < m_uiNbrStreams; uiStream++)
    {
        if(m_pStreams[uiStream].pPatchedStream != NULL)
        {
            UINT b = m_pStreams[uiStream].pPatchedStream->Release();
        }

        if(!m_pStreams[uiStream].bUsedCached)
        {

            if(this->m_bAllocatedStreamZeroData)
            {
                free(m_pNewVertexStreamZeroData);
				// Cleanup, just to be sure :
				m_pNewVertexStreamZeroData = nullptr;
				this->m_bAllocatedStreamZeroData = false;
            }
        }
        else
        {
            m_pStreams[uiStream].bUsedCached = false;
        }

    }

    return true;
}

VOID EmuFlushIVB()
{
    EmuUpdateDeferredStates();
	EmuUpdateActiveTextureStages();

    DWORD *pdwVB = (DWORD*)g_pIVBVertexBuffer;

    UINT uiStride = 0;

    // Parse IVB table with current FVF shader if possible.
    bool bFVF = !VshHandleIsVertexShader(g_CurrentVertexShader);
    DWORD dwCurFVF;
    if(bFVF && ((g_CurrentVertexShader & D3DFVF_POSITION_MASK) != D3DFVF_XYZRHW))
    {
        dwCurFVF = g_CurrentVertexShader;

		// HACK: Halo...
		if(dwCurFVF == 0)
		{
			EmuWarning("EmuFlushIVB(): using g_IVBFVF instead of current FVF!");
			dwCurFVF = g_IVBFVF;
		}
    }
    else
    {
        dwCurFVF = g_IVBFVF;
    }

    DbgPrintf("g_IVBTblOffs := %d\n", g_IVBTblOffs);

    DWORD dwPos = dwCurFVF & D3DFVF_POSITION_MASK;
	DWORD dwTexN = (dwCurFVF & D3DFVF_TEXCOUNT_MASK) >> D3DFVF_TEXCOUNT_SHIFT;

	for(uint v=0;v<g_IVBTblOffs;v++)
    {
        if(dwPos == D3DFVF_XYZ)
        {
            *(FLOAT*)pdwVB++ = g_IVBTable[v].Position.x;
            *(FLOAT*)pdwVB++ = g_IVBTable[v].Position.y;
            *(FLOAT*)pdwVB++ = g_IVBTable[v].Position.z;

            DbgPrintf("IVB Position := {%f, %f, %f}\n", g_IVBTable[v].Position.x, g_IVBTable[v].Position.y, g_IVBTable[v].Position.z);
        }
        else if(dwPos == D3DFVF_XYZRHW)
        {
            *(FLOAT*)pdwVB++ = g_IVBTable[v].Position.x;
            *(FLOAT*)pdwVB++ = g_IVBTable[v].Position.y;
            *(FLOAT*)pdwVB++ = g_IVBTable[v].Position.z;
            *(FLOAT*)pdwVB++ = g_IVBTable[v].Rhw;

            DbgPrintf("IVB Position := {%f, %f, %f, %f, %f}\n", g_IVBTable[v].Position.x, g_IVBTable[v].Position.y, g_IVBTable[v].Position.z, g_IVBTable[v].Position.z, g_IVBTable[v].Rhw);
        }
		else if(dwPos == D3DFVF_XYZB1)
        {
            *(FLOAT*)pdwVB++ = g_IVBTable[v].Position.x;
            *(FLOAT*)pdwVB++ = g_IVBTable[v].Position.y;
            *(FLOAT*)pdwVB++ = g_IVBTable[v].Position.z;
			*(FLOAT*)pdwVB++ = g_IVBTable[v].Blend1;

			DbgPrintf("IVB Position := {%f, %f, %f, %f}\n", g_IVBTable[v].Position.x, g_IVBTable[v].Position.y, g_IVBTable[v].Position.z, g_IVBTable[v].Blend1);
        }
        else
        {
			CxbxKrnlCleanup("Unsupported Position Mask (FVF := 0x%.08X dwPos := 0x%.08X)", dwCurFVF, dwPos);
        }

//      if(dwPos == D3DFVF_NORMAL)	// <- This didn't look right but if it is, change it back...
		if(dwCurFVF & D3DFVF_NORMAL)
        {
            *(FLOAT*)pdwVB++ = g_IVBTable[v].Normal.x;
            *(FLOAT*)pdwVB++ = g_IVBTable[v].Normal.y;
            *(FLOAT*)pdwVB++ = g_IVBTable[v].Normal.z;

            DbgPrintf("IVB Normal := {%f, %f, %f}\n", g_IVBTable[v].Normal.x, g_IVBTable[v].Normal.y, g_IVBTable[v].Normal.z);
        }

        if(dwCurFVF & D3DFVF_DIFFUSE)
        {
            *(DWORD*)pdwVB++ = g_IVBTable[v].dwDiffuse;

            DbgPrintf("IVB Diffuse := 0x%.08X\n", g_IVBTable[v].dwDiffuse);
        }

        if(dwCurFVF & D3DFVF_SPECULAR)
        {
            *(DWORD*)pdwVB++ = g_IVBTable[v].dwSpecular;

            DbgPrintf("IVB Specular := 0x%.08X\n", g_IVBTable[v].dwSpecular);
        }

        if(dwTexN >= 1)
        {
            *(FLOAT*)pdwVB++ = g_IVBTable[v].TexCoord1.x;
            *(FLOAT*)pdwVB++ = g_IVBTable[v].TexCoord1.y;

            DbgPrintf("IVB TexCoord1 := {%f, %f}\n", g_IVBTable[v].TexCoord1.x, g_IVBTable[v].TexCoord1.y);
        }

        if(dwTexN >= 2)
        {
            *(FLOAT*)pdwVB++ = g_IVBTable[v].TexCoord2.x;
            *(FLOAT*)pdwVB++ = g_IVBTable[v].TexCoord2.y;

            DbgPrintf("IVB TexCoord2 := {%f, %f}\n", g_IVBTable[v].TexCoord2.x, g_IVBTable[v].TexCoord2.y);
        }

        if(dwTexN >= 3)
        {
            *(FLOAT*)pdwVB++ = g_IVBTable[v].TexCoord3.x;
            *(FLOAT*)pdwVB++ = g_IVBTable[v].TexCoord3.y;

            DbgPrintf("IVB TexCoord3 := {%f, %f}\n", g_IVBTable[v].TexCoord3.x, g_IVBTable[v].TexCoord3.y);
        }

        if(dwTexN >= 4)
        {
            *(FLOAT*)pdwVB++ = g_IVBTable[v].TexCoord4.x;
            *(FLOAT*)pdwVB++ = g_IVBTable[v].TexCoord4.y;

            DbgPrintf("IVB TexCoord4 := {%f, %f}\n", g_IVBTable[v].TexCoord4.x, g_IVBTable[v].TexCoord4.y);
        }

		uint VertexBufferUsage = (BYTE *)pdwVB - (BYTE *)g_pIVBVertexBuffer;

		if (v == 0)
		{
			// Stride is equal to the delta of the first vertex
			uiStride = VertexBufferUsage;
		}

		if (VertexBufferUsage >= IVB_BUFFER_SIZE)
		{
			CxbxKrnlCleanup("Overflow g_pIVBVertexBuffer  : %d", v);
		}
	}

    VertexPatchDesc VPDesc;

    VPDesc.PrimitiveType = g_IVBPrimitiveType;
    VPDesc.dwVertexCount = g_IVBTblOffs;
    VPDesc.dwOffset = 0;
    VPDesc.pVertexStreamZeroData = g_pIVBVertexBuffer;
    VPDesc.uiVertexStreamZeroStride = uiStride;
    VPDesc.hVertexShader = g_CurrentVertexShader;

    VertexPatcher VertPatch;

    bool bPatched = VertPatch.Apply(&VPDesc, NULL);

    if(bFVF)
    {
        g_pD3DDevice8->SetVertexShader(dwCurFVF);
    }

    g_pD3DDevice8->DrawPrimitiveUP(
		EmuXB2PC_D3DPrimitiveType(VPDesc.PrimitiveType),
        VPDesc.dwPrimitiveCount,
        VPDesc.pVertexStreamZeroData,
        VPDesc.uiVertexStreamZeroStride);

	g_dwPrimPerFrame += VPDesc.dwPrimitiveCount;

    if(bFVF)
    {
        g_pD3DDevice8->SetVertexShader(g_CurrentVertexShader);
    }

    VertPatch.Restore();

    g_IVBTblOffs = 0;

    return;
}
