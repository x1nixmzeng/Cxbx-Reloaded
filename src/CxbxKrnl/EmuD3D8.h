// ******************************************************************
// *
// *    .,-:::::    .,::      .::::::::.    .,::      .:
// *  ,;;;'````'    `;;;,  .,;;  ;;;'';;'   `;;;,  .,;;
// *  [[[             '[[,,[['   [[[__[[\.    '[[,,[['
// *  $$$              Y$$$P     $$""""Y$$     Y$$$P
// *  `88bo,__,o,    oP"``"Yo,  _88o,,od8P   oP"``"Yo,
// *    "YUMMMMMP",m"       "Mm,""YUMMMP" ,m"       "Mm,
// *
// *   Cxbx->Win32->CxbxKrnl->EmuD3D8.h
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
#ifndef EMUD3D8_H
#define EMUD3D8_H

#include "EmuD3D8Types.h"
#include "Cxbx.h"
#include "CxbxKrnl.h"
#include "Emu.h"

// Beware of this namespace choice as DirectX8 is both Xbox AND Native
namespace Native
{
#define DIRECTDRAW_VERSION 0x0700
#include <ddraw.h>
}

namespace Xbox
{

// initialize render window
extern VOID CxbxInitWindow(bool bFullInit);

extern VOID CxbxSetPixelContainerHeader
(
	X_D3DPixelContainer* pPixelContainer,
	XDK::DWORD           	Common,
	XDK::UINT				Width,
	XDK::UINT				Height,
	XDK::UINT				Levels,
	X_D3DFORMAT	Format,
	XDK::UINT				Dimensions,
	XDK::UINT				Pitch
);

extern uint8 *ConvertD3DTextureToARGB(
	X_D3DPixelContainer *pXboxPixelContainer,
	uint8 *pSrc,
	int *pWidth, int *pHeight,
	int TextureStage = 0
);

// initialize direct3d
extern VOID EmuD3DInit();

// cleanup direct3d
extern VOID EmuD3DCleanup();

// EmuD3DTileCache (8 tiles maximum)
extern X_D3DTILE EmuD3DTileCache[0x08];

// EmuD3DActiveTexture
extern X_D3DBaseTexture *EmuD3DActiveTexture[TEXTURE_STAGES];

// ******************************************************************
// * patch: Direct3D_CreateDevice
// ******************************************************************
XDK::HRESULT WINAPI EMUPATCH(Direct3D_CreateDevice)
(
	XDK::UINT                        Adapter,
    Native::D3DDEVTYPE                  DeviceType,
	Native::HWND                        hFocusWindow,
	XDK::DWORD                       BehaviorFlags,
    X_D3DPRESENT_PARAMETERS    *pPresentationParameters,
	Native::IDirect3DDevice8          **ppReturnedDeviceInterface
);

// ******************************************************************
// * patch: IDirect3DResource8_IsBusy
// ******************************************************************
XDK::BOOL WINAPI EMUPATCH(D3DDevice_IsBusy)();

// ******************************************************************
// * patch: D3DDevice_GetCreationParameters
// ******************************************************************
VOID WINAPI EMUPATCH(D3DDevice_GetCreationParameters)
(
	Native::D3DDEVICE_CREATION_PARAMETERS *pParameters
);

#if 0 // patch disabled
// ******************************************************************
// * patch: D3D_CheckDeviceFormat
// ******************************************************************
XDK::HRESULT WINAPI EMUPATCH(D3D_CheckDeviceFormat)
(
	XDK::UINT                        Adapter,
    D3DDEVTYPE                  DeviceType,
    X_D3DFORMAT                 AdapterFormat,
	XDK::DWORD                       Usage,
    X_D3DRESOURCETYPE           RType,
    X_D3DFORMAT                 CheckFormat
);
#endif

#if 0 // patch disabled
// ******************************************************************
// * patch: D3DDevice_GetDeviceCaps
// ******************************************************************
VOID WINAPI EMUPATCH(D3DDevice_GetDeviceCaps)
(
	NativeD3DCAPS8                   *pCaps
);
#endif

// ******************************************************************
// * patch: D3DDevice_GetDisplayFieldStatus
// ******************************************************************
VOID WINAPI EMUPATCH(D3DDevice_GetDisplayFieldStatus)
(
	X_D3DFIELD_STATUS *pFieldStatus
);

// ******************************************************************
// * patch: D3DDevice_BeginPush
// ******************************************************************
XDK::PDWORD WINAPI EMUPATCH(D3DDevice_BeginPush)(XDK::DWORD Count);

// ******************************************************************
// * patch: D3DDevice_EndPush
// ******************************************************************
VOID WINAPI EMUPATCH(D3DDevice_EndPush)(XDK::DWORD *pPush);

// ******************************************************************
// * patch: D3DDevice_BeginVisibilityTest
// ******************************************************************
VOID WINAPI EMUPATCH(D3DDevice_BeginVisibilityTest)();

// ******************************************************************
// * patch: D3DDevice_EndVisibilityTest
// ******************************************************************
XDK::HRESULT WINAPI EMUPATCH(D3DDevice_EndVisibilityTest)
(
	XDK::DWORD                       Index
);

// ******************************************************************
// * patch: D3DDevice_GetVisibilityTestResult
// ******************************************************************
XDK::HRESULT WINAPI EMUPATCH(D3DDevice_GetVisibilityTestResult)
(
    XDK::DWORD                       Index,
    XDK::UINT                       *pResult,
    XDK::ULONGLONG                  *pTimeStamp
);

// ******************************************************************
// * patch: D3DDevice_SetBackBufferScale
// ******************************************************************
VOID WINAPI EMUPATCH(D3DDevice_SetBackBufferScale)(Native::FLOAT x, Native::FLOAT y);

// ******************************************************************
// * patch: D3DDevice_LoadVertexShader
// ******************************************************************
VOID WINAPI EMUPATCH(D3DDevice_LoadVertexShader)
(
    XDK::DWORD                       Handle,
    XDK::DWORD                       Address
);

// ******************************************************************
// * patch: D3DDevice_SelectVertexShader
// ******************************************************************
VOID WINAPI EMUPATCH(D3DDevice_SelectVertexShader)
(
    XDK::DWORD                       Handle,
    XDK::DWORD                       Address
);

// ******************************************************************
// * patch: D3D_KickOffAndWaitForIdle
// ******************************************************************
VOID WINAPI EMUPATCH(D3D_KickOffAndWaitForIdle)();

// ******************************************************************
// * patch: D3D_KickOffAndWaitForIdle
// ******************************************************************
VOID WINAPI EMUPATCH(D3D_KickOffAndWaitForIdle2)(XDK::DWORD dwDummy1, XDK::DWORD dwDummy2);

// ******************************************************************
// * patch: D3DDevice_SetGammaRamp
// ******************************************************************
VOID WINAPI EMUPATCH(D3DDevice_SetGammaRamp)
(
	XDK::DWORD                   dwFlags,
    CONST X_D3DGAMMARAMP   *pRamp
);

// ******************************************************************
// * patch: D3DDevice_AddRef
// ******************************************************************
XDK::ULONG WINAPI EMUPATCH(D3DDevice_AddRef)();

// ******************************************************************
// * patch: D3DDevice_BeginStateBlock
// ******************************************************************
VOID WINAPI EMUPATCH(D3DDevice_BeginStateBlock)();

// ******************************************************************
// * patch: D3DDevice_CaptureStateBlock
// ******************************************************************
VOID WINAPI EMUPATCH(D3DDevice_CaptureStateBlock)(XDK::DWORD Token);

// ******************************************************************
// * patch: D3DDevice_ApplyStateBlock
// ******************************************************************
VOID WINAPI EMUPATCH(D3DDevice_ApplyStateBlock)(XDK::DWORD Token);

// ******************************************************************
// * patch: D3DDevice_EndStateBlock
// ******************************************************************
XDK::HRESULT WINAPI EMUPATCH(D3DDevice_EndStateBlock)(XDK::DWORD *pToken);

// ******************************************************************
// * patch: D3DDevice_CopyRects
// ******************************************************************
VOID WINAPI EMUPATCH(D3DDevice_CopyRects)
(
    X_D3DSurface       *pSourceSurface,
    CONST Native::RECT         *pSourceRectsArray,
	XDK::UINT                cRects,
    X_D3DSurface       *pDestinationSurface,
    CONST Native::POINT        *pDestPointsArray
);

// ******************************************************************
// * patch: D3DDevice_CreateImageSurface
// ******************************************************************
XDK::HRESULT WINAPI EMUPATCH(D3DDevice_CreateImageSurface)
(
    XDK::UINT                Width,
    XDK::UINT                Height,
    X_D3DFORMAT         Format,
    X_D3DSurface      **ppBackBuffer
);

// ******************************************************************
// * patch: D3DDevice_GetGammaRamp
// ******************************************************************
VOID WINAPI EMUPATCH(D3DDevice_GetGammaRamp)
(
    X_D3DGAMMARAMP     *pRamp
);

// ******************************************************************
// * patch: D3DDevice_GetBackBuffer2
// ******************************************************************
X_D3DSurface* WINAPI EMUPATCH(D3DDevice_GetBackBuffer2)
(
	XDK::INT                 BackBuffer
);

// ******************************************************************
// * patch: D3DDevice_GetBackBuffer
// ******************************************************************
VOID WINAPI EMUPATCH(D3DDevice_GetBackBuffer)
(
	XDK::INT                 BackBuffer,
    Native::D3DBACKBUFFER_TYPE  Type,
    X_D3DSurface      **ppBackBuffer
);

// ******************************************************************
// * patch: D3DDevice_SetViewport
// ******************************************************************
VOID WINAPI EMUPATCH(D3DDevice_SetViewport)
(
    CONST Native::D3DVIEWPORT8 *pViewport
);

// ******************************************************************
// * patch: D3DDevice_GetViewport
// ******************************************************************
VOID WINAPI EMUPATCH(D3DDevice_GetViewport)
(
	Native::D3DVIEWPORT8 *pViewport
);

// ******************************************************************
// * patch: D3DDevice_GetViewportOffsetAndScale
// ******************************************************************
VOID WINAPI EMUPATCH(D3DDevice_GetViewportOffsetAndScale)
(
    Native::D3DXVECTOR4 *pOffset,
    Native::D3DXVECTOR4 *pScale
);

// ******************************************************************
// * patch: D3DDevice_SetShaderConstantMode
// ******************************************************************
VOID WINAPI EMUPATCH(D3DDevice_SetShaderConstantMode)
(
    X_VERTEXSHADERCONSTANTMODE Mode
);

// ******************************************************************
// * patch: D3DDevice_Reset
// ******************************************************************
XDK::HRESULT WINAPI EMUPATCH(D3DDevice_Reset)
(
    X_D3DPRESENT_PARAMETERS *pPresentationParameters
);

// ******************************************************************
// * patch: D3DDevice_GetRenderTarget
// ******************************************************************
XDK::HRESULT WINAPI EMUPATCH(D3DDevice_GetRenderTarget)
(
    X_D3DSurface  **ppRenderTarget
);

// ******************************************************************
// * patch: D3DDevice_GetRenderTarget
// ******************************************************************
X_D3DSurface * WINAPI EMUPATCH(D3DDevice_GetRenderTarget2)();

// ******************************************************************
// * patch: D3DDevice_GetDepthStencilSurface
// ******************************************************************
XDK::HRESULT WINAPI EMUPATCH(D3DDevice_GetDepthStencilSurface)
(
    X_D3DSurface  **ppZStencilSurface
);

// ******************************************************************
// * patch: D3DDevice_GetDepthStencilSurface
// ******************************************************************
X_D3DSurface * WINAPI EMUPATCH(D3DDevice_GetDepthStencilSurface2)();

// ******************************************************************
// * patch: D3DDevice_GetTile
// ******************************************************************
VOID WINAPI EMUPATCH(D3DDevice_GetTile)
(
	XDK::DWORD           Index,
    X_D3DTILE      *pTile
);

// ******************************************************************
// * patch: D3DDevice_SetTile
// ******************************************************************
VOID WINAPI EMUPATCH(D3DDevice_SetTile)
(
	XDK::DWORD               Index,
    CONST X_D3DTILE    *pTile
);

// ******************************************************************
// * patch: D3DDevice_CreateVertexShader
// ******************************************************************
XDK::HRESULT WINAPI EMUPATCH(D3DDevice_CreateVertexShader)
(
    CONST XDK::DWORD    *pDeclaration,
    CONST XDK::DWORD    *pFunction,
    XDK::DWORD          *pHandle,
    XDK::DWORD           Usage
);

// ******************************************************************
// * patch: D3DDevice_SetPixelShaderConstant
// ******************************************************************
VOID WINAPI EMUPATCH(D3DDevice_SetPixelShaderConstant)
(
	XDK::DWORD       Register,
    CONST XDK::PVOID pConstantData,
	XDK::DWORD       ConstantCount
);

// ******************************************************************
// * patch: D3DDevice_SetVertexShaderConstant
// ******************************************************************
VOID WINAPI EMUPATCH(D3DDevice_SetVertexShaderConstant)
(
	XDK::INT         Register,
    CONST XDK::PVOID pConstantData,
	XDK::DWORD       ConstantCount
);

// ******************************************************************
// * patch: D3DDevice_SetVertexShaderConstant1
// ******************************************************************
VOID __fastcall EMUPATCH(D3DDevice_SetVertexShaderConstant1)
(
	XDK::INT         Register,
    CONST XDK::PVOID pConstantData
);

// ******************************************************************
// * patch: D3DDevice_SetVertexShaderConstant1Fast
// ******************************************************************
VOID __fastcall EMUPATCH(D3DDevice_SetVertexShaderConstant1Fast)
(
	XDK::INT         Register,
    CONST XDK::PVOID pConstantData
);

// ******************************************************************
// * patch: D3DDevice_SetVertexShaderConstant4
// ******************************************************************
VOID __fastcall EMUPATCH(D3DDevice_SetVertexShaderConstant4)
(
	XDK::INT         Register,
    CONST XDK::PVOID pConstantData
);

// ******************************************************************
// * patch: D3DDevice_SetVertexShaderConstantNotInline
// ******************************************************************
VOID __fastcall EMUPATCH(D3DDevice_SetVertexShaderConstantNotInline)
(
	XDK::INT         Register,
    CONST XDK::PVOID pConstantData,
	XDK::DWORD       ConstantCount
);

// ******************************************************************
// * patch: D3DDevice_SetVertexShaderConstantNotInlineFast
// ******************************************************************
VOID __fastcall EMUPATCH(D3DDevice_SetVertexShaderConstantNotInlineFast)
(
	XDK::INT         Register,
    CONST XDK::PVOID pConstantData,
	XDK::DWORD       ConstantCount
);

// ******************************************************************
// * patch: D3DDevice_DeletePixelShader
// ******************************************************************
VOID WINAPI EMUPATCH(D3DDevice_DeletePixelShader)
(
    XDK::DWORD          Handle
);

// ******************************************************************
// * patch: D3DDevice_CreatePixelShader
// ******************************************************************
XDK::HRESULT WINAPI EMUPATCH(D3DDevice_CreatePixelShader)
(
    X_D3DPIXELSHADERDEF    *pPSDef,
    XDK::DWORD				   *pHandle
);

// ******************************************************************
// * patch: D3DDevice_SetPixelShader
// ******************************************************************
VOID WINAPI EMUPATCH(D3DDevice_SetPixelShader)
(
    XDK::DWORD           Handle
);

// ******************************************************************
// * patch: D3DDevice_CreateTexture2
// ******************************************************************
X_D3DResource * WINAPI EMUPATCH(D3DDevice_CreateTexture2)
(
    XDK::UINT                Width,
    XDK::UINT                Height,
    XDK::UINT                Depth,
    XDK::UINT                Levels,
    XDK::DWORD               Usage,
    X_D3DFORMAT         Format,
    X_D3DRESOURCETYPE   D3DResource
);

// ******************************************************************
// * patch: D3DDevice_CreateTexture
// ******************************************************************
XDK::HRESULT WINAPI EMUPATCH(D3DDevice_CreateTexture)
(
    XDK::UINT                Width,
    XDK::UINT                Height,
    XDK::UINT                Levels,
    XDK::DWORD               Usage,
    X_D3DFORMAT         Format,
	Native::D3DPOOL             Pool,
    X_D3DTexture      **ppTexture
);

// ******************************************************************
// * patch: D3DDevice_CreateVolumeTexture
// ******************************************************************
XDK::HRESULT WINAPI EMUPATCH(D3DDevice_CreateVolumeTexture)
(
    XDK::UINT                 Width,
    XDK::UINT                 Height,
    XDK::UINT                 Depth,
    XDK::UINT                 Levels,
    XDK::DWORD                Usage,
    X_D3DFORMAT          Format,
	Native::D3DPOOL              Pool,
    X_D3DVolumeTexture **ppVolumeTexture
);

// ******************************************************************
// * patch: D3DDevice_CreateCubeTexture
// ******************************************************************
XDK::HRESULT WINAPI EMUPATCH(D3DDevice_CreateCubeTexture)
(
    XDK::UINT                 EdgeLength,
    XDK::UINT                 Levels,
    XDK::DWORD                Usage,
    X_D3DFORMAT          Format,
	Native::D3DPOOL              Pool,
    X_D3DCubeTexture  **ppCubeTexture
);

// ******************************************************************
// * patch: D3DDevice_CreateIndexBuffer
// ******************************************************************
XDK::HRESULT WINAPI EMUPATCH(D3DDevice_CreateIndexBuffer)
(
    XDK::UINT                 Length,
    XDK::DWORD                Usage,
    X_D3DFORMAT          Format,
	Native::D3DPOOL              Pool,
    X_D3DIndexBuffer   **ppIndexBuffer
);

// ******************************************************************
// * patch: D3DDevice_CreateIndexBuffer2
// ******************************************************************
X_D3DIndexBuffer * WINAPI EMUPATCH(D3DDevice_CreateIndexBuffer2)(XDK::UINT Length);

// ******************************************************************
// * patch: D3DDevice_SetIndices
// ******************************************************************
XDK::HRESULT WINAPI EMUPATCH(D3DDevice_SetIndices)
(
    X_D3DIndexBuffer   *pIndexData,
    XDK::UINT                BaseVertexIndex
);

// ******************************************************************
// * patch: D3DDevice_SetTexture
// ******************************************************************
VOID WINAPI EMUPATCH(D3DDevice_SetTexture)
(
    XDK::DWORD           Stage,
	X_D3DBaseTexture  *pTexture
);

// ******************************************************************
// * patch: D3DDevice_SwitchTexture
// ******************************************************************
VOID __fastcall EMUPATCH(D3DDevice_SwitchTexture)
(
    XDK::DWORD           Method,
    XDK::DWORD           Data,
    XDK::DWORD           Format
);

// ******************************************************************
// * patch: D3DDevice_GetDisplayMode
// ******************************************************************
VOID WINAPI EMUPATCH(D3DDevice_GetDisplayMode)
(
    X_D3DDISPLAYMODE         *pModes
);

// ******************************************************************
// * patch: D3DDevice_Begin
// ******************************************************************
VOID WINAPI EMUPATCH(D3DDevice_Begin)
(
    X_D3DPRIMITIVETYPE     PrimitiveType
);

// ******************************************************************
// * patch: D3DDevice_SetVertexData2f
// ******************************************************************
VOID WINAPI EMUPATCH(D3DDevice_SetVertexData2f)
(
    int     Register,
    Native::FLOAT   a,
    Native::FLOAT   b
);

// ******************************************************************
// * patch: D3DDevice_SetVertexData2s
// ******************************************************************
VOID WINAPI EMUPATCH(D3DDevice_SetVertexData2s)
(
    int     Register,
    XDK::SHORT   a,
    XDK::SHORT   b
);

// ******************************************************************
// * patch: D3DDevice_SetVertexData4f
// ******************************************************************
VOID WINAPI EMUPATCH(D3DDevice_SetVertexData4f)
(
    int     Register,
    Native::FLOAT   a,
    Native::FLOAT   b,
    Native::FLOAT   c,
    Native::FLOAT   d
);

// ******************************************************************
// * patch: D3DDevice_SetVertexData4ub
// ******************************************************************
VOID WINAPI EMUPATCH(D3DDevice_SetVertexData4ub)
(
	XDK::INT		Register,
	XDK::BYTE	a,
	XDK::BYTE	b,
	XDK::BYTE	c,
	XDK::BYTE	d
);

// ******************************************************************
// * patch: D3DDevice_SetVertexData4s
// ******************************************************************
VOID WINAPI EMUPATCH(D3DDevice_SetVertexData4s)
(
	XDK::INT		Register,
	XDK::SHORT	a,
	XDK::SHORT	b,
	XDK::SHORT	c,
	XDK::SHORT	d
);

// ******************************************************************
// * patch: D3DDevice_SetVertexDataColor
// ******************************************************************
VOID WINAPI EMUPATCH(D3DDevice_SetVertexDataColor)
(
    int         Register,
	Native::D3DCOLOR    Color
);

// ******************************************************************
// * patch: D3DDevice_End
// ******************************************************************
VOID WINAPI EMUPATCH(D3DDevice_End)();

// ******************************************************************
// * patch: D3DDevice_RunPushBuffer
// ******************************************************************
VOID WINAPI EMUPATCH(D3DDevice_RunPushBuffer)
(
    X_D3DPushBuffer       *pPushBuffer,
    X_D3DFixup            *pFixup
);

// ******************************************************************
// * patch: D3DDevice_Clear
// ******************************************************************
VOID WINAPI EMUPATCH(D3DDevice_Clear)
(
    XDK::DWORD                  Count,
    CONST Native::D3DRECT         *pRects,
    XDK::DWORD                  Flags,
	Native::D3DCOLOR               Color,
    float                  Z,
    XDK::DWORD                  Stencil
);

// ******************************************************************
// * patch: D3DDevice_Present
// ******************************************************************
VOID WINAPI EMUPATCH(D3DDevice_Present)
(
    CONST Native::RECT* pSourceRect,
    CONST Native::RECT* pDestRect,
    XDK::PVOID       pDummy1,
    XDK::PVOID       pDummy2
);

// ******************************************************************
// * patch: D3DDevice_Swap
// ******************************************************************
XDK::DWORD WINAPI EMUPATCH(D3DDevice_Swap)
(
    XDK::DWORD Flags
);

// ******************************************************************
// * patch: IDirect3DResource8_Register
// ******************************************************************
VOID WINAPI EMUPATCH(D3DResource_Register)
(
    X_D3DResource      *pThis,
	XDK::PVOID               pBase
);

// ******************************************************************
// * patch: IDirect3DResource8_Release
// ******************************************************************
XDK::ULONG WINAPI EMUPATCH(D3DResource_Release)
(
    X_D3DResource      *pThis
);

#if 0 // patch disabled
// ******************************************************************
// * patch: IDirect3DResource8_GetType
// ******************************************************************
X_D3DRESOURCETYPE WINAPI EMUPATCH(D3DResource_GetType)
(
    X_D3DResource      *pThis
);
#endif

// ******************************************************************
// * patch: IDirect3DResource8_AddRef
// ******************************************************************
XDK::ULONG WINAPI EMUPATCH(D3DResource_AddRef)
(
    X_D3DResource      *pThis
);

// ******************************************************************
// * patch: IDirect3DResource8_IsBusy
// ******************************************************************
XDK::BOOL WINAPI EMUPATCH(D3DResource_IsBusy)
(
    X_D3DResource      *pThis
);

// ******************************************************************
// * patch: Lock2DSurface
// ******************************************************************
VOID WINAPI EMUPATCH(Lock2DSurface)
(
    X_D3DPixelContainer *pPixelContainer,
	Native::D3DCUBEMAP_FACES     FaceType,
    XDK::UINT                 Level,
	Native::D3DLOCKED_RECT      *pLockedRect,
	Native::RECT                *pRect,
    XDK::DWORD                Flags
);

// ******************************************************************
// * patch: Lock3DSurface
// ******************************************************************
VOID WINAPI EMUPATCH(Lock3DSurface)
(
    X_D3DPixelContainer *pPixelContainer,
	XDK::UINT				Level,
	Native::D3DLOCKED_BOX		*pLockedVolume,
	Native::D3DBOX				*pBox,
	XDK::DWORD				Flags
);

// ******************************************************************
// * patch: Get2DSurfaceDesc
// ******************************************************************
VOID WINAPI EMUPATCH(Get2DSurfaceDesc)
(
    X_D3DPixelContainer *pPixelContainer,
    XDK::DWORD                dwLevel,
    X_D3DSURFACE_DESC   *pDesc
);

// ******************************************************************
// * patch: IDirect3DSurface8_GetDesc
// ******************************************************************
VOID WINAPI EMUPATCH(D3DSurface_GetDesc)
(
    X_D3DResource      *pThis,
    X_D3DSURFACE_DESC  *pDesc
);

// ******************************************************************
// * patch: IDirect3DSurface8_LockRect
// ******************************************************************
VOID WINAPI EMUPATCH(D3DSurface_LockRect)
(
    X_D3DResource  *pThis,
	Native::D3DLOCKED_RECT *pLockedRect,
    CONST Native::RECT     *pRect,
    XDK::DWORD           Flags
);

// ******************************************************************
// * patch: IDirect3DBaseTexture8_GetLevelCount
// ******************************************************************
XDK::DWORD WINAPI EMUPATCH(D3DBaseTexture_GetLevelCount)
(
    X_D3DBaseTexture   *pThis
);

// ******************************************************************
// * patch: IDirect3DTexture8_GetSurfaceLevel2
// ******************************************************************
X_D3DSurface * WINAPI EMUPATCH(D3DTexture_GetSurfaceLevel2)
(
    X_D3DTexture   *pThis,
    XDK::UINT            Level
);

// ******************************************************************
// * patch: IDirect3DTexture8_LockRect
// ******************************************************************
VOID WINAPI EMUPATCH(D3DTexture_LockRect)
(
    X_D3DTexture   *pThis,
    XDK::UINT            Level,
	Native::D3DLOCKED_RECT *pLockedRect,
    CONST Native::RECT     *pRect,
    XDK::DWORD           Flags
);

// ******************************************************************
// * patch: IDirect3DTexture8_GetSurfaceLevel
// ******************************************************************
XDK::HRESULT WINAPI EMUPATCH(D3DTexture_GetSurfaceLevel)
(
    X_D3DTexture   *pThis,
    XDK::UINT            Level,
    X_D3DSurface  **ppSurfaceLevel
);

// ******************************************************************
// * patch: IDirect3DVolumeTexture8_LockBox
// ******************************************************************
VOID WINAPI EMUPATCH(D3DVolumeTexture_LockBox)
(
    X_D3DVolumeTexture *pThis,
    XDK::UINT                Level,
	Native::D3DLOCKED_BOX      *pLockedVolume,
    CONST Native::D3DBOX       *pBox,
    XDK::DWORD               Flags
);

// ******************************************************************
// * patch: IDirect3DCubeTexture8_LockRect
// ******************************************************************
VOID WINAPI EMUPATCH(D3DCubeTexture_LockRect)
(
    X_D3DCubeTexture   *pThis,
	Native::D3DCUBEMAP_FACES    FaceType,
    XDK::UINT                Level,
	Native::D3DLOCKED_RECT     *pLockedBox,
    CONST Native::RECT         *pRect,
    XDK::DWORD               Flags
);

#if 0 // patch disabled
// ******************************************************************
// * patch: D3DDevice_CreateVertexBuffer
// ******************************************************************
XDK::HRESULT WINAPI EMUPATCH(D3DDevice_CreateVertexBuffer)
(
    XDK::UINT                Length,
    XDK::DWORD               Usage,
    XDK::DWORD               FVF,
	Native::D3DPOOL             Pool,
    X_D3DVertexBuffer **ppVertexBuffer
);
#endif

#if 0 // patch disabled
// ******************************************************************
// * patch: D3DDevice_CreateVertexBuffer2
// ******************************************************************
X_D3DVertexBuffer* WINAPI EMUPATCH(D3DDevice_CreateVertexBuffer2)
(
    XDK::UINT Length
);
#endif

// ******************************************************************
// * patch: D3DDevice_EnableOverlay
// ******************************************************************
VOID WINAPI EMUPATCH(D3DDevice_EnableOverlay)
(
    XDK::BOOL Enable
);

// ******************************************************************
// * patch: D3DDevice_UpdateOverlay
// ******************************************************************
VOID WINAPI EMUPATCH(D3DDevice_UpdateOverlay)
(
    X_D3DSurface *pSurface,
    CONST Native::RECT   *SrcRect,
    CONST Native::RECT   *DstRect,
    XDK::BOOL          EnableColorKey,
	Native::D3DCOLOR      ColorKey
);

// ******************************************************************
// * patch: D3DDevice_GetOverlayUpdateStatus
// ******************************************************************
XDK::BOOL WINAPI EMUPATCH(D3DDevice_GetOverlayUpdateStatus)();

// ******************************************************************
// * patch: D3DDevice_BlockUntilVerticalBlank
// ******************************************************************
VOID WINAPI EMUPATCH(D3DDevice_BlockUntilVerticalBlank)();

// ******************************************************************
// * patch: D3DDevice_SetVerticalBlankCallback
// ******************************************************************
VOID WINAPI EMUPATCH(D3DDevice_SetVerticalBlankCallback)
(
    D3DVBLANKCALLBACK pCallback
);

// ******************************************************************
// * patch: D3DDevice_SetTextureState_TexCoordIndex
// ******************************************************************
VOID WINAPI EMUPATCH(D3DDevice_SetTextureState_TexCoordIndex)
(
    XDK::DWORD Stage,
    XDK::DWORD Value
);

// ******************************************************************
// * patch: D3DDevice_SetRenderState_TwoSidedLighting
// ******************************************************************
VOID WINAPI EMUPATCH(D3DDevice_SetRenderState_TwoSidedLighting)
(
    XDK::DWORD Value
);

// ******************************************************************
// * patch: D3DDevice_SetRenderState_BackFillMode
// ******************************************************************
VOID WINAPI EMUPATCH(D3DDevice_SetRenderState_BackFillMode)
(
    XDK::DWORD Value
);

// ******************************************************************
// * patch: D3DDevice_SetTextureState_BorderColor
// ******************************************************************
VOID WINAPI EMUPATCH(D3DDevice_SetTextureState_BorderColor)
(
    XDK::DWORD Stage,
    XDK::DWORD Value
);

// ******************************************************************
// * patch: D3DDevice_SetTextureState_ColorKeyColor
// ******************************************************************
VOID WINAPI EMUPATCH(D3DDevice_SetTextureState_ColorKeyColor)
(
    XDK::DWORD Stage,
    XDK::DWORD Value
);

// ******************************************************************
// * patch: D3DDevice_SetTextureState_BumpEnv
// ******************************************************************
VOID WINAPI EMUPATCH(D3DDevice_SetTextureState_BumpEnv)
(
    XDK::DWORD                      Stage,
    X_D3DTEXTURESTAGESTATETYPE Type,
    XDK::DWORD                      Value
);

// ******************************************************************
// * patch: D3DDevice_SetRenderState_FrontFace
// ******************************************************************
VOID WINAPI EMUPATCH(D3DDevice_SetRenderState_FrontFace)
(
    XDK::DWORD Value
);

// ******************************************************************
// * patch: D3DDevice_SetRenderState_LogicOp
// ******************************************************************
VOID WINAPI EMUPATCH(D3DDevice_SetRenderState_LogicOp)
(
    XDK::DWORD Value
);

// ******************************************************************
// * patch: D3DDevice_SetRenderState_NormalizeNormals
// ******************************************************************
VOID WINAPI EMUPATCH(D3DDevice_SetRenderState_NormalizeNormals)
(
    XDK::DWORD Value
);

// ******************************************************************
// * patch: D3DDevice_SetRenderState_TextureFactor
// ******************************************************************
VOID WINAPI EMUPATCH(D3DDevice_SetRenderState_TextureFactor)
(
    XDK::DWORD Value
);

// ******************************************************************
// * patch: D3DDevice_SetRenderState_ZBias
// ******************************************************************
VOID WINAPI EMUPATCH(D3DDevice_SetRenderState_ZBias)
(
    XDK::DWORD Value
);

// ******************************************************************
// * patch: D3DDevice_SetRenderState_EdgeAntiAlias
// ******************************************************************
VOID WINAPI EMUPATCH(D3DDevice_SetRenderState_EdgeAntiAlias)
(
    XDK::DWORD Value
);

// ******************************************************************
// * patch: D3DDevice_SetRenderState_FillMode
// ******************************************************************
VOID WINAPI EMUPATCH(D3DDevice_SetRenderState_FillMode)
(
    XDK::DWORD Value
);

// ******************************************************************
// * patch: D3DDevice_SetRenderState_FogColor
// ******************************************************************
VOID WINAPI EMUPATCH(D3DDevice_SetRenderState_FogColor)
(
    XDK::DWORD Value
);

// ******************************************************************
// * patch: D3DDevice_SetRenderState_Dxt1NoiseEnable
// ******************************************************************
VOID WINAPI EMUPATCH(D3DDevice_SetRenderState_Dxt1NoiseEnable)
(
    XDK::DWORD Value
);

// ******************************************************************
// * patch: D3DDevice_SetRenderState_Simple
// ******************************************************************
VOID __fastcall EMUPATCH(D3DDevice_SetRenderState_Simple)
(
    XDK::DWORD Method,
    XDK::DWORD Value
);

// ******************************************************************
// * patch: D3DDevice_SetRenderState_VertexBlend
// ******************************************************************
VOID WINAPI EMUPATCH(D3DDevice_SetRenderState_VertexBlend)
(
    XDK::DWORD Value
);

// ******************************************************************
// * patch: D3DDevice_SetRenderState_PSTextureModes
// ******************************************************************
VOID WINAPI EMUPATCH(D3DDevice_SetRenderState_PSTextureModes)
(
    XDK::DWORD Value
);

// ******************************************************************
// * patch: D3DDevice_SetRenderState_CullMode
// ******************************************************************
VOID WINAPI EMUPATCH(D3DDevice_SetRenderState_CullMode)
(
    XDK::DWORD Value
);

// ******************************************************************
// * patch: D3DDevice_SetRenderState_LineWidth
// ******************************************************************
VOID WINAPI EMUPATCH(D3DDevice_SetRenderState_LineWidth)
(
    XDK::DWORD Value
);

// ******************************************************************
// * patch: D3DDevice_SetRenderState_StencilFail
// ******************************************************************
VOID WINAPI EMUPATCH(D3DDevice_SetRenderState_StencilFail)
(
    XDK::DWORD Value
);

// ******************************************************************
// * patch: D3DDevice_SetRenderState_OcclusionCullEnable
// ******************************************************************
VOID WINAPI EMUPATCH(D3DDevice_SetRenderState_OcclusionCullEnable)
(
    XDK::DWORD Value
);

// ******************************************************************
// * patch: D3DDevice_SetRenderState_StencilCullEnable
// ******************************************************************
VOID WINAPI EMUPATCH(D3DDevice_SetRenderState_StencilCullEnable)
(
    XDK::DWORD Value
);

// ******************************************************************
// * patch: D3DDevice_SetRenderState_RopZCmpAlwaysRead
// ******************************************************************
VOID WINAPI EMUPATCH(D3DDevice_SetRenderState_RopZCmpAlwaysRead)
(
    XDK::DWORD Value
);

// ******************************************************************
// * patch: D3DDevice_SetRenderState_RopZRead
// ******************************************************************
VOID WINAPI EMUPATCH(D3DDevice_SetRenderState_RopZRead)
(
    XDK::DWORD Value
);

// ******************************************************************
// * patch: D3DDevice_SetRenderState_DoNotCullUncompressed
// ******************************************************************
VOID WINAPI EMUPATCH(D3DDevice_SetRenderState_DoNotCullUncompressed)
(
    XDK::DWORD Value
);

// ******************************************************************
// * patch: D3DDevice_SetRenderState_ZEnable
// ******************************************************************
VOID WINAPI EMUPATCH(D3DDevice_SetRenderState_ZEnable)
(
    XDK::DWORD Value
);

// ******************************************************************
// * patch: D3DDevice_SetRenderState_StencilEnable
// ******************************************************************
VOID WINAPI EMUPATCH(D3DDevice_SetRenderState_StencilEnable)
(
    XDK::DWORD Value
);

// ******************************************************************
// * patch: D3DDevice_SetRenderState_MultiSampleMask
// ******************************************************************
VOID WINAPI EMUPATCH(D3DDevice_SetRenderState_MultiSampleMask)
(
    XDK::DWORD Value
);

// ******************************************************************
// * patch: D3DDevice_SetRenderState_MultiSampleMode
// ******************************************************************
VOID WINAPI EMUPATCH(D3DDevice_SetRenderState_MultiSampleMode)
(
    XDK::DWORD Value
);

// ******************************************************************
// * patch: D3DDevice_SetRenderState_MultiSampleRenderTargetMode
// ******************************************************************
VOID WINAPI EMUPATCH(D3DDevice_SetRenderState_MultiSampleRenderTargetMode)
(
    XDK::DWORD Value
);

// ******************************************************************
// * patch: D3DDevice_SetRenderState_MultiSampleAntiAlias
// ******************************************************************
VOID WINAPI EMUPATCH(D3DDevice_SetRenderState_MultiSampleAntiAlias)
(
    XDK::DWORD Value
);

// ******************************************************************
// * patch: D3DDevice_SetRenderState_ShadowFunc
// ******************************************************************
VOID WINAPI EMUPATCH(D3DDevice_SetRenderState_ShadowFunc)
(
    XDK::DWORD Value
);

// ******************************************************************
// * patch: D3DDevice_SetRenderState_YuvEnable
// ******************************************************************
VOID WINAPI EMUPATCH(D3DDevice_SetRenderState_YuvEnable)
(
	XDK::BOOL Enable
);

// ******************************************************************
// * patch: D3DDevice_SetTransform
// ******************************************************************
VOID WINAPI EMUPATCH(D3DDevice_SetTransform)
(
	Native::D3DTRANSFORMSTATETYPE State,
    CONST Native::D3DMATRIX      *pMatrix
);

// ******************************************************************
// * patch: D3DDevice_GetTransform
// ******************************************************************
VOID WINAPI EMUPATCH(D3DDevice_GetTransform)
(
	Native::D3DTRANSFORMSTATETYPE State,
	Native::D3DMATRIX            *pMatrix
);

// ******************************************************************
// * patch: IDirect3DVertexBuffer8_Lock
// ******************************************************************
VOID WINAPI EMUPATCH(D3DVertexBuffer_Lock)
(
    X_D3DVertexBuffer   *pVertexBuffer,
    XDK::UINT                OffsetToLock,
    XDK::UINT                SizeToLock,
    XDK::BYTE              **ppbData,
    XDK::DWORD               Flags
);

// ******************************************************************
// * patch: IDirect3DVertexBuffer8_Lock2
// ******************************************************************
XDK::BYTE* WINAPI EMUPATCH(D3DVertexBuffer_Lock2)
(
    X_D3DVertexBuffer  *pVertexBuffer,
    XDK::DWORD               Flags
);

// ******************************************************************
// * patch: D3DDevice_GetStreamSource2
// ******************************************************************
X_D3DVertexBuffer* WINAPI EMUPATCH(D3DDevice_GetStreamSource2)
(
    XDK::UINT  StreamNumber,
    XDK::UINT *pStride
);

// ******************************************************************
// * patch: D3DDevice_SetStreamSource
// ******************************************************************
VOID WINAPI EMUPATCH(D3DDevice_SetStreamSource)
(
	XDK::UINT                StreamNumber,
    X_D3DVertexBuffer  *pStreamData,
	XDK::UINT                Stride
);

// ******************************************************************
// * patch: D3DDevice_SetVertexShader
// ******************************************************************
VOID WINAPI EMUPATCH(D3DDevice_SetVertexShader)
(
    XDK::DWORD            Handle
);

// ******************************************************************
// * patch: D3DDevice_DrawVertices
// ******************************************************************
VOID WINAPI EMUPATCH(D3DDevice_DrawVertices)
(
    X_D3DPRIMITIVETYPE  PrimitiveType,
    XDK::UINT                StartVertex,
    XDK::UINT                VertexCount
);

// ******************************************************************
// * patch: D3DDevice_DrawVerticesUP
// ******************************************************************
VOID WINAPI EMUPATCH(D3DDevice_DrawVerticesUP)
(
    X_D3DPRIMITIVETYPE  PrimitiveType,
    XDK::UINT                VertexCount,
    CONST XDK::PVOID         pVertexStreamZeroData,
    XDK::UINT                VertexStreamZeroStride
);

// ******************************************************************
// * patch: D3DDevice_DrawIndexedVertices
// ******************************************************************
VOID WINAPI EMUPATCH(D3DDevice_DrawIndexedVertices)
(
    X_D3DPRIMITIVETYPE  PrimitiveType,
    XDK::UINT                VertexCount,
    CONST Native::PWORD         pIndexData
);

// ******************************************************************
// * patch: D3DDevice_DrawIndexedVerticesUP
// ******************************************************************
VOID WINAPI EMUPATCH(D3DDevice_DrawIndexedVerticesUP)
(
    X_D3DPRIMITIVETYPE  PrimitiveType,
    XDK::UINT                VertexCount,
    CONST XDK::PVOID         pIndexData,
    CONST XDK::PVOID         pVertexStreamZeroData,
    XDK::UINT                VertexStreamZeroStride
);

// ******************************************************************
// * patch: D3DDevice_GetLight
// ******************************************************************
VOID WINAPI EMUPATCH(D3DDevice_GetLight)
(
    XDK::DWORD            Index,
	Native::D3DLIGHT8       *pLight
);

// ******************************************************************
// * patch: D3DDevice_SetLight
// ******************************************************************
XDK::HRESULT WINAPI EMUPATCH(D3DDevice_SetLight)
(
    XDK::DWORD            Index,
    CONST Native::D3DLIGHT8 *pLight
);

// ******************************************************************
// * patch: D3DDevice_SetMaterial
// ******************************************************************
VOID WINAPI EMUPATCH(D3DDevice_SetMaterial)
(
    CONST Native::D3DMATERIAL8 *pMaterial
);

// ******************************************************************
// * patch: D3DDevice_LightEnable
// ******************************************************************
XDK::HRESULT WINAPI EMUPATCH(D3DDevice_LightEnable)
(
    XDK::DWORD            Index,
    XDK::BOOL             bEnable
);

// ******************************************************************
// * patch: D3DDevice_Release
// ******************************************************************
XDK::ULONG WINAPI EMUPATCH(D3DDevice_Release)();

// ******************************************************************
// * patch: D3DDevice_CreatePalette
// ******************************************************************
XDK::HRESULT WINAPI EMUPATCH(D3DDevice_CreatePalette)
(
    X_D3DPALETTESIZE    Size,
    X_D3DPalette      **ppPalette
);

// ******************************************************************
// * patch: D3DDevice_CreatePalette2
// ******************************************************************
X_D3DPalette * WINAPI EMUPATCH(D3DDevice_CreatePalette2)
(
    X_D3DPALETTESIZE    Size
);

// ******************************************************************
// * patch: D3DDevice_SetRenderTarget
// ******************************************************************
VOID WINAPI EMUPATCH(D3DDevice_SetRenderTarget)
(
    X_D3DSurface    *pRenderTarget,
    X_D3DSurface    *pNewZStencil
);

// ******************************************************************
// * patch: D3DDevice_SetPalette
// ******************************************************************
VOID WINAPI EMUPATCH(D3DDevice_SetPalette)
(
    XDK::DWORD         Stage,
    X_D3DPalette *pPalette
);

// ******************************************************************
// * patch: D3DDevice_SetFlickerFilter
// ******************************************************************
void WINAPI EMUPATCH(D3DDevice_SetFlickerFilter)
(
    XDK::DWORD         Filter
);

// ******************************************************************
// * patch: D3DDevice_SetSoftDisplayFilter
// ******************************************************************
void WINAPI EMUPATCH(D3DDevice_SetSoftDisplayFilter)
(
	XDK::BOOL Enable
);

// ******************************************************************
// * patch: IDirect3DPalette8_Lock
// ******************************************************************
VOID WINAPI EMUPATCH(D3DPalette_Lock)
(
    X_D3DPalette   *pThis,
	Native::D3DCOLOR      **ppColors,
    XDK::DWORD           Flags
);

// ******************************************************************
// * patch: IDirect3DPalette8_Lock2
// ******************************************************************
Native::D3DCOLOR * WINAPI EMUPATCH(D3DPalette_Lock2)
(
    X_D3DPalette   *pThis,
    XDK::DWORD           Flags
);

// ******************************************************************
// * patch: D3DDevice_GetVertexShaderSize
// ******************************************************************
VOID WINAPI EMUPATCH(D3DDevice_GetVertexShaderSize)
(
    XDK::DWORD Handle,
	XDK::PUINT pSize
);

// ******************************************************************
// * patch: D3DDevice_DeleteVertexShader
// ******************************************************************
VOID WINAPI EMUPATCH(D3DDevice_DeleteVertexShader)
(
    XDK::DWORD Handle
);

// ******************************************************************
// * patch: D3DDevice_SelectVertexShaderDirect
// ******************************************************************
VOID WINAPI EMUPATCH(D3DDevice_SelectVertexShaderDirect)
(
    X_VERTEXATTRIBUTEFORMAT *pVAF,
    XDK::DWORD                    Address
);

// ******************************************************************
// * patch: D3DDevice_GetShaderConstantMode
// ******************************************************************
VOID WINAPI EMUPATCH(D3DDevice_GetShaderConstantMode)
(
    XDK::DWORD *pMode
);

// ******************************************************************
// * patch: D3DDevice_GetVertexShader
// ******************************************************************
VOID WINAPI EMUPATCH(D3DDevice_GetVertexShader)
(
    XDK::DWORD *pHandle
);

// ******************************************************************
// * patch: D3DDevice_GetVertexShaderConstant
// ******************************************************************
VOID WINAPI EMUPATCH(D3DDevice_GetVertexShaderConstant)
(
	XDK::INT   Register,
    void  *pConstantData,
    XDK::DWORD ConstantCount
);

// ******************************************************************
// * patch: D3DDevice_SetVertexShaderInputDirect
// ******************************************************************
VOID WINAPI EMUPATCH(D3DDevice_SetVertexShaderInputDirect)
(
    X_VERTEXATTRIBUTEFORMAT *pVAF,
    XDK::UINT                     StreamCount,
    X_STREAMINPUT           *pStreamInputs
);

// ******************************************************************
// * patch: D3DDevice_GetVertexShaderInput
// ******************************************************************
XDK::HRESULT WINAPI EMUPATCH(D3DDevice_GetVertexShaderInput)
(
    XDK::DWORD              *pHandle,
    XDK::UINT               *pStreamCount,
    X_STREAMINPUT      *pStreamInputs
);

// ******************************************************************
// * patch: D3DDevice_GetVertexShaderInput
// ******************************************************************
VOID WINAPI EMUPATCH(D3DDevice_SetVertexShaderInput)
(
    XDK::DWORD              Handle,
    XDK::UINT               StreamCount,
    X_STREAMINPUT     *pStreamInputs
);

// ******************************************************************
// * patch: D3DDevice_RunVertexStateShader
// ******************************************************************
VOID WINAPI EMUPATCH(D3DDevice_RunVertexStateShader)
(
    XDK::DWORD        Address,
    CONST Native::FLOAT *pData
);

// ******************************************************************
// * patch: D3DDevice_LoadVertexShaderProgram
// ******************************************************************
VOID WINAPI EMUPATCH(D3DDevice_LoadVertexShaderProgram)
(
    CONST XDK::DWORD *pFunction,
    XDK::DWORD        Address
);

// ******************************************************************
// * patch: D3DDevice_GetVertexShaderType
// ******************************************************************
VOID WINAPI EMUPATCH(D3DDevice_GetVertexShaderType)
(
    XDK::DWORD  Handle,
    XDK::DWORD *pType
);

// ******************************************************************
// * patch: D3DDevice_GetVertexShaderDeclaration
// ******************************************************************
XDK::HRESULT WINAPI EMUPATCH(D3DDevice_GetVertexShaderDeclaration)
(
    XDK::DWORD  Handle,
	XDK::PVOID  pData,
    XDK::DWORD *pSizeOfData
);

// ******************************************************************
// * patch: D3DDevice_GetVertexShaderFunction
// ******************************************************************
XDK::HRESULT WINAPI EMUPATCH(D3DDevice_GetVertexShaderFunction)
(
    XDK::DWORD  Handle,
	XDK::PVOID *pData,
    XDK::DWORD *pSizeOfData
);

// ******************************************************************
// * patch: D3DDevice_SetDepthClipPlanes
// ******************************************************************
XDK::HRESULT WINAPI EMUPATCH(D3DDevice_SetDepthClipPlanes)
(
    Native::FLOAT Near,
    Native::FLOAT Far,
    XDK::DWORD Flags
);

#if 0 // DISABLED (Just calls MmAllocateContiguousMemory)
// ******************************************************************
// * patch: D3D_AllocContiguousMemory
// ******************************************************************
PVOID WINAPI EMUPATCH(D3D_AllocContiguousMemory)
(
    SIZE_T dwSize,
    XDK::DWORD dwAllocAttributes
);
#endif

#if 0 // DISABLED (Just calls Get2DSurfaceDesc)
// ******************************************************************
// * patch: D3DTexture_GetLevelDesc
// ******************************************************************
XDK::HRESULT WINAPI EMUPATCH(D3DTexture_GetLevelDesc)
(
    XDK::UINT Level,
    X_D3DSURFACE_DESC* pDesc
);
#endif

#if 0 // patch disabled
// ******************************************************************
// * patch: Direct3D_CheckDeviceMultiSampleType
// ******************************************************************
XDK::HRESULT WINAPI EMUPATCH(Direct3D_CheckDeviceMultiSampleType)
(
    XDK::UINT                 Adapter,
    D3DDEVTYPE           DeviceType,
    X_D3DFORMAT          SurfaceFormat,
	XDK::BOOL                 Windowed,
    D3DMULTISAMPLE_TYPE  MultiSampleType
);
#endif

#if 0 // patch disabled
// ******************************************************************
// * patch: D3D_GetDeviceCaps
// ******************************************************************
XDK::HRESULT WINAPI EMUPATCH(D3D_GetDeviceCaps)
(
    XDK::UINT        Adapter,
    Native::D3DDEVTYPE  DeviceType,
    Native::D3DCAPS8    *pCaps
);
#endif

#if 0 // patch disabled
// ******************************************************************
// * patch: D3D_SetPushBufferSize
// ******************************************************************
XDK::HRESULT WINAPI EMUPATCH(D3D_SetPushBufferSize)
(
    XDK::DWORD PushBufferSize,
    XDK::DWORD KickOffSize
);
#endif

// ******************************************************************
// * patch: D3DDevice_InsertFence
// ******************************************************************
XDK::DWORD WINAPI EMUPATCH(D3DDevice_InsertFence)();

// ******************************************************************
// * patch: D3DDevice_IsFencePending
// ******************************************************************
XDK::BOOL WINAPI EMUPATCH(D3DDevice_IsFencePending)
(
    XDK::DWORD Fence
);

// ******************************************************************
// * patch: D3DDevice_BlockOnFence
// ******************************************************************
VOID WINAPI EMUPATCH(D3DDevice_BlockOnFence)
(
    XDK::DWORD Fence
);

// ******************************************************************
// * patch: D3DResource_BlockUntilNotBusy
// ******************************************************************
VOID WINAPI EMUPATCH(D3DResource_BlockUntilNotBusy)
(
    X_D3DResource *pThis
);

#if 0 // patch DISABLED
// ******************************************************************
// * patch: D3DVertexBuffer_GetDesc
// ******************************************************************
VOID WINAPI EMUPATCH(D3DVertexBuffer_GetDesc)
(
    X_D3DVertexBuffer    *pThis,
    X_D3DVERTEXBUFFER_DESC *pDesc
);
#endif

// ******************************************************************
// * patch: D3DDevice_SetScissors
// ******************************************************************
VOID WINAPI EMUPATCH(D3DDevice_SetScissors)
(
    XDK::DWORD          Count,
	XDK::BOOL           Exclusive,
    CONST Native::D3DRECT  *pRects
);

// ******************************************************************
// * patch: D3DDevice_SetScreenSpaceOffset
// ******************************************************************
VOID WINAPI EMUPATCH(D3DDevice_SetScreenSpaceOffset)
(
    Native::FLOAT x,
    Native::FLOAT y
);

// ******************************************************************
// * patch: D3DDevice_SetPixelShaderProgram
// ******************************************************************
VOID WINAPI EMUPATCH(D3DDevice_SetPixelShaderProgram)
(
	X_D3DPIXELSHADERDEF *pPSDef
);

// ******************************************************************
// * patch: D3DDevice_CreateStateBlock
// ******************************************************************
XDK::HRESULT WINAPI EMUPATCH(D3DDevice_CreateStateBlock)
(
	Native::D3DSTATEBLOCKTYPE Type,
	XDK::DWORD			  *pToken
);

// ******************************************************************
// * patch: D3DDevice_InsertCallback
// ******************************************************************
VOID WINAPI EMUPATCH(D3DDevice_InsertCallback)
(
	X_D3DCALLBACKTYPE	Type,
	X_D3DCALLBACK		pCallback,
	XDK::DWORD				Context
);

// ******************************************************************
// * patch: D3DDevice_DrawRectPatch
// ******************************************************************
XDK::HRESULT WINAPI EMUPATCH(D3DDevice_DrawRectPatch)
(
	XDK::UINT					Handle,
	CONST Native::FLOAT				*pNumSegs,
	CONST Native::D3DRECTPATCH_INFO *pRectPatchInfo
);

// ******************************************************************
// * patch: D3DDevice_GetProjectionViewportMatrix
// ******************************************************************
VOID WINAPI EMUPATCH(D3DDevice_GetProjectionViewportMatrix)
(
	Native::D3DXMATRIX *pProjectionViewport
);

// ******************************************************************
// * patch: D3DDevice_KickOff (D3D::CDevice::KickOff)
// ******************************************************************
VOID WINAPI EMUPATCH(D3DDevice_KickOff)();

// ******************************************************************
// * patch: D3DDevice_KickPushBuffer
// ******************************************************************
VOID WINAPI EMUPATCH(D3DDevice_KickPushBuffer)();

// ******************************************************************
// * patch: D3DDevice_GetTexture2
// ******************************************************************
X_D3DBaseTexture* WINAPI EMUPATCH(D3DDevice_GetTexture2)(XDK::DWORD Stage);

// ******************************************************************
// * patch: D3DDevice_GetTexture
// ******************************************************************
VOID WINAPI EMUPATCH(D3DDevice_GetTexture)
(
	XDK::DWORD           Stage,
	X_D3DBaseTexture  **pTexture
);

// ******************************************************************
// * patch: D3DDevice_SetStateVB (D3D::CDevice::SetStateVB)
// ******************************************************************
VOID WINAPI EMUPATCH(D3DDevice_SetStateVB)(XDK::ULONG Unknown1 );

// ******************************************************************
// * patch: D3DDevice_SetStateUP (D3D::CDevice::SetStateUP)
// ******************************************************************
VOID WINAPI EMUPATCH(D3DDevice_SetStateUP)();

// ******************************************************************
// * patch: D3DDevice_SetStipple
// ******************************************************************
void WINAPI EMUPATCH(D3DDevice_SetStipple)(XDK::DWORD* pPattern );

// ******************************************************************
// * patch: D3DDevice_SetSwapCallback
// ******************************************************************
void WINAPI EMUPATCH(D3DDevice_SetSwapCallback)
(
	D3DSWAPCALLBACK		pCallback
);

// ******************************************************************
// * patch: D3DDevice_PersistDisplay
// ******************************************************************
XDK::HRESULT WINAPI EMUPATCH(D3DDevice_PersistDisplay)();

// ******************************************************************
// * patch: D3DDevice_GetPersistedSurface
// ******************************************************************
VOID WINAPI EMUPATCH(D3DDevice_GetPersistedSurface)(X_D3DSurface **ppSurface);
X_D3DSurface* WINAPI EMUPATCH(D3DDevice_GetPersistedSurface2)();

// ******************************************************************
// * patch: D3D_CMiniport_GetDisplayCapabilities
// ******************************************************************
XDK::DWORD WINAPI EMUPATCH(D3D_CMiniport_GetDisplayCapabilities)();

// ******************************************************************
// * patch: D3DDevice_PrimeVertexCache
// ******************************************************************
VOID WINAPI EMUPATCH(D3DDevice_PrimeVertexCache)
(
	XDK::UINT VertexCount,
	XDK::WORD *pIndexData
);

// ******************************************************************
// * patch: D3DDevice_SetRenderState_SampleAlpha
// ******************************************************************
XDK::HRESULT WINAPI EMUPATCH(D3DDevice_SetRenderState_SampleAlpha)
(
	XDK::DWORD dwSampleAlpha
);

// ******************************************************************
// * patch: D3DDevice_SetRenderState_Deferred
// ******************************************************************
VOID __fastcall EMUPATCH(D3DDevice_SetRenderState_Deferred)
(
	XDK::DWORD State,
	XDK::DWORD Value
);

// ******************************************************************
// * patch: D3DDevice_DeleteStateBlock
// ******************************************************************
VOID WINAPI EMUPATCH(D3DDevice_DeleteStateBlock)
(
	XDK::DWORD Token
);

// ******************************************************************
// * patch: D3DDevice_SetModelView
// ******************************************************************
VOID WINAPI EMUPATCH(D3DDevice_SetModelView)
(
	CONST Native::D3DMATRIX *pModelView, 
	CONST Native::D3DMATRIX *pInverseModelView, 
	CONST Native::D3DMATRIX *pComposite
);

// ******************************************************************
// * patch: D3DDevice_FlushVertexCache
// ******************************************************************
void WINAPI EMUPATCH(D3DDevice_FlushVertexCache)();

// ******************************************************************
// * patch: D3DDevice_BeginPushBuffer
// ******************************************************************
VOID WINAPI EMUPATCH(D3DDevice_BeginPushBuffer)
(
	X_D3DPushBuffer *pPushBuffer
);

// ******************************************************************
// * patch: D3DDevice_EndPushBuffer
// ******************************************************************
XDK::HRESULT WINAPI EMUPATCH(D3DDevice_EndPushBuffer)();

// ******************************************************************
// * patch: XMETAL_StartPush
// ******************************************************************
XDK::PDWORD WINAPI EMUPATCH(XMETAL_StartPush)(void* Unknown);

// ******************************************************************
// * patch: D3DDevice_GetModelView
// ******************************************************************
XDK::HRESULT WINAPI EMUPATCH(D3DDevice_GetModelView)(Native::D3DXMATRIX* pModelView);

// ******************************************************************
// * patch: D3DDevice_SetBackMaterial
// ******************************************************************
VOID WINAPI EMUPATCH(D3DDevice_SetBackMaterial)(Native::D3DMATERIAL8* pMaterial);

#if 0 // patch disabled
// ******************************************************************
// * patch: D3D_GetAdapterIdentifier
// ******************************************************************
XDK::HRESULT WINAPI EMUPATCH(D3D_GetAdapterIdentifier)
(
	XDK::UINT					Adapter,
	XDK::DWORD					Flags,
	D3DADAPTER_IDENTIFIER8* pIdentifier
);
#endif

// ******************************************************************
// * patch: D3D::MakeRequestedSpace
// ******************************************************************
XDK::PDWORD WINAPI EMUPATCH(D3D_MakeRequestedSpace)
(
	XDK::DWORD MinimumSpace,
	XDK::DWORD RequestedSpace
);

// ******************************************************************
// * patch: D3DDevice_MakeSpace
// ******************************************************************
void WINAPI EMUPATCH(D3DDevice_MakeSpace)();

// ******************************************************************
// * patch: D3D_SetCommonDebugRegisters
// ******************************************************************
void WINAPI EMUPATCH(D3D_SetCommonDebugRegisters)();

// ******************************************************************
// * patch: D3D_BlockOnTime
// ******************************************************************
void WINAPI EMUPATCH(D3D_BlockOnTime)(XDK::DWORD Unknown1, int Unknown2 );

// ******************************************************************
// * patch: D3D_BlockOnResource
// ******************************************************************
void WINAPI EMUPATCH(D3D_BlockOnResource)( X_D3DResource* pResource );

// ******************************************************************
// * patch: D3DDevice_GetPushBufferOffset
// ******************************************************************
VOID WINAPI EMUPATCH(D3DDevice_GetPushBufferOffset)
(
	XDK::DWORD *pOffset
);

// ******************************************************************
// * patch: IDirect3DCubeTexture8_GetCubeMapSurface
// ******************************************************************
XDK::HRESULT WINAPI EMUPATCH(D3DCubeTexture_GetCubeMapSurface)
(
	X_D3DCubeTexture*	pThis,
	Native::D3DCUBEMAP_FACES	FaceType,
	XDK::UINT				Level,
	X_D3DSurface**		ppCubeMapSurface
);

// ******************************************************************
// * patch: IDirect3DCubeTexture8_GetCubeMapSurface2
// ******************************************************************
X_D3DSurface* WINAPI EMUPATCH(D3DCubeTexture_GetCubeMapSurface2)
(
	X_D3DCubeTexture*	pThis,
	Native::D3DCUBEMAP_FACES	FaceType,
	XDK::UINT				Level
);

// ******************************************************************
// * patch: D3DDevice_GetPixelShader
// ******************************************************************
VOID WINAPI EMUPATCH(D3DDevice_GetPixelShader)
(
	XDK::DWORD  Name,
	XDK::DWORD* pHandle
);

// ******************************************************************
// * patch: D3DDevice_SetRenderTargetFast
// ******************************************************************
VOID WINAPI EMUPATCH(D3DDevice_SetRenderTargetFast)
(
    X_D3DSurface	*pRenderTarget,
    X_D3DSurface	*pNewZStencil,
	XDK::DWORD			Flags
);

// ******************************************************************
// * patch: D3DDevice_GetScissors
// ******************************************************************
VOID WINAPI EMUPATCH(D3DDevice_GetScissors)
(
	XDK::DWORD	*pCount, 
	XDK::BOOL	*pExclusive, 
	Native::D3DRECT *pRects
);
// ******************************************************************
// * patch: D3DDevice_GetBackMaterial
// ******************************************************************
VOID WINAPI EMUPATCH(D3DDevice_GetBackMaterial)(Native::D3DMATERIAL8* pMaterial);

// ******************************************************************
// * patch: D3D::LazySetPointParams
// ******************************************************************
void WINAPI EMUPATCH(D3D_LazySetPointParams)( void* Device );

// ******************************************************************
// * patch: D3DDevice_GetMaterial
// ******************************************************************
VOID WINAPI EMUPATCH(D3DDevice_GetMaterial)(Native::D3DMATERIAL8* pMaterial);

} // Xbox

#endif
