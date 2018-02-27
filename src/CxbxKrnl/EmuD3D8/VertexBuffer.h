// ******************************************************************
// *
// *    .,-:::::    .,::      .::::::::.    .,::      .:
// *  ,;;;'````'    `;;;,  .,;;  ;;;'';;'   `;;;,  .,;;
// *  [[[             '[[,,[['   [[[__[[\.    '[[,,[['
// *  $$$              Y$$$P     $$""""Y$$     Y$$$P
// *  `88bo,__,o,    oP"``"Yo,  _88o,,od8P   oP"``"Yo,
// *    "YUMMMMMP",m"       "Mm,""YUMMMP" ,m"       "Mm,
// *
// *   Cxbx->Win32->CxbxKrnl->EmuD3D8->VertexBuffer.h
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
// *  (c) 2002-2003 Aaron Robinson <caustik@caustik.com>
// *
// *  All rights reserved
// *
// ******************************************************************
#ifndef VERTEXBUFFER_H
#define VERTEXBUFFER_H

#include "Cxbx.h"
//#include <ctime> // Conflict with io.h

#define MAX_NBR_STREAMS 16

typedef struct _VertexPatchDesc
{
    IN     X_D3DPRIMITIVETYPE    PrimitiveType;
    IN     DWORD                 dwVertexCount;
    IN     DWORD                 dwPrimitiveCount;
    IN     DWORD                 dwOffset;
    // Data if Draw...UP call
    IN OUT PVOID                 pVertexStreamZeroData;
    IN     UINT                  uiVertexStreamZeroStride;
    // The current vertex shader, used to identify the streams
    IN     DWORD                 hVertexShader;
	IN	   PWORD				 pIndexData = nullptr;
}
VertexPatchDesc;

typedef struct _PATCHEDSTREAM
{
    X_D3DVertexBuffer *pOriginalStream;
    IDirect3DVertexBuffer8 *pPatchedStream;
    UINT                    uiOrigStride;
    UINT                    uiNewStride;
    bool                    bUsedCached;
} PATCHEDSTREAM;

typedef struct _CACHEDSTREAM
{
    uint32_t       uiHash;
    uint32         uiCheckFrequency;
    uint32         uiCacheHit;
    bool           bIsUP;
    PATCHEDSTREAM  Stream;
    void          *pStreamUP;           // Draw..UP (instead of pOriginalStream)
    uint32         uiLength;            // The length of the stream
    uint32         uiCount;             // XXHash32::hash() check count
    uint32         dwPrimitiveCount;
    long           lLastUsed;           // For cache removal purposes
} CACHEDSTREAM;

class VertexPatcher
{
    public:
        VertexPatcher();
       ~VertexPatcher();

        bool Apply(VertexPatchDesc *pPatchDesc, bool *pbFatalError);
        bool Restore();

        // Dumps the cache to the console
        static void DumpCache(void);

    private:

        UINT m_uiNbrStreams;
        PATCHEDSTREAM m_pStreams[MAX_NBR_STREAMS];

        PVOID m_pNewVertexStreamZeroData;

        bool m_bPatched;
        bool m_bAllocatedStreamZeroData;

        VERTEX_DYNAMIC_PATCH *m_pDynamicPatch;

        // Returns the number of streams of a patch
        UINT GetNbrStreams(VertexPatchDesc *pPatchDesc);

        // Caches a patched stream
        void CacheStream(VertexPatchDesc *pPatchDesc,
                         UINT             uiStream,
						 uint32_t		  uiHash);

        // Frees a cached, patched stream
        void FreeCachedStream(void *pStream);

        // Tries to apply a previously patched stream from the cache
        bool ApplyCachedStream(VertexPatchDesc *pPatchDesc,
                               UINT             uiStream,
							   bool			   *pbFatalError,
							   uint32_t        *uiHash);

        // Patches the types of the stream
        bool PatchStream(VertexPatchDesc *pPatchDesc, UINT uiStream);

        // Normalize texture coordinates in FVF stream if needed
        bool NormalizeTexCoords(VertexPatchDesc *pPatchDesc, UINT uiStream);

        // Patches the primitive of the stream
        bool PatchPrimitive(VertexPatchDesc *pPatchDesc, UINT uiStream);
};

// inline vertex buffer emulation
extern DWORD                  *g_pIVBVertexBuffer;
extern X_D3DPRIMITIVETYPE      g_IVBPrimitiveType;
extern DWORD                   g_IVBFVF;

#define IVB_TABLE_SIZE  4096 // This should be more than enough. Tweak as necessary if it overflows or the resulting VertexBuffer fails to allocate
#define IVB_BUFFER_SIZE sizeof(_D3DIVB) * IVB_TABLE_SIZE

// TODO : Enlarge IVB_TABLE_SIZE and IVB_BUFFER_SIZE
// TODO : Calculate IVB_BUFFER_SIZE using sizeof(DWORD)

struct _D3DIVB
{
    D3DXVECTOR3 Position;   // Position
    FLOAT            Rhw;        // Rhw
	FLOAT			 Blend1;	 // Blend1		
    DWORD       dwSpecular; // Specular
    DWORD       dwDiffuse;  // Diffuse
    D3DXVECTOR3 Normal;     // Normal
    D3DXVECTOR2 TexCoord1;  // TexCoord1
    D3DXVECTOR2 TexCoord2;  // TexCoord2
    D3DXVECTOR2 TexCoord3;  // TexCoord3
    D3DXVECTOR2 TexCoord4;  // TexCoord4
};

extern _D3DIVB g_IVBTable[IVB_TABLE_SIZE];

extern UINT g_IVBTblOffs;

extern VOID EmuFlushIVB();

extern VOID EmuUpdateActiveTexture();

extern DWORD g_dwPrimPerFrame;
 
#endif