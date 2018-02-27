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
// *   Cxbx->Win32->CxbxKrnl->EmuD3D8.cpp
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
#define _XBOXKRNL_DEFEXTRN_
#include "xxhash32.h"
#include <condition_variable>

// prevent name collisions
namespace xboxkrnl
{
    #include <xboxkrnl/xboxkrnl.h>
};

#include "CxbxUtil.h"
#include "CxbxVersion.h"
#include "CxbxKrnl.h"
#include "Emu.h"
#include "EmuFS.h"
#include "EmuShared.h"
#include "DbgConsole.h"
#include "ResourceTracker.h"
#include "VMManager.h" // for g_VMManager
#include "EmuXTL.h"
#include "HLEDatabase.h"
#include "Logging.h"
#include "EmuD3D8Logging.h"
#include "HLEIntercept.h" // for bLLE_GPU
#include "Cxbx\\ResCxbx.h"

#include <assert.h>
#include <process.h>
#include <clocale>

// Allow use of time duration literals (making 16ms, etc possible)
using namespace std::literals::chrono_literals;

// This doesn't work : #include <dxerr8.h> // See DXGetErrorString8A below

// Global(s)
HWND                                g_hEmuWindow   = NULL; // rendering window
LPDIRECT3DDEVICE8              g_pD3DDevice8  = NULL; // Direct3D8 Device
LPDIRECTDRAWSURFACE7           g_pDDSPrimary  = NULL; // DirectDraw7 Primary Surface
LPDIRECTDRAWCLIPPER            g_pDDClipper   = nullptr; // DirectDraw7 Clipper
DWORD                               g_CurrentVertexShader = 0;
DWORD								g_dwCurrentPixelShader = 0;
PIXEL_SHADER                  *g_CurrentPixelShader = NULL;
BOOL                                g_bFakePixelShaderLoaded = FALSE;
BOOL                                g_bIsFauxFullscreen = FALSE;
BOOL								g_bHackUpdateSoftwareOverlay = FALSE;
DWORD								g_CurrentFillMode = D3DFILL_SOLID;	// Used to backup/restore the fill mode when WireFrame is enabled

// Static Function(s)
static BOOL WINAPI                  EmuEnumDisplayDevices(GUID FAR *lpGUID, LPSTR lpDriverDescription, LPSTR lpDriverName, LPVOID lpContext, HMONITOR hm);
static DWORD WINAPI                 EmuRenderWindow(LPVOID);
static DWORD WINAPI                 EmuCreateDeviceProxy(LPVOID);
static LRESULT WINAPI               EmuMsgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
static DWORD WINAPI                 EmuUpdateTickCount(LPVOID);
static inline void                  EmuVerifyResourceIsRegistered(X_D3DResource *pResource, DWORD dwSize);
static void                         EmuAdjustPower2(UINT *dwWidth, UINT *dwHeight);
static void							UpdateCurrentMSpFAndFPS(); // Used for benchmarking/fps count

// Static Variable(s)
static HMONITOR                     g_hMonitor      = NULL; // Handle to DirectDraw monitor
static bool                         g_bSupportsTextureFormat[X_D3DFMT_LIN_R8G8B8A8 + 1] = { false };// Does device support texture format?
static LPDIRECTDRAW7           g_pDD7          = NULL; // DirectDraw7
static DDCAPS                  g_DriverCaps          = { 0 };
static DWORD                        g_dwOverlayW    = 640;  // Cached Overlay Width
static DWORD                        g_dwOverlayH    = 480;  // Cached Overlay Height
static DWORD                        g_dwOverlayP    = 640;  // Cached Overlay Pitch
static HBRUSH                       g_hBgBrush      = NULL; // Background Brush
static volatile bool                g_bRenderWindowActive = false;
static XBVideo                      g_XBVideo;
static D3DVBLANKCALLBACK       g_pVBCallback   = NULL; // Vertical-Blank callback routine
static std::condition_variable		g_VBConditionVariable;	// Used in BlockUntilVerticalBlank
static std::mutex					g_VBConditionMutex;		// Used in BlockUntilVerticalBlank
static D3DSWAPCALLBACK			g_pSwapCallback = NULL;	// Swap/Present callback routine
static D3DCALLBACK				g_pCallback		= NULL;	// D3DDevice::InsertCallback routine
static X_D3DCALLBACKTYPE		g_CallbackType;			// Callback type
static DWORD						g_CallbackParam;		// Callback param
static BOOL                         g_bHasDepthStencil = FALSE;  // Does device have a Depth/Stencil Buffer?
static clock_t						g_DeltaTime = 0;			 // Used for benchmarking/fps count
static unsigned int					g_Frames = 0;				 // Used for benchmarking/fps count
//static DWORD						g_dwPrimPerFrame = 0;	// Number of primitives within one frame

// D3D based variables
static GUID                         g_ddguid;               // DirectDraw driver GUID
static LPDIRECT3D8             g_pD3D8 = NULL;			// Direct3D8
static D3DCAPS8                g_D3DCaps;              // Direct3D8 Caps

// wireframe toggle
static int                          g_iWireframe    = 0;

// build version
extern uint32						g_BuildVersion;

typedef uint64_t resource_key_t;

// resource caching for _Register
std::vector<resource_key_t> g_RegisteredResources;

// current active index buffer
static DWORD                        g_dwBaseVertexIndex = 0;// current active index buffer base index

// current active vertex stream
static X_D3DVertexBuffer      *g_pVertexBuffer = NULL; // current active vertex buffer
static IDirect3DVertexBuffer8 *g_pDummyBuffer = NULL;  // Dummy buffer, used to set unused stream sources with
static DWORD						g_dwLastSetStream = 0;	// The last stream set by D3DDevice::SetStreamSource

// current vertical blank information
static D3DVBLANKDATA			g_VBData = {0};
static DWORD                        g_VBLastSwap = 0;

// current swap information
static D3DSWAPDATA				g_SwapData = {0};
static DWORD						g_SwapLast = 0;

// cached Direct3D state variable(s)
static X_D3DSurface           *g_pCachedRenderTarget = NULL;
static X_D3DSurface           *g_pCachedDepthStencil = NULL;
static X_D3DSurface           *g_pCachedYuvSurface = NULL;
static BOOL                         g_fYuvEnabled = FALSE;
static DWORD                        g_dwVertexShaderUsage = 0;
static DWORD                        g_VertexShaderSlots[136];

// Active D3D Vertex Streams (and strides)
X_D3DVertexBuffer*g_D3DStreams[16];
UINT g_D3DStreamStrides[16];

// cached palette pointer
static PVOID g_pCurrentPalette[TEXTURE_STAGES] = { nullptr, nullptr, nullptr, nullptr };

static X_VERTEXSHADERCONSTANTMODE g_VertexShaderConstantMode = X_D3DSCM_192CONSTANTS;

// cached Direct3D tiles
X_D3DTILE EmuD3DTileCache[0x08] = {0};

// cached active texture
X_D3DBaseTexture *EmuD3DActiveTexture[TEXTURE_STAGES] = {0,0,0,0};


// information passed to the create device proxy thread
struct EmuD3D8CreateDeviceProxyData
{
    UINT                        Adapter;
    D3DDEVTYPE                  DeviceType;
    HWND                             hFocusWindow;
    DWORD                       BehaviorFlags;
    X_D3DPRESENT_PARAMETERS    *pPresentationParameters;
    IDirect3DDevice8          **ppReturnedDeviceInterface;
    volatile bool                    bReady;

    union
    {
        volatile HRESULT  hRet;
        volatile bool     bCreate;   // false : release
    };
}
g_EmuCDPD = {0};

#ifdef _DEBUG_TRACE

#define DEBUG_D3DRESULT(hRet, message) \
	do { \
		if (FAILED(hRet)) \
			if(g_bPrintfOn) \
				printf("%s : %s D3D error (0x%.08X: %s)\n", _logFuncPrefix.c_str(), message, hRet, D3DErrorString(hRet)); \
	} while (0)

#else

#define DEBUG_D3DRESULT(hRet, message) \
	do { \
		if (FAILED(hRet)) \
			if(g_bPrintfOn) \
				DbgPrintf("%s : %s D3D error (0x%.08X: %s)\n", __func__, message, hRet, D3DErrorString(hRet)); \
	} while (0)

#endif

// Declares an unpatched Xbox function trampoline, callable by name (with a 'XB_' prefix attached)
#define XB_trampoline(ret, conv, name, arguments) \
	typedef ret(conv *XB_##name##_t)arguments; \
	static XB_##name##_t XB_##name = (XB_##name##_t)GetXboxFunctionPointer(#name);

// TODO: This should be a D3DDevice structure
DWORD g_XboxD3DDevice[64 * ONE_KB / sizeof(DWORD)] = { 0 };

const char *CxbxGetErrorDescription(HRESULT hResult)
{
	// TODO : For D3D9, Use DXGetErrorDescription9(hResult) (requires another DLL though)
	// See : http://www.fairyengine.com/articles/dxmultiviews.htm
	// and : http://www.gamedev.net/community/forums/showfaq.asp?forum_id=10
	// and : http://www.gamedev.net/community/forums/topic.asp?topic_id=16157
	switch (hResult)
	{
	case D3DERR_INVALIDCALL: return "Invalid Call";
	case D3DERR_NOTAVAILABLE: return "Not Available";
	// case D3DERR_OUTOFVIDEOMEMORY: return "Out of Video Memory"; // duplicate of DDERR_OUTOFVIDEOMEMORY

	case D3D_OK: return "No error occurred.";
#if 0
	case D3DERR_BADMAJORVERSION: return "The service that you requested is unavailable in this major version of DirectX. (A major version denotes a primary release, such as DirectX 6.0.) ";
	case D3DERR_BADMINORVERSION: return "The service that you requested is available in this major version of DirectX, but not in this minor version. Get the latest version of the component run time from Microsoft. (A minor version denotes a secondary release, such as DirectX 6.1.) ";
	case D3DERR_COLORKEYATTACHED: return "The application attempted to create a texture with a surface that uses a color key for transparency. ";
#endif
	case D3DERR_CONFLICTINGTEXTUREFILTER: return "The current texture filters cannot be used together. ";
	case D3DERR_CONFLICTINGTEXTUREPALETTE: return "The current textures cannot be used simultaneously. This generally occurs when a multitexture device requires that all palettized textures simultaneously enabled also share the same palette. ";
	case D3DERR_CONFLICTINGRENDERSTATE: return "The currently set render states cannot be used together. ";
#if 0
	case D3DERR_DEVICEAGGREGATED: return "The IDirect3DDevice7::SetRenderTarget method was called on a device that was retrieved from the render target surface. ";
	case D3DERR_EXECUTE_CLIPPED_FAILED: return "The execute buffer could not be clipped during execution. ";
	case D3DERR_EXECUTE_CREATE_FAILED: return "The execute buffer could not be created. This typically occurs when no memory is available to allocate the execute buffer. ";
	case D3DERR_EXECUTE_DESTROY_FAILED: return "The memory for the execute buffer could not be deallocated. ";
	case D3DERR_EXECUTE_FAILED: return "The contents of the execute buffer are invalid and cannot be executed. ";
	case D3DERR_EXECUTE_LOCK_FAILED: return "The execute buffer could not be locked. ";
	case D3DERR_EXECUTE_LOCKED: return "The operation requested by the application could not be completed because the execute buffer is locked. ";
	case D3DERR_EXECUTE_NOT_LOCKED: return "The execute buffer could not be unlocked because it is not currently locked. ";
	case D3DERR_EXECUTE_UNLOCK_FAILED: return "The execute buffer could not be unlocked. ";
	case D3DERR_INBEGIN: return "The requested operation cannot be completed while scene rendering is taking place. Try again after the scene is completed and the IDirect3DDevice7::EndScene method is called. ";
	case D3DERR_INBEGINSTATEBLOCK: return "The operation cannot be completed while recording states for a state block. Complete recording by calling the IDirect3DDevice7::EndStateBlock method, and try again. ";
	case D3DERR_INITFAILED: return "A rendering device could not be created because the new device could not be initialized. ";
	case D3DERR_INVALID_DEVICE: return "The requested device type is not valid. ";
	case D3DERR_INVALIDCURRENTVIEWPORT: return "The currently selected viewport is not valid. ";
	case D3DERR_INVALIDMATRIX: return "The requested operation could not be completed because the combination of the currently set world, view, and projection matrices is invalid (the determinant of the combined matrix is 0). ";
	case D3DERR_INVALIDPALETTE: return "The palette associated with a surface is invalid. ";
	case D3DERR_INVALIDPRIMITIVETYPE: return "The primitive type specified by the application is invalid. ";
	case D3DERR_INVALIDRAMPTEXTURE: return "Ramp mode is being used, and the texture handle in the current material does not match the current texture handle that is set as a render state. ";
	case D3DERR_INVALIDSTATEBLOCK: return "The state block handle is invalid. ";
	case D3DERR_INVALIDVERTEXFORMAT: return "The combination of flexible vertex format flags specified by the application is not valid. ";
	case D3DERR_INVALIDVERTEXTYPE: return "The vertex type specified by the application is invalid. ";
	case D3DERR_LIGHT_SET_FAILED: return "The attempt to set lighting parameters for a light object failed. ";
	case D3DERR_LIGHTHASVIEWPORT: return "The requested operation failed because the light object is associated with another viewport. ";
	case D3DERR_LIGHTNOTINTHISVIEWPORT: return "The requested operation failed because the light object has not been associated with this viewport. ";
	case D3DERR_MATERIAL_CREATE_FAILED: return "The material could not be created. This typically occurs when no memory is available to allocate for the material. ";
	case D3DERR_MATERIAL_DESTROY_FAILED: return "The memory for the material could not be deallocated. ";
	case D3DERR_MATERIAL_GETDATA_FAILED: return "The material parameters could not be retrieved. ";
	case D3DERR_MATERIAL_SETDATA_FAILED: return "The material parameters could not be set. ";
	case D3DERR_MATRIX_CREATE_FAILED: return "The matrix could not be created. This can occur when no memory is available to allocate for the matrix. ";
	case D3DERR_MATRIX_DESTROY_FAILED: return "The memory for the matrix could not be deallocated. ";
	case D3DERR_MATRIX_GETDATA_FAILED: return "The matrix data could not be retrieved. This can occur when the matrix was not created by the current device. ";
	case D3DERR_MATRIX_SETDATA_FAILED: return "The matrix data could not be set. This can occur when the matrix was not created by the current device. ";
	case D3DERR_NOCURRENTVIEWPORT: return "The viewport parameters could not be retrieved because none have been set. ";
	case D3DERR_NOTINBEGIN: return "The requested rendering operation could not be completed because scene rendering has not begun. Call IDirect3DDevice7::BeginScene to begin rendering, and try again. ";
	case D3DERR_NOTINBEGINSTATEBLOCK: return "The requested operation could not be completed because it is only valid while recording a state block. Call the IDirect3DDevice7::BeginStateBlock method, and try again. ";
	case D3DERR_NOVIEWPORTS: return "The requested operation failed because the device currently has no viewports associated with it. ";
	case D3DERR_SCENE_BEGIN_FAILED: return "Scene rendering could not begin. ";
	case D3DERR_SCENE_END_FAILED: return "Scene rendering could not be completed. ";
	case D3DERR_SCENE_IN_SCENE: return "Scene rendering could not begin because a previous scene was not completed by a call to the IDirect3DDevice7::EndScene method. ";
	case D3DERR_SCENE_NOT_IN_SCENE: return "Scene rendering could not be completed because a scene was not started by a previous call to the IDirect3DDevice7::BeginScene method. ";
	case D3DERR_SETVIEWPORTDATA_FAILED: return "The viewport parameters could not be set. ";
	case D3DERR_STENCILBUFFER_NOTPRESENT: return "The requested stencil buffer operation could not be completed because there is no stencil buffer attached to the render target surface. ";
	case D3DERR_SURFACENOTINVIDMEM: return "The device could not be created because the render target surface is not located in video memory. (Hardware-accelerated devices require video-memory render target surfaces.) ";
	case D3DERR_TEXTURE_BADSIZE: return "The dimensions of a current texture are invalid. This can occur when an application attempts to use a texture that has dimensions that are not a power of 2 with a device that requires them. ";
	case D3DERR_TEXTURE_CREATE_FAILED: return "The texture handle for the texture could not be retrieved from the driver. ";
	case D3DERR_TEXTURE_DESTROY_FAILED: return "The device was unable to deallocate the texture memory. ";
	case D3DERR_TEXTURE_GETSURF_FAILED: return "The DirectDraw surface used to create the texture could not be retrieved. ";
	case D3DERR_TEXTURE_LOAD_FAILED: return "The texture could not be loaded. ";
	case D3DERR_TEXTURE_LOCK_FAILED: return "The texture could not be locked. ";
	case D3DERR_TEXTURE_LOCKED: return "The requested operation could not be completed because the texture surface is currently locked. ";
	case D3DERR_TEXTURE_NO_SUPPORT: return "The device does not support texture mapping. ";
	case D3DERR_TEXTURE_NOT_LOCKED: return "The requested operation could not be completed because the texture surface is not locked. ";
	case D3DERR_TEXTURE_SWAP_FAILED: return "The texture handles could not be swapped. ";
	case D3DERR_TEXTURE_UNLOCK_FAILED: return "The texture surface could not be unlocked. ";
#endif
	case D3DERR_TOOMANYOPERATIONS: return "The application is requesting more texture-filtering operations than the device supports. ";
#if 0
	case D3DERR_TOOMANYPRIMITIVES: return "The device is unable to render the provided number of primitives in a single pass. ";
#endif
	case D3DERR_UNSUPPORTEDALPHAARG: return "The device does not support one of the specified texture-blending arguments for the alpha channel. ";
	case D3DERR_UNSUPPORTEDALPHAOPERATION: return "The device does not support one of the specified texture-blending operations for the alpha channel. ";
	case D3DERR_UNSUPPORTEDCOLORARG: return "The device does not support one of the specified texture-blending arguments for color values. ";
	case D3DERR_UNSUPPORTEDCOLOROPERATION: return "The device does not support one of the specified texture-blending operations for color values. ";
	case D3DERR_UNSUPPORTEDFACTORVALUE: return "The specified texture factor value is not supported by the device. ";
	case D3DERR_UNSUPPORTEDTEXTUREFILTER: return "The specified texture filter is not supported by the device. ";
#if 0
	case D3DERR_VBUF_CREATE_FAILED: return "The vertex buffer could not be created. This can happen when there is insufficient memory to allocate a vertex buffer. ";
	case D3DERR_VERTEXBUFFERLOCKED: return "The requested operation could not be completed because the vertex buffer is locked. ";
	case D3DERR_VERTEXBUFFEROPTIMIZED: return "The requested operation could not be completed because the vertex buffer is optimized. (The contents of optimized vertex buffers are driver-specific and considered private.) ";
	case D3DERR_VERTEXBUFFERUNLOCKFAILED: return "The vertex buffer could not be unlocked because the vertex buffer memory was overrun. Be sure that your application does not write beyond the size of the vertex buffer. ";
	case D3DERR_VIEWPORTDATANOTSET: return "The requested operation could not be completed because viewport parameters have not yet been set. Set the viewport parameters by calling the IDirect3DDevice7::SetViewport method, and try again. ";
	case D3DERR_VIEWPORTHASNODEVICE: return "The requested operation could not be completed because the viewport has not yet been associated with a device. Associate the viewport with a rendering device by calling the IDirect3DDevice3::AddViewport method, and try again. ";
#endif
	case D3DERR_WRONGTEXTUREFORMAT: return "The pixel format of the texture surface is not valid. ";
#if 0
	case D3DERR_ZBUFF_NEEDS_SYSTEMMEMORY: return "The requested operation could not be completed because the specified device requires system-memory depth-buffer surfaces. (Software rendering devices require system-memory depth buffers.) ";
	case D3DERR_ZBUFF_NEEDS_VIDEOMEMORY: return "The requested operation could not be completed because the specified device requires video-memory depth-buffer surfaces. (Hardware-accelerated devices require video-memory depth buffers.) ";
	case D3DERR_ZBUFFER_NOTPRESENT: return "The requested operation could not be completed because the render target surface does not have an attached depth buffer. ";
	case DD_OK: return "The request completed successfully.";
#endif
	case DDERR_ALREADYINITIALIZED: return "The object has already been initialized.";
	case DDERR_BLTFASTCANTCLIP: return "A DirectDrawClipper object is attached to a source surface that has passed into a call to the IDirectDrawSurface7::BltFast method.";
	case DDERR_CANNOTATTACHSURFACE: return "A surface cannot be attached to another requested surface.";
	case DDERR_CANNOTDETACHSURFACE: return "A surface cannot be detached from another requested surface.";
	case DDERR_CANTCREATEDC: return "Windows cannot create any more device contexts (DCs), or a DC has requested a palette-indexed surface when the surface had no palette and the display mode was not palette-indexed (in this case DirectDraw cannot select a proper palette into the DC).";
	case DDERR_CANTDUPLICATE: return "Primary and 3-D surfaces, or surfaces that are implicitly created, cannot be duplicated.";
	case DDERR_CANTLOCKSURFACE: return "Access to this surface is refused because an attempt was made to lock the primary surface without DCI support.";
	case DDERR_CANTPAGELOCK: return "An attempt to page-lock a surface failed. Page lock does not work on a display-memory surface or an emulated primary surface.";
	case DDERR_CANTPAGEUNLOCK: return "An attempt to page-unlock a surface failed. Page unlock does not work on a display-memory surface or an emulated primary surface.";
	case DDERR_CLIPPERISUSINGHWND: return "An attempt was made to set a clip list for a DirectDrawClipper object that is already monitoring a window handle.";
	case DDERR_COLORKEYNOTSET: return "No source color key is specified for this operation.";
	case DDERR_CURRENTLYNOTAVAIL: return "No support is currently available.";
	case DDERR_DDSCAPSCOMPLEXREQUIRED: return "New for DirectX 7.0. The surface requires the DDSCAPS_COMPLEX flag.";
	case DDERR_DCALREADYCREATED: return "A device context (DC) has already been returned for this surface. Only one DC can be retrieved for each surface.";
	case DDERR_DEVICEDOESNTOWNSURFACE: return "Surfaces created by one DirectDraw device cannot be used directly by another DirectDraw device.";
	case DDERR_DIRECTDRAWALREADYCREATED: return "A DirectDraw object representing this driver has already been created for this process.";
	case DDERR_EXCEPTION: return "An exception was encountered while performing the requested operation.";
	case DDERR_EXCLUSIVEMODEALREADYSET: return "An attempt was made to set the cooperative level when it was already set to exclusive.";
	case DDERR_EXPIRED: return "The data has expired and is therefore no longer valid.";
	case DDERR_GENERIC: return "There is an undefined error condition.";
	case DDERR_HEIGHTALIGN: return "The height of the provided rectangle is not a multiple of the required alignment.";
	case DDERR_HWNDALREADYSET: return "The DirectDraw cooperative-level window handle has already been set. It cannot be reset while the process has surfaces or palettes created.";
	case DDERR_HWNDSUBCLASSED: return "DirectDraw is prevented from restoring state because the DirectDraw cooperative-level window handle has been subclassed.";
	case DDERR_IMPLICITLYCREATED: return "The surface cannot be restored because it is an implicitly created surface.";
	case DDERR_INCOMPATIBLEPRIMARY: return "The primary surface creation request does not match the existing primary surface.";
	case DDERR_INVALIDCAPS: return "One or more of the capability bits passed to the callback function are incorrect.";
	case DDERR_INVALIDCLIPLIST: return "DirectDraw does not support the provided clip list.";
	case DDERR_INVALIDDIRECTDRAWGUID: return "The globally unique identifier (GUID) passed to the DirectDrawCreate function is not a valid DirectDraw driver identifier.";
	case DDERR_INVALIDMODE: return "DirectDraw does not support the requested mode.";
	case DDERR_INVALIDOBJECT: return "DirectDraw received a pointer that was an invalid DirectDraw object.";
	case DDERR_INVALIDPARAMS: return "One or more of the parameters passed to the method are incorrect.";
	case DDERR_INVALIDPIXELFORMAT: return "The pixel format was invalid as specified.";
	case DDERR_INVALIDPOSITION: return "The position of the overlay on the destination is no longer legal.";
	case DDERR_INVALIDRECT: return "The provided rectangle was invalid.";
	case DDERR_INVALIDSTREAM: return "The specified stream contains invalid data.";
	case DDERR_INVALIDSURFACETYPE: return "The surface was of the wrong type.";
	case DDERR_LOCKEDSURFACES: return "One or more surfaces are locked, causing the failure of the requested operation.";
	case DDERR_MOREDATA: return "There is more data available than the specified buffer size can hold.";
	case DDERR_NEWMODE: return "New for DirectX 7.0. When IDirectDraw7::StartModeTest is called with the DDSMT_ISTESTREQUIRED flag, it may return this value to denote that some or all of the resolutions can and should be tested. IDirectDraw7::EvaluateMode returns this value to indicate that the test has switched to a new display mode.";
	case DDERR_NO3D: return "No 3-D hardware or emulation is present.";
	case DDERR_NOALPHAHW: return "No alpha-acceleration hardware is present or available, causing the failure of the requested operation.";
	case DDERR_NOBLTHW: return "No blitter hardware is present.";
	case DDERR_NOCLIPLIST: return "No clip list is available.";
	case DDERR_NOCLIPPERATTACHED: return "No DirectDrawClipper object is attached to the surface object.";
	case DDERR_NOCOLORCONVHW: return "No color-conversion hardware is present or available.";
	case DDERR_NOCOLORKEY: return "The surface does not currently have a color key.";
	case DDERR_NOCOLORKEYHW: return "There is no hardware support for the destination color key.";
	case DDERR_NOCOOPERATIVELEVELSET: return "A create function was called without the IDirectDraw7::SetCooperativeLevel method.";
	case DDERR_NODC: return "No device context (DC) has ever been created for this surface.";
	case DDERR_NODDROPSHW: return "No DirectDraw raster-operation (ROP) hardware is available.";
	case DDERR_NODIRECTDRAWHW: return "Hardware-only DirectDraw object creation is not possible; the driver does not support any hardware.";
	case DDERR_NODIRECTDRAWSUPPORT: return "DirectDraw support is not possible with the current display driver.";
	case DDERR_NODRIVERSUPPORT: return "New for DirectX 7.0. Testing cannot proceed because the display adapter driver does not enumerate refresh rates.";
	case DDERR_NOEMULATION: return "Software emulation is not available.";
	case DDERR_NOEXCLUSIVEMODE: return "The operation requires the application to have exclusive mode, but the application does not have exclusive mode.";
	case DDERR_NOFLIPHW: return "Flipping visible surfaces is not supported.";
	case DDERR_NOFOCUSWINDOW: return "An attempt was made to create or set a device window without first setting the focus window.";
	case DDERR_NOGDI: return "No GDI is present.";
	case DDERR_NOHWND: return "Clipper notification requires a window handle, or no window handle has been previously set as the cooperative level window handle.";
	case DDERR_NOMIPMAPHW: return "No mipmap-capable texture mapping hardware is present or available.";
	case DDERR_NOMIRRORHW: return "No mirroring hardware is present or available.";
	case DDERR_NOMONITORINFORMATION: return "New for DirectX 7.0. Testing cannot proceed because the monitor has no associated EDID data.";
	case DDERR_NONONLOCALVIDMEM: return "An attempt was made to allocate nonlocal video memory from a device that does not support nonlocal video memory.";
	case DDERR_NOOPTIMIZEHW: return "The device does not support optimized surfaces.";
	case DDERR_NOOVERLAYDEST: return "The IDirectDrawSurface7::GetOverlayPosition method is called on an overlay that the IDirectDrawSurface7::UpdateOverlay method has not been called on to establish as a destination.";
	case DDERR_NOOVERLAYHW: return "No overlay hardware is present or available.";
	case DDERR_NOPALETTEATTACHED: return "No palette object is attached to this surface.";
	case DDERR_NOPALETTEHW: return "There is no hardware support for 16- or 256-color palettes.";
	case DDERR_NORASTEROPHW: return "No appropriate raster-operation hardware is present or available.";
	case DDERR_NOROTATIONHW: return "No rotation hardware is present or available.";
	case DDERR_NOSTEREOHARDWARE: return "There is no stereo hardware present or available.";
	case DDERR_NOSTRETCHHW: return "There is no hardware support for stretching.";
	case DDERR_NOSURFACELEFT: return "There is no hardware present that supports stereo surfaces.";
	case DDERR_NOT4BITCOLOR: return "The DirectDrawSurface object is not using a 4-bit color palette, and the requested operation requires a 4-bit color palette.";
	case DDERR_NOT4BITCOLORINDEX: return "The DirectDrawSurface object is not using a 4-bit color index palette, and the requested operation requires a 4-bit color index palette.";
	case DDERR_NOT8BITCOLOR: return "The DirectDrawSurface object is not using an 8-bit color palette, and the requested operation requires an 8-bit color palette.";
	case DDERR_NOTAOVERLAYSURFACE: return "An overlay component is called for a nonoverlay surface.";
	case DDERR_NOTEXTUREHW: return "The operation cannot be carried out because no texture-mapping hardware is present or available.";
	case DDERR_NOTFLIPPABLE: return "An attempt was made to flip a surface that cannot be flipped.";
	case DDERR_NOTFOUND: return "The requested item was not found.";
	case DDERR_NOTINITIALIZED: return "An attempt was made to call an interface method of a DirectDraw object created by CoCreateInstance before the object was initialized.";
	case DDERR_NOTLOADED: return "The surface is an optimized surface, but it has not yet been allocated any memory.";
	case DDERR_NOTLOCKED: return "An attempt was made to unlock a surface that was not locked.";
	case DDERR_NOTPAGELOCKED: return "An attempt was made to page-unlock a surface with no outstanding page locks.";
	case DDERR_NOTPALETTIZED: return "The surface being used is not a palette-based surface.";
	case DDERR_NOVSYNCHW: return "There is no hardware support for vertical blanksynchronized operations.";
	case DDERR_NOZBUFFERHW: return "The operation to create a z-buffer in display memory or to perform a blit, using a z-buffer cannot be carried out because there is no hardware support for z-buffers.";
	case DDERR_NOZOVERLAYHW: return "The overlay surfaces cannot be z-layered, based on the z-order because the hardware does not support z-ordering of overlays.";
	case DDERR_OUTOFCAPS: return "The hardware needed for the requested operation has already been allocated.";
	case DDERR_OUTOFMEMORY: return "DirectDraw does not have enough memory to perform the operation.";
	case DDERR_OUTOFVIDEOMEMORY: return "DirectDraw does not have enough display memory to perform the operation.";
	case DDERR_OVERLAPPINGRECTS: return "The source and destination rectangles are on the same surface and overlap each other.";
	case DDERR_OVERLAYCANTCLIP: return "The hardware does not support clipped overlays.";
	case DDERR_OVERLAYCOLORKEYONLYONEACTIVE: return "An attempt was made to have more than one color key active on an overlay.";
	case DDERR_OVERLAYNOTVISIBLE: return "The IDirectDrawSurface7::GetOverlayPosition method was called on a hidden overlay.";
	case DDERR_PALETTEBUSY: return "Access to this palette is refused because the palette is locked by another thread.";
	case DDERR_PRIMARYSURFACEALREADYEXISTS: return "This process has already created a primary surface.";
	case DDERR_REGIONTOOSMALL: return "The region passed to the IDirectDrawClipper::GetClipList method is too small.";
	case DDERR_SURFACEALREADYATTACHED: return "An attempt was made to attach a surface to another surface to which it is already attached.";
	case DDERR_SURFACEALREADYDEPENDENT: return "An attempt was made to make a surface a dependency of another surface on which it is already dependent.";
	case DDERR_SURFACEBUSY: return "Access to the surface is refused because the surface is locked by another thread.";
	case DDERR_SURFACEISOBSCURED: return "Access to the surface is refused because the surface is obscured.";
	case DDERR_SURFACELOST: return "Access to the surface is refused because the surface memory is gone. Call the IDirectDrawSurface7::Restore method on this surface to restore the memory associated with it.";
	case DDERR_SURFACENOTATTACHED: return "The requested surface is not attached.";
	case DDERR_TESTFINISHED: return "New for DirectX 7.0. When returned by the IDirectDraw7::StartModeTest method, this value means that no test could be initiated because all the resolutions chosen for testing already have refresh rate information in the registry. When returned by IDirectDraw7::EvaluateMode, the value means that DirectDraw has completed a refresh rate test.";
	case DDERR_TOOBIGHEIGHT: return "The height requested by DirectDraw is too large.";
	case DDERR_TOOBIGSIZE: return "The size requested by DirectDraw is too large. However, the individual height and width are valid sizes.";
	case DDERR_TOOBIGWIDTH: return "The width requested by DirectDraw is too large.";
	case DDERR_UNSUPPORTED: return "The operation is not supported.";
	case DDERR_UNSUPPORTEDFORMAT: return "The pixel format requested is not supported by DirectDraw.";
	case DDERR_UNSUPPORTEDMASK: return "The bitmask in the pixel format requested is not supported by DirectDraw.";
	case DDERR_UNSUPPORTEDMODE: return "The display is currently in an unsupported mode.";
	case DDERR_VERTICALBLANKINPROGRESS: return "A vertical blank is in progress.";
	case DDERR_VIDEONOTACTIVE: return "The video port is not active.";
	case DDERR_WASSTILLDRAWING: return "The previous blit operation that is transferring information to or from this surface is incomplete.";
	case DDERR_WRONGMODE: return "This surface cannot be restored because it was created in a different mode.";
	case DDERR_XALIGN: return "The provided rectangle was not horizontally aligned on a required boundary.";
	}

	return nullptr;
}

const char *D3DErrorString(HRESULT hResult)
{
	static char buffer[1024];
	buffer[0] = 0; // Reset static buffer!

	const char* errorCodeString = DXGetErrorString8A(hResult);
	if (errorCodeString)
	{
		strcat(buffer, errorCodeString);
		strcat(buffer, ": ");
	}

	const char* errorDescription = CxbxGetErrorDescription(hResult);
	if (errorDescription)
		strcat(buffer, errorDescription);
	else
		strcat(buffer, "Unknown D3D error.");

	return buffer;
}

VOID CxbxInitWindow(bool bFullInit)
{
    g_EmuShared->GetXBVideo(&g_XBVideo);

    if(g_XBVideo.GetFullscreen())
        CxbxKrnl_hEmuParent = NULL;

    // create timing thread
	if (bFullInit)
    {
        DWORD dwThreadId;

        HANDLE hThread = CreateThread(NULL, NULL, EmuUpdateTickCount, NULL, NULL, &dwThreadId);
        // We set the priority of this thread a bit higher, to assure reliable timing :
        SetThreadPriority(hThread, THREAD_PRIORITY_ABOVE_NORMAL);

        CxbxKrnlRegisterThread(hThread);
    }

/* TODO : Port this Dxbx code :
  // create vblank handling thread
    {
        dwThreadId = 0;
        {hThread :=} CreateThread(NULL, 0, EmuThreadHandleVBlank, NULL, 0, &dwThreadId);
    }
*/
    // create window message processing thread
    {
        DWORD dwThreadId;

        g_bRenderWindowActive = false;

        HANDLE hRenderWindowThread = CreateThread(NULL, NULL, EmuRenderWindow, NULL, NULL, &dwThreadId);

		if (hRenderWindowThread == NULL) {
			char szBuffer[1024] = { 0 };
			sprintf(szBuffer, "Creating EmuRenderWindowThread Failed: %08X", GetLastError());
			CxbxPopupMessage(szBuffer);
			EmuShared::Cleanup();
			ExitProcess(0);
		}

		// Ported from Dxbx :
		// If possible, assign this thread to another core than the one that runs Xbox1 code :
		if (bFullInit)
			SetThreadAffinityMask(hRenderWindowThread, g_CPUOthers);

        while(!g_bRenderWindowActive)
            SwitchToThread(); 

		SwitchToThread();
    }

	SetFocus(g_hEmuWindow);
}

void DrawUEM(HWND hWnd)
{
	// Draw the universal error message (UEM)
	// See http://xboxdevwiki.net/Fatal_Error
	// Only call this from WM_PAINT message!

	PAINTSTRUCT ps;

	BeginPaint(hWnd, &ps);

	HDC hDC = GetDC(hWnd);
	HDC hMemDC = CreateCompatibleDC(hDC);
	HBITMAP hUEMBmp = CreateCompatibleBitmap(hDC, 640, 480);
	HBITMAP hOriUEMBmp = (HBITMAP)SelectObject(hMemDC, hUEMBmp);


	int nHeight = -MulDiv(8, GetDeviceCaps(hMemDC, LOGPIXELSY), 72);

	HFONT hFont = CreateFont(nHeight, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY, FF_ROMAN, "Verdana");

	HGDIOBJ tmpObj = SelectObject(hMemDC, hFont);

	SetBkColor(hMemDC, RGB(0, 0, 0));

	SetTextColor(hMemDC, RGB(0, 204, 0));

	wchar_t buff[500];
	LoadStringW(GetModuleHandle(NULL), IDS_UEM, buff, sizeof(buff) / sizeof(wchar_t));
	std::wstring wstr(buff);

	// Unfortunately, DrawTextW doesn't support vertical alignemnt, so we have to do the calculation
	// ourselves. See here: https://social.msdn.microsoft.com/Forums/vstudio/en-US/abd89aae-16a0-41c6-8db6-b119ea90b42a/win32-drawtext-how-center-in-vertical-with-new-lines-and-tabs?forum=vclanguage

	RECT rect = { 0, 0, 640, 480 };
	RECT textrect = { 0, 0, 640, 480 };
	DrawTextW(hMemDC, wstr.c_str(), wstr.length(), &textrect, DT_CALCRECT);
	rect.top = (rect.bottom - textrect.bottom) / 2;
	DrawTextW(hMemDC, wstr.c_str(), wstr.length(), &rect, DT_CENTER);


	// Draw the Xbox error code

	SetTextColor(hMemDC, RGB(255, 255, 255));
	std::string err_str(std::to_string(g_CxbxFatalErrorCode));
	rect.left = 20;
	DrawText(hMemDC, err_str.c_str(), err_str.length(), &rect, DT_LEFT);

	GetClientRect(hWnd, &rect);
	SetStretchBltMode(hDC, COLORONCOLOR);
	StretchBlt(hDC, rect.left, rect.top, rect.right, rect.bottom, hMemDC, 0, 0, 640, 480, SRCCOPY);

	SelectObject(hMemDC, hOriUEMBmp);
	SelectObject(hDC, tmpObj);

	DeleteObject(hUEMBmp);
	DeleteObject(hFont);
	DeleteObject(hMemDC);

	if (hDC != NULL)
		ReleaseDC(hWnd, hDC);

	EndPaint(hWnd, &ps);
}

inline DWORD GetXboxCommonResourceType(const X_D3DResource *pXboxResource)
{
	// Don't pass in unassigned Xbox resources
	assert(pXboxResource != NULL);

	DWORD dwCommonType = pXboxResource->Common & X_D3DCOMMON_TYPE_MASK;
	return dwCommonType;
}

inline X_D3DFORMAT GetXboxPixelContainerFormat(const X_D3DPixelContainer *pXboxPixelContainer)
{
	// Don't pass in unassigned Xbox pixel container
	assert(pXboxPixelContainer != NULL);

	return (X_D3DFORMAT)((pXboxPixelContainer->Format & X_D3DFORMAT_FORMAT_MASK) >> X_D3DFORMAT_FORMAT_SHIFT);
}

inline int GetXboxPixelContainerDimensionCount(const X_D3DPixelContainer *pXboxPixelContainer)
{
	// Don't pass in unassigned Xbox pixel container
	assert(pXboxPixelContainer != NULL);

	return (X_D3DFORMAT)((pXboxPixelContainer->Format & X_D3DFORMAT_DIMENSION_MASK) >> X_D3DFORMAT_DIMENSION_SHIFT);
}

X_D3DRESOURCETYPE GetXboxD3DResourceType(const X_D3DResource *pXboxResource)
{
	DWORD Type = GetXboxCommonResourceType(pXboxResource);
	switch (Type)
	{
	case X_D3DCOMMON_TYPE_VERTEXBUFFER:
		return X_D3DRTYPE_VERTEXBUFFER;
	case X_D3DCOMMON_TYPE_INDEXBUFFER:
		return X_D3DRTYPE_INDEXBUFFER;
	case X_D3DCOMMON_TYPE_PUSHBUFFER:
		return X_D3DRTYPE_PUSHBUFFER;
	case X_D3DCOMMON_TYPE_PALETTE:
		return X_D3DRTYPE_PALETTE;
	case X_D3DCOMMON_TYPE_TEXTURE:
	{
		DWORD Format = ((X_D3DPixelContainer *)pXboxResource)->Format;
		if (Format & X_D3DFORMAT_CUBEMAP)
			return X_D3DRTYPE_CUBETEXTURE;

		if (GetXboxPixelContainerDimensionCount((X_D3DPixelContainer *)pXboxResource) > 2)
			return X_D3DRTYPE_VOLUMETEXTURE;

		return X_D3DRTYPE_TEXTURE;
	}
	case X_D3DCOMMON_TYPE_SURFACE:
	{
		if (GetXboxPixelContainerDimensionCount((X_D3DPixelContainer *)pXboxResource) > 2)
			return X_D3DRTYPE_VOLUME;

		return X_D3DRTYPE_SURFACE;
	}
	case X_D3DCOMMON_TYPE_FIXUP:
		return X_D3DRTYPE_FIXUP;
	}

	return X_D3DRTYPE_NONE;
}

inline boolean IsSpecialXboxResource(const X_D3DResource *pXboxResource)
{
	// Don't pass in unassigned Xbox resources
	assert(pXboxResource != NULL);

	return ((pXboxResource->Data & X_D3DRESOURCE_DATA_FLAG_SPECIAL) == X_D3DRESOURCE_DATA_FLAG_SPECIAL);
}

// This can be used to determine if resource Data adddresses
// need the MM_SYSTEM_PHYSICAL_MAP bit set or cleared
inline boolean IsResourceTypeGPUReadable(const DWORD ResourceType)
{
	switch (ResourceType) {
	case X_D3DCOMMON_TYPE_VERTEXBUFFER:
		return true;
	case X_D3DCOMMON_TYPE_INDEXBUFFER:
		/// assert(false); // Index buffers are not allowed to be registered
		break;
	case X_D3DCOMMON_TYPE_PUSHBUFFER:
		return false;
	case X_D3DCOMMON_TYPE_PALETTE:
		return true;
	case X_D3DCOMMON_TYPE_TEXTURE:
		return true;
	case X_D3DCOMMON_TYPE_SURFACE:
		return true;
	case X_D3DCOMMON_TYPE_FIXUP:
		// assert(false); // Fixup's are not allowed to be registered
		break;
	default:
		CxbxKrnlCleanup("Unhandled resource type");
	}

	return false;
}

inline bool IsYuvSurfaceOrTexture(const X_D3DResource *pXboxResource)
{
	// Was : return (pXboxResource->Data == X_D3DRESOURCE_DATA_YUV_SURFACE);
	if (GetXboxPixelContainerFormat((X_D3DPixelContainer *)pXboxResource) == X_D3DFMT_YUY2)
		return true;

	return false;
}

inline bool IsXboxResourceLocked(const X_D3DResource *pXboxResource)
{
	bool result = !!(pXboxResource->Common & X_D3DCOMMON_ISLOCKED);
	return result;
}

inline bool IsXboxResourceD3DCreated(const X_D3DResource *pXboxResource)
{
	bool result = !!(pXboxResource->Common & X_D3DCOMMON_D3DCREATED);
	return result;
}

void *GetDataFromXboxResource(X_D3DResource *pXboxResource)
{
	// Don't pass in unassigned Xbox resources
	if (pXboxResource == NULL)
		return nullptr;

	xbaddr pData = pXboxResource->Data;
	if (pData == NULL)
		return nullptr;

	if (IsSpecialXboxResource(pXboxResource))
	{
		switch (pData) {
		case X_D3DRESOURCE_DATA_BACK_BUFFER:
			return nullptr;
		case X_D3DRESOURCE_DATA_RENDER_TARGET:
			return nullptr;
		case X_D3DRESOURCE_DATA_DEPTH_STENCIL:
			return nullptr;
		case X_D3DRESOURCE_DATA_SURFACE_LEVEL:
			return nullptr;
		default:
			CxbxKrnlCleanup("Unhandled special resource type");
		}
	}

	DWORD Type = GetXboxCommonResourceType(pXboxResource);
	switch (Type) {
	case X_D3DCOMMON_TYPE_VERTEXBUFFER:
	case X_D3DCOMMON_TYPE_PALETTE:
	case X_D3DCOMMON_TYPE_TEXTURE:
	case X_D3DCOMMON_TYPE_SURFACE:
			pData |= MM_SYSTEM_PHYSICAL_MAP;
		break;
	case X_D3DCOMMON_TYPE_PUSHBUFFER:
	case X_D3DCOMMON_TYPE_INDEXBUFFER:
	case X_D3DCOMMON_TYPE_FIXUP:
		break;
	default:
		CxbxKrnlCleanup("Unhandled resource type");
	}

	return (uint08*)pData;
}

typedef struct {
	IDirect3DResource8* pHostResource = nullptr;
	X_D3DResource* pXboxResource = nullptr;
	void* pXboxData = nullptr;
	size_t szXboxDataSize = 0;
	uint32_t hash = 0;
	bool forceRehash = false;
	std::chrono::time_point<std::chrono::high_resolution_clock> nextHashTime;
	std::chrono::milliseconds hashLifeTime = 1ms;
    std::chrono::time_point<std::chrono::high_resolution_clock> lastUpdate;
} host_resource_info_t;

std::map <resource_key_t, host_resource_info_t> g_HostResources;

resource_key_t GetHostResourceKey(X_D3DResource* pXboxResource)
{
	return (resource_key_t)(((uint64_t)pXboxResource->Data << 32) | (DWORD)pXboxResource);
}

void FreeHostResource(resource_key_t key)
{
	// Cleanup RegisteredResources array
	auto registeredResourceIterator = std::find(g_RegisteredResources.begin(), g_RegisteredResources.end(), key);
	// We can remove this soon, after a little more cleanup
	if (registeredResourceIterator != g_RegisteredResources.end()) {
		g_RegisteredResources.erase(registeredResourceIterator);
	}

	// Release the host resource and remove it from the list
	auto hostResourceIterator = g_HostResources.find(key);
	if (hostResourceIterator != g_HostResources.end()) {
		if (hostResourceIterator->second.pHostResource) {
			(hostResourceIterator->second.pHostResource)->Release();
		}
		g_HostResources.erase(hostResourceIterator);
	}
}

void ForceResourceRehash(X_D3DResource* pXboxResource)
{
	auto key = GetHostResourceKey(pXboxResource);
	auto it = g_HostResources.find(key);
	if (it != g_HostResources.end()) {
		it->second.forceRehash = true;
	}
}

IDirect3DResource8 *GetHostResource(X_D3DResource *pXboxResource, bool shouldRegister = true, DWORD dwSize = 0)
{
	if (pXboxResource == NULL || pXboxResource->Data == NULL)
		return nullptr;

	if (shouldRegister) {
		EmuVerifyResourceIsRegistered(pXboxResource, dwSize);
	}

	if (IsSpecialXboxResource(pXboxResource)) // Was X_D3DRESOURCE_DATA_YUV_SURFACE
		return nullptr;

	if (pXboxResource->Lock == X_D3DRESOURCE_LOCK_PALETTE)
		return nullptr;

	auto key = GetHostResourceKey(pXboxResource);
	auto it = g_HostResources.find(key);
	if (it == g_HostResources.end()) {
		// Prevent logging a warning when we expect a null result (for example, D3DResource_Release)
		if (shouldRegister) {
			EmuWarning("EmuResource is not a valid pointer!");
		}
		return nullptr;
	}

	return it->second.pHostResource;
}

// Forward declaration of CxbxGetPixelContainerMeasures to prevent
// polluting the diff too much by reshuffling functions around
VOID CxbxGetPixelContainerMeasures
(
	X_D3DPixelContainer *pPixelContainer,
	DWORD dwLevel,
	UINT *pWidth,
	UINT *pHeight,
	UINT *pPitch,
	UINT *pSize
);

size_t GetXboxResourceSize(X_D3DResource* pXboxResource)
{
	// TODO: Smart size calculation based around format of resource
	switch (GetXboxCommonResourceType(pXboxResource)) {
	case X_D3DCOMMON_TYPE_SURFACE:
	case X_D3DCOMMON_TYPE_TEXTURE:
		uint SrcPitch, SrcSize, Width, Height;
		CxbxGetPixelContainerMeasures(
			(X_D3DPixelContainer*)pXboxResource,
			0, // dwLevel
			&Width,
			&Height,
			&SrcPitch,
			&SrcSize
		);

		return SrcSize;
	default:
		// Fallback to querying the allocation size, if no other calculation was present
		return xboxkrnl::MmQueryAllocationSize(GetDataFromXboxResource(pXboxResource));
	}
	
}

bool HostResourceRequiresUpdate(resource_key_t key, DWORD dwSize)
{
	auto it = g_HostResources.find(key);
	if (it == g_HostResources.end()) {
		return false;
	}

	// Currently, we only dynamically update Textures and Surfaces, so if our resource
	// isn't of these types, do nothing
	DWORD type = GetXboxCommonResourceType(it->second.pXboxResource);
	if (type != X_D3DCOMMON_TYPE_SURFACE && type != X_D3DCOMMON_TYPE_TEXTURE && type != X_D3DCOMMON_TYPE_VERTEXBUFFER) {
		return false;
	}

	// If the resource size got bigger, we need to re-create it
	// if it got smaller, just hashing will suffice
	if (dwSize > it->second.szXboxDataSize) {
		return true;
	}

	bool modified = false;

	auto now = std::chrono::high_resolution_clock::now();
	if (now > it->second.nextHashTime || it->second.forceRehash) {
		uint32_t oldHash = it->second.hash;
		it->second.hash = XXHash32::hash(it->second.pXboxData, it->second.szXboxDataSize, 0);

		if (it->second.hash != oldHash) {
			// The data changed, so reset the hash lifetime
			it->second.hashLifeTime = 1ms;
            it->second.lastUpdate = now;
			modified = true;
		} else if (it->second.lastUpdate + 1000ms < now) {
			// The data did not change, so increase the hash lifetime
			// TODO: choose a sensible upper limit
			if (it->second.hashLifeTime < 1000ms) {
				it->second.hashLifeTime += 10ms;
			}
		}
		
		it->second.forceRehash = false;
	}

	// Update the next hash time based on the hash lifetime
	it->second.nextHashTime = now + it->second.hashLifeTime;

	return modified;
}

void SetHostResource(X_D3DResource* pXboxResource, IDirect3DResource8* pHostResource, DWORD dwSize = 0)
{
	auto key = GetHostResourceKey(pXboxResource);
	auto it = g_HostResources.find(key);
	if (it != g_HostResources.end()) {
		EmuWarning("SetHostResource: Overwriting an existing host resource");
	}

	host_resource_info_t hostResourceInfo;
	hostResourceInfo.pHostResource = pHostResource;
	hostResourceInfo.pXboxResource = pXboxResource;
	hostResourceInfo.pXboxData = GetDataFromXboxResource(pXboxResource);
	hostResourceInfo.szXboxDataSize = dwSize > 0 ? dwSize : GetXboxResourceSize(pXboxResource);
	hostResourceInfo.hash = XXHash32::hash(hostResourceInfo.pXboxData, hostResourceInfo.szXboxDataSize, 0);
	hostResourceInfo.hashLifeTime = 1ms;
    hostResourceInfo.lastUpdate = std::chrono::high_resolution_clock::now();
	hostResourceInfo.nextHashTime = hostResourceInfo.lastUpdate + hostResourceInfo.hashLifeTime;
	hostResourceInfo.forceRehash = false;

	g_HostResources[key] = hostResourceInfo;

}

IDirect3DSurface8 *GetHostSurface(X_D3DResource *pXboxResource)
{
	if (pXboxResource == NULL)
		return nullptr;

	if(GetXboxCommonResourceType(pXboxResource) != X_D3DCOMMON_TYPE_SURFACE) // Allows breakpoint below
		assert(GetXboxCommonResourceType(pXboxResource) == X_D3DCOMMON_TYPE_SURFACE);

	return (IDirect3DSurface8*) GetHostResource(pXboxResource);
}

IDirect3DBaseTexture8 *GetHostBaseTexture(X_D3DResource *pXboxResource)
{
	if (pXboxResource == NULL)
		return nullptr;

	if (GetXboxCommonResourceType(pXboxResource) != X_D3DCOMMON_TYPE_TEXTURE) // Allows breakpoint below
		assert(GetXboxCommonResourceType(pXboxResource) == X_D3DCOMMON_TYPE_TEXTURE);

	return (IDirect3DBaseTexture8*)GetHostResource(pXboxResource);
}

IDirect3DTexture8 *GetHostTexture(X_D3DResource *pXboxResource)
{
	return (IDirect3DTexture8 *)GetHostBaseTexture(pXboxResource);

	// TODO : Check for 1 face (and 2 dimensions)?
}

IDirect3DCubeTexture8 *GetHostCubeTexture(X_D3DResource *pXboxResource)
{
	return (IDirect3DCubeTexture8 *)GetHostBaseTexture(pXboxResource);

	// TODO : Check for 6 faces (and 2 dimensions)?
}

IDirect3DVolumeTexture8 *GetHostVolumeTexture(X_D3DResource *pXboxResource)
{
	return (IDirect3DVolumeTexture8 *)GetHostBaseTexture(pXboxResource);

	// TODO : Check for 3 dimensions?
}

IDirect3DIndexBuffer8 *GetHostIndexBuffer(X_D3DResource *pXboxResource)
{
	if (pXboxResource == NULL)
		return nullptr;

	assert(GetXboxCommonResourceType(pXboxResource) == X_D3DCOMMON_TYPE_INDEXBUFFER);

	return (IDirect3DIndexBuffer8*)GetHostResource(pXboxResource);;
}

IDirect3DVertexBuffer8 *GetHostVertexBuffer(X_D3DResource *pXboxResource, DWORD dwSize)
{
	if (pXboxResource == NULL)
		return nullptr;

	assert(GetXboxCommonResourceType(pXboxResource) == X_D3DCOMMON_TYPE_VERTEXBUFFER);

	return (IDirect3DVertexBuffer8*)GetHostResource(pXboxResource, true, dwSize);
}

void SetHostSurface(X_D3DResource *pXboxResource, IDirect3DSurface8 *pHostSurface)
{
	assert(pXboxResource != NULL);
	assert(GetXboxCommonResourceType(pXboxResource) == X_D3DCOMMON_TYPE_SURFACE);

	SetHostResource(pXboxResource, (IDirect3DResource8*)pHostSurface);
}

void SetHostTexture(X_D3DResource *pXboxResource, IDirect3DTexture8 *pHostTexture)
{
	assert(pXboxResource != NULL);
	assert(GetXboxCommonResourceType(pXboxResource) == X_D3DCOMMON_TYPE_TEXTURE);

	SetHostResource(pXboxResource, (IDirect3DResource8*)pHostTexture);
}

void SetHostCubeTexture(X_D3DResource *pXboxResource, IDirect3DCubeTexture8 *pHostCubeTexture)
{
	assert(pXboxResource != NULL);
	assert(GetXboxCommonResourceType(pXboxResource) == X_D3DCOMMON_TYPE_TEXTURE);

	SetHostResource(pXboxResource, (IDirect3DResource8*)pHostCubeTexture);
}

void SetHostVolumeTexture(X_D3DResource *pXboxResource, IDirect3DVolumeTexture8 *pHostVolumeTexture)
{
	assert(pXboxResource != NULL);
	assert(GetXboxCommonResourceType(pXboxResource) == X_D3DCOMMON_TYPE_TEXTURE);

	SetHostResource(pXboxResource, (IDirect3DResource8*)pHostVolumeTexture);
}

void SetHostIndexBuffer(X_D3DResource *pXboxResource, IDirect3DIndexBuffer8 *pHostIndexBuffer)
{
	assert(pXboxResource != NULL);
	assert(GetXboxCommonResourceType(pXboxResource) == X_D3DCOMMON_TYPE_INDEXBUFFER);

	SetHostResource(pXboxResource, (IDirect3DResource8*)pHostIndexBuffer);
}

void SetHostVertexBuffer(X_D3DResource *pXboxResource, IDirect3DVertexBuffer8 *pHostVertexBuffer, DWORD dwSize)
{
	assert(pXboxResource != NULL);
	assert(GetXboxCommonResourceType(pXboxResource) == X_D3DCOMMON_TYPE_VERTEXBUFFER);

	SetHostResource(pXboxResource, (IDirect3DResource8*)pHostVertexBuffer, dwSize);
}

int XboxD3DPaletteSizeToBytes(const X_D3DPALETTESIZE Size)
{
	static int lk[4] =
	{
		256 * sizeof(D3DCOLOR),    // D3DPALETTE_256
		128 * sizeof(D3DCOLOR),    // D3DPALETTE_128
		64 * sizeof(D3DCOLOR),     // D3DPALETTE_64
		32 * sizeof(D3DCOLOR)      // D3DPALETTE_32
	};

	return lk[Size];
}

inline X_D3DPALETTESIZE GetXboxPaletteSize(const X_D3DPalette *pPalette)
{
	X_D3DPALETTESIZE PaletteSize = (X_D3DPALETTESIZE)
		((pPalette->Common & X_D3DPALETTE_COMMON_PALETTESIZE_MASK) >> X_D3DPALETTE_COMMON_PALETTESIZE_SHIFT);

	return PaletteSize;
}

int GetD3DResourceRefCount(IDirect3DResource8 *EmuResource)
{
	if (EmuResource != nullptr)
	{
		// Get actual reference count by increasing it using AddRef,
		// and relying on the return value of Release (which is
		// probably more reliable than AddRef)
		EmuResource->AddRef();
		return EmuResource->Release();
	}

	return 0;
}

X_D3DSurface *EmuNewD3DSurface()
{
	X_D3DSurface *result = (X_D3DSurface *)g_VMManager.AllocateZeroed(sizeof(X_D3DSurface));
	result->Common = X_D3DCOMMON_D3DCREATED | X_D3DCOMMON_TYPE_SURFACE | 1; // Set refcount to 1
	return result;
}

X_D3DTexture *EmuNewD3DTexture()
{
	X_D3DTexture *result = (X_D3DTexture *)g_VMManager.AllocateZeroed(sizeof(X_D3DTexture));
	result->Common = X_D3DCOMMON_D3DCREATED | X_D3DCOMMON_TYPE_TEXTURE | 1; // Set refcount to 1
	return result;
}

X_D3DVolumeTexture *EmuNewD3DVolumeTexture()
{
	X_D3DVolumeTexture *result = (X_D3DVolumeTexture *)g_VMManager.AllocateZeroed(sizeof(X_D3DVolumeTexture));
	result->Common = X_D3DCOMMON_D3DCREATED | X_D3DCOMMON_TYPE_TEXTURE | 1; // Set refcount to 1
	return result;
}

X_D3DCubeTexture *EmuNewD3DCubeTexture()
{
	X_D3DCubeTexture *result = (X_D3DCubeTexture *)g_VMManager.AllocateZeroed(sizeof(X_D3DCubeTexture));
	result->Common = X_D3DCOMMON_D3DCREATED | X_D3DCOMMON_TYPE_TEXTURE | 1; // Set refcount to 1
	return result;
}

X_D3DIndexBuffer *EmuNewD3DIndexBuffer()
{
	X_D3DIndexBuffer *result = (X_D3DIndexBuffer *)g_VMManager.AllocateZeroed(sizeof(X_D3DIndexBuffer));
	result->Common = X_D3DCOMMON_D3DCREATED | X_D3DCOMMON_TYPE_INDEXBUFFER | 1; // Set refcount to 1
	return result;
}

X_D3DVertexBuffer *EmuNewD3DVertexBuffer()
{
	X_D3DVertexBuffer *result = (X_D3DVertexBuffer *)g_VMManager.AllocateZeroed(sizeof(X_D3DVertexBuffer));
	result->Common = X_D3DCOMMON_D3DCREATED | X_D3DCOMMON_TYPE_VERTEXBUFFER | 1; // Set refcount to 1
	return result;
}

X_D3DPalette *EmuNewD3DPalette()
{
	X_D3DPalette *result = (X_D3DPalette *)g_VMManager.AllocateZeroed(sizeof(X_D3DPalette));
	result->Common = X_D3DCOMMON_D3DCREATED | X_D3DCOMMON_TYPE_PALETTE | 1; // Set refcount to 1
	return result;
}

VOID CxbxSetPixelContainerHeader
(
	X_D3DPixelContainer* pPixelContainer,
	DWORD				Common,
	UINT				Width,
	UINT				Height,
	UINT				Levels,
	X_D3DFORMAT	Format,
	UINT				Dimensions,
	UINT				Pitch
)
{
	// Set X_D3DResource field(s) :
	pPixelContainer->Common = Common;
	// DON'T SET pPixelContainer->Data
	// DON'T SET pPixelContainer->Lock

	// Are Width and Height both a power of two?
	DWORD l2w; _BitScanReverse(&l2w, Width); // MSVC intrinsic; GCC has __builtin_clz
	DWORD l2h; _BitScanReverse(&l2h, Height);
	if (((1 << l2w) == Width) && ((1 << l2h) == Height)) {
		Width = Height = Pitch = 1; // When setting Format, clear Size field
	}
	else {
		l2w = l2h = 0; // When setting Size, clear D3DFORMAT_USIZE and VSIZE
	}

	// TODO : Must this be set using Usage / Pool / something else?
	const int Depth = 1;

	// Set X_D3DPixelContainer field(s) :
	pPixelContainer->Format = 0
		| ((Dimensions << X_D3DFORMAT_DIMENSION_SHIFT) & X_D3DFORMAT_DIMENSION_MASK)
		| (((DWORD)Format << X_D3DFORMAT_FORMAT_SHIFT) & X_D3DFORMAT_FORMAT_MASK)
		| ((Levels << X_D3DFORMAT_MIPMAP_SHIFT) & X_D3DFORMAT_MIPMAP_MASK)
		| ((l2w << X_D3DFORMAT_USIZE_SHIFT) & X_D3DFORMAT_USIZE_MASK)
		| ((l2h << X_D3DFORMAT_VSIZE_SHIFT) & X_D3DFORMAT_VSIZE_MASK)
		| ((Depth << X_D3DFORMAT_PSIZE_SHIFT) & X_D3DFORMAT_PSIZE_MASK)
		;
	pPixelContainer->Size = 0
		| (((Width - 1) /*X_D3DSIZE_WIDTH_SHIFT*/) & X_D3DSIZE_WIDTH_MASK)
		| (((Height - 1) << X_D3DSIZE_HEIGHT_SHIFT) & X_D3DSIZE_HEIGHT_MASK)
		| (((Pitch - 1) << X_D3DSIZE_PITCH_SHIFT) & X_D3DSIZE_PITCH_MASK)
		;
}

VOID CxbxGetPixelContainerMeasures
(
	X_D3DPixelContainer *pPixelContainer,
	DWORD dwLevel,
	UINT *pWidth,
	UINT *pHeight,
	UINT *pPitch,
	UINT *pSize
)
{
	DWORD Size = pPixelContainer->Size;
	X_D3DFORMAT X_Format = GetXboxPixelContainerFormat(pPixelContainer);

	if (Size != 0)
	{
		*pWidth = ((Size & X_D3DSIZE_WIDTH_MASK) /* >> X_D3DSIZE_WIDTH_SHIFT*/) + 1;
		*pHeight = ((Size & X_D3DSIZE_HEIGHT_MASK) >> X_D3DSIZE_HEIGHT_SHIFT) + 1;
		*pPitch = (((Size & X_D3DSIZE_PITCH_MASK) >> X_D3DSIZE_PITCH_SHIFT) + 1) * X_D3DTEXTURE_PITCH_ALIGNMENT;
	}
	else
	{
		DWORD l2w = (pPixelContainer->Format & X_D3DFORMAT_USIZE_MASK) >> X_D3DFORMAT_USIZE_SHIFT;
		DWORD l2h = (pPixelContainer->Format & X_D3DFORMAT_VSIZE_MASK) >> X_D3DFORMAT_VSIZE_SHIFT;
		DWORD dwBPP = EmuXBFormatBitsPerPixel(X_Format);

		*pHeight = 1 << l2h;
		*pWidth = 1 << l2w;
		*pPitch = *pWidth * dwBPP / 8;
	}

	*pSize = *pHeight * *pPitch;

	if (EmuXBFormatIsCompressed(X_Format)) {
		*pPitch *= 4;
	}
}

bool ConvertD3DTextureToARGBBuffer(
	X_D3DFORMAT X_Format,
	uint8 *pSrc,
	int SrcWidth, int SrcHeight,
	uint SrcPitch,
	uint8 *pDest, int DestPitch,
	int TextureStage = 0
)
{
	const FormatToARGBRow ConvertRowToARGB = EmuXBFormatComponentConverter(X_Format);
	if (ConvertRowToARGB == nullptr)
		return false; // Unhandled conversion

	uint8 *unswizleBuffer = nullptr;
	if (EmuXBFormatIsSwizzled(X_Format)) {
		unswizleBuffer = (uint8*)malloc(SrcPitch * SrcHeight); // TODO : Reuse buffer when performance is important
		// First we need to unswizzle the texture data
		EmuUnswizzleRect(
			pSrc, SrcWidth, SrcHeight, 1, unswizleBuffer,
			SrcPitch, {}, {}, EmuXBFormatBytesPerPixel(X_Format)
		);
		// Convert colors from the unswizzled buffer
		pSrc = unswizleBuffer;
	}

	int AdditionalArgument;
	if (X_Format == X_D3DFMT_P8)
		AdditionalArgument = (int)g_pCurrentPalette[TextureStage];
	else
		AdditionalArgument = DestPitch;

	DWORD SrcRowOff = 0;
	uint8 *pDestRow = pDest;
	if (EmuXBFormatIsCompressed(X_Format)) {
		// All compressed formats (DXT1, DXT3 and DXT5) encode blocks of 4 pixels on 4 lines
		SrcHeight = (SrcHeight + 3) / 4;
		DestPitch *= 4;
	}

	for (int y = 0; y < SrcHeight; y++) {
		*(int*)pDestRow = AdditionalArgument; // Dirty hack, to avoid an extra parameter to all conversion callbacks
		ConvertRowToARGB(pSrc + SrcRowOff, pDestRow, SrcWidth);
		SrcRowOff += SrcPitch;
		pDestRow += DestPitch;
	}

	if (unswizleBuffer)
		free(unswizleBuffer);

	return true;
}

uint8 *ConvertD3DTextureToARGB(
	X_D3DPixelContainer *pXboxPixelContainer,
	uint8 *pSrc,
	int *pWidth, int *pHeight,
	int TextureStage // default = 0
)
{
	// Avoid allocating pDest when ConvertD3DTextureToARGBBuffer will fail anyway
	X_D3DFORMAT X_Format = GetXboxPixelContainerFormat(pXboxPixelContainer);
	const FormatToARGBRow ConvertRowToARGB = EmuXBFormatComponentConverter(X_Format);
	if (ConvertRowToARGB == nullptr)
		return nullptr; // Unhandled conversion

	uint SrcPitch, SrcSize;
	CxbxGetPixelContainerMeasures(
		pXboxPixelContainer,
		0, // dwLevel
		(UINT*)pWidth,
		(UINT*)pHeight,
		&SrcPitch,
		&SrcSize
	);

	// Now we know ConvertD3DTextureToARGBBuffer will do it's thing, allocate the resulting buffer
	int DestPitch = *pWidth * sizeof(DWORD);
	uint8 *pDest = (uint8 *)malloc(DestPitch * *pHeight);

	// And convert the source towards that buffer
	/*ignore result*/ConvertD3DTextureToARGBBuffer(
		X_Format,
		pSrc, *pWidth, *pHeight, SrcPitch,
		pDest, DestPitch,
		TextureStage);

	// NOTE : Caller must take ownership!
	return pDest;
}

VOID CxbxReleaseBackBufferLock()
{
	IDirect3DSurface8 *pBackBuffer = nullptr;

	if (D3D_OK == g_pD3DDevice8->GetBackBuffer(0, D3DBACKBUFFER_TYPE_MONO, &pBackBuffer))
	{
		assert(pBackBuffer != nullptr);

		pBackBuffer->UnlockRect(); // remove old lock
		pBackBuffer->Release();
	}
}


// Direct3D initialization (called before emulation begins)
VOID EmuD3DInit()
{
	// create the create device proxy thread
	{
		DWORD dwThreadId;

		CreateThread(NULL, NULL, EmuCreateDeviceProxy, NULL, NULL, &dwThreadId);
		// Ported from Dxbx :
		// If possible, assign this thread to another core than the one that runs Xbox1 code :
		SetThreadAffinityMask(&dwThreadId, g_CPUOthers);
	}

	// create Direct3D8 and retrieve caps
    {
        

        // xbox Direct3DCreate8 returns "1" always, so we need our own ptr
        g_pD3D8 = Direct3DCreate8(D3D_SDK_VERSION);

        if(g_pD3D8 == NULL)
            CxbxKrnlCleanup("Could not initialize Direct3D8!");

        D3DDEVTYPE DevType = (g_XBVideo.GetDirect3DDevice() == 0) ? D3DDEVTYPE_HAL : D3DDEVTYPE_REF;

        g_pD3D8->GetDeviceCaps(g_XBVideo.GetDisplayAdapter(), DevType, &g_D3DCaps);
    }

    // create default device
    {
        X_D3DPRESENT_PARAMETERS PresParam;

        ZeroMemory(&PresParam, sizeof(PresParam));

        PresParam.BackBufferWidth  = 640;
        PresParam.BackBufferHeight = 480;
        PresParam.BackBufferFormat = X_D3DFMT_A8R8G8B8;
        PresParam.BackBufferCount  = 1;
        PresParam.EnableAutoDepthStencil = TRUE;
		PresParam.AutoDepthStencilFormat = X_D3DFMT_D24S8;
        PresParam.SwapEffect = D3DSWAPEFFECT_DISCARD;

            
        EMUPATCH(Direct3D_CreateDevice)(0, D3DDEVTYPE_HAL, 0, D3DCREATE_HARDWARE_VERTEXPROCESSING, &PresParam, &g_pD3DDevice8);
            
    }
}

// cleanup Direct3D
VOID EmuD3DCleanup()
{
    EmuDInputCleanup();
}

// enumeration procedure for locating display device GUIDs
static BOOL WINAPI EmuEnumDisplayDevices(GUID FAR *lpGUID, LPSTR lpDriverDescription, LPSTR lpDriverName, LPVOID lpContext, HMONITOR hm)
{
    static DWORD dwEnumCount = 0;

    if(dwEnumCount++ == g_XBVideo.GetDisplayAdapter()+1)
    {
        g_hMonitor = hm;
        dwEnumCount = 0;
        if(lpGUID != 0)
        {
            memcpy(&g_ddguid, lpGUID, sizeof(GUID));
        }
        else
        {
            memset(&g_ddguid, 0, sizeof(GUID));
        }

        return FALSE;
    }

    return TRUE;
}

// window message processing thread
static DWORD WINAPI EmuRenderWindow(LPVOID lpVoid)
{
	CxbxSetThreadName("Cxbx Render Window");

    // register window class
    {
        LOGBRUSH logBrush = {BS_SOLID, RGB(0,0,0)};

        g_hBgBrush = CreateBrushIndirect(&logBrush);

        WNDCLASSEX wc =
        {
            sizeof(WNDCLASSEX),
            CS_CLASSDC,
            EmuMsgProc,
            0, 0, GetModuleHandle(NULL),
			0, // TODO : LoadIcon(hmodule, ?)
            LoadCursor(NULL, IDC_ARROW),
            (HBRUSH)(g_hBgBrush), NULL,
            "CxbxRender",
            NULL
        };

        RegisterClassEx(&wc);
    }

	bool bQuickReboot;
	g_EmuShared->GetQuickRebootFlag(&bQuickReboot);

	// precaution for multi-xbe titles in the case CrashMonitor has still not destoyed the previous mutex
	while (bQuickReboot)
	{
		g_EmuShared->GetQuickRebootFlag(&bQuickReboot);
	}

	HANDLE hCrashMutex = CreateMutex(NULL, TRUE, "CrashMutex");

    // create the window
    {
        DWORD dwStyle = (g_XBVideo.GetFullscreen() || (CxbxKrnl_hEmuParent == 0))? WS_OVERLAPPEDWINDOW : WS_CHILD;

        int nTitleHeight  = GetSystemMetrics(SM_CYCAPTION);
        int nBorderWidth  = GetSystemMetrics(SM_CXSIZEFRAME);
        int nBorderHeight = GetSystemMetrics(SM_CYSIZEFRAME);

        int x = 100, y = 100, nWidth = 640, nHeight = 480;

        nWidth  += nBorderWidth*2;
        nHeight += nBorderHeight*2 + nTitleHeight;

        sscanf(g_XBVideo.GetVideoResolution(), "%d x %d", &nWidth, &nHeight);

        if(g_XBVideo.GetFullscreen())
        {
            x = y = nWidth = nHeight = 0;
            dwStyle = WS_POPUP;
        }

        HWND hwndParent = GetDesktopWindow();

        if(!g_XBVideo.GetFullscreen())
        {
            hwndParent = CxbxKrnl_hEmuParent;
        }

        g_hEmuWindow = CreateWindow
        (
            "CxbxRender", "Cxbx-Reloaded",
            dwStyle, x, y, nWidth, nHeight,
            hwndParent, NULL, GetModuleHandle(NULL), NULL
        );
    }

    ShowWindow(g_hEmuWindow, ((CxbxKrnl_hEmuParent == 0) || g_XBVideo.GetFullscreen()) ? SW_SHOWDEFAULT : SW_SHOWMAXIMIZED);
    UpdateWindow(g_hEmuWindow);

    if(!g_XBVideo.GetFullscreen() && (CxbxKrnl_hEmuParent != NULL))
    {
        SetFocus(CxbxKrnl_hEmuParent);
    }

    // initialize direct input
    if(!EmuDInputInit())
        CxbxKrnlCleanup("Could not initialize DirectInput!");

    DbgPrintf("EmuD3D8: Message-Pump thread is running.\n");

    SetFocus(g_hEmuWindow);

    DbgConsole *dbgConsole = new DbgConsole();

    // message processing loop
    {
        MSG msg;

        ZeroMemory(&msg, sizeof(msg));

        bool lPrintfOn = g_bPrintfOn;

        g_bRenderWindowActive = true;

        while(msg.message != WM_QUIT)
        {
            if(PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE))
            {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
            else
            {
				SwitchToThread();

                // if we've just switched back to display off, clear buffer & display prompt
                if(!g_bPrintfOn && lPrintfOn)
                {
                    dbgConsole->Reset();
                }

                lPrintfOn = g_bPrintfOn;

                dbgConsole->Process();
            }
        }

        g_bRenderWindowActive = false;

        delete dbgConsole;

		if (hCrashMutex != NULL) { ReleaseMutex(hCrashMutex); }

        CxbxKrnlCleanup(NULL);
    }

    return 0;
}

// simple helper function
void ToggleFauxFullscreen(HWND hWnd)
{
    if(g_XBVideo.GetFullscreen())
        return;

    static LONG lRestore = 0, lRestoreEx = 0;
    static RECT lRect = {0};

    if(!g_bIsFauxFullscreen)
    {
        if(CxbxKrnl_hEmuParent != NULL)
        {
            SetParent(hWnd, NULL);
        }
        else
        {
            lRestore = GetWindowLong(hWnd, GWL_STYLE);
            lRestoreEx = GetWindowLong(hWnd, GWL_EXSTYLE);

            GetWindowRect(hWnd, &lRect);
        }

        SetWindowLong(hWnd, GWL_STYLE, WS_POPUP);
        ShowWindow(hWnd, SW_MAXIMIZE);
        SetWindowPos(hWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE);
    }
    else
    {
        if(CxbxKrnl_hEmuParent != NULL)
        {
            SetParent(hWnd, CxbxKrnl_hEmuParent);
            SetWindowLong(hWnd, GWL_STYLE, WS_CHILD);
            ShowWindow(hWnd, SW_MAXIMIZE);
            SetFocus(CxbxKrnl_hEmuParent);
        }
        else
        {
            SetWindowLong(hWnd, GWL_STYLE, lRestore);
            SetWindowLong(hWnd, GWL_EXSTYLE, lRestoreEx);
            ShowWindow(hWnd, SW_RESTORE);
            SetWindowPos(hWnd, HWND_NOTOPMOST, lRect.left, lRect.top, lRect.right - lRect.left, lRect.bottom - lRect.top, 0);
            SetFocus(hWnd);
        }
    }

    g_bIsFauxFullscreen = !g_bIsFauxFullscreen;
}

// rendering window message procedure
static LRESULT WINAPI EmuMsgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    static bool bAutoPaused = false;

    switch(msg)
    {
        case WM_DESTROY:
        {
            DeleteObject(g_hBgBrush);
            PostQuitMessage(0);
            return D3D_OK; // = 0
        }
        break;

		case WM_PAINT:
		{
			if (g_CxbxPrintUEM)
			{
				DrawUEM(hWnd);
			}
		}
		break;

        case WM_SYSKEYDOWN:
        {
            if(wParam == VK_RETURN)
            {
                ToggleFauxFullscreen(hWnd);
            }
            else if(wParam == VK_F4)
            {
                PostMessage(hWnd, WM_CLOSE, 0, 0);
            }
        }
        break;

        case WM_KEYDOWN:
        {
            /*! disable fullscreen if we are set to faux mode, and faux fullscreen is active */
            if(wParam == VK_ESCAPE)
            {
                if(g_XBVideo.GetFullscreen())
                {
                    SendMessage(hWnd, WM_CLOSE, 0, 0);
                }
                else if(g_bIsFauxFullscreen)
                {
                    ToggleFauxFullscreen(hWnd);
                }
            }
            else if (wParam == VK_F6)
            {
                // For some unknown reason, F6 isn't handled in WndMain::WndProc
                // sometimes, so detect it and stop emulation from here too :
                SendMessage(hWnd, WM_CLOSE, 0, 0); // See StopEmulation();
            }
            else if(wParam == VK_F8)
            {
                g_bPrintfOn = !g_bPrintfOn;
            }
            else if(wParam == VK_F9)
            {
                g_bBrkPush = TRUE;
            }
            else if(wParam == VK_F10)
            {
                ToggleFauxFullscreen(hWnd);
            }
            else if(wParam == VK_F11)
            {
                if(g_iWireframe++ == 2)
                    g_iWireframe = 0;
            }
            else if(wParam == VK_F12)
            {
                g_bStepPush = !g_bStepPush;
            }
        }
        break;

        case WM_SIZE:
        {
            switch(wParam)
            {
                case SIZE_RESTORED:
                case SIZE_MAXIMIZED:
                {
                    if(bAutoPaused)
                    {
                        bAutoPaused = false;
                        CxbxKrnlResume();
                    }
                }
                break;

                case SIZE_MINIMIZED:
                {
                    if(g_XBVideo.GetFullscreen())
                        CxbxKrnlCleanup(NULL);

                    if(!g_bEmuSuspended)
                    {
                        bAutoPaused = true;
                        CxbxKrnlSuspend();
                    }
                }
                break;
            }
        }
        break;

        case WM_CLOSE:
            DestroyWindow(hWnd);
            break;

        case WM_SETFOCUS:
        {
            if(CxbxKrnl_hEmuParent != NULL)
            {
                SetFocus(CxbxKrnl_hEmuParent);
            }
        }
        break;

        case WM_SETCURSOR:
        {
            if(g_XBVideo.GetFullscreen() || g_bIsFauxFullscreen)
            {
                SetCursor(NULL);
                return D3D_OK; // = 0
            }

            return DefWindowProc(hWnd, msg, wParam, lParam);
        }
        break;

        default:
            return DefWindowProc(hWnd, msg, wParam, lParam);
    }

    return D3D_OK; // = 0
}

std::chrono::time_point<std::chrono::steady_clock, std::chrono::duration<double, std::nano>> GetNextVBlankTime()
{
	// TODO: Read display frequency from Xbox Display Adapter
	// This is accessed by calling CMiniport::GetRefreshRate(); 
	// This reads from the structure located at CMinpPort::m_CurrentAvInfo
	// This will require at least Direct3D_CreateDevice being unpatched
	// otherwise, m_CurrentAvInfo will never be initialised!
	// 20ms should be used in the case of 50hz
	return std::chrono::steady_clock::now() + 16.6666666667ms;
}


// timing thread procedure
static DWORD WINAPI EmuUpdateTickCount(LPVOID)
{
	CxbxSetThreadName("Cxbx Timing Thread");

    // since callbacks come from here
	InitXboxThread(g_CPUOthers); // avoid Xbox1 core for lowest possible latency

    DbgPrintf("EmuD3D8: Timing thread is running.\n");

    // current vertical blank count
    int curvb = 0;

	// Calculate Next VBlank time
	auto nextVBlankTime = GetNextVBlankTime();

    while(true)
    {
        xboxkrnl::KeTickCount = timeGetTime();	
		SwitchToThread();

        //
        // Poll input
        //

        {
            int v;

            for(v=0;v<XINPUT_SETSTATE_SLOTS;v++)
            {
                HANDLE hDevice = g_pXInputSetStateStatus[v].hDevice;

                if(hDevice == 0)
                    continue;

                DWORD dwLatency = g_pXInputSetStateStatus[v].dwLatency++;

                if(dwLatency < XINPUT_SETSTATE_LATENCY)
                    continue;

                g_pXInputSetStateStatus[v].dwLatency = 0;

                PXINPUT_FEEDBACK pFeedback = (PXINPUT_FEEDBACK)g_pXInputSetStateStatus[v].pFeedback;

                if(pFeedback == 0)
                    continue;

                //
                // Only update slot if it has not already been updated
                //

                if(pFeedback->Header.dwStatus != ERROR_SUCCESS)
                {
                    if(pFeedback->Header.hEvent != 0)
                    {
                        SetEvent(pFeedback->Header.hEvent);
                    }

                    pFeedback->Header.dwStatus = ERROR_SUCCESS;
                }
            }
        }

		// If VBlank Interval has passed, trigger VBlank callback
        // Note: This whole code block can be removed once NV2A interrupts are implemented
		// And Both Swap and Present can be ran unpatched
		// Once that is in place, MiniPort + Direct3D will handle this on it's own!
		// We check for LLE flag as NV2A handles it's own VBLANK if LLE is enabled!
		if (!(bLLE_GPU) && std::chrono::steady_clock::now() > nextVBlankTime)
        {
			nextVBlankTime = GetNextVBlankTime();

			// Increment the VBlank Counter and Wake all threads there were waiting for the VBlank to occur
			std::unique_lock<std::mutex> lk(g_VBConditionMutex);
			g_VBData.VBlank++;
			g_VBConditionVariable.notify_all();

			// TODO: Fixme.  This may not be right...
			g_SwapData.SwapVBlank = 1;

            if(g_pVBCallback != NULL)
            {
                    
                g_pVBCallback(&g_VBData);
                    
            }

            g_VBData.Swap = 0;

			// TODO: This can't be accurate...
			g_SwapData.TimeUntilSwapVBlank = 0;

			// TODO: Recalculate this for PAL version if necessary.
			// Also, we should check the D3DPRESENT_INTERVAL value for accurracy.
		//	g_SwapData.TimeBetweenSwapVBlanks = 1/60;
			g_SwapData.TimeBetweenSwapVBlanks = 0;
        }
    }
}

// thread dedicated to create devices
static DWORD WINAPI EmuCreateDeviceProxy(LPVOID)
{
	LOG_FUNC();

	CxbxSetThreadName("Cxbx CreateDevice Proxy");

    DbgPrintf("EmuD3D8: CreateDevice proxy thread is running.\n");

    while(true)
    {
        // if we have been signalled, create the device with cached parameters
        if(g_EmuCDPD.bReady)
        {
            DbgPrintf("EmuD3D8: CreateDevice proxy thread received request.\n");

            if(g_EmuCDPD.bCreate)
            {
                // only one device should be created at once
                // TODO: ensure all surfaces are somehow cleaned up?
                if(g_pD3DDevice8 != nullptr)
                {
                    DbgPrintf("EmuD3D8: CreateDevice proxy thread releasing old Device.\n");

                    g_pD3DDevice8->EndScene();

                    // Address DirectX Debug Runtime reported error in _DEBUG builds
                    // Direct3D8: (ERROR) :Not all objects were freed: the following indicate the types of unfreed objects.
                    #ifndef _DEBUG
                        while(g_pD3DDevice8->Release() != 0);
                    #endif

                    g_pD3DDevice8 = nullptr;
                }

                if(g_EmuCDPD.pPresentationParameters->BufferSurfaces[0] != NULL)
                    EmuWarning("BufferSurfaces[0] : 0x%.08X", g_EmuCDPD.pPresentationParameters->BufferSurfaces[0]);

                if(g_EmuCDPD.pPresentationParameters->DepthStencilSurface != NULL)
                    EmuWarning("DepthStencilSurface : 0x%.08X", g_EmuCDPD.pPresentationParameters->DepthStencilSurface);

                // make adjustments to parameters to make sense with windows Direct3D
                {
                    g_EmuCDPD.DeviceType = (g_XBVideo.GetDirect3DDevice() == 0) ? D3DDEVTYPE_HAL : D3DDEVTYPE_REF;
                    g_EmuCDPD.Adapter = g_XBVideo.GetDisplayAdapter();

                    g_EmuCDPD.pPresentationParameters->Windowed = !g_XBVideo.GetFullscreen();

                    if(g_XBVideo.GetVSync())
                        g_EmuCDPD.pPresentationParameters->SwapEffect = D3DSWAPEFFECT_COPY_VSYNC;

                    g_EmuCDPD.hFocusWindow = g_hEmuWindow;

                    g_EmuCDPD.pPresentationParameters->BackBufferFormat       = (X_D3DFORMAT)EmuXB2PC_D3DFormat(g_EmuCDPD.pPresentationParameters->BackBufferFormat);
					g_EmuCDPD.pPresentationParameters->AutoDepthStencilFormat = (X_D3DFORMAT)EmuXB2PC_D3DFormat(g_EmuCDPD.pPresentationParameters->AutoDepthStencilFormat);

                    if(!g_XBVideo.GetVSync() && (g_D3DCaps.PresentationIntervals & D3DPRESENT_INTERVAL_IMMEDIATE) && g_XBVideo.GetFullscreen())
                        g_EmuCDPD.pPresentationParameters->FullScreen_PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;
                    else
                    {
                        if(g_D3DCaps.PresentationIntervals & D3DPRESENT_INTERVAL_ONE && g_XBVideo.GetFullscreen())
                            g_EmuCDPD.pPresentationParameters->FullScreen_PresentationInterval = D3DPRESENT_INTERVAL_ONE;
                        else
                            g_EmuCDPD.pPresentationParameters->FullScreen_PresentationInterval = D3DPRESENT_INTERVAL_DEFAULT;
                    }

					// HACK: Disable Tripple Buffering for now...
					// TODO: Enumerate maximum BackBufferCount if possible.
					if(g_EmuCDPD.pPresentationParameters->BackBufferCount > 1)
					{
						EmuWarning("Limiting BackBufferCount to 1...");
						g_EmuCDPD.pPresentationParameters->BackBufferCount = 1;
					}

                    // TODO: Support Xbox extensions if possible
                    if(g_EmuCDPD.pPresentationParameters->MultiSampleType != 0)
                    {
                        EmuWarning("MultiSampleType 0x%.08X is not supported!", g_EmuCDPD.pPresentationParameters->MultiSampleType);

                        g_EmuCDPD.pPresentationParameters->MultiSampleType = D3DMULTISAMPLE_NONE;

                        // TODO: Check card for multisampling abilities
            //            if(pPresentationParameters->MultiSampleType == X_D3DMULTISAMPLE_2_SAMPLES_MULTISAMPLE_QUINCUNX) // = 0x00001121
            //                pPresentationParameters->MultiSampleType = D3DMULTISAMPLE_2_SAMPLES;
            //            else
            //                CxbxKrnlCleanup("Unknown MultiSampleType (0x%.08X)", pPresentationParameters->MultiSampleType);
                    }

                    g_EmuCDPD.pPresentationParameters->Flags = D3DPRESENTFLAG_LOCKABLE_BACKBUFFER;

                    // retrieve resolution from configuration
                    if(g_EmuCDPD.pPresentationParameters->Windowed)
                    {
                        sscanf(g_XBVideo.GetVideoResolution(), "%d x %d", &g_EmuCDPD.pPresentationParameters->BackBufferWidth, &g_EmuCDPD.pPresentationParameters->BackBufferHeight);

                        D3DDISPLAYMODE D3DDisplayMode;

                        g_pD3D8->GetAdapterDisplayMode(g_XBVideo.GetDisplayAdapter(), &D3DDisplayMode);

                        g_EmuCDPD.pPresentationParameters->BackBufferFormat = (X_D3DFORMAT)D3DDisplayMode.Format;
                        g_EmuCDPD.pPresentationParameters->FullScreen_RefreshRateInHz = 0;
                    }
                    else
                    {
                        char szBackBufferFormat[16];

                        sscanf(g_XBVideo.GetVideoResolution(), "%d x %d %*dbit %s (%d hz)",
                            &g_EmuCDPD.pPresentationParameters->BackBufferWidth,
                            &g_EmuCDPD.pPresentationParameters->BackBufferHeight,
                            szBackBufferFormat,
                            &g_EmuCDPD.pPresentationParameters->FullScreen_RefreshRateInHz);

                        if(strcmp(szBackBufferFormat, "x1r5g5b5") == 0)
							g_EmuCDPD.pPresentationParameters->BackBufferFormat = (X_D3DFORMAT)D3DFMT_X1R5G5B5;
                        else if(strcmp(szBackBufferFormat, "r5g6r5") == 0)
							g_EmuCDPD.pPresentationParameters->BackBufferFormat = (X_D3DFORMAT)D3DFMT_R5G6B5;
                        else if(strcmp(szBackBufferFormat, "x8r8g8b8") == 0)
							g_EmuCDPD.pPresentationParameters->BackBufferFormat = (X_D3DFORMAT)D3DFMT_X8R8G8B8;
                        else if(strcmp(szBackBufferFormat, "a8r8g8b8") == 0)
							g_EmuCDPD.pPresentationParameters->BackBufferFormat = (X_D3DFORMAT)D3DFMT_A8R8G8B8;
                    }
                }

                // detect vertex processing capabilities
                if((g_D3DCaps.DevCaps & D3DDEVCAPS_HWTRANSFORMANDLIGHT) && g_EmuCDPD.DeviceType == D3DDEVTYPE_HAL)
                {
                    DbgPrintf("EmuD3D8: Using hardware vertex processing\n");

                    g_EmuCDPD.BehaviorFlags = D3DCREATE_HARDWARE_VERTEXPROCESSING;
                    g_dwVertexShaderUsage = 0;
                }
                else
                {
                    DbgPrintf("EmuD3D8: Using software vertex processing\n");

                    g_EmuCDPD.BehaviorFlags = D3DCREATE_SOFTWARE_VERTEXPROCESSING;
                    g_dwVertexShaderUsage = D3DUSAGE_SOFTWAREPROCESSING;
                }

				// Dxbx addition : Prevent Direct3D from changing the FPU Control word :
				g_EmuCDPD.BehaviorFlags |= D3DCREATE_FPU_PRESERVE;

	            // Address debug DirectX runtime warning in _DEBUG builds
                // Direct3D8: (WARN) :Device that was created without D3DCREATE_MULTITHREADED is being used by a thread other than the creation thread.
                #ifdef _DEBUG
                    g_EmuCDPD.BehaviorFlags |= D3DCREATE_MULTITHREADED;
                #endif

                // redirect to windows Direct3D
                g_EmuCDPD.hRet = g_pD3D8->CreateDevice
                (
                    g_EmuCDPD.Adapter,
                    g_EmuCDPD.DeviceType,
                    g_EmuCDPD.hFocusWindow,
                    g_EmuCDPD.BehaviorFlags,
                    (D3DPRESENT_PARAMETERS*)g_EmuCDPD.pPresentationParameters,
                    g_EmuCDPD.ppReturnedDeviceInterface
                );
				DEBUG_D3DRESULT(g_EmuCDPD.hRet, "IDirect3D8::CreateDevice");

                if(FAILED(g_EmuCDPD.hRet))
                    CxbxKrnlCleanup("IDirect3D8::CreateDevice failed");

				// cache device pointer
				g_pD3DDevice8 = *g_EmuCDPD.ppReturnedDeviceInterface;

				// Which texture formats does this device support?
				for (int X_Format = X_D3DFMT_L8; X_Format <= X_D3DFMT_LIN_R8G8B8A8; X_Format++) {
					// Only process Xbox formats that are directly mappable to host
					if (!EmuXBFormatRequiresConversionToARGB((X_D3DFORMAT)X_Format)) {
						// Convert the Xbox format into host format (without warning, thanks to the above restriction)
						D3DFORMAT PCFormat = EmuXB2PC_D3DFormat((X_D3DFORMAT)X_Format);
						// Index g_bSupportsTextureFormat with Xbox D3DFormat, because host FourCC codes are too big to be used as indices
						g_bSupportsTextureFormat[X_Format] = (PCFormat != D3DFMT_UNKNOWN) &&
							// Ask the Direct3D device if this format is supported (for textures, that is)
							(D3D_OK == g_pD3D8->CheckDeviceFormat(
								g_EmuCDPD.Adapter, g_EmuCDPD.DeviceType,
								(D3DFORMAT)g_EmuCDPD.pPresentationParameters->BackBufferFormat, 0,
								D3DRTYPE_TEXTURE, PCFormat));
					}
				}

                // default NULL guid
                ZeroMemory(&g_ddguid, sizeof(GUID));

				HRESULT hRet;

                // enumerate device guid for this monitor, for directdraw
				hRet = DirectDrawEnumerateExA(EmuEnumDisplayDevices, NULL, DDENUM_ATTACHEDSECONDARYDEVICES);
				DEBUG_D3DRESULT(hRet, "DirectDrawEnumerateExA");

                // create DirectDraw7
                {
                    if(FAILED(hRet)) {
                        hRet = DirectDrawCreateEx(NULL, (void**)&g_pDD7, IID_IDirectDraw7, NULL);
						DEBUG_D3DRESULT(hRet, "DirectDrawCreateEx(NULL)");
					} else {
						hRet = DirectDrawCreateEx(&g_ddguid, (void**)&g_pDD7, IID_IDirectDraw7, NULL);
						DEBUG_D3DRESULT(hRet, "DirectDrawCreateEx(&g_ddguid)");
					}

					if(FAILED(hRet))
                        CxbxKrnlCleanup("Could not initialize DirectDraw7");

					hRet = g_pDD7->GetCaps(&g_DriverCaps, nullptr);
					DEBUG_D3DRESULT(hRet, "g_pDD7->GetCaps");

                    hRet = g_pDD7->SetCooperativeLevel(0, DDSCL_NORMAL);
					DEBUG_D3DRESULT(hRet, "g_pDD7->SetCooperativeLevel");

                    if(FAILED(hRet))
                        CxbxKrnlCleanup("Could not set cooperative level");
                }

				// Dump all supported DirectDraw FourCC format codes
				{
                    DWORD  dwCodes = 0;
                    DWORD *lpCodes = nullptr;

                    g_pDD7->GetFourCCCodes(&dwCodes, lpCodes);
                    lpCodes = (DWORD*)malloc(dwCodes*sizeof(DWORD));
                    g_pDD7->GetFourCCCodes(&dwCodes, lpCodes);
                    for(DWORD v=0;v<dwCodes;v++)
                    {
						DbgPrintf("EmuD3D8: FourCC[%d] = %.4s\n", v, (char *)&(lpCodes[v]));
						// Map known FourCC codes to Xbox Format
						int X_Format;
						switch (lpCodes[v]) {
						case MAKEFOURCC('Y', 'U', 'Y', '2'):
							X_Format = X_D3DFMT_YUY2;
							break;
						case MAKEFOURCC('U', 'Y', 'V', 'Y'):
							X_Format = X_D3DFMT_UYVY;
							break;
						case MAKEFOURCC('D', 'X', 'T', '1'):
							X_Format = X_D3DFMT_DXT1;
							break;
						case MAKEFOURCC('D', 'X', 'T', '3'):
							X_Format = X_D3DFMT_DXT3;
							break;
						case MAKEFOURCC('D', 'X', 'T', '5'):
							X_Format = X_D3DFMT_DXT5;
							break;
						default:
							continue;
						}

						// Warn if CheckDeviceFormat didn't report this format
						if (!g_bSupportsTextureFormat[X_Format]) {
							EmuWarning("EmuD3D8: FourCC format %.4s not previously detected via CheckDeviceFormat()! Enabling it.", (char *)&(lpCodes[v]));
							// TODO : If this warning never shows, detecting FourCC's could be removed entirely. For now, enable the format :
							g_bSupportsTextureFormat[X_Format] = true;
						}
                    }

                    free(lpCodes);						
				}

                // update render target cache
				IDirect3DSurface8 *pNewHostSurface = nullptr;
                hRet = g_pD3DDevice8->GetRenderTarget(&pNewHostSurface);
				DEBUG_D3DRESULT(hRet, "g_pD3DDevice8->GetRenderTarget");

				// update z-stencil surface cache
				pNewHostSurface = nullptr;
				hRet = g_pD3DDevice8->GetDepthStencilSurface(&pNewHostSurface);
				DEBUG_D3DRESULT(hRet, "g_pD3DDevice8->GetDepthStencilSurface");

				g_bHasDepthStencil = SUCCEEDED(hRet);

				hRet = g_pD3DDevice8->CreateVertexBuffer
                (
                    1, 0, 0, D3DPOOL_MANAGED,
                    &g_pDummyBuffer
                );
				DEBUG_D3DRESULT(hRet, "g_pD3DDevice8->CreateVertexBuffer");

                for(int Streams = 0; Streams < 16; Streams++)
                {
                    hRet = g_pD3DDevice8->SetStreamSource(Streams, g_pDummyBuffer, 1);
					DEBUG_D3DRESULT(hRet, "g_pD3DDevice8->SetStreamSource");
				}

                // initially, show a black screen
                // Only clear depth buffer and stencil if present
                //
                // Avoids following DirectX Debug Runtime error report
                //    [424] Direct3D8: (ERROR) :Invalid flag D3DCLEAR_ZBUFFER: no zbuffer is associated with device. Clear failed. 
                //
                hRet = g_pD3DDevice8->Clear(
					/*Count=*/0, 
					/*pRects=*/nullptr, 
					D3DCLEAR_TARGET | (g_bHasDepthStencil ? D3DCLEAR_ZBUFFER|D3DCLEAR_STENCIL : 0),
					/*Color=*/0xFF000000, // TODO : Use constant for this
					/*Z=*/g_bHasDepthStencil ? 1.0f : 0.0f,
					/*Stencil=*/0);
				DEBUG_D3DRESULT(hRet, "g_pD3DDevice8->Clear");

				hRet = g_pD3DDevice8->BeginScene();
				DEBUG_D3DRESULT(hRet, "g_pD3DDevice8->BeginScene");

				hRet = g_pD3DDevice8->EndScene();
				DEBUG_D3DRESULT(hRet, "g_pD3DDevice8->EndScene");

				hRet = g_pD3DDevice8->Present(0, 0, 0, 0);
				DEBUG_D3DRESULT(hRet, "g_pD3DDevice8->Present");

                // begin scene
                hRet = g_pD3DDevice8->BeginScene();
				DEBUG_D3DRESULT(hRet, "g_pD3DDevice8->BeginScene(2nd)");

                // signal completion
                g_EmuCDPD.bReady = false;
            }
            else
            {
                // release direct3d
                if(g_pD3DDevice8 != nullptr)
                {
                    DbgPrintf("EmuD3D8: CreateDevice proxy thread releasing old Device.\n");

                    g_pD3DDevice8->EndScene();

                    g_EmuCDPD.hRet = g_pD3DDevice8->Release();
                    if(g_EmuCDPD.hRet == 0)
                        g_pD3DDevice8 = nullptr;
                }

				// cleanup overlay clipper
				if (g_pDDClipper != nullptr)
				{
					g_pDDClipper->Release();
					g_pDDClipper = nullptr;
				}

				// cleanup directdraw surface
                if(g_pDDSPrimary != nullptr)
                {
                    g_pDDSPrimary->Release();
                    g_pDDSPrimary = nullptr;
                }

                // cleanup directdraw
                if(g_pDD7 != nullptr)
                {
                    g_pDD7->Release();
                    g_pDD7 = nullptr;
                }

                // signal completion
                g_EmuCDPD.bReady = false;
            }
        }

		Sleep(1);
    }

    return 0;
}

// check if a resource has been registered yet (if not, register it)
VOID WINAPI CreateHostResource(X_D3DResource *pThis, DWORD dwSize); // Forward declartion to prevent restructure of code
static void EmuVerifyResourceIsRegistered(X_D3DResource *pResource, DWORD dwSize = 0)
{
	// Skip resources without data
	if (pResource->Data == NULL)
		return;

    // Already "Registered" implicitly
    if(IsSpecialXboxResource(pResource))
        return;

	auto key = GetHostResourceKey(pResource);
	if (std::find(g_RegisteredResources.begin(), g_RegisteredResources.end(), key) != g_RegisteredResources.end()) {
		if (!HostResourceRequiresUpdate(key, dwSize)) {
			return;
		}

		FreeHostResource(key);
	}

	CreateHostResource(pResource, dwSize);
        
	g_RegisteredResources.push_back(key);
}

// ensure a given width/height are powers of 2
static void EmuAdjustPower2(UINT *dwWidth, UINT *dwHeight)
{
    UINT NewWidth=0, NewHeight=0;

    int v;

    for(v=0;v<32;v++)
    {
        int mask = 1 << v;

        if(*dwWidth & mask)
            NewWidth = mask;

        if(*dwHeight & mask)
            NewHeight = mask;
    }

    if(*dwWidth != NewWidth)
    {
        NewWidth <<= 1;
        EmuWarning("Needed to resize width (%d->%d)", *dwWidth, NewWidth);
    }

    if(*dwHeight != NewHeight)
    {
        NewHeight <<= 1;
        EmuWarning("Needed to resize height (%d->%d)", *dwHeight, NewHeight);
    }

    *dwWidth = NewWidth;
    *dwHeight = NewHeight;
}

// Derived from EmuUnswizzleActiveTexture
static void EmuUnswizzleTextureStages()
{
	LOG_INIT; // Allows use of DEBUG_D3DRESULT

	for( int i = 0; i < TEXTURE_STAGES; i++ )
	{
		X_D3DBaseTexture *pBaseTexture = EmuD3DActiveTexture[i];
		if (pBaseTexture == NULL)
			continue;

		HRESULT hRet;
		IDirect3DTexture8 *pHostTexture = GetHostTexture(pBaseTexture);
		if (pHostTexture != nullptr)
		{
			if (pHostTexture->GetType() == D3DRTYPE_CUBETEXTURE) continue; // Prevent exceptions - skip cubes for now
			hRet = pHostTexture->UnlockRect(0); // remove old lock
			DEBUG_D3DRESULT(hRet, "pHostTexture->UnlockRect");
		}

		if(!IsXboxResourceLocked(pBaseTexture))
			continue;

		X_D3DFORMAT XBFormat = GetXboxPixelContainerFormat(pBaseTexture);
		if(!EmuXBFormatIsSwizzled(XBFormat))
			continue;

		DWORD dwBPP = EmuXBFormatBytesPerPixel(XBFormat);
		pBaseTexture->Common &= ~X_D3DCOMMON_ISLOCKED;

		// TODO: potentially XXHash32::hash() to see if this surface was actually modified..

		if (pHostTexture == nullptr)
			continue;

		//
		// unswizzle texture
		//

		{

			DWORD dwLevelCount = pHostTexture->GetLevelCount();

			for(uint32 v=0;v<dwLevelCount;v++)
			{
				D3DSURFACE_DESC SurfaceDesc;

				HRESULT hRet = pHostTexture->GetLevelDesc(v, &SurfaceDesc);
				DEBUG_D3DRESULT(hRet, "pHostTexture->GetLevelDesc");

				if(FAILED(hRet))
					continue;

				//
				// perform unswizzle
				//

				{
					D3DLOCKED_RECT LockedRect;

					//if(SurfaceDesc.Format != D3DFMT_A8R8G8B8)
					//    break;
					//CxbxKrnlCleanup("Temporarily unsupported format for active texture unswizzle (0x%.08X)", SurfaceDesc.Format);

					hRet = pHostTexture->LockRect(v, &LockedRect, NULL, 0);
					DEBUG_D3DRESULT(hRet, "pHostTexture->LockRect");

					if(FAILED(hRet))
						continue;

					DWORD dwWidth = SurfaceDesc.Width;
					DWORD dwHeight = SurfaceDesc.Height;
					DWORD dwDepth = 1;
					DWORD dwPitch = LockedRect.Pitch;
					RECT  iRect = {0,0,0,0};
					POINT iPoint = {0,0};

					void *pTemp = malloc(dwPitch*dwHeight);

					EmuUnswizzleRect
					(
						LockedRect.pBits, dwWidth, dwHeight, dwDepth,
						pTemp, dwPitch, iRect, iPoint, dwBPP
					);

					memcpy(LockedRect.pBits, pTemp, dwPitch*dwHeight);

					pHostTexture->UnlockRect(0);

					free(pTemp);
				}
			}

			DbgPrintf("Texture Stage %d was unswizzled\n", i);
		}
	}
}

typedef struct {
	DWORD Hash = 0;
	DWORD IndexCount = 0;;
	IDirect3DIndexBuffer8* pHostIndexBuffer = nullptr;
} ConvertedIndexBuffer;

std::map<PWORD, ConvertedIndexBuffer> g_ConvertedIndexBuffers;
	
void CxbxRemoveIndexBuffer(PWORD pData)
{
	// HACK: Never Free
}

void CxbxUpdateActiveIndexBuffer
(
	PWORD         pIndexData,
	UINT          IndexCount
)
{
	LOG_INIT; // Allows use of DEBUG_D3DRESULT

	// Create a reference to the active buffer
	ConvertedIndexBuffer& indexBuffer = g_ConvertedIndexBuffers[pIndexData];

	// If the size has changed, free the buffer so it will be re-created
	if (indexBuffer.pHostIndexBuffer != nullptr &&
		indexBuffer.IndexCount < IndexCount) {
		indexBuffer.pHostIndexBuffer->Release();
		indexBuffer = {};
	}

	// If we need to create an index buffer, do so.
	if (indexBuffer.pHostIndexBuffer == nullptr) {
		// Create a new native index buffer of the above determined size :
		HRESULT hRet = g_pD3DDevice8->CreateIndexBuffer(
			IndexCount * 2,
			D3DUSAGE_WRITEONLY,
			D3DFMT_INDEX16,
			D3DPOOL_MANAGED,
			&indexBuffer.pHostIndexBuffer);
		DEBUG_D3DRESULT(hRet, "g_pD3DDevice8->CreateIndexBuffer");

		if (FAILED(hRet))
			CxbxKrnlCleanup("CxbxUpdateActiveIndexBuffer: IndexBuffer Create Failed!");
	}

	// If the data needs updating, do so
	uint32_t uiHash = XXHash32::hash(pIndexData, IndexCount * 2, 0);
	if (uiHash != indexBuffer.Hash)	{
		// Update the Index Count and the hash
		indexBuffer.IndexCount = IndexCount;
		indexBuffer.Hash = uiHash;

		// Update the host index buffer
		BYTE* pData = nullptr;
		indexBuffer.pHostIndexBuffer->Lock(0, 0, &pData, D3DLOCK_DISCARD);
		if (pData == nullptr) {
			CxbxKrnlCleanup("CxbxUpdateActiveIndexBuffer: Could not lock index buffer!");
		}

		DbgPrintf("CxbxUpdateActiveIndexBuffer: Copying %d indices (D3DFMT_INDEX16)\n", IndexCount);
		memcpy(pData, pIndexData, IndexCount * 2);

		indexBuffer.pHostIndexBuffer->Unlock();
	}

	// Determine active the vertex index
	// This reads from g_pDevice->m_IndexBase in Xbox D3D
	// TODO: Move this into a global symbol, similar to RenderState/Texture State
	static DWORD *pdwXboxD3D_IndexBase = &g_XboxD3DDevice[7];

	DWORD indexBase = 0;
	indexBase = *pdwXboxD3D_IndexBase;

	// Activate the new native index buffer :
	HRESULT hRet = g_pD3DDevice8->SetIndices(indexBuffer.pHostIndexBuffer, indexBase);
	DEBUG_D3DRESULT(hRet, "g_pD3DDevice8->SetIndices");

	if (FAILED(hRet))
		CxbxKrnlCleanup("CxbxUpdateActiveIndexBuffer: SetIndices Failed!");
}

// ******************************************************************
// * patch: Direct3D_CreateDevice
// ******************************************************************
HRESULT WINAPI EMUPATCH(Direct3D_CreateDevice)
(
    UINT                        Adapter,
    D3DDEVTYPE                  DeviceType,
    HWND                        hFocusWindow,
    DWORD                       BehaviorFlags,
    X_D3DPRESENT_PARAMETERS    *pPresentationParameters,
    IDirect3DDevice8          **ppReturnedDeviceInterface
)
{
	//FUNC_EXPORTS

	LOG_FUNC_BEGIN
		LOG_FUNC_ARG(Adapter)
		LOG_FUNC_ARG(DeviceType)
		LOG_FUNC_ARG(hFocusWindow)
		LOG_FUNC_ARG(BehaviorFlags) // Xbox BehaviorFlags are ignored
		LOG_FUNC_ARG(pPresentationParameters)
		LOG_FUNC_ARG(ppReturnedDeviceInterface)
		LOG_FUNC_END;

	// Print a few of the pPresentationParameters contents to the console
	DbgPrintf("BackBufferWidth:        = %d\n"
			  "BackBufferHeight:       = %d\n"
			  "BackBufferFormat:       = 0x%.08X\n"
			  "BackBufferCount:        = 0x%.08X\n"
			  "SwapEffect:             = 0x%.08X\n"
			  "EnableAutoDepthStencil: = 0x%.08X\n"
			  "AutoDepthStencilFormat: = 0x%.08X\n"
			  "Flags:                  = 0x%.08X\n\n",
			  pPresentationParameters->BackBufferWidth, pPresentationParameters->BackBufferHeight,
			  pPresentationParameters->BackBufferFormat, pPresentationParameters->BackBufferCount,
			  pPresentationParameters->SwapEffect, pPresentationParameters->EnableAutoDepthStencil,
			  pPresentationParameters->AutoDepthStencilFormat, pPresentationParameters->Flags );

    // Cache parameters
    g_EmuCDPD.Adapter = Adapter;
    g_EmuCDPD.DeviceType = DeviceType;
    g_EmuCDPD.hFocusWindow = hFocusWindow;
    g_EmuCDPD.pPresentationParameters = pPresentationParameters;
    g_EmuCDPD.ppReturnedDeviceInterface = ppReturnedDeviceInterface;

    // Wait until proxy is done with an existing call (i highly doubt this situation will come up)
    while(g_EmuCDPD.bReady)
        Sleep(10);

    // Signal proxy thread, and wait for completion
    g_EmuCDPD.bReady = true;
    g_EmuCDPD.bCreate = true;

    // Wait until proxy is completed
    while(g_EmuCDPD.bReady)
        Sleep(10);
	
	// Set the Xbox g_pD3DDevice pointer to our D3D Device object
	if ((DWORD*)XRefDataBase[XREF_D3DDEVICE] != nullptr && ((DWORD)XRefDataBase[XREF_D3DDEVICE]) != XREF_ADDR_DERIVE) {
		*((DWORD*)XRefDataBase[XREF_D3DDEVICE]) = (DWORD)g_XboxD3DDevice;
	}

	// Finally, set all default render and texture states
	// All commented out states are unsupported on this version of DirectX 8
	g_pD3DDevice8->SetRenderState(D3DRS_ZFUNC, D3DCMP_LESSEQUAL);
	g_pD3DDevice8->SetRenderState(D3DRS_ALPHAFUNC, D3DCMP_ALWAYS);
	g_pD3DDevice8->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE);
	g_pD3DDevice8->SetRenderState(D3DRS_ALPHAREF, 0);
	g_pD3DDevice8->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_ONE);
	g_pD3DDevice8->SetRenderState(D3DRS_DESTBLEND, D3DCMP_ALWAYS);
	g_pD3DDevice8->SetRenderState(D3DRS_ALPHAFUNC, D3DBLEND_ZERO);
	g_pD3DDevice8->SetRenderState(D3DRS_ZWRITEENABLE, TRUE);
	g_pD3DDevice8->SetRenderState(D3DRS_DITHERENABLE, FALSE);
	g_pD3DDevice8->SetRenderState(D3DRS_SHADEMODE, D3DSHADE_GOURAUD);
	g_pD3DDevice8->SetRenderState(D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_RED | D3DCOLORWRITEENABLE_GREEN |
		D3DCOLORWRITEENABLE_BLUE | D3DCOLORWRITEENABLE_ALPHA);
	g_pD3DDevice8->SetRenderState(D3DRS_STENCILZFAIL, D3DSTENCILOP_KEEP);
	g_pD3DDevice8->SetRenderState(D3DRS_STENCILPASS, D3DSTENCILOP_KEEP);
	g_pD3DDevice8->SetRenderState(D3DRS_STENCILFUNC, D3DCMP_ALWAYS);
	g_pD3DDevice8->SetRenderState(D3DRS_STENCILREF, 0);
	g_pD3DDevice8->SetRenderState(D3DRS_STENCILMASK, 0xffffffff);
	g_pD3DDevice8->SetRenderState(D3DRS_STENCILWRITEMASK, 0xffffffff);
	g_pD3DDevice8->SetRenderState(D3DRS_BLENDOP, D3DBLENDOP_ADD);
	//g_pD3DDevice8->SetRenderState(D3DRS_BLENDCOLOR, 0);
	//g_pD3DDevice8->SetRenderState(D3DRS_SWATHWIDTH, D3DSWATH_128);
	//g_pD3DDevice8->SetRenderState(D3DRS_POLYGONOFFSETZSLOPESCALE, 0);
	//g_pD3DDevice8->SetRenderState(D3DRS_POLYGONOFFSETZOFFSET, 0);
	//g_pD3DDevice8->SetRenderState(D3DRS_POINTOFFSETENABLE, FALSE);
	//g_pD3DDevice8->SetRenderState(D3DRS_WIREFRAMEOFFSETENABLE, FALSE);
	//g_pD3DDevice8->SetRenderState(D3DRS_SOLIDOFFSETENABLE, FALSE);
	g_pD3DDevice8->SetRenderState(D3DRS_FOGENABLE, FALSE);
	g_pD3DDevice8->SetRenderState(D3DRS_FOGTABLEMODE, D3DFOG_NONE);
	g_pD3DDevice8->SetRenderState(D3DRS_FOGSTART, 0);
	g_pD3DDevice8->SetRenderState(D3DRS_FOGEND, (DWORD)(1.0f));
	g_pD3DDevice8->SetRenderState(D3DRS_FOGDENSITY, (DWORD)(1.0f));
	g_pD3DDevice8->SetRenderState(D3DRS_RANGEFOGENABLE, FALSE);
	g_pD3DDevice8->SetRenderState(D3DRS_WRAP0, 0);
	g_pD3DDevice8->SetRenderState(D3DRS_WRAP1, 0);
	g_pD3DDevice8->SetRenderState(D3DRS_WRAP2, 0);
	g_pD3DDevice8->SetRenderState(D3DRS_WRAP3, 0);
	g_pD3DDevice8->SetRenderState(D3DRS_LIGHTING, TRUE);
	g_pD3DDevice8->SetRenderState(D3DRS_SPECULARENABLE, FALSE);
	g_pD3DDevice8->SetRenderState(D3DRS_LOCALVIEWER, TRUE);
	g_pD3DDevice8->SetRenderState(D3DRS_COLORVERTEX, TRUE);
	//g_pD3DDevice8->SetRenderState(D3DRS_BACKSPECULARMATERIALSOURCE, D3DMCS_COLOR2);
	//g_pD3DDevice8->SetRenderState(D3DRS_BACKDIFFUSEMATERIALSOURCE, D3DMCS_COLOR1);
	//g_pD3DDevice8->SetRenderState(D3DRS_BACKAMBIENTMATERIALSOURCE, D3DMCS_MATERIAL);
	//g_pD3DDevice8->SetRenderState(D3DRS_BACKEMISSIVEMATERIALSOURCE, D3DMCS_MATERIAL);
	g_pD3DDevice8->SetRenderState(D3DRS_SPECULARMATERIALSOURCE, D3DMCS_COLOR2);
	g_pD3DDevice8->SetRenderState(D3DRS_DIFFUSEMATERIALSOURCE, D3DMCS_COLOR1);
	g_pD3DDevice8->SetRenderState(D3DRS_AMBIENTMATERIALSOURCE, D3DMCS_MATERIAL);
	g_pD3DDevice8->SetRenderState(D3DRS_EMISSIVEMATERIALSOURCE, D3DMCS_MATERIAL);
	//g_pD3DDevice8->SetRenderState(D3DRS_BACKAMBIENT, 0);
	g_pD3DDevice8->SetRenderState(D3DRS_AMBIENT, 0);
	g_pD3DDevice8->SetRenderState(D3DRS_POINTSIZE, (DWORD)(1.0f));
	g_pD3DDevice8->SetRenderState(D3DRS_POINTSIZE_MIN, 0);
	g_pD3DDevice8->SetRenderState(D3DRS_POINTSPRITEENABLE, 0);
	g_pD3DDevice8->SetRenderState(D3DRS_POINTSCALEENABLE, 0);
	g_pD3DDevice8->SetRenderState(D3DRS_POINTSCALE_A, (DWORD)(1.0f));
	g_pD3DDevice8->SetRenderState(D3DRS_POINTSCALE_B, 0);
	g_pD3DDevice8->SetRenderState(D3DRS_POINTSCALE_C, 0);
	g_pD3DDevice8->SetRenderState(D3DRS_POINTSIZE_MAX, (DWORD)(64.0f));
	g_pD3DDevice8->SetRenderState(D3DRS_PATCHEDGESTYLE, D3DPATCHEDGE_DISCRETE);
	g_pD3DDevice8->SetRenderState(D3DRS_PATCHSEGMENTS, (DWORD)(1.0f));
	//g_pD3DDevice8->SetRenderState(D3DRS_PSTEXTUREMODES, 0);
	g_pD3DDevice8->SetRenderState(D3DRS_VERTEXBLEND, D3DVBF_DISABLE);
	g_pD3DDevice8->SetRenderState(D3DRS_FOGCOLOR, 0);
	g_pD3DDevice8->SetRenderState(D3DRS_FILLMODE, D3DFILL_SOLID);
	//g_pD3DDevice8->SetRenderState(D3DRS_BACKFILLMODE, D3DFILL_SOLID);
	//g_pD3DDevice8->SetRenderState(D3DRS_TWOSIDEDLIGHTING, FALSE);
	g_pD3DDevice8->SetRenderState(D3DRS_NORMALIZENORMALS, FALSE);
	g_pD3DDevice8->SetRenderState(D3DRS_ZENABLE, D3DZB_USEW);
	g_pD3DDevice8->SetRenderState(D3DRS_STENCILENABLE, FALSE);
	g_pD3DDevice8->SetRenderState(D3DRS_STENCILFAIL, D3DSTENCILOP_KEEP);
	//g_pD3DDevice8->SetRenderState(D3DRS_FRONTFACE, D3DFRONT_CW);
	g_pD3DDevice8->SetRenderState(D3DRS_CULLMODE, D3DCULL_CCW);
	g_pD3DDevice8->SetRenderState(D3DRS_TEXTUREFACTOR, 0xffffffff);
	g_pD3DDevice8->SetRenderState(D3DRS_ZBIAS, 0);
	//g_pD3DDevice8->SetRenderState(D3DRS_LOGICOP, D3DLOGICOP_NONE);
	g_pD3DDevice8->SetRenderState(D3DRS_EDGEANTIALIAS, FALSE);
	g_pD3DDevice8->SetRenderState(D3DRS_MULTISAMPLEANTIALIAS, TRUE);
	g_pD3DDevice8->SetRenderState(D3DRS_MULTISAMPLEMASK, 0xffffffff);
	//g_pD3DDevice8->SetRenderState(D3DRS_MULTISAMPLEMODERENDERTARGETS, D3DMULTISAMPLEMODE_1X);
	//g_pD3DDevice8->SetRenderState(D3DRS_SHADOWFUNC, D3DCMP_NEVER);	
	//g_pD3DDevice8->SetRenderState(D3DRS_LINEWIDTH, (DWORD)(1.0f));
	//g_pD3DDevice8->SetRenderState(D3DRS_DXT1NOISEENABLE, TRUE);
	//g_pD3DDevice8->SetRenderState(D3DRS_YUVENABLE, FALSE);
	//g_pD3DDevice8->SetRenderState(D3DRS_OCCLUSIONCULLENABLE, TRUE);
	//g_pD3DDevice8->SetRenderState(D3DRS_STENCILCULLENABLE, TRUE);
	//g_pD3DDevice8->SetRenderState(D3DRS_ROPZCMPALWAYSREAD, FALSE);
	//g_pD3DDevice8->SetRenderState(D3DRS_ROPZREAD, FALSE);
	//g_pD3DDevice8->SetRenderState(D3DRS_DONOTCULLUNCOMPRESSED, FALSE);

	// Xbox has 4 Texture Stages
	for (int stage = 0; stage < 4; stage++)	{
		g_pD3DDevice8->SetTextureStageState(stage, D3DTSS_ADDRESSU, D3DTADDRESS_WRAP);
		g_pD3DDevice8->SetTextureStageState(stage, D3DTSS_ADDRESSV, D3DTADDRESS_WRAP);
		g_pD3DDevice8->SetTextureStageState(stage, D3DTSS_ADDRESSW, D3DTADDRESS_WRAP);
		g_pD3DDevice8->SetTextureStageState(stage, D3DTSS_MAGFILTER, D3DTEXF_POINT);
		g_pD3DDevice8->SetTextureStageState(stage, D3DTSS_MINFILTER, D3DTEXF_POINT);
		g_pD3DDevice8->SetTextureStageState(stage, D3DTSS_MIPFILTER, D3DTEXF_NONE);
		g_pD3DDevice8->SetTextureStageState(stage, D3DTSS_MIPMAPLODBIAS, 0);
		g_pD3DDevice8->SetTextureStageState(stage, D3DTSS_MAXMIPLEVEL, 0);
		g_pD3DDevice8->SetTextureStageState(stage, D3DTSS_MAXANISOTROPY, 1);
		//g_pD3DDevice8->SetTextureStageState(stage, D3DTSS_COLORKEYOP, D3DTCOLORKEYOP_DISABLE);
		//g_pD3DDevice8->SetTextureStageState(stage, D3DTSS_COLORSIGN, 0);
		//g_pD3DDevice8->SetTextureStageState(stage, D3DTSS_ALPHAKILL, D3DTALPHAKILL_DISABLE);
		g_pD3DDevice8->SetTextureStageState(stage, D3DTSS_COLOROP, D3DTOP_MODULATE); //  D3DTOP_DISABLE caused textures to go opaque
		g_pD3DDevice8->SetTextureStageState(stage, D3DTSS_COLORARG0, D3DTA_CURRENT);
		g_pD3DDevice8->SetTextureStageState(stage, D3DTSS_COLORARG1, D3DTA_TEXTURE);
		g_pD3DDevice8->SetTextureStageState(stage, D3DTSS_COLORARG2, D3DTA_CURRENT);
		g_pD3DDevice8->SetTextureStageState(stage, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);
		g_pD3DDevice8->SetTextureStageState(stage, D3DTSS_ALPHAARG0, D3DTA_CURRENT);
		g_pD3DDevice8->SetTextureStageState(stage, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
		g_pD3DDevice8->SetTextureStageState(stage, D3DTSS_ALPHAARG2, D3DTA_CURRENT);
		g_pD3DDevice8->SetTextureStageState(stage, D3DTSS_RESULTARG, D3DTA_CURRENT);
		g_pD3DDevice8->SetTextureStageState(stage, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_DISABLE);
		g_pD3DDevice8->SetTextureStageState(stage, D3DTSS_BUMPENVMAT00, 0);
		g_pD3DDevice8->SetTextureStageState(stage, D3DTSS_BUMPENVMAT01, 0);
		g_pD3DDevice8->SetTextureStageState(stage, D3DTSS_BUMPENVMAT11, 0);
		g_pD3DDevice8->SetTextureStageState(stage, D3DTSS_BUMPENVMAT10, 0);
		g_pD3DDevice8->SetTextureStageState(stage, D3DTSS_BUMPENVLSCALE, 0);
		g_pD3DDevice8->SetTextureStageState(stage, D3DTSS_BUMPENVLOFFSET, 0);
		g_pD3DDevice8->SetTextureStageState(stage, D3DTSS_TEXCOORDINDEX, stage);
		g_pD3DDevice8->SetTextureStageState(stage, D3DTSS_BORDERCOLOR, 0);
		//g_pD3DDevice8->SetTextureStageState(stage, D3DTSS_COLORKEYCOLOR, 0);
	}

    return g_EmuCDPD.hRet;
}

// ******************************************************************
// * patch: IDirect3DResource8_IsBusy
// ******************************************************************
BOOL WINAPI EMUPATCH(D3DDevice_IsBusy)()
{
	FUNC_EXPORTS

	LOG_FUNC();
    
    return FALSE;
}

// ******************************************************************
// * patch: D3DDevice_GetDisplayFieldStatus
// ******************************************************************
VOID WINAPI EMUPATCH(D3DDevice_GetDisplayFieldStatus)(X_D3DFIELD_STATUS *pFieldStatus)
{
	// NOTE: This can be unpatched only when NV2A does it's own VBlank and HLE _Swap function is unpatched
	FUNC_EXPORTS

	LOG_FUNC_ONE_ARG(pFieldStatus);

	// Read AV Flags to determine if Progressive or Interlaced
	// The xbox does this by reading from pDevice->m_Miniport.m_CurrentAvInfo
	// but we don't have an OOVPA for that. Instead, we call the Xbox implementation of 
	// D3DDevice_GetDisplayMode and read the result

	// Get a function pointer to the unpatched xbox function D3DDevice_GetDisplayMode
	XB_trampoline(VOID, WINAPI, D3DDevice_GetDisplayMode, (X_D3DDISPLAYMODE*));
	
	X_D3DDISPLAYMODE displayMode;

	// If we can find the Xbox version of GetDisplayMode, use the real data returned, otherwise
	// use a sensible default
	if (XB_D3DDevice_GetDisplayMode != nullptr) {
		XB_D3DDevice_GetDisplayMode(&displayMode);
	} else {
		// We don't show a warning because doing so pollutes the kernel debug log as this function gets called every frame
		displayMode.Flags = X_D3DPRESENTFLAG_INTERLACED;
	}
	
	// Set the VBlank count
	pFieldStatus->VBlankCount = g_VBData.VBlank;

	// If we are interlaced, return the current field, otherwise, return progressive scan
	if (displayMode.Flags & X_D3DPRESENTFLAG_INTERLACED) {
		pFieldStatus->Field = (g_VBData.VBlank % 2 == 0) ? X_D3DFIELD_ODD : X_D3DFIELD_EVEN;
	} else {
		pFieldStatus->Field = X_D3DFIELD_PROGRESSIVE;
	}
}

// ******************************************************************
// * patch: D3DDevice_BeginPush
// TODO: Find a test case and verify this
// At least one XDK has this as return VOID with a second input parameter.
// Is this definition incorrect, or did it change at some point?
// ******************************************************************
PDWORD WINAPI EMUPATCH(D3DDevice_BeginPush)(DWORD Count)
{
	FUNC_EXPORTS

	LOG_FUNC_ONE_ARG(Count);

	if (g_pPrimaryPB != nullptr)
	{
		EmuWarning("D3DDevice_BeginPush called without D3DDevice_EndPush in between?!");
		delete[] g_pPrimaryPB; // prevent a memory leak
	}

    DWORD *pRet = new DWORD[Count];

    g_dwPrimaryPBCount = Count;
    g_pPrimaryPB = pRet;

    return pRet;
}

// ******************************************************************
// * patch: D3DDevice_EndPush
// ******************************************************************
VOID WINAPI EMUPATCH(D3DDevice_EndPush)(DWORD *pPush)
{
	FUNC_EXPORTS

	LOG_FUNC_ONE_ARG(pPush);

#ifdef _DEBUG_TRACK_PB
//	DbgDumpPushBuffer(g_pPrimaryPB, g_dwPrimaryPBCount*sizeof(DWORD));
#endif

	if (g_pPrimaryPB == nullptr)
		EmuWarning("D3DDevice_EndPush called without preceding D3DDevice_BeginPush?!");
	else
	{
		EmuUnswizzleTextureStages();

		EmuExecutePushBufferRaw(g_pPrimaryPB);

		delete[] g_pPrimaryPB;
		g_pPrimaryPB = nullptr;
	}
}

// ******************************************************************
// * patch: D3DDevice_BeginVisibilityTest
// ******************************************************************
VOID WINAPI EMUPATCH(D3DDevice_BeginVisibilityTest)()
{
	FUNC_EXPORTS

	LOG_FUNC();

	LOG_UNIMPLEMENTED();
}

// ******************************************************************
// * patch: D3DDevice_EndVisibilityTest
// ******************************************************************
HRESULT WINAPI EMUPATCH(D3DDevice_EndVisibilityTest)
(
    DWORD                       Index
)
{
	FUNC_EXPORTS

	LOG_FUNC_ONE_ARG(Index);

	LOG_UNIMPLEMENTED();

    return D3D_OK;
}

// ******************************************************************
// * patch: D3DDevice_SetBackBufferScale
// ******************************************************************
VOID WINAPI EMUPATCH(D3DDevice_SetBackBufferScale)(FLOAT x, FLOAT y)
{
	FUNC_EXPORTS

	LOG_FUNC_BEGIN
		LOG_FUNC_ARG(x)
		LOG_FUNC_ARG(y)
		LOG_FUNC_END;

	LOG_IGNORED();
}

// ******************************************************************
// * patch: D3DDevice_GetVisibilityTestResult
// ******************************************************************
HRESULT WINAPI EMUPATCH(D3DDevice_GetVisibilityTestResult)
(
    DWORD                       Index,
    UINT                       *pResult,
    ULONGLONG                  *pTimeStamp
)
{
	FUNC_EXPORTS

	LOG_FUNC_BEGIN
		LOG_FUNC_ARG(Index)
		LOG_FUNC_ARG(pResult)
		LOG_FUNC_ARG(pTimeStamp)
		LOG_FUNC_END;

    // TODO: actually emulate this!?

    if(pResult != 0)
        *pResult = 640*480;

    if(pTimeStamp != 0)
        *pTimeStamp = 0;

    

    return D3D_OK;
}

// ******************************************************************
// * patch: D3DDevice_LoadVertexShader
// ******************************************************************
VOID WINAPI EMUPATCH(D3DDevice_LoadVertexShader)
(
    DWORD                       Handle,
    DWORD                       Address
)
{
	FUNC_EXPORTS

	LOG_FUNC_BEGIN
		LOG_FUNC_ARG(Handle)
		LOG_FUNC_ARG(Address)
		LOG_FUNC_END;

    if(Address < 136 && VshHandleIsVertexShader(Handle))
    {
        VERTEX_SHADER *pVertexShader = (VERTEX_SHADER *)(VshHandleGetVertexShader(Handle))->Handle;
        for (DWORD i = Address; i < pVertexShader->Size; i++)
        {
            // TODO: This seems very fishy
            g_VertexShaderSlots[i] = Handle;
        }
    }
}

// ******************************************************************
// * patch: D3DDevice_SelectVertexShader
// ******************************************************************
VOID WINAPI EMUPATCH(D3DDevice_SelectVertexShader)
(
    DWORD                       Handle,
    DWORD                       Address
)
{
	FUNC_EXPORTS

	LOG_FUNC_BEGIN
		LOG_FUNC_ARG(Handle)
		LOG_FUNC_ARG(Address)
		LOG_FUNC_END;

	HRESULT hRet;
	
	g_CurrentVertexShader = Handle;

    if(VshHandleIsVertexShader(Handle))
    {
        VERTEX_SHADER *pVertexShader = (VERTEX_SHADER *)(((X_D3DVertexShader *)(Handle & 0x7FFFFFFF))->Handle);
        hRet = g_pD3DDevice8->SetVertexShader(pVertexShader->Handle);
		DEBUG_D3DRESULT(hRet, "g_pD3DDevice8->SetVertexShader(VshHandleIsVertexShader)");
    }
    else if(Handle == NULL)
    {
		hRet = g_pD3DDevice8->SetVertexShader(D3DFVF_XYZ | D3DFVF_TEX0);
		DEBUG_D3DRESULT(hRet, "g_pD3DDevice8->SetVertexShader(D3DFVF_XYZ | D3DFVF_TEX0)");
	}
    else if(Address < 136)
    {
        X_D3DVertexShader *pVertexShader = (X_D3DVertexShader*)g_VertexShaderSlots[Address];

        if(pVertexShader != NULL)
        {
			hRet = g_pD3DDevice8->SetVertexShader(((VERTEX_SHADER *)((X_D3DVertexShader *)g_VertexShaderSlots[Address])->Handle)->Handle);
			DEBUG_D3DRESULT(hRet, "g_pD3DDevice8->SetVertexShader(pVertexShader)");
		}
        else
        {
            EmuWarning("g_VertexShaderSlots[%d] = 0", Address);
			hRet = D3D_OK;
		}
    }

	if (FAILED(hRet))
	{
		EmuWarning("We're lying about setting a vertext shader!");

		hRet = D3D_OK;
	}
}

// ******************************************************************
// * patch: D3DDevice_SetGammaRamp
// ******************************************************************
VOID WINAPI EMUPATCH(D3DDevice_SetGammaRamp)
(
    DWORD                   dwFlags,
    CONST X_D3DGAMMARAMP   *pRamp
)
{
	FUNC_EXPORTS

	LOG_FUNC_BEGIN
		LOG_FUNC_ARG(dwFlags)
		LOG_FUNC_ARG(pRamp)
		LOG_FUNC_END;

    // remove D3DSGR_IMMEDIATE
    DWORD dwPCFlags = dwFlags & (~0x00000002);
    D3DGAMMARAMP PCRamp;

    for(int v=0;v<255;v++)
    {
        PCRamp.red[v]   = pRamp->red[v];
        PCRamp.green[v] = pRamp->green[v];
        PCRamp.blue[v]  = pRamp->blue[v];
    }

//    g_pD3DDevice8->SetGammaRamp(dwPCFlags, &PCRamp);
}

// ******************************************************************
// * patch: D3DDevice_BeginStateBlock
// ******************************************************************
VOID WINAPI EMUPATCH(D3DDevice_BeginStateBlock)()
{
	FUNC_EXPORTS

	LOG_FUNC();

    ULONG ret = g_pD3DDevice8->BeginStateBlock();
	DEBUG_D3DRESULT(ret, "g_pD3DDevice8->BeginStateBlock");
}

/*// ******************************************************************
// * patch: D3DDevice_BeginStateBig
// ******************************************************************
HRESULT WINAPI EMUPATCH(D3DDevice_BeginStateBig)()
{
    FUNC_EXPORTS

   	LOG_FUNC();

    //ULONG hRet = g_pD3DDevice8->BeginStateBlock();
	//DEBUG_D3DRESULT(hRet, "g_pD3DDevice8->BeginStateBlock");

	LOG_UNIMPLEMENTED();
    CxbxKrnlCleanup("BeginStateBig is not implemented");    

    return hRet;
}*/

// ******************************************************************
// * patch: D3DDevice_CaptureStateBlock
// ******************************************************************
VOID WINAPI EMUPATCH(D3DDevice_CaptureStateBlock)(DWORD Token)
{
	FUNC_EXPORTS

	LOG_FUNC_ONE_ARG(Token);

    ULONG ret = g_pD3DDevice8->CaptureStateBlock(Token);
	DEBUG_D3DRESULT(ret, "g_pD3DDevice8->CaptureStateBlock");
}

// ******************************************************************
// * patch: D3DDevice_ApplyStateBlock
// ******************************************************************
VOID WINAPI EMUPATCH(D3DDevice_ApplyStateBlock)(DWORD Token)
{
	FUNC_EXPORTS

	LOG_FUNC_ONE_ARG(Token);

    ULONG ret = g_pD3DDevice8->ApplyStateBlock(Token);
	DEBUG_D3DRESULT(ret, "g_pD3DDevice8->ApplyStateBlock");
}

// ******************************************************************
// * patch: D3DDevice_EndStateBlock
// ******************************************************************
HRESULT WINAPI EMUPATCH(D3DDevice_EndStateBlock)(DWORD *pToken)
{
	FUNC_EXPORTS

	LOG_FUNC_ONE_ARG(pToken);

    ULONG ret = g_pD3DDevice8->EndStateBlock(pToken);
	DEBUG_D3DRESULT(ret, "g_pD3DDevice8->EndStateBlock");

    return ret;
}

// ******************************************************************
// * patch: D3DDevice_GetGammaRamp
// ******************************************************************
VOID WINAPI EMUPATCH(D3DDevice_GetGammaRamp)
(
    X_D3DGAMMARAMP     *pRamp
)
{
	FUNC_EXPORTS

	LOG_FUNC_ONE_ARG(pRamp);

    D3DGAMMARAMP *pGammaRamp = (D3DGAMMARAMP *)malloc(sizeof(D3DGAMMARAMP));

    g_pD3DDevice8->GetGammaRamp(pGammaRamp);

    for(int v=0;v<256;v++)
    {
        pRamp->red[v] = (BYTE)pGammaRamp->red[v];
        pRamp->green[v] = (BYTE)pGammaRamp->green[v];
        pRamp->blue[v] = (BYTE)pGammaRamp->blue[v];
    }

	free(pGammaRamp);
}

// ******************************************************************
// * patch: D3DDevice_GetBackBuffer2
// ******************************************************************
#define COPY_BACKBUFFER_TO_XBOX_SURFACE // Uncomment to enable writing Host Backbuffers back to Xbox surfaces
X_D3DSurface* WINAPI EMUPATCH(D3DDevice_GetBackBuffer2)
(
    INT                 BackBuffer
)
{
	FUNC_EXPORTS

	LOG_FUNC_ONE_ARG(BackBuffer);

	X_D3DSurface* pXboxBackBuffer = nullptr;


#ifndef COPY_BACKBUFFER_TO_XBOX_SURFACE
	/** unsafe, somehow
	HRESULT hRet = D3D_OK;

	X_D3DSurface *pBackBuffer = EmuNewD3DSurface();

	if(BackBuffer == -1)
	{
	static IDirect3DSurface8 *pCachedPrimarySurface = nullptr;

	if(pCachedPrimarySurface == nullptr)
	{
	// create a buffer to return
	// TODO: Verify the surface is always 640x480
	hRet = g_pD3DDevice8->CreateImageSurface(640, 480, D3DFMT_A8R8G8B8, &pCachedPrimarySurface);
	DEBUG_D3DRESULT(hRet, "g_pD3DDevice8->CreateImageSurface");
	}

	SetHostSurface(pBackBuffer, pCachedPrimarySurface);

	hRet = g_pD3DDevice8->GetFrontBuffer(pCachedPrimarySurface);
	DEBUG_D3DRESULT(hRet, "g_pD3DDevice8->GetFrontBuffer");

	if(FAILED(hRet))
	{
	EmuWarning("Could not retrieve primary surface, using backbuffer");
	SetHostSurface(pBackBuffer, nullptr);
	pCachedPrimarySurface->Release();
	pCachedPrimarySurface = nullptr;
	BackBuffer = 0;
	}

	// Debug: Save this image temporarily
	//D3DXSaveSurfaceToFile("C:\\Aaron\\Textures\\FrontBuffer.bmp", D3DXIFF_BMP, GetHostSurface(pBackBuffer), NULL, NULL);
	}

	if(BackBuffer != -1) {
	hRet = g_pD3DDevice8->GetBackBuffer(BackBuffer, D3DBACKBUFFER_TYPE_MONO, &pCachedPrimarySurface);
	DEBUG_D3DRESULT(hRet, "g_pD3DDevice8->GetBackBuffer");
	}
	//*/

	static X_D3DSurface *pBackBuffer = EmuNewD3DSurface();
	IDirect3DSurface8 *pNewHostSurface = nullptr;

	 STATUS_SUCCESS;

	 if (BackBuffer == -1) {
		 BackBuffer = 0;
	 }

	HRESULT hRet = g_pD3DDevice8->GetBackBuffer(BackBuffer, D3DBACKBUFFER_TYPE_MONO, &pNewHostSurface);
	DEBUG_D3DRESULT(hRet, "g_pD3DDevice8->GetBackBuffer");

	if (FAILED(hRet))
		CxbxKrnlCleanup("Unable to retrieve back buffer");

	SetHostSurface(pBackBuffer, pNewHostSurface);

	// Increment reference count
	pBackBuffer->Common++; // EMUPATCH(D3DResource_AddRef)(pBackBuffer);

	return pBackBuffer;
#else
	// Rather than create a new surface, we should forward to the Xbox version of GetBackBuffer,
	// This gives us the correct Xbox surface to update.
	// We get signatures for both backbuffer functions as it changed in later XDKs
	XB_trampoline(X_D3DSurface*, WINAPI, D3DDevice_GetBackBuffer2, (INT));

	XB_trampoline(VOID, WINAPI, D3DDevice_GetBackBuffer, (INT, D3DBACKBUFFER_TYPE, X_D3DSurface**));

	// This also updates the reference count, so we don't need to do this ourselves
	if (XB_D3DDevice_GetBackBuffer != nullptr) {
		XB_D3DDevice_GetBackBuffer(BackBuffer, D3DBACKBUFFER_TYPE_MONO, &pXboxBackBuffer);
	}
	else {
		pXboxBackBuffer = XB_D3DDevice_GetBackBuffer2(BackBuffer);
	}

	// Now pXboxBackbuffer points to the requested Xbox backbuffer
	if (pXboxBackBuffer == nullptr) {
		EmuWarning("D3DDevice_GetBackBuffer2: Could not get Xbox backbuffer, creating a fake backbuffer");
		pXboxBackBuffer = EmuNewD3DSurface();
	}

	// Now we can fetch the host backbuffer
	IDirect3DSurface8 *pNewHostSurface = nullptr;


	HRESULT hRet = STATUS_SUCCESS;

	if (BackBuffer == -1) {
		BackBuffer = 0;
	}

	hRet = g_pD3DDevice8->GetBackBuffer(BackBuffer, D3DBACKBUFFER_TYPE_MONO, &pNewHostSurface);
	DEBUG_D3DRESULT(hRet, "g_pD3DDevice8->GetBackBuffer");

	if (FAILED(hRet)) {
		CxbxKrnlCleanup("Unable to retrieve back buffer");
	}

	D3DSURFACE_DESC hostSurfaceDesc;
	if (pNewHostSurface->GetDesc(&hostSurfaceDesc) != D3D_OK) {
		EmuWarning("Could not get Back Buffer Host Surface Desc");
		goto skip_backbuffer_copy;
	}

	// Get the host surface for the destination Xbox surface
	IDirect3DSurface8* pCopySrcSurface = GetHostSurface(pXboxBackBuffer);
		
	// Now we can copy the host back buffer to the Xbox
	D3DLOCKED_RECT lockedRect;
	hRet = pNewHostSurface->LockRect(&lockedRect, NULL, D3DLOCK_READONLY);
	if (hRet != STATUS_SUCCESS) {
		EmuWarning("Could not lock Host Back Buffer");
		goto skip_backbuffer_copy;
	}

	// Do the copy from the host backbuffer to the host surface representing the backbuffer
	// This handles format conversion back to Xbox format (assuming the host surface is the same format), and makes sure our HostResource is up to date
	// That way if it gets used as a trex
	D3DXLoadSurfaceFromMemory(pCopySrcSurface, NULL, NULL, lockedRect.pBits, hostSurfaceDesc.Format, lockedRect.Pitch, NULL, NULL, 0, 0);
	pNewHostSurface->UnlockRect();

	if (pNewHostSurface->GetDesc(&hostSurfaceDesc) != D3D_OK) {
		EmuWarning("Could not get Xbox Back Buffer Host Surface Desc");
		goto skip_backbuffer_copy;
	}

	// Finally, do the copy from the converted host resource to the xbox resource
	hRet = pCopySrcSurface->LockRect(&lockedRect, NULL, D3DLOCK_READONLY);
	if (hRet != STATUS_SUCCESS) {
		EmuWarning("Could not lock Host Resource for Xbox Back Buffer");
		goto skip_backbuffer_copy;
	}
	
	memcpy(pXboxBackBuffer, lockedRect.pBits, hostSurfaceDesc.Size);

	pCopySrcSurface->UnlockRect();
	
	
skip_backbuffer_copy:
    return pXboxBackBuffer;
#endif
}

// ******************************************************************
// * patch: D3DDevice_GetBackBuffer
// ******************************************************************
VOID WINAPI EMUPATCH(D3DDevice_GetBackBuffer)
(
    INT                 BackBuffer,
    D3DBACKBUFFER_TYPE  Type,
    X_D3DSurface      **ppBackBuffer
)
{
	FUNC_EXPORTS

	LOG_FORWARD("D3DDevice_GetBackBuffer2");

    *ppBackBuffer = EMUPATCH(D3DDevice_GetBackBuffer2)(BackBuffer);
}

// ******************************************************************
// * patch: D3DDevice_SetViewport
// ******************************************************************
VOID WINAPI EMUPATCH(D3DDevice_SetViewport)
(
    CONST D3DVIEWPORT8 *pViewport
)
{
	FUNC_EXPORTS

	LOG_FUNC_ONE_ARG(pViewport);

    DWORD dwWidth  = pViewport->Width;
    DWORD dwHeight = pViewport->Height;

    // resize to fit screen (otherwise crashes occur)
    /*{
        if(dwWidth > 640)
        {
            EmuWarning("Resizing Viewport->Width to 640");
            ((D3DVIEWPORT8*)pViewport)->Width = 640;
        }

        if(dwHeight > 480)
        {
            EmuWarning("Resizing Viewport->Height to 480");
            ((D3DVIEWPORT8*)pViewport)->Height = 480;
        }
    }*/

    HRESULT hRet = g_pD3DDevice8->SetViewport(pViewport);

    // restore originals
    /*{
        if(dwWidth > 640)
            ((D3DVIEWPORT8*)pViewport)->Width = dwWidth;

        if(dwHeight > 480)
            ((D3DVIEWPORT8*)pViewport)->Height = dwHeight;
    }

	DEBUG_D3DRESULT(hRet, "g_pD3DDevice8->SetViewport");

	if(FAILED(hRet))
    {
        EmuWarning("Unable to set viewport! We're lying");
        hRet = D3D_OK;
    }*/
}

// ******************************************************************
// * patch: D3DDevice_GetViewport
// ******************************************************************
VOID WINAPI EMUPATCH(D3DDevice_GetViewport)
(
    D3DVIEWPORT8 *pViewport
)
{
	FUNC_EXPORTS

	LOG_FUNC_ONE_ARG(pViewport);

    HRESULT hRet = g_pD3DDevice8->GetViewport(pViewport);
	DEBUG_D3DRESULT(hRet, "g_pD3DDevice8->GetViewport");

    if(FAILED(hRet))
    {
        EmuWarning("Unable to get viewport! - We're lying");

        hRet = D3D_OK;
    }
}

// ******************************************************************
// * patch: D3DDevice_GetViewportOffsetAndScale
// ******************************************************************
VOID WINAPI EMUPATCH(D3DDevice_GetViewportOffsetAndScale)
(
    D3DXVECTOR4 *pOffset,
    D3DXVECTOR4 *pScale
)
{
	FUNC_EXPORTS

	LOG_FUNC_BEGIN
		LOG_FUNC_ARG(pOffset)
		LOG_FUNC_ARG(pScale)
		LOG_FUNC_END;

    float fScaleX = 1.0f;
    float fScaleY = 1.0f;
    float fScaleZ = 1.0f;
    float fOffsetX = 0.5 + 1.0/32;
    float fOffsetY = 0.5 + 1.0/32;
    D3DVIEWPORT8 Viewport;

    
	EMUPATCH(D3DDevice_GetViewport)(&Viewport);
    


    pScale->x = 1.0f;
    pScale->y = 1.0f;
    pScale->z = 1.0f;
    pScale->w = 1.0f;

    pOffset->x = 0.0f;
    pOffset->y = 0.0f;
    pOffset->z = 0.0f;
    pOffset->w = 0.0f;

/*
    pScale->x = (float)Viewport.Width * 0.5f * fScaleX;
    pScale->y = (float)Viewport.Height * -0.5f * fScaleY;
    pScale->z = (Viewport.MaxZ - Viewport.MinZ) * fScaleZ;
    pScale->w = 0;

    pOffset->x = (float)Viewport.Width * fScaleX * 0.5f + (float)Viewport.X * fScaleX + fOffsetX;
    pOffset->y = (float)Viewport.Height * fScaleY * 0.5f + (float)Viewport.Y * fScaleY + fOffsetY;
    pOffset->z = Viewport.MinZ * fScaleZ;
    pOffset->w = 0;
*/

    
}

// ******************************************************************
// * patch: D3DDevice_SetShaderConstantMode
// ******************************************************************
VOID WINAPI EMUPATCH(D3DDevice_SetShaderConstantMode)
(
    X_VERTEXSHADERCONSTANTMODE Mode
)
{
	FUNC_EXPORTS

	LOG_FUNC_ONE_ARG(Mode);

    g_VertexShaderConstantMode = Mode;
}

// ******************************************************************
// * patch: D3DDevice_GetRenderTarget
// ******************************************************************
HRESULT WINAPI EMUPATCH(D3DDevice_GetRenderTarget)
(
    X_D3DSurface  **ppRenderTarget
)
{
	FUNC_EXPORTS

	LOG_FORWARD("D3DDevice_GetRenderTarget2");

	*ppRenderTarget = EMUPATCH(D3DDevice_GetRenderTarget2)();

    return D3D_OK;
}

// ******************************************************************
// * patch: D3DDevice_GetRenderTarget2
// ******************************************************************
X_D3DSurface * WINAPI EMUPATCH(D3DDevice_GetRenderTarget2)()
{
	FUNC_EXPORTS

	LOG_FUNC();

	X_D3DSurface *result = g_pCachedRenderTarget;

	if (result)
		result->Common++; // EMUPATCH(D3DResource_AddRef)(result);

    RETURN(result);
}

// ******************************************************************
// * patch: D3DDevice_GetDepthStencilSurface
// ******************************************************************
HRESULT WINAPI EMUPATCH(D3DDevice_GetDepthStencilSurface)
(
    X_D3DSurface  **ppZStencilSurface
)
{
	FUNC_EXPORTS

	LOG_FORWARD("D3DDevice_GetDepthStencilSurface2");

    *ppZStencilSurface = EMUPATCH(D3DDevice_GetDepthStencilSurface2)();

    return D3D_OK;
}

// ******************************************************************
// * patch: D3DDevice_GetDepthStencilSurface2
// ******************************************************************
X_D3DSurface * WINAPI EMUPATCH(D3DDevice_GetDepthStencilSurface2)()
{
	FUNC_EXPORTS

	LOG_FUNC();

	X_D3DSurface *result = g_pCachedDepthStencil;

	if (result)
		result->Common++; // EMUPATCH(D3DResource_AddRef)(result);
		
	RETURN(result);
}

// ******************************************************************
// * patch: D3DDevice_CreatePalette
// ******************************************************************
HRESULT WINAPI EMUPATCH(D3DDevice_CreatePalette)
(
	X_D3DPALETTESIZE    Size,
	X_D3DPalette      **ppPalette
	)
{
	FUNC_EXPORTS

	LOG_FORWARD("D3DDevice_CreatePalette2");

	*ppPalette = EMUPATCH(D3DDevice_CreatePalette2)(Size);

	return D3D_OK;
}

// ******************************************************************
// * patch: D3DDevice_CreatePalette2
// ******************************************************************
X_D3DPalette * WINAPI EMUPATCH(D3DDevice_CreatePalette2)
(
	X_D3DPALETTESIZE    Size
)
{
	FUNC_EXPORTS

	LOG_FUNC_ONE_ARG(Size);

	X_D3DPalette *pPalette = EmuNewD3DPalette();

	pPalette->Common |= (Size << X_D3DPALETTE_COMMON_PALETTESIZE_SHIFT);
	pPalette->Data = (DWORD)g_VMManager.Allocate(XboxD3DPaletteSizeToBytes(Size), PageType::Contiguous, 0, (~((::ULONG_PTR)0)), PAGE_SIZE, PAGE_EXECUTE_READWRITE);
	pPalette->Lock = X_D3DRESOURCE_LOCK_PALETTE; // emulated reference count for palettes

												 // TODO: Should't we register the palette with a call to
												 // EmuIDirect3DResource8_Register? So far, it doesn't look
												 // like the palette registration code gets used.  If not, then we
												 // need to cache the palette manually during any calls to
												 // EmuD3DDevice_SetPalette for 8-bit textures to work properly.

	DbgPrintf("pPalette: = 0x%.08X\n", pPalette);



	return pPalette;
}


// ******************************************************************
// * patch: D3DDevice_CreateVertexShader
// ******************************************************************
HRESULT WINAPI EMUPATCH(D3DDevice_CreateVertexShader)
(
    CONST DWORD    *pDeclaration,
    CONST DWORD    *pFunction,
    DWORD          *pHandle,
    DWORD           Usage
)
{
	FUNC_EXPORTS

	LOG_FUNC_BEGIN
		LOG_FUNC_ARG(pDeclaration)
		LOG_FUNC_ARG(pFunction)
		LOG_FUNC_ARG(pHandle)
		LOG_FUNC_ARG_TYPE(X_D3DUSAGE, Usage)
		LOG_FUNC_END;

    // create emulated shader struct
    X_D3DVertexShader *pD3DVertexShader = (X_D3DVertexShader*)g_VMManager.AllocateZeroed(sizeof(X_D3DVertexShader));
    VERTEX_SHADER     *pVertexShader = (VERTEX_SHADER*)g_VMManager.AllocateZeroed(sizeof(VERTEX_SHADER));

    // TODO: Intelligently fill out these fields as necessary

    // HACK: TODO: support this situation
    if(pDeclaration == NULL)
    {
        *pHandle = NULL;

        

        return D3D_OK;
    }

    LPD3DXBUFFER pRecompiledBuffer = NULL;
    DWORD        *pRecompiledDeclaration = NULL;
    DWORD        *pRecompiledFunction = NULL;
    DWORD        VertexShaderSize = 0;
    DWORD        DeclarationSize = 0;
    DWORD        Handle = 0;

    HRESULT hRet = EmuRecompileVshDeclaration((DWORD*)pDeclaration,
                                                   &pRecompiledDeclaration,
                                                   &DeclarationSize,
                                                   pFunction == NULL,
                                                   &pVertexShader->VertexDynamicPatch);

    if(SUCCEEDED(hRet) && pFunction)
    {
		boolean bUseDeclarationOnly = 0;

        hRet = EmuRecompileVshFunction((DWORD*)pFunction,
                                            &pRecompiledBuffer,
                                            &VertexShaderSize,
                                            g_VertexShaderConstantMode == X_D3DSCM_NORESERVEDCONSTANTS,
											&bUseDeclarationOnly);
        if(SUCCEEDED(hRet))
        {
			if(!bUseDeclarationOnly)
				pRecompiledFunction = (DWORD*)pRecompiledBuffer->GetBufferPointer();
			else
				pRecompiledFunction = NULL;
        }
        else
        {
            pRecompiledFunction = NULL;
            EmuWarning("Couldn't recompile vertex shader function.");
            hRet = D3D_OK; // Try using a fixed function vertex shader instead
        }
    }

    //DbgPrintf("MaxVertexShaderConst = %d\n", g_D3DCaps.MaxVertexShaderConst);

    if(SUCCEEDED(hRet))
    {
        hRet = g_pD3DDevice8->CreateVertexShader
        (
            pRecompiledDeclaration,
            pRecompiledFunction,
            &Handle,
            g_dwVertexShaderUsage   // TODO: HACK: Xbox has extensions!
        );
		DEBUG_D3DRESULT(hRet, "g_pD3DDevice8->CreateVertexShader");

        if(pRecompiledBuffer != nullptr)
        {
            pRecompiledBuffer->Release();
            pRecompiledBuffer = nullptr;
        }

        //* Fallback to dummy shader.
        if(FAILED(hRet))
        {
            static const char dummy[] =
                "vs.1.1\n"
				"dp4 oPos.x, v0, c96\n"
				"dp4 oPos.y, v0, c97\n"
				"dp4 oPos.z, v0, c98\n"
				"dp4 oPos.w, v0, c99\n";

            EmuWarning("Trying fallback:\n%s", dummy);
            hRet = D3DXAssembleShader(dummy,
                                      strlen(dummy),
                                      D3DXASM_SKIPVALIDATION,
                                      NULL,
                                      &pRecompiledBuffer,
                                      NULL);
			DEBUG_D3DRESULT(hRet, "D3DXAssembleShader");

			hRet = g_pD3DDevice8->CreateVertexShader
            (
                pRecompiledDeclaration,
                (DWORD*)pRecompiledBuffer->GetBufferPointer(),
                &Handle,
                g_dwVertexShaderUsage
            );
			DEBUG_D3DRESULT(hRet, "g_pD3DDevice8->CreateVertexShader(fallback)");
		}
        //*/
    }
    // Save the status, to remove things later
    pVertexShader->Status = hRet;

    free(pRecompiledDeclaration);

    pVertexShader->pDeclaration = (DWORD*)g_VMManager.Allocate(DeclarationSize);
    memcpy(pVertexShader->pDeclaration, pDeclaration, DeclarationSize);

    pVertexShader->FunctionSize = 0;
    pVertexShader->pFunction = NULL;
    pVertexShader->Type = X_VST_NORMAL;
    pVertexShader->Size = (VertexShaderSize - sizeof(VSH_SHADER_HEADER)) / VSH_INSTRUCTION_SIZE_BYTES;
    pVertexShader->DeclarationSize = DeclarationSize;

    if(SUCCEEDED(hRet))
    {
        if(pFunction != NULL)
        {
            pVertexShader->pFunction = (DWORD*)g_VMManager.Allocate(VertexShaderSize);
            memcpy(pVertexShader->pFunction, pFunction, VertexShaderSize);
            pVertexShader->FunctionSize = VertexShaderSize;
        }
        else
        {
            pVertexShader->pFunction = NULL;
            pVertexShader->FunctionSize = 0;
        }
        pVertexShader->Handle = Handle;
    }
    else
    {
        pVertexShader->Handle = D3DFVF_XYZ | D3DFVF_TEX0;
    }

    pD3DVertexShader->Handle = (DWORD)pVertexShader;

	*pHandle = (DWORD)pD3DVertexShader; // DON'T collide with MM_SYSTEM_PHYSICAL_MAP (see VshHandleIsFVF and VshHandleIsVertexShader)

    if(FAILED(hRet))
    {
#ifdef _DEBUG_TRACK_VS
        if (pFunction)
        {
            char pFileName[30];
            static int FailedShaderCount = 0;
            VSH_SHADER_HEADER *pHeader = (VSH_SHADER_HEADER*)pFunction;
            EmuWarning("Couldn't create vertex shader!");
            sprintf(pFileName, "failed%05d.xvu", FailedShaderCount);
            FILE *f = fopen(pFileName, "wb");
            if(f)
            {
                fwrite(pFunction, sizeof(VSH_SHADER_HEADER) + pHeader->NumInst * 16, 1, f);
                fclose(f);
            }
            FailedShaderCount++;
        }
#endif // _DEBUG_TRACK_VS
        //hRet = D3D_OK;
    }

    

    return hRet;
}

// ******************************************************************
// * patch: D3DDevice_SetPixelShaderConstant
// ******************************************************************
VOID WINAPI EMUPATCH(D3DDevice_SetPixelShaderConstant)
(
    DWORD       Register,
    CONST PVOID pConstantData,
    DWORD       ConstantCount
)
{
	FUNC_EXPORTS

	LOG_FUNC_BEGIN
		LOG_FUNC_ARG(Register)
		LOG_FUNC_ARG(pConstantData)
		LOG_FUNC_ARG(ConstantCount)
		LOG_FUNC_END;

	// TODO: This hack is necessary for Vertex Shaders on XDKs prior to 4361, but if this
	// causes problems with pixel shaders, feel free to comment out the hack below.
	if(g_BuildVersion <= 4361)
		Register += 96;

    HRESULT hRet = g_pD3DDevice8->SetPixelShaderConstant
    (
        Register,
        pConstantData,
        ConstantCount
    );
	DEBUG_D3DRESULT(hRet, "g_pD3DDevice8->SetPixelShaderConstant");

    if(FAILED(hRet))
    {
        EmuWarning("We're lying about setting a pixel shader constant!");

        hRet = D3D_OK;
    }
}

// ******************************************************************
// * patch: D3DDevice_SetVertexShaderConstant
// ******************************************************************
VOID WINAPI EMUPATCH(D3DDevice_SetVertexShaderConstant)
(
    INT         Register,
    CONST PVOID pConstantData,
    DWORD       ConstantCount
)
{
	FUNC_EXPORTS

	LOG_FUNC_BEGIN
		LOG_FUNC_ARG(Register)
		LOG_FUNC_ARG(pConstantData)
		LOG_FUNC_ARG(ConstantCount)
		LOG_FUNC_END;

/*#ifdef _DEBUG_TRACK_VS_CONST
    for (uint32 i = 0; i < ConstantCount; i++)
    {
        printf("SetVertexShaderConstant, c%d (c%d) = { %f, %f, %f, %f }\n",
               Register - 96 + i, Register + i,
               *((float*)pConstantData + 4 * i),
               *((float*)pConstantData + 4 * i + 1),
               *((float*)pConstantData + 4 * i + 2),
               *((float*)pConstantData + 4 * i + 3));
    }
#endif*/ // _DEBUG_TRACK_VS_CONST

	// TODO: HACK: Since Xbox vertex shader constants range from -96 to 96, during conversion
	// some shaders need to add 96 to use ranges 0 to 192.  This fixes 3911 - 4361 games and XDK
	// samples, but breaks Turok.

	if(g_BuildVersion <= 4361)
		Register += 96;

    HRESULT hRet = g_pD3DDevice8->SetVertexShaderConstant
    (
        Register,
        pConstantData,
        ConstantCount
    );
	DEBUG_D3DRESULT(hRet, "g_pD3DDevice8->SetVertexShaderConstant");

    if(FAILED(hRet))
    {
        EmuWarning("We're lying about setting a vertex shader constant!");
        hRet = D3D_OK;
    }   
}

// ******************************************************************
// * patch: D3DDevice_SetVertexShaderConstant1
// ******************************************************************
VOID __fastcall EMUPATCH(D3DDevice_SetVertexShaderConstant1)
(
    INT         Register,
    CONST PVOID pConstantData
)
{
	FUNC_EXPORTS

	LOG_FORWARD("D3DDevice_SetVertexShaderConstant");

    EMUPATCH(D3DDevice_SetVertexShaderConstant)(Register, pConstantData, 1);
}

// ******************************************************************
// * patch: D3DDevice_SetVertexShaderConstant1Fast
// ******************************************************************
VOID __fastcall EMUPATCH(D3DDevice_SetVertexShaderConstant1Fast)
(
    INT         Register,
    CONST PVOID pConstantData
)
{
	FUNC_EXPORTS

	LOG_FORWARD("D3DDevice_SetVertexShaderConstant");

	// Redirect to the standard version.

    EMUPATCH(D3DDevice_SetVertexShaderConstant)(Register, pConstantData, 1);
}

// ******************************************************************
// * patch: D3DDevice_SetVertexShaderConstant4
// ******************************************************************
VOID __fastcall EMUPATCH(D3DDevice_SetVertexShaderConstant4)
(
    INT         Register,
    CONST PVOID pConstantData
)
{
	FUNC_EXPORTS

	LOG_FORWARD("D3DDevice_SetVertexShaderConstant");

	EMUPATCH(D3DDevice_SetVertexShaderConstant)(Register, pConstantData, 4);
}

// ******************************************************************
// * patch: D3DDevice_SetVertexShaderConstantNotInline
// ******************************************************************
VOID __fastcall EMUPATCH(D3DDevice_SetVertexShaderConstantNotInline)
(
    INT         Register,
    CONST PVOID pConstantData,
    DWORD       ConstantCount
)
{
	FUNC_EXPORTS

	LOG_FORWARD("D3DDevice_SetVertexShaderConstant");

	EMUPATCH(D3DDevice_SetVertexShaderConstant)(Register, pConstantData, ConstantCount / 4);
}

// ******************************************************************
// * patch: D3DDevice_SetVertexShaderConstantNotInlineFast
// ******************************************************************
VOID __fastcall EMUPATCH(D3DDevice_SetVertexShaderConstantNotInlineFast)
(
    INT         Register,
    CONST PVOID pConstantData,
    DWORD       ConstantCount
)
{
	FUNC_EXPORTS

	LOG_FORWARD("D3DDevice_SetVertexShaderConstant");

	// Redirect to the standard version.

	EMUPATCH(D3DDevice_SetVertexShaderConstant)(Register, pConstantData, ConstantCount / 4);
}

// Cache pixel shaders, the main reason for this is so that
// CreatePixelShader can return a good Xbox handle rather than 
// a host handle
std::map<DWORD, DWORD> g_HostPixelShaderCache;

// ******************************************************************
// * patch: D3DDevice_DeletePixelShader
// ******************************************************************
VOID WINAPI EMUPATCH(D3DDevice_DeletePixelShader)
(
    DWORD          Handle
)
{
	FUNC_EXPORTS
	LOG_FUNC_ONE_ARG(Handle);

	// Forward to the Xbox version of DeletePixelShader, to free the xbox resource correctly
	XB_trampoline(VOID, WINAPI, D3DDevice_DeletePixelShader, (DWORD));

	XB_D3DDevice_DeletePixelShader(Handle);

	DWORD hostShaderHandle = 0;

	// Fetch the host handle
	auto it = g_HostPixelShaderCache.find(Handle);
	if (it != g_HostPixelShaderCache.end()) {
		hostShaderHandle = it->second;
		g_HostPixelShaderCache.erase(it);
	}

    if(hostShaderHandle == X_PIXELSHADER_FAKE_HANDLE)
    {
        // Do Nothing!
    }
    else
    {
        HRESULT hRet = g_pD3DDevice8->DeletePixelShader(hostShaderHandle);
		DEBUG_D3DRESULT(hRet, "g_pD3DDevice8->DeletePixelShader");
	}

	/*PIXEL_SHADER *pPixelShader = (PIXEL_SHADER*)Handle;

	if (pPixelShader)
	{
		if(pPixelShader->Handle != X_PIXELSHADER_FAKE_HANDLE)
		{
			HRESULT hRet = g_pD3DDevice8->DeletePixelShader(pPixelShader->Handle);
			DEBUG_D3DRESULT(hRet, "g_pD3DDevice8->DeletePixelShader");
		}

		g_VMManager.Deallocate((VAddr)pPixelShader);
	}*/
}

// ******************************************************************
// * patch: D3DDevice_CreatePixelShader
// ******************************************************************
HRESULT WINAPI EMUPATCH(D3DDevice_CreatePixelShader)
(
    X_D3DPIXELSHADERDEF    *pPSDef,
    DWORD				   *pHandle
)
{
	FUNC_EXPORTS

	LOG_FUNC_BEGIN
		LOG_FUNC_ARG(pPSDef)
		LOG_FUNC_ARG(pHandle)
		LOG_FUNC_END;

	HRESULT hRet = E_FAIL;
	bool noXboxCreate = false;

	// Call the Xbox version of CreatePixelShader, this gives us a handle we can safely return to Xbox code
	// and properly sets up the internal shader structure, pointed to by the handle
	XB_trampoline(HRESULT, WINAPI, D3DDevice_CreatePixelShader, (X_D3DPIXELSHADERDEF*, DWORD*));

	if (XB_D3DDevice_CreatePixelShader) {
		hRet = XB_D3DDevice_CreatePixelShader(pPSDef, pHandle);

		// If the pixel shader function succeeded, continue, else, return
		if (FAILED(hRet)) {
			EmuWarning("XB_D3DDevice_CreatePixelShader Failed");
			RETURN(hRet);
		}
	} else {
		// We couldn't find the Xbox CreatePixelShader function
		// This may happen if this function is called by SetPixelShaderProgram
		noXboxCreate = true;
	}

	DWORD hostShaderHandle = 0;

	// If PixelShader Disable Hack is enabled, return a dummy handle
	if (g_DisablePixelShaders) {
		g_HostPixelShaderCache[*pHandle] = X_PIXELSHADER_FAKE_HANDLE;
		RETURN(D3D_OK);
	}

#if 0 // PatrickvL Dxbx pixel shader translation
	// Attempt to recompile PixelShader
	hRet = DxbxUpdateActivePixelShader(pPSDef, &hostShaderHandle);
	// redirect to windows d3d
	DEBUG_D3DRESULT(hRet, "g_pD3DDevice8->CreatePixelShader");
#endif
#if 1 // Kingofc's pixel shader translation
	DWORD* pFunction = NULL;
	LPD3DXBUFFER pRecompiledBuffer = NULL;

	hRet = CreatePixelShaderFunction(pPSDef, &pRecompiledBuffer);
	DEBUG_D3DRESULT(hRet, "CreatePixelShaderFunction");

	if (SUCCEEDED(hRet))
	{
		pFunction = (DWORD*)pRecompiledBuffer->GetBufferPointer();

		// Redirect to Windows D3D
		hRet = g_pD3DDevice8->CreatePixelShader
		(
			pFunction,
			&hostShaderHandle
			/*&Handle*/
		);
		DEBUG_D3DRESULT(hRet, "g_pD3DDevice8->CreatePixelShader");
	}

	if (pRecompiledBuffer != nullptr)
	{
		pRecompiledBuffer->Release();
	}

	// This additional layer of Cxbx internal indirection seems problematic, as 
	// CreatePixelShader() is expected to return a pHandle directly to a shader interface.

	/*
	PIXEL_SHADER *pPixelShader = (PIXEL_SHADER*)g_VMManager.AllocateZeroed(sizeof(PIXEL_SHADER)); // Clear, to prevent side-effects on random contents

	memcpy(&pPixelShader->PSDef, pPSDef, sizeof(X_D3DPIXELSHADERDEF));

	DWORD Handle = 0; // ??
	pPixelShader->Handle = Handle;
	pPixelShader->dwStatus = hRet;
	*pHandle = (DWORD)pPixelShader;
	*/
#endif
#if 0 // Older Cxbx pixel shader translation
	DWORD* pFunction = NULL;

	pFunction = (DWORD*) pPSDef;

	// Attempt to recompile PixelShader
	EmuRecompilePshDef( pPSDef, NULL );

    // redirect to windows d3d
    hRet = g_pD3DDevice8->CreatePixelShader
    (
        pFunction,
        pHandle
    );
	DEBUG_D3DRESULT(hRet, "g_pD3DDevice8->CreatePixelShader");
#endif

    if(FAILED(hRet))
    {
        hostShaderHandle = X_PIXELSHADER_FAKE_HANDLE;

		// This is called too frequently as Azurik creates and destroys a
		// pixel shader every frame, and makes debugging harder.
		// EmuWarning("We're lying about the creation of a pixel shader!");

        hRet = D3D_OK;
    }
    else
    {
        DbgPrintf("pHandle = 0x%.08X (0x%.08X)\n", pHandle, *pHandle);
    }

	// If the Xbox CreatePixelShader could not be found, set the handle
	// has the host handle
	if (noXboxCreate) {
		*pHandle = hostShaderHandle;
	} 

	g_HostPixelShaderCache[*pHandle] = hostShaderHandle;

	RETURN(hRet);
}

// ******************************************************************
// * patch: D3DDevice_SetPixelShader
// ******************************************************************
VOID WINAPI EMUPATCH(D3DDevice_SetPixelShader)
(
    DWORD           Handle
)
{
	FUNC_EXPORTS

	LOG_FUNC_ONE_ARG(Handle);

    // Redirect to Windows D3D
    HRESULT hRet = D3D_OK;

	DWORD hostShaderHandle = 0;

	// Fetch the host handle
	auto it = g_HostPixelShaderCache.find(Handle);
	if (it != g_HostPixelShaderCache.end()) {
		hostShaderHandle = it->second;
	}

    // Fake Programmable Pipeline
    if(hostShaderHandle == X_PIXELSHADER_FAKE_HANDLE)
    {
        // programmable pipeline
        //*
        static DWORD dwHandle = 0;

        if(dwHandle == 0)
        {
            // simplest possible pixel shader, simply output the texture input
            static const char szDiffusePixelShader[] =
                "ps.1.0\n"
                "tex t0\n"
                "mov r0, t0\n";

            LPD3DXBUFFER pShader = 0;
            LPD3DXBUFFER pErrors = 0;

            // assemble the shader
            D3DXAssembleShader(szDiffusePixelShader, strlen(szDiffusePixelShader) - 1, 0, NULL, &pShader, &pErrors);

            // create the shader device handle
            hRet = g_pD3DDevice8->CreatePixelShader((DWORD*)pShader->GetBufferPointer(), &dwHandle);
			DEBUG_D3DRESULT(hRet, "g_pD3DDevice8->CreatePixelShader");
			g_dwCurrentPixelShader = 0;
        }

		if (SUCCEEDED(hRet))
		{
			hRet = g_pD3DDevice8->SetPixelShader(g_iWireframe == 0 ? dwHandle : 0);
			DEBUG_D3DRESULT(hRet, "g_pD3DDevice8->SetPixelShader");
		}

        //*/

        g_bFakePixelShaderLoaded = TRUE;
    }
    // Fixed Pipeline, or Recompiled Programmable Pipeline
    else if(hostShaderHandle != NULL)
    {
        EmuWarning("Trying fixed or recompiled programmable pipeline pixel shader!");
        g_bFakePixelShaderLoaded = FALSE;
		g_dwCurrentPixelShader = hostShaderHandle;
        hRet = g_pD3DDevice8->SetPixelShader(g_iWireframe == 0 ? hostShaderHandle : 0);
		DEBUG_D3DRESULT(hRet, "g_pD3DDevice8->SetPixelShader(fixed)");
	}

    if(FAILED(hRet))
    {
        EmuWarning("We're lying about setting a pixel shader!");

        hRet = D3D_OK;
    }
}

BOOL g_bBadIndexData = FALSE;

// ******************************************************************
// * patch: D3DDevice_SetTexture
// ******************************************************************
VOID WINAPI EMUPATCH(D3DDevice_SetTexture)
(
    DWORD           Stage,
	X_D3DBaseTexture  *pTexture
)
{
	FUNC_EXPORTS

	LOG_FUNC_BEGIN
		LOG_FUNC_ARG(Stage)
		LOG_FUNC_ARG(pTexture)
		LOG_FUNC_END;

	// Call the Xbox implementation of this function, to properly handle reference counting for us
	XB_trampoline(VOID, WINAPI, D3DDevice_SetTexture, (DWORD, X_D3DBaseTexture*));

	XB_D3DDevice_SetTexture(Stage, pTexture);

	EmuD3DActiveTexture[Stage] = pTexture;
}

// ******************************************************************
// * patch: D3DDevice_SwitchTexture
// ******************************************************************
VOID __fastcall EMUPATCH(D3DDevice_SwitchTexture)
(
    DWORD           Method,
    DWORD           Data,
    DWORD           Format
)
{
	FUNC_EXPORTS

	LOG_FUNC_BEGIN
		LOG_FUNC_ARG(Method)
		LOG_FUNC_ARG(Data)
		LOG_FUNC_ARG(Format)
		LOG_FUNC_END;

    DWORD StageLookup[TEXTURE_STAGES] = { 0x00081b00, 0x00081b40, 0x00081b80, 0x00081bc0 };
    DWORD Stage = -1;

    for(int v=0;v<TEXTURE_STAGES;v++)
    {
        if(StageLookup[v] == Method)
        {
            Stage = v;
        }
    }

    if(Stage == -1)
    {
        EmuWarning("Unknown Method (0x%.08X)", Method);
    }
    else
    {
		// Switch Texture updates the data pointer of an active texture using pushbuffer commands
		// The closest thing we can do with HLE is update the data pointer ourselves and hope for the best
		X_D3DBaseTexture* pActiveTexture = EmuD3DActiveTexture[Stage];
		if (pActiveTexture != nullptr) {
			pActiveTexture->Data = Data;
		}
    }
}

// ******************************************************************
// * patch: D3DDevice_Begin
// ******************************************************************
VOID WINAPI EMUPATCH(D3DDevice_Begin)
(
    X_D3DPRIMITIVETYPE     PrimitiveType
)
{
	FUNC_EXPORTS

	LOG_FUNC_ONE_ARG(PrimitiveType);

    g_IVBPrimitiveType = PrimitiveType;

    g_IVBTblOffs = 0;
    g_IVBFVF = 0;

    // default values
    ZeroMemory(g_IVBTable, sizeof(_D3DIVB)*IVB_TABLE_SIZE);

    if(g_pIVBVertexBuffer == nullptr)
    {
        g_pIVBVertexBuffer = (DWORD*)g_VMManager.Allocate(IVB_BUFFER_SIZE);
    }
}

// ******************************************************************
// * patch: D3DDevice_SetVertexData2f
// ******************************************************************
VOID WINAPI EMUPATCH(D3DDevice_SetVertexData2f)
(
    int     Register,
    FLOAT   a,
    FLOAT   b
)
{
	FUNC_EXPORTS

	LOG_FORWARD("D3DDevice_SetVertexData4f");

    EMUPATCH(D3DDevice_SetVertexData4f)(Register, a, b, 0.0f, 1.0f);
}

static inline DWORD FtoDW(FLOAT f) { return *((DWORD*)&f); }
static inline FLOAT DWtoF(DWORD f) { return *((FLOAT*)&f); }

// ******************************************************************
// * patch: D3DDevice_SetVertexData2s
// ******************************************************************
VOID WINAPI EMUPATCH(D3DDevice_SetVertexData2s)
(
    int     Register,
    SHORT   a,
    SHORT   b
)
{
	FUNC_EXPORTS

	LOG_FORWARD("D3DDevice_SetVertexData4f");

    DWORD dwA = a, dwB = b;

    EMUPATCH(D3DDevice_SetVertexData4f)(Register, DWtoF(dwA), DWtoF(dwB), 0.0f, 1.0f);
}

// ******************************************************************
// * patch: D3DDevice_SetVertexData4f
// ******************************************************************
VOID WINAPI EMUPATCH(D3DDevice_SetVertexData4f)
(
    int     Register,
    FLOAT   a,
    FLOAT   b,
    FLOAT   c,
    FLOAT   d
)
{
	FUNC_EXPORTS

	LOG_FUNC_BEGIN
		LOG_FUNC_ARG(Register)
		LOG_FUNC_ARG(a)
		LOG_FUNC_ARG(b)
		LOG_FUNC_ARG(c)
		LOG_FUNC_ARG(d)
		LOG_FUNC_END;

    HRESULT hRet = D3D_OK;

	int o = g_IVBTblOffs;

	if (o >= IVB_TABLE_SIZE) {
		CxbxKrnlCleanup("Overflow g_IVBTblOffs : %d", o);
	}

    switch(Register)
    {
        // TODO: Blend weight.

        case X_D3DVSDE_POSITION:
        {
            g_IVBTable[o].Position.x = a;
            g_IVBTable[o].Position.y = b;
            g_IVBTable[o].Position.z = c;
            g_IVBTable[o].Rhw = 1.0f;

            g_IVBTblOffs++;

            g_IVBFVF |= D3DFVF_XYZRHW;
	        break;
        }

		case X_D3DVSDE_BLENDWEIGHT:
		{
            g_IVBTable[o].Position.x = a;
            g_IVBTable[o].Position.y = b;
            g_IVBTable[o].Position.z = c;
			g_IVBTable[o].Blend1 = d;

            g_IVBTblOffs++;

            g_IVBFVF |= D3DFVF_XYZB1;
		    break;
        }

		case X_D3DVSDE_NORMAL:
        {
            g_IVBTable[o].Normal.x = a;
            g_IVBTable[o].Normal.y = b;
            g_IVBTable[o].Normal.z = c;

            g_IVBTblOffs++;

            g_IVBFVF |= D3DFVF_NORMAL;
			break;
        }

        case X_D3DVSDE_DIFFUSE:
        {
            DWORD ca = FtoDW(d) << 24;
            DWORD cr = FtoDW(a) << 16;
            DWORD cg = FtoDW(b) << 8;
            DWORD cb = FtoDW(c) << 0;

            g_IVBTable[o].dwDiffuse = ca | cr | cg | cb;

            g_IVBFVF |= D3DFVF_DIFFUSE;
			break;
        }

		case X_D3DVSDE_SPECULAR:
        {
            DWORD ca = FtoDW(d) << 24;
            DWORD cr = FtoDW(a) << 16;
            DWORD cg = FtoDW(b) << 8;
            DWORD cb = FtoDW(c) << 0;

            g_IVBTable[o].dwSpecular = ca | cr | cg | cb;

            g_IVBFVF |= D3DFVF_SPECULAR;
			break;
        }

		case X_D3DVSDE_TEXCOORD0:
        {
            g_IVBTable[o].TexCoord1.x = a;
            g_IVBTable[o].TexCoord1.y = b;

            if( (g_IVBFVF & D3DFVF_TEXCOUNT_MASK) < D3DFVF_TEX1)
            {
                g_IVBFVF |= D3DFVF_TEX1;
            }

			break;
        }

		case X_D3DVSDE_TEXCOORD1:
        {
            g_IVBTable[o].TexCoord2.x = a;
            g_IVBTable[o].TexCoord2.y = b;

            if( (g_IVBFVF & D3DFVF_TEXCOUNT_MASK) < D3DFVF_TEX2)
            {
                g_IVBFVF |= D3DFVF_TEX2;
            }

			break;
        }

		case X_D3DVSDE_TEXCOORD2:
        {
            g_IVBTable[o].TexCoord3.x = a;
            g_IVBTable[o].TexCoord3.y = b;

            if( (g_IVBFVF & D3DFVF_TEXCOUNT_MASK) < D3DFVF_TEX3)
            {
                g_IVBFVF |= D3DFVF_TEX3;
            }

			break;
        }

		case X_D3DVSDE_TEXCOORD3:
        {
            g_IVBTable[o].TexCoord4.x = a;
            g_IVBTable[o].TexCoord4.y = b;

            if( (g_IVBFVF & D3DFVF_TEXCOUNT_MASK) < D3DFVF_TEX4)
            {
                g_IVBFVF |= D3DFVF_TEX4;
            }

	        break;
        }

        case X_D3DVSDE_VERTEX:
        {
            g_IVBTable[o].Position.x = a;
            g_IVBTable[o].Position.y = b;
            g_IVBTable[o].Position.z = c;
            g_IVBTable[o].Rhw = d;

            // Copy current color to next vertex
            g_IVBTable[o+1].dwDiffuse  = g_IVBTable[o].dwDiffuse;
            g_IVBTable[o+1].dwSpecular = g_IVBTable[o].dwSpecular;

            g_IVBTblOffs++;

            g_IVBFVF |= D3DFVF_XYZRHW;

			break;
        }

        default:
            EmuWarning("Unknown IVB Register : %d", Register);
    }   
}

// ******************************************************************
// * patch: D3DDevice_SetVertexData4ub
// ******************************************************************
VOID WINAPI EMUPATCH(D3DDevice_SetVertexData4ub)
(
	INT		Register,
	BYTE	a,
	BYTE	b,
	BYTE	c,
	BYTE	d
)
{
	FUNC_EXPORTS

	LOG_FORWARD("D3DDevice_SetVertexData4f");

	DWORD dwA = a, dwB = b, dwC = c, dwD = d;

    EMUPATCH(D3DDevice_SetVertexData4f)(Register, DWtoF(dwA), DWtoF(dwB), DWtoF(dwC), DWtoF(dwD));
}

// ******************************************************************
// * patch: D3DDevice_SetVertexData4s
// ******************************************************************
VOID WINAPI EMUPATCH(D3DDevice_SetVertexData4s)
(
	INT		Register,
	SHORT	a,
	SHORT	b,
	SHORT	c,
	SHORT	d
)
{
	FUNC_EXPORTS

	LOG_FORWARD("D3DDevice_SetVertexData4f");

	DWORD dwA = a, dwB = b, dwC = c, dwD = d;

    EMUPATCH(D3DDevice_SetVertexData4f)(Register, DWtoF(dwA), DWtoF(dwB), DWtoF(dwC), DWtoF(dwD));
}

// ******************************************************************
// * patch: D3DDevice_SetVertexDataColor
// ******************************************************************
VOID WINAPI EMUPATCH(D3DDevice_SetVertexDataColor)
(
    int         Register,
    D3DCOLOR    Color
)
{
	FUNC_EXPORTS

	LOG_FORWARD("D3DDevice_SetVertexData4f");

    FLOAT a = DWtoF((Color & 0xFF000000) >> 24);
    FLOAT r = DWtoF((Color & 0x00FF0000) >> 16);
    FLOAT g = DWtoF((Color & 0x0000FF00) >> 8);
    FLOAT b = DWtoF((Color & 0x000000FF) >> 0);

    EMUPATCH(D3DDevice_SetVertexData4f)(Register, r, g, b, a);
}

// ******************************************************************
// * patch: D3DDevice_End
// ******************************************************************
VOID WINAPI EMUPATCH(D3DDevice_End)()
{
	FUNC_EXPORTS

	LOG_FUNC();

    if(g_IVBTblOffs != 0)
        EmuFlushIVB();

    // TODO: Should technically clean this up at some point..but on XP doesnt matter much
//    g_VMManager.Deallocate((VAddr)g_pIVBVertexBuffer);
//    g_VMManager.Deallocate((VAddr)g_IVBTable);
}

// ******************************************************************
// * patch: D3DDevice_RunPushBuffer
// ******************************************************************
VOID WINAPI EMUPATCH(D3DDevice_RunPushBuffer)
(
    X_D3DPushBuffer       *pPushBuffer,
    X_D3DFixup            *pFixup
)
{
	FUNC_EXPORTS

	LOG_FUNC_BEGIN
		LOG_FUNC_ARG(pPushBuffer)
		LOG_FUNC_ARG(pFixup)
		LOG_FUNC_END;

	EmuUnswizzleTextureStages();

	EmuExecutePushBuffer(pPushBuffer, pFixup);    
}

// ******************************************************************
// * patch: D3DDevice_Clear
// ******************************************************************
VOID WINAPI EMUPATCH(D3DDevice_Clear)
(
    DWORD           Count,
    CONST D3DRECT  *pRects,
    DWORD           Flags,
    D3DCOLOR        Color,
    float           Z,
    DWORD           Stencil
)
{
	FUNC_EXPORTS

	LOG_FUNC_BEGIN
		LOG_FUNC_ARG(Count)
		LOG_FUNC_ARG(pRects)
		LOG_FUNC_ARG(Flags)
		LOG_FUNC_ARG(Color)
		LOG_FUNC_ARG(Z)
		LOG_FUNC_ARG(Stencil)
		LOG_FUNC_END;

    // make adjustments to parameters to make sense with windows d3d
    {
        DWORD newFlags = 0;

		if (Flags & X_D3DCLEAR_TARGET) {
			// TODO: D3DCLEAR_TARGET_A, *R, *G, *B don't exist on windows
			if ((Flags & X_D3DCLEAR_TARGET) != X_D3DCLEAR_TARGET)
				EmuWarning("Unsupported : Partial D3DCLEAR_TARGET flag(s) for D3DDevice_Clear : 0x%.08X", Flags & X_D3DCLEAR_TARGET);
		
			newFlags |= D3DCLEAR_TARGET;
		}

        // Do not needlessly clear Z Buffer
		if (Flags & X_D3DCLEAR_ZBUFFER) {
			if (g_bHasDepthStencil)
				newFlags |= D3DCLEAR_ZBUFFER;
			else
				EmuWarning("Unsupported : D3DCLEAR_ZBUFFER flag for D3DDevice_Clear without ZBuffer");
		}

		// Only clear depth buffer and stencil if present
		//
		// Avoids following DirectX Debug Runtime error report
		//    [424] Direct3D8: (ERROR) :Invalid flag D3DCLEAR_ZBUFFER: no zbuffer is associated with device. Clear failed. 
		if (Flags & X_D3DCLEAR_STENCIL) {
			if (g_bHasDepthStencil) // TODO : Introduce/use g_bHasStencil
				newFlags |= D3DCLEAR_STENCIL;
			else
				EmuWarning("Unsupported : D3DCLEAR_STENCIL flag for D3DDevice_Clear without ZBuffer");
		}

        if(Flags & ~(X_D3DCLEAR_TARGET | X_D3DCLEAR_ZBUFFER | X_D3DCLEAR_STENCIL))
            EmuWarning("Unsupported Flag(s) for D3DDevice_Clear : 0x%.08X", Flags & ~(X_D3DCLEAR_TARGET | X_D3DCLEAR_ZBUFFER | X_D3DCLEAR_STENCIL));

        Flags = newFlags;
    }

	DWORD dwFillMode;

	if(g_iWireframe == 0)
        dwFillMode = g_CurrentFillMode;
    else if(g_iWireframe == 1)
        dwFillMode = D3DFILL_WIREFRAME;
    else
        dwFillMode = D3DFILL_POINT;

	HRESULT hRet;

    hRet = g_pD3DDevice8->SetRenderState(D3DRS_FILLMODE, dwFillMode);
	DEBUG_D3DRESULT(hRet, "g_pD3DDevice8->SetRenderState");

    hRet = g_pD3DDevice8->Clear(Count, pRects, Flags, Color, Z, Stencil);
	DEBUG_D3DRESULT(hRet, "g_pD3DDevice8->Clear");
}

#define CXBX_SWAP_PRESENT_FORWARD (256 + 4 + 1) // = CxbxPresentForwardMarker + D3DSWAP_FINISH + D3DSWAP_COPY

// ******************************************************************
// * patch: D3DDevice_Present
// ******************************************************************
VOID WINAPI EMUPATCH(D3DDevice_Present)
(
    CONST RECT* pSourceRect,
    CONST RECT* pDestRect,
    PVOID       pDummy1,
    PVOID       pDummy2
)
{
	FUNC_EXPORTS

	// LOG_FORWARD("D3DDevice_Swap");
	LOG_FUNC_BEGIN
		LOG_FUNC_ARG(pSourceRect)
		LOG_FUNC_ARG(pDestRect)
		LOG_FUNC_ARG(pDummy1)
		LOG_FUNC_ARG(pDummy2)
		LOG_FUNC_END;

	EMUPATCH(D3DDevice_Swap)(CXBX_SWAP_PRESENT_FORWARD); // Xbox present ignores
}

// ******************************************************************
// * patch: D3DDevice_Swap
// ******************************************************************
DWORD WINAPI EMUPATCH(D3DDevice_Swap)
(
    DWORD Flags
)
{
	FUNC_EXPORTS

	LOG_FUNC_ONE_ARG(Flags);

	static clock_t lastDrawFunctionCallTime = 0;

    // TODO: Ensure this flag is always the same across library versions
    if(Flags != 0)
		if (Flags != CXBX_SWAP_PRESENT_FORWARD) // Avoid a warning when forwarded
			EmuWarning("EmuD3DDevice_Swap: Flags != 0");

	CxbxReleaseBackBufferLock();	

	// TODO: Make a video option to wait for VBlank before calling Present.
	// Makes syncing to 30fps easier (which is the native frame rate for Azurik
	// and Halo).
//	g_pDD7->WaitForVerticalBlank( DDWAITVB_BLOCKEND, NULL );
//	g_pDD7->WaitForVerticalBlank( DDWAITVB_BLOCKEND, NULL );

	clock_t currentDrawFunctionCallTime = clock();
	HRESULT hRet = g_pD3DDevice8->Present(0, 0, 0, 0);
	DEBUG_D3DRESULT(hRet, "g_pD3DDevice8->Present");

	g_DeltaTime += currentDrawFunctionCallTime - lastDrawFunctionCallTime;
	lastDrawFunctionCallTime = currentDrawFunctionCallTime;
	g_Frames++;

	if (g_DeltaTime >= CLOCKS_PER_SEC) {
		UpdateCurrentMSpFAndFPS();
		g_Frames = 0;
		g_DeltaTime -= CLOCKS_PER_SEC;
	}
	
	if (Flags == CXBX_SWAP_PRESENT_FORWARD) // Only do this when forwarded from Present
	{
		// Put primitives per frame in the title
		/*{
			char szString[64];

			sprintf( szString, "Cxbx: PPF(%d)", g_dwPrimPerFrame );

			SetWindowText( CxbxKrnl_hEmuParent, szString );

			g_dwPrimPerFrame = 0;
		}*/

		// TODO : Check if this should be done at Swap-not-Present-time too :
		// not really accurate because you definately dont always present on every vblank
		g_VBData.Swap = g_VBData.VBlank;

		if (g_VBData.VBlank == g_VBLastSwap + 1)
			g_VBData.Flags = 1; // D3DVBLANK_SWAPDONE
		else
		{
			g_VBData.Flags = 2; // D3DVBLANK_SWAPMISSED
			g_SwapData.MissedVBlanks++;
		}
	}

	// Handle Swap Callback function
	{
		g_SwapData.Swap++;

		if(g_pSwapCallback != NULL) 
		{
				
			g_pSwapCallback(&g_SwapData);
				
		}
	}

	g_bHackUpdateSoftwareOverlay = FALSE;

	DWORD result;
	if (Flags == CXBX_SWAP_PRESENT_FORWARD) // Only do this when forwarded from Present
		result = D3D_OK; // Present always returns success
	else
		result = g_SwapData.Swap; // Swap returns number of swaps

    return result;
}

// ******************************************************************
// * patch: IDirect3DResource8_Register
// ******************************************************************
VOID WINAPI CreateHostResource
(
    X_D3DResource      *pThis,
    DWORD               dwSize
)
{
	
	//FUNC_EXPORTS

	LOG_FUNC_BEGIN
		LOG_FUNC_ARG(pThis)
		LOG_FUNC_ARG(dwSize)
		LOG_FUNC_END;

    HRESULT hRet = D3D_OK;

	const int TextureStage = 0;

    X_D3DResource *pResource = pThis;

    DWORD dwCommonType = GetXboxCommonResourceType(pResource);
   

    // Determine the resource type, and initialize
    switch(dwCommonType)
    {
		// 
		case X_D3DCOMMON_TYPE_INDEXBUFFER:	return;
        case X_D3DCOMMON_TYPE_VERTEXBUFFER:
        {
            DbgPrintf("EmuIDirect3DResource8_Register : Creating VertexBuffer...\n");

            X_D3DVertexBuffer *pVertexBuffer = (X_D3DVertexBuffer*)pResource;
			IDirect3DVertexBuffer8  *pNewHostVertexBuffer = nullptr;

			// Vertex buffers live in Physical Memory Region
			void* pVirtualAddr = GetDataFromXboxResource(pResource);

            // create vertex buffer
            {
				// If we didn't get a size passed in, use QuerySize
				if (dwSize == 0) {
					g_VMManager.QuerySize((VAddr)pVirtualAddr);
				}

				// If we still didn't get a valid size, make a wild guess
                if(dwSize == 0)
                {
                    // TODO: once this is known to be working, remove the warning
                    EmuWarning("Vertex buffer allocation size unknown");
                    dwSize = PAGE_SIZE;  // temporarily assign a small buffer, which will be increased later
					/*hRet = E_FAIL;
					goto fail;*/
                }

                hRet = g_pD3DDevice8->CreateVertexBuffer
                (
                    dwSize, 0, 0, D3DPOOL_MANAGED,
                    &pNewHostVertexBuffer
                );
				DEBUG_D3DRESULT(hRet, "g_pD3DDevice8->CreateVertexBuffer");

				if(FAILED(hRet))
				{
					char szString[256];
					sprintf( szString, "CreateVertexBuffer Failed!\n\nVB Size = 0x%X\n\nError: \nDesc: ", dwSize/*,
						DXGetErrorString8A(hRet)*//*, DXGetErrorDescription8A(hRet)*/);

					EmuWarning( szString );
				}

				SetHostVertexBuffer(pResource, pNewHostVertexBuffer, dwSize);

                #ifdef _DEBUG_TRACK_VB
                g_VBTrackTotal.insert(pNewHostVertexBuffer);
                #endif

                BYTE *pNativeData = nullptr;

                hRet = pNewHostVertexBuffer->Lock(
					/*OffsetToLock=*/0,
					/*SizeToLock=*/0/*=entire buffer*/,
					&pNativeData,
					/*Flags=*/0);
				DEBUG_D3DRESULT(hRet, "pNewHostVertexBuffer->Lock");

				if(FAILED(hRet))
                    CxbxKrnlCleanup("VertexBuffer Lock Failed!\n\nError: \nDesc: "/*,
						DXGetErrorString8A(hRet)*//*, DXGetErrorDescription8A(hRet)*/);

                memcpy(pNativeData, (void*)pVirtualAddr, dwSize);
                pNewHostVertexBuffer->Unlock();
			}

            DbgPrintf("EmuIDirect3DResource8_Register : Successfully Created VertexBuffer (0x%.08X)\n", pNewHostVertexBuffer);
        }
        break;

        case X_D3DCOMMON_TYPE_PUSHBUFFER:
        {
            DbgPrintf("EmuIDirect3DResource8_Register :-> PushBuffer...\n");

            X_D3DPushBuffer *pPushBuffer = (X_D3DPushBuffer*)pResource;

            // create push buffer
            {
                DWORD dwSize = g_VMManager.QuerySize((VAddr)pThis->Data | MM_SYSTEM_PHYSICAL_MAP);

                if(dwSize == 0)
                {
                    // TODO: once this is known to be working, remove the warning
                    EmuWarning("Push buffer allocation size unknown");

                    pPushBuffer->Lock = X_D3DRESOURCE_LOCK_FLAG_NOSIZE;

                    break;
                }
            }

            DbgPrintf("EmuIDirect3DResource8_Register : Successfully Created PushBuffer (0x%.08X, 0x%.08X, 0x%.08X)\n", pResource->Data, pPushBuffer->Size, pPushBuffer->AllocationSize);
        }
        break;

        case X_D3DCOMMON_TYPE_SURFACE:
        case X_D3DCOMMON_TYPE_TEXTURE:
        {
            if(dwCommonType == X_D3DCOMMON_TYPE_SURFACE) {
                DbgPrintf("EmuIDirect3DResource8_Register :-> Surface...\n");
            } else {
                DbgPrintf("EmuIDirect3DResource8_Register :-> Texture...\n");
            }

			void* pVirtualAddr = GetDataFromXboxResource(pResource);

			X_D3DPixelContainer *pPixelContainer = (X_D3DPixelContainer*)pResource;

            X_D3DFORMAT X_Format = GetXboxPixelContainerFormat(pPixelContainer);
            D3DFORMAT   PCFormat = EmuXB2PC_D3DFormat(X_Format);
            D3DFORMAT   CacheFormat = (D3DFORMAT)0;
            // TODO: check for dimensions

            // TODO: HACK: Temporary?
            if(X_Format == X_D3DFMT_LIN_D24S8)
            {
                /*CxbxKrnlCleanup*/EmuWarning("D3DFMT_LIN_D24S8 not yet supported!");
                X_Format = X_D3DFMT_LIN_A8R8G8B8;
                PCFormat = D3DFMT_A8R8G8B8;
            }

			if(X_Format == X_D3DFMT_LIN_D16)
            {
                /*CxbxKrnlCleanup*/EmuWarning("D3DFMT_LIN_D16 not yet supported!");
                X_Format = X_D3DFMT_LIN_R5G6B5;
                PCFormat = D3DFMT_R5G6B5;
            }

			// TODO: HACK: Since I have trouble with this texture format on modern hardware,
			// Let's try using some 16-bit format instead...
			if(X_Format == X_D3DFMT_X1R5G5B5 )
			{
#ifdef OLD_COLOR_CONVERSION // Current approach
				EmuWarning( "X_D3DFMT_X1R5G5B5 -> D3DFMT_R5GB5" );
				X_Format = X_D3DFMT_R5G6B5;
				PCFormat = D3DFMT_R5G6B5;
#else // Later, convert to ARGB :
				CacheFormat = PCFormat;       // Save this for later
				PCFormat = D3DFMT_A8R8G8B8;   // ARGB
			}

			// Detect formats that must be converted to ARGB
			if (EmuXBFormatRequiresConversionToARGB(X_Format)) {
				CacheFormat = PCFormat;       // Save this for later
				PCFormat = D3DFMT_A8R8G8B8;   // ARGB
#endif // !OLD_COLOR_CONVERSION
			}

            DWORD dwWidth, dwHeight, dwBPP, dwDepth = 1, dwPitch = 0, dwMipMapLevels = 1;
            BOOL  bSwizzled = EmuXBFormatIsSwizzled(X_Format), bCompressed = FALSE, dwCompressedSize = 0;
            BOOL  bCubemap = pPixelContainer->Format & X_D3DFORMAT_CUBEMAP;
			dwBPP = EmuXBFormatBytesPerPixel(X_Format);

            // Interpret Width/Height/BPP
            if(X_Format == X_D3DFMT_X8R8G8B8 || X_Format == X_D3DFMT_A8R8G8B8
			|| X_Format == X_D3DFMT_A8B8G8R8)
            {
                // Swizzled 32 Bit
                dwWidth  = 1 << ((pPixelContainer->Format & X_D3DFORMAT_USIZE_MASK) >> X_D3DFORMAT_USIZE_SHIFT);
                dwHeight = 1 << ((pPixelContainer->Format & X_D3DFORMAT_VSIZE_MASK) >> X_D3DFORMAT_VSIZE_SHIFT);
                dwMipMapLevels = (pPixelContainer->Format & X_D3DFORMAT_MIPMAP_MASK) >> X_D3DFORMAT_MIPMAP_SHIFT;
                dwDepth  = 1;// HACK? 1 << ((pPixelContainer->Format & X_D3DFORMAT_PSIZE_MASK) >> X_D3DFORMAT_PSIZE_SHIFT);
                dwPitch  = dwWidth * dwBPP;
            }
            else if(X_Format == X_D3DFMT_R5G6B5 || X_Format == X_D3DFMT_A4R4G4B4
                 || X_Format == X_D3DFMT_A1R5G5B5 || X_Format == X_D3DFMT_X1R5G5B5
                 || X_Format == X_D3DFMT_G8B8 || X_Format == X_D3DFMT_A8L8)
            {
                // Swizzled 16 Bit
                dwWidth  = 1 << ((pPixelContainer->Format & X_D3DFORMAT_USIZE_MASK) >> X_D3DFORMAT_USIZE_SHIFT);
                dwHeight = 1 << ((pPixelContainer->Format & X_D3DFORMAT_VSIZE_MASK) >> X_D3DFORMAT_VSIZE_SHIFT);
                dwMipMapLevels = (pPixelContainer->Format & X_D3DFORMAT_MIPMAP_MASK) >> X_D3DFORMAT_MIPMAP_SHIFT;
                dwDepth  = 1;// HACK? 1 << ((pPixelContainer->Format & X_D3DFORMAT_PSIZE_MASK) >> X_D3DFORMAT_PSIZE_SHIFT);
                dwPitch  = dwWidth * dwBPP;
            }
            else if(X_Format == X_D3DFMT_L8 || X_Format == X_D3DFMT_P8
                || X_Format == X_D3DFMT_AL8
                || X_Format == X_D3DFMT_A8)
            {
                // Swizzled 8 Bit
                dwWidth  = 1 << ((pPixelContainer->Format & X_D3DFORMAT_USIZE_MASK) >> X_D3DFORMAT_USIZE_SHIFT);
                dwHeight = 1 << ((pPixelContainer->Format & X_D3DFORMAT_VSIZE_MASK) >> X_D3DFORMAT_VSIZE_SHIFT);
                dwMipMapLevels = (pPixelContainer->Format & X_D3DFORMAT_MIPMAP_MASK) >> X_D3DFORMAT_MIPMAP_SHIFT;
                dwDepth  = 1;// HACK? 1 << ((pPixelContainer->Format & X_D3DFORMAT_PSIZE_MASK) >> X_D3DFORMAT_PSIZE_SHIFT);
                dwPitch  = dwWidth * dwBPP;
            }
            else if(X_Format == X_D3DFMT_LIN_X8R8G8B8 || X_Format == X_D3DFMT_LIN_A8R8G8B8
				 || X_Format == X_D3DFMT_LIN_D24S8 || X_Format == X_D3DFMT_LIN_A8B8G8R8)
            {
                // Linear 32 Bit
                dwWidth  = (pPixelContainer->Size & X_D3DSIZE_WIDTH_MASK) + 1;
                dwHeight = ((pPixelContainer->Size & X_D3DSIZE_HEIGHT_MASK) >> X_D3DSIZE_HEIGHT_SHIFT) + 1;
                dwPitch  = (((pPixelContainer->Size & X_D3DSIZE_PITCH_MASK) >> X_D3DSIZE_PITCH_SHIFT)+1)*64;
            }
            else if(X_Format == X_D3DFMT_LIN_R5G6B5 || X_Format == X_D3DFMT_LIN_D16
				 || X_Format == X_D3DFMT_LIN_A4R4G4B4 || X_Format == X_D3DFMT_LIN_A1R5G5B5
				 || X_Format == X_D3DFMT_LIN_X1R5G5B5 )
            {
                // Linear 16 Bit
                dwWidth  = (pPixelContainer->Size & X_D3DSIZE_WIDTH_MASK) + 1;
                dwHeight = ((pPixelContainer->Size & X_D3DSIZE_HEIGHT_MASK) >> X_D3DSIZE_HEIGHT_SHIFT) + 1;
                dwPitch  = (((pPixelContainer->Size & X_D3DSIZE_PITCH_MASK) >> X_D3DSIZE_PITCH_SHIFT)+1)*64;
            }
            else if(X_Format == X_D3DFMT_DXT1 || X_Format == X_D3DFMT_DXT3 || X_Format == X_D3DFMT_DXT5)
            {
                bCompressed = TRUE;

                // Compressed
                dwWidth  = 1 << ((pPixelContainer->Format & X_D3DFORMAT_USIZE_MASK) >> X_D3DFORMAT_USIZE_SHIFT);
                dwHeight = 1 << ((pPixelContainer->Format & X_D3DFORMAT_VSIZE_MASK) >> X_D3DFORMAT_VSIZE_SHIFT);
                dwDepth  = 1 << ((pPixelContainer->Format & X_D3DFORMAT_PSIZE_MASK) >> X_D3DFORMAT_PSIZE_SHIFT);
                dwMipMapLevels = (pPixelContainer->Format & X_D3DFORMAT_MIPMAP_MASK) >> X_D3DFORMAT_MIPMAP_SHIFT;

                // D3DFMT_DXT2...D3DFMT_DXT5 : 128bits per block/per 16 texels
                dwCompressedSize = dwWidth*dwHeight;

                if(X_Format == X_D3DFMT_DXT1) // D3DFMT_DXT1 : 64bits per block/per 16 texels
                    dwCompressedSize /= 2;
            }
            else if(X_Format == X_D3DFMT_YUY2)
            {
                // Linear 32 Bit
                dwWidth  = (pPixelContainer->Size & X_D3DSIZE_WIDTH_MASK) + 1;
                dwHeight = ((pPixelContainer->Size & X_D3DSIZE_HEIGHT_MASK) >> X_D3DSIZE_HEIGHT_SHIFT) + 1;
                dwPitch  = (((pPixelContainer->Size & X_D3DSIZE_PITCH_MASK) >> X_D3DSIZE_PITCH_SHIFT)+1)*64;
            }
            else
            {
				// Unhandled format: Guess reasonable values, show a warning and continue
                EmuWarning("0x%.08X is not a supported format!\n", X_Format);
				dwWidth = (pPixelContainer->Size & X_D3DSIZE_WIDTH_MASK) + 1;
				dwHeight = ((pPixelContainer->Size & X_D3DSIZE_HEIGHT_MASK) >> X_D3DSIZE_HEIGHT_SHIFT) + 1;
				dwPitch = (((pPixelContainer->Size & X_D3DSIZE_PITCH_MASK) >> X_D3DSIZE_PITCH_SHIFT) + 1) * 64;
            }

            if(bSwizzled || bCompressed)
            {
                uint32 w = dwWidth;
                uint32 h = dwHeight;

                for(uint32 v=0;v<dwMipMapLevels;v++)
                {
                    if( ((1u << v) >= w) || ((1u << v) >= h))
                    {
                        dwMipMapLevels = v + 1;
                        break;
                    }
                }
            }

			// One of these will be created :
			IDirect3DSurface8 *pNewHostSurface = nullptr;
			IDirect3DCubeTexture8 *pNewHostCubeTexture = nullptr;
			IDirect3DTexture8 *pNewHostTexture = nullptr;

            // create the happy little texture
            if(dwCommonType == X_D3DCOMMON_TYPE_SURFACE)
            {

				hRet = g_pD3DDevice8->CreateImageSurface(dwWidth, dwHeight, PCFormat, &pNewHostSurface);
				DEBUG_D3DRESULT(hRet, "g_pD3DDevice8->CreateImageSurface");

				// First fail, retry with a fallback format
				// If this succeeds, the texture may not render correctly, but it won't crash
                if(FAILED(hRet)) {
                    EmuWarning("CreateImageSurface Failed\n\nError: %s\nDesc: %s",
							DXGetErrorString8A(hRet), DXGetErrorDescription8A(hRet));

					EmuWarning("Trying Fallback");
					hRet = g_pD3DDevice8->CreateImageSurface(dwWidth, dwHeight, D3DFMT_A8R8G8B8, &pNewHostSurface);
				}

				// If the fallback failed, show an error and exit execution. We cannot safely continue in this state.
				if (FAILED(hRet)) {
					CxbxKrnlCleanup("CreateImageSurface Failed!\n\nError: %s\nDesc: %s",
						DXGetErrorString8A(hRet), DXGetErrorDescription8A(hRet));
				}

				SetHostSurface(pResource, pNewHostSurface);
				DbgPrintf("EmuIDirect3DResource8_Register : Successfully Created ImageSurface (0x%.08X, 0x%.08X)\n", pResource, pNewHostSurface);
                DbgPrintf("EmuIDirect3DResource8_Register : Width : %d, Height : %d, Format : %d\n", dwWidth, dwHeight, PCFormat);
            }
            else
            {
                // TODO: HACK: Figure out why this is necessary!
                // TODO: This is necessary for DXT1 textures at least (4x4 blocks minimum)
                if(dwWidth < 4)
                {
                    EmuWarning("Expanding texture width (%d->4)", dwWidth);
                    dwWidth = 4;

                    dwMipMapLevels = 3;
                }

                if(dwHeight < 4)
                {
                    EmuWarning("Expanding texture height (%d->4)", dwHeight);
                    dwHeight = 4;

                    dwMipMapLevels = 3;
                }

    #ifdef OLD_COLOR_CONVERSION // Current palette approach - Later, use ______P8ToARGBRow_C()                // HACK HACK HACK HACK HACK HACK HACK HACK HACK HACK
                    // Since most modern graphics cards does not support
                    // palette based textures we need to expand it to
                    // ARGB texture format
					if ((PCFormat == D3DFMT_P8 && !g_bSupportsTextureFormat[X_D3DFMT_P8]) || EmuXBFormatRequiresConversionToARGB(X_Format))
                    {
						if (PCFormat == D3DFMT_P8) //Palette
							EmuWarning("D3DFMT_P8 -> D3DFMT_A8R8G8B8");
						else
							EmuWarning("X_Format RequiresConversionToARGB");

                    CacheFormat = PCFormat;       // Save this for later
                    PCFormat = D3DFMT_A8R8G8B8;   // ARGB
                }
#endif // OLD_COLOR_CONVERSION

                if(bCubemap)
                {
                    DbgPrintf("CreateCubeTexture(%d, %d, 0, %d, D3DPOOL_MANAGED)\n", dwWidth,
                        dwMipMapLevels, PCFormat);

                    hRet = g_pD3DDevice8->CreateCubeTexture
                    (
                        dwWidth, dwMipMapLevels, 0, PCFormat,
                        D3DPOOL_MANAGED, &pNewHostCubeTexture
                    );
					DEBUG_D3DRESULT(hRet, "g_pD3DDevice8->CreateCubeTexture");

                    if(FAILED(hRet))
                        CxbxKrnlCleanup("CreateCubeTexture Failed!\n\nError: \nDesc: "/*,
							DXGetErrorString8A(hRet), DXGetErrorDescription8A(hRet)*/);

					SetHostCubeTexture(pResource, pNewHostCubeTexture);
					DbgPrintf("EmuIDirect3DResource8_Register : Successfully Created CubeTexture (0x%.08X, 0x%.08X)\n", pResource, pNewHostCubeTexture);
                }
                else
                {
                //    printf("CreateTexture(%d, %d, %d, 0, %d (X=0x%.08X), D3DPOOL_MANAGED)\n", dwWidth, dwHeight,
                    //       dwMipMapLevels, PCFormat, X_Format);

					// HACK: Quantum Redshift
					/*if( dwMipMapLevels == 8 && X_Format == X_D3DFMT_DXT1 )
					{
						printf( "Dirty Quantum Redshift hack applied!\n" );
						dwMipMapLevels = 1;
					}*/

					hRet = g_pD3DDevice8->CreateTexture
                    (
                        dwWidth, dwHeight, dwMipMapLevels, 0, PCFormat,
                        D3DPOOL_MANAGED, &pNewHostTexture
                    );
					DEBUG_D3DRESULT(hRet, "g_pD3DDevice8->CreateTexture");

					/*if(FAILED(hRet))
					{
						hRet = g_pD3DDevice8->CreateTexture
						(
							dwWidth, dwHeight, dwMipMapLevels, 0, PCFormat,
							D3DPOOL_SYSTEMMEM, &pNewHostTexture
						);
						DEBUG_D3DRESULT(hRet, "g_pD3DDevice8->CreateTexture(D3DPOOL_SYSTEMMEM)");
					}*/


					if(FAILED(hRet))
						EmuWarning("CreateTexture Failed!\n\n"
							"Error: 0x%X\nFormat: %d\nDimensions: %dx%d", hRet, PCFormat, dwWidth, dwHeight);
					else
					{
						DbgPrintf("EmuIDirect3DResource8_Register : Successfully Created Texture (0x%.08X, 0x%.08X)\n", pResource, pNewHostTexture);

						SetHostTexture(pResource, pNewHostTexture);
					}
				}

				uint32 stop = bCubemap ? 6 : 1;

				for(uint32 r=0;r<stop;r++)
                {
                    // as we iterate through mipmap levels, we'll adjust the source resource offset
                    DWORD dwCompressedOffset = 0;

                    DWORD dwMipOffs = 0;
                    DWORD dwMipWidth = dwWidth;
                    DWORD dwMipHeight = dwHeight;
                    DWORD dwMipPitch = dwPitch;

                    // iterate through the number of mipmap levels
                    for(uint level=0;level<dwMipMapLevels;level++)
                    {
                        D3DLOCKED_RECT LockedRect;

                        // copy over data (deswizzle if necessary)
                        if(dwCommonType == X_D3DCOMMON_TYPE_SURFACE)
                            hRet = pNewHostSurface->LockRect(&LockedRect, NULL, 0);
                        else
                        {
                            if(bCubemap)
                            {
                                hRet = pNewHostCubeTexture->LockRect((D3DCUBEMAP_FACES)r, 0, &LockedRect, NULL, 0);
                            }
                            else
                            {
                                hRet = pNewHostTexture->LockRect(level, &LockedRect, NULL, 0);
                            }
                        }

						BYTE *pSrc = (BYTE*)GetDataFromXboxResource(pResource); // TODO : Fix (look at Dxbx) this, as it gives cube textures identical sides

                        if(( pResource->Data == X_D3DRESOURCE_DATA_BACK_BUFFER)
                         ||( (DWORD)pThis->Data == X_D3DRESOURCE_DATA_BACK_BUFFER))
                        {
                            EmuWarning("Attempt to registered to another resource's data (eww!)");

                            // TODO: handle this horrible situation
                            BYTE *pDest = (BYTE*)LockedRect.pBits;
                            for(DWORD v=0;v<dwMipHeight;v++)
                            {
                                memset(pDest, 0, dwMipWidth*dwBPP);

                                pDest += LockedRect.Pitch;
                                pSrc  += dwMipPitch;
                            }
                        }
						else
						{
							if((DWORD)pSrc == 0x80000000)
							{
								// TODO: Fix or handle this situation..?
								// This is probably an unallocated resource, mapped into contiguous memory (0x80000000)
							}
							else if (pSrc == nullptr)
							{
								// TODO: Fix or handle this situation..?
							}
#ifndef OLD_COLOR_CONVERSION // Later, use ConvertD3DTextureToARGBBuffer
							else if (CacheFormat != 0) // Do we need to convert to ARGB?
							{
								EmuWarning("Unsupported texture format, expanding to D3DFMT_A8R8G8B8");

								BYTE *pPixelData = (BYTE*)LockedRect.pBits;
								DWORD dwDataSize = dwMipWidth*dwMipHeight;
								DWORD* pExpandedTexture = (DWORD*)malloc(dwDataSize * sizeof(DWORD));

								uint8 *pSrc = pPixelData;
								uint8 *pDest = (uint8 *)pExpandedTexture;

								DWORD dwSrcPitch = dwMipWidth * dwBPP;//sizeof(DWORD);
								DWORD dwDestPitch = dwMipWidth * sizeof(DWORD);
								DWORD dwMipSizeInBytes = dwDataSize;

								// Convert a row at a time, using a libyuv-like callback approach :
								if (!ConvertD3DTextureToARGBBuffer(
									X_Format,
									pSrc, dwMipWidth, dwMipHeight, dwSrcPitch,
									pDest, dwDestPitch,
									TextureStage)) {
									CxbxKrnlCleanup("Unhandled conversion!");
								}

								//__asm int 3;
								// Copy the expanded texture back to the buffer
								memcpy(pPixelData, pExpandedTexture, dwDataSize * sizeof(DWORD));

								// Flush unused data buffers
								free(pExpandedTexture);
							}
#endif // !OLD_COLOR_CONVERSION
							else
							{
								if (bSwizzled)
								{
									RECT  iRect = { 0,0,0,0 };
									POINT iPoint = { 0,0 };

									// First we need to unswizzle the texture data
									EmuUnswizzleRect
									(
										pSrc + dwMipOffs, dwMipWidth, dwMipHeight, dwDepth, LockedRect.pBits,
										LockedRect.Pitch, iRect, iPoint, dwBPP
									);
								}
								else if (bCompressed)
								{
									// NOTE: compressed size is (dwWidth/2)*(dwHeight/2)/2, so each level divides by 4

									memcpy(LockedRect.pBits, pSrc + dwCompressedOffset, dwCompressedSize >> (level * 2));

									dwCompressedOffset += (dwCompressedSize >> (level * 2));
								}
								else
								{
									/* TODO : // Let DirectX convert the surface (including palette formats) :
									if(!EmuXBFormatRequiresConversionToARGB) {
										D3DXLoadSurfaceFromMemory(
											GetHostSurface(pResource),
											nullptr, // no destination palette
											&destRect,
											pSrc, // Source buffer
											dwMipPitch, // Source pitch
											g_pCurrentPalette,
											&SrcRect,
											D3DX_DEFAULT, // D3DX_FILTER_NONE,
											0 // No ColorKey?
											);
									} else {
									*/
									BYTE *pDest = (BYTE*)LockedRect.pBits;

									if ((DWORD)LockedRect.Pitch == dwMipPitch && dwMipPitch == dwMipWidth*dwBPP)
									{
										memcpy(pDest, pSrc + dwMipOffs, dwMipWidth*dwMipHeight*dwBPP);
									}
									else
									{
										for (DWORD v = 0; v < dwMipHeight; v++)
										{
											memcpy(pDest, pSrc + dwMipOffs, dwMipWidth*dwBPP);

											pDest += LockedRect.Pitch;
											pSrc += dwMipPitch;
										}
									}
								}

#ifdef OLD_COLOR_CONVERSION // Currently, convert here. Later, use ConvertD3DTextureToARGBBuffer above
								if (CacheFormat != 0) // Do we need to convert to ARGB?
								{
									EmuWarning("Unsupported texture format, expanding to D3DFMT_A8R8G8B8");

									BYTE *pPixelData = (BYTE*)LockedRect.pBits;
									DWORD dwDataSize = dwMipWidth*dwMipHeight;
									DWORD* pExpandedTexture = (DWORD*)malloc(dwDataSize * sizeof(DWORD));

									//__asm int 3;
									unsigned int w = 0;
									unsigned int x = 0;
									if (CacheFormat == D3DFMT_P8) // Palette
									{
										DWORD* pTexturePalette = (DWORD*)g_pCurrentPalette[TextureStage]; // For D3DFMT_P8

										for (unsigned int y = 0;y < dwDataSize;y++)
										{
											if (pTexturePalette != nullptr) {
												// Read P8 pixel :
												unsigned char p = (unsigned char)pPixelData[w++];

												// Read the corresponding ARGB from the palette and store it in the new texture :
												// HACK: Prevent crash if a pallete has not been loaded yet
												pExpandedTexture[y] = pTexturePalette[p];
											} else {
												pExpandedTexture[y] = 0;
											}

											// are we at the end of a line?
											if (++x == dwMipWidth)
											{
												x = 0;
												// Since P8 contains byte pixels instead of dword ARGB pixels,
												// the next line resides 3 bytes additional per pixel further :
												w += dwMipWidth * (sizeof(DWORD) - dwBPP);
											}
										}
									}
									else
									{
										const ComponentEncodingInfo *encoding = EmuXBFormatComponentEncodingInfo(X_Format);

										for (unsigned int y = 0; y < dwDataSize; y++)
										{
											uint32 value = 0;

											switch (dwBPP) {
											case 1:
												value = pPixelData[w++];
												break;
											case 2:
												value = ((WORD *)pPixelData)[w++];
												break;
											case 4:
												value = ((DWORD *)pPixelData)[w++];
												break;
											}

											pExpandedTexture[y] = DecodeUInt32ToColor(encoding, value);
											// are we at the end of a line?
											if(++x == dwMipWidth)
											{
												x = 0;
												w += dwMipWidth * (sizeof(DWORD) - dwBPP);
											}
										}
									}

									//__asm int 3;
									// Copy the expanded texture back to the buffer
									memcpy(pPixelData, pExpandedTexture, dwDataSize * sizeof(DWORD));

									// Flush unused data buffers
									free(pExpandedTexture);
								}
#endif // !OLD_COLOR_CONVERSION
							}
						}

                        if(dwCommonType == X_D3DCOMMON_TYPE_SURFACE)
							pNewHostSurface->UnlockRect();
                        else
                        {
                            if(bCubemap)
								pNewHostCubeTexture->UnlockRect((D3DCUBEMAP_FACES)r, 0);
                            else
								pNewHostTexture->UnlockRect(level);
                        }

                        dwMipOffs += dwMipWidth*dwMipHeight*dwBPP;

                        dwMipWidth /= 2;
                        dwMipHeight /= 2;
                        dwMipPitch /= 2;
                    }
                }

                // Debug Texture Dumping
                #ifdef _DEBUG_DUMP_TEXTURE_REGISTER
                if(dwCommonType == X_D3DCOMMON_TYPE_SURFACE)
                {
                    static int dwDumpSurface = 0;

                    char szBuffer[255];

                    sprintf(szBuffer, _DEBUG_DUMP_TEXTURE_REGISTER "%.03d-RegSurface%.03d.dds", X_Format, dwDumpSurface++);

                    D3DXSaveSurfaceToFile(szBuffer, D3DXIFF_DDS, GetHostSurface(pResource), NULL, NULL);
                }
                else
                {
                    if(bCubemap)
                    {
                        static int dwDumpCube = 0;

                        char szBuffer[255];

                        for(int v=0;v<6;v++)
                        {
                            IDirect3DSurface8 *pSurface=0;

                            sprintf(szBuffer, _DEBUG_DUMP_TEXTURE_REGISTER "%.03d-RegCubeTex%.03d-%d.dds", X_Format, dwDumpCube++, v);

							GetHostCubeTexture(pResource)->GetCubeMapSurface((D3DCUBEMAP_FACES)v, 0, &pSurface);

                            D3DXSaveSurfaceToFile(szBuffer, D3DXIFF_DDS, pSurface, NULL, NULL);
                        }
                    }
                    else
                    {
                        static int dwDumpTex = 0;

                        char szBuffer[255];

                        sprintf(szBuffer, _DEBUG_DUMP_TEXTURE_REGISTER "%.03d-RegTexture%.03d.dds", X_Format, dwDumpTex++);

                        D3DXSaveTextureToFile(szBuffer, D3DXIFF_DDS, GetHostTexture(pResource), NULL);
                    }
                }
                #endif
            }
        }
        break;

        case X_D3DCOMMON_TYPE_PALETTE:
        {
            DbgPrintf("EmuIDirect3DResource8_Register :-> Palette...\n");

            X_D3DPalette *pPalette = (X_D3DPalette*)pResource;

            // create palette
            {
				DWORD dwSize = XboxD3DPaletteSizeToBytes(GetXboxPaletteSize(pPalette));

                g_pCurrentPalette[TextureStage] = (PVOID)pThis->Data;
            }

            //DbgPrintf("EmuIDirect3DResource8_Register (0x%X) : Successfully Created Palette (0x%.08X, 0x%.08X, 0x%.08X)\n", pResource->Data, pResource->Size, pResource->AllocationSize);
        }
        break;

        case X_D3DCOMMON_TYPE_FIXUP:
        {
            X_D3DFixup *pFixup = (X_D3DFixup*)pResource;

            EmuWarning("IDirect3DResource8::Register -> X_D3DCOMMON_TYPE_FIXUP is not yet supported\n"
            "0x%.08X (pFixup->Common) \n"
            "0x%.08X (pFixup->Data)   \n"
            "0x%.08X (pFixup->Lock)   \n"
            "0x%.08X (pFixup->Run)    \n"
            "0x%.08X (pFixup->Next)   \n"
            "0x%.08X (pFixup->Size)   \n", pFixup->Common, pFixup->Data, pFixup->Lock, pFixup->Run, pFixup->Next, pFixup->Size);
        }

        default:
            EmuWarning("IDirect3DResource8::Register -> Common Type 0x%.08X not yet supported", dwCommonType);
    }
}


// ******************************************************************
// * patch: IDirect3DResource8_Release
// ******************************************************************
ULONG WINAPI EMUPATCH(D3DResource_Release)
(
	X_D3DResource      *pThis
	)
{
	FUNC_EXPORTS
		LOG_FUNC_ONE_ARG(pThis);

	// Backup the key now, as the Xbox resource may be wiped out by the following release call!
	auto key = GetHostResourceKey(pThis);

	// Call the Xbox version of D3DResource_Release and store the result
	XB_trampoline(ULONG, WINAPI, D3DResource_Release, (X_D3DResource*));

	ULONG uRet = XB_D3DResource_Release(pThis);

	// Was the Xbox resource freed?
	if (uRet == 0) {

		// If this was a cached renter target or depth surface, clear the cache variable too!
		if (pThis == g_pCachedRenderTarget) {
			g_pCachedRenderTarget = nullptr;
		}

		if (pThis == g_pCachedDepthStencil) {
			g_pCachedDepthStencil = nullptr;
		}

		// Also release the host copy (if it exists!)
		FreeHostResource(key); 
	}

    return uRet;
}

// ******************************************************************
// * patch: IDirect3DResource8_IsBusy
// ******************************************************************
BOOL WINAPI EMUPATCH(D3DResource_IsBusy)
(
    X_D3DResource      *pThis
)
{
	FUNC_EXPORTS

    /* too much output
	LOG_FUNC_ONE_ARG(pThis);
    //*/

    return FALSE;
}

// ******************************************************************
// * patch: D3DDevice_EnableOverlay
// ******************************************************************
VOID WINAPI EMUPATCH(D3DDevice_EnableOverlay)
(
    BOOL Enable
)
{
	FUNC_EXPORTS

	LOG_FUNC_ONE_ARG(Enable);
	g_fYuvEnabled = Enable;

	// We can safely ignore this, as long as we properly handle UpdateOverlay
}

// ******************************************************************
// * patch: D3DDevice_UpdateOverlay
// ******************************************************************
VOID WINAPI EMUPATCH(D3DDevice_UpdateOverlay)
(
    X_D3DSurface *pSurface,
    CONST RECT   *SrcRect,
    CONST RECT   *DstRect,
    BOOL          EnableColorKey,
    D3DCOLOR      ColorKey
)
{
	FUNC_EXPORTS

	LOG_FUNC_BEGIN
		LOG_FUNC_ARG(pSurface)
		LOG_FUNC_ARG(SrcRect)
		LOG_FUNC_ARG(DstRect)
		LOG_FUNC_ARG(EnableColorKey)
		LOG_FUNC_ARG(ColorKey)
		LOG_FUNC_END;

	if (!g_fYuvEnabled) {
		return;
	}

	if (pSurface == NULL) {
		EmuWarning("pSurface == NULL!");
	} else {
		uint08 *pYUY2SourceBuffer = (uint08*)GetDataFromXboxResource(pSurface);

		g_dwOverlayW = (pSurface->Size & X_D3DSIZE_WIDTH_MASK) + 1;
		g_dwOverlayH = ((pSurface->Size & X_D3DSIZE_HEIGHT_MASK) >> X_D3DSIZE_HEIGHT_SHIFT) + 1;
		g_dwOverlayP = (((pSurface->Size & X_D3DSIZE_PITCH_MASK) >> X_D3DSIZE_PITCH_SHIFT) + 1) * 64;

		RECT EmuSourRect;
		RECT EmuDestRect;

		if (SrcRect != NULL) {
			EmuSourRect = *SrcRect;
		} else {
			SetRect(&EmuSourRect, 0, 0, g_dwOverlayW, g_dwOverlayH);
		}

		if (DstRect != NULL) {
			// If there's a destination rectangle given, copy that into our local variable :
			EmuDestRect = *DstRect;
		} else {
			GetClientRect(g_hEmuWindow, &EmuDestRect);
		}

		IDirect3DSurface8 *pBackBufferSurface = nullptr;
		HRESULT hRet = g_pD3DDevice8->GetBackBuffer(0, D3DBACKBUFFER_TYPE_MONO, &pBackBufferSurface);
		DEBUG_D3DRESULT(hRet, "g_pD3DDevice8->GetBackBuffer - Unable to get backbuffer surface!");

		// if we obtained the backbuffer, load the YUY2 into the backbuffer
		if (SUCCEEDED(hRet)) {
			// Get backbuffer dimenions; TODO : remember this once, at creation/resize time
			D3DSURFACE_DESC BackBufferDesc;
			pBackBufferSurface->GetDesc(&BackBufferDesc);

			// Limit the width and height of the output to the backbuffer dimensions.
			// This will (hopefully) prevent exceptions in Blinx - The Time Sweeper
			// (see https://github.com/Cxbx-Reloaded/Cxbx-Reloaded/issues/285)
			{
				// Use our (bounded) copy when bounds exceed :
				if (EmuDestRect.right > (LONG)BackBufferDesc.Width) {
					EmuDestRect.right = (LONG)BackBufferDesc.Width;
					DstRect = &EmuDestRect;
				}

				if (EmuDestRect.bottom > (LONG)BackBufferDesc.Height) {
					EmuDestRect.bottom = (LONG)BackBufferDesc.Height;
					DstRect = &EmuDestRect;
				}
			}

			// Use D3DXLoadSurfaceFromMemory() to do conversion, stretching and filtering
			// avoiding the need for YUY2toARGB() (might become relevant when porting to D3D9 or OpenGL)
			// see https://msdn.microsoft.com/en-us/library/windows/desktop/bb172902(v=vs.85).aspx
			hRet = D3DXLoadSurfaceFromMemory(
				/* pDestSurface = */ pBackBufferSurface,
				/* pDestPalette = */ nullptr, // Palette not needed for YUY2
				/* pDestRect = */DstRect, // Either the unmodified original (can be NULL) or a pointer to our local variable
				/* pSrcMemory = */ pYUY2SourceBuffer, // Source buffer
				/* SrcFormat = */ D3DFMT_YUY2,
				/* SrcPitch = */ g_dwOverlayP,
				/* pSrcPalette = */ nullptr, // Palette not needed for YUY2
				/* SrcRect = */ &EmuSourRect,
				/* Filter = */ D3DX_FILTER_POINT, // Dxbx note : D3DX_FILTER_LINEAR gives a smoother image, but 'bleeds' across borders
				/* ColorKey = */ EnableColorKey ? ColorKey : 0);
			DEBUG_D3DRESULT(hRet, "D3DXLoadSurfaceFromMemory - UpdateOverlay could not convert buffer!\n");

			pBackBufferSurface->Release();
		}

		// Update overlay if present was not called since the last call to
		// EmuD3DDevice_UpdateOverlay.
		if(g_bHackUpdateSoftwareOverlay)
			g_pD3DDevice8->Present(0, 0, 0, 0);

		g_bHackUpdateSoftwareOverlay = TRUE;
	}   
}

// ******************************************************************
// * patch: D3DDevice_GetOverlayUpdateStatus
// ******************************************************************
BOOL WINAPI EMUPATCH(D3DDevice_GetOverlayUpdateStatus)()
{
	FUNC_EXPORTS

	LOG_FUNC();    

	LOG_UNIMPLEMENTED();

    // TODO: Actually check for update status
    return TRUE;
}

// ******************************************************************
// * patch: D3DDevice_BlockUntilVerticalBlank
// ******************************************************************
VOID WINAPI EMUPATCH(D3DDevice_BlockUntilVerticalBlank)()
{
	FUNC_EXPORTS

	LOG_FUNC();

	std::unique_lock<std::mutex> lk(g_VBConditionMutex);
	g_VBConditionVariable.wait(lk);
}

// ******************************************************************
// * patch: D3DDevice_SetVerticalBlankCallback
// ******************************************************************
VOID WINAPI EMUPATCH(D3DDevice_SetVerticalBlankCallback)
(
    D3DVBLANKCALLBACK pCallback
)
{
	FUNC_EXPORTS

	LOG_FUNC_ONE_ARG(pCallback);

    g_pVBCallback = pCallback;    
}

// ******************************************************************
// * patch: D3DDevice_SetTextureState_TexCoordIndex
// ******************************************************************
VOID WINAPI EMUPATCH(D3DDevice_SetTextureState_TexCoordIndex)
(
    DWORD Stage,
    DWORD Value
)
{
	FUNC_EXPORTS

	LOG_FUNC_BEGIN
		LOG_FUNC_ARG(Stage)
		LOG_FUNC_ARG(Value)
		LOG_FUNC_END;

	// TODO: Xbox Direct3D supports sphere mapping OpenGL style.

	// BUG FIX: The lower 16 bits were causing false Unknown TexCoordIndex errors.
	// Check for 0x00040000 instead.

    if(Value >= 0x00040000)
        CxbxKrnlCleanup("EmuD3DDevice_SetTextureState_TexCoordIndex: Unknown TexCoordIndex Value (0x%.08X)", Value);

    HRESULT hRet = g_pD3DDevice8->SetTextureStageState(Stage, D3DTSS_TEXCOORDINDEX, Value);
	DEBUG_D3DRESULT(hRet, "g_pD3DDevice8->SetTextureStageState");
}

// ******************************************************************
// * patch: D3DDevice_SetRenderState_TwoSidedLighting
// ******************************************************************
VOID WINAPI EMUPATCH(D3DDevice_SetRenderState_TwoSidedLighting)
(
    DWORD Value
)
{
	FUNC_EXPORTS

	LOG_FUNC_ONE_ARG(Value);

	LOG_NOT_SUPPORTED();
}

// ******************************************************************
// * patch: D3DDevice_SetRenderState_BackFillMode
// ******************************************************************
VOID WINAPI EMUPATCH(D3DDevice_SetRenderState_BackFillMode)
(
    DWORD Value
)
{
	FUNC_EXPORTS

	LOG_FUNC_ONE_ARG(Value);

	// blueshogun96 12/4/07
	// I haven't had access to Cxbx sources in a few months, great to be back :)
	//
	// Anyway, since standard Direct3D doesn't support the back fill mode
	// operation, this function will be ignored.  Things like this make me
	// think even more that an OpenGL port wouldn't hurt since OpenGL supports
	// nearly all of the missing features that Direct3D lacks.  The Xbox's version
	// of Direct3D was specifically created to take advantage of certain NVIDIA
	// GPU registers and provide more OpenGL-like features IHMO.
	LOG_NOT_SUPPORTED();
}

// ******************************************************************
// * patch: D3DDevice_SetTextureState_BorderColor
// ******************************************************************
VOID WINAPI EMUPATCH(D3DDevice_SetTextureState_BorderColor)
(
    DWORD Stage,
    DWORD Value
)
{
	FUNC_EXPORTS

	LOG_FUNC_BEGIN
		LOG_FUNC_ARG(Stage)
		LOG_FUNC_ARG(Value)
		LOG_FUNC_END;

    HRESULT hRet = g_pD3DDevice8->SetTextureStageState(Stage, D3DTSS_BORDERCOLOR, Value);
	DEBUG_D3DRESULT(hRet, "g_pD3DDevice8->SetTextureStageState");
}

// ******************************************************************
// * patch: D3DDevice_SetTextureState_ColorKeyColor
// ******************************************************************
VOID WINAPI EMUPATCH(D3DDevice_SetTextureState_ColorKeyColor)
(
    DWORD Stage,
    DWORD Value
)
{
	FUNC_EXPORTS

	LOG_FUNC_BEGIN
		LOG_FUNC_ARG(Stage)
		LOG_FUNC_ARG(Value)
		LOG_FUNC_END;

	LOG_NOT_SUPPORTED();
}

// ******************************************************************
// * patch: D3DDevice_SetTextureState_BumpEnv
// ******************************************************************
VOID WINAPI EMUPATCH(D3DDevice_SetTextureState_BumpEnv)
(
    DWORD                      Stage,
    X_D3DTEXTURESTAGESTATETYPE Type,
    DWORD                      Value
)
{
	FUNC_EXPORTS

	LOG_FUNC_BEGIN
		LOG_FUNC_ARG(Stage)
		LOG_FUNC_ARG(Type)
		LOG_FUNC_ARG(Value)
		LOG_FUNC_END;

	HRESULT hRet = D3D_OK;

    switch(Type)
    {
        case 22:    // X_D3DTSS_BUMPENVMAT00
            hRet = g_pD3DDevice8->SetTextureStageState(Stage, D3DTSS_BUMPENVMAT00, Value);
            break;
        case 23:    // X_D3DTSS_BUMPENVMAT01
			hRet = g_pD3DDevice8->SetTextureStageState(Stage, D3DTSS_BUMPENVMAT01, Value);
			break;
        case 24:    // X_D3DTSS_BUMPENVMAT11
			hRet = g_pD3DDevice8->SetTextureStageState(Stage, D3DTSS_BUMPENVMAT11, Value);
            break;
        case 25:    // X_D3DTSS_BUMPENVMAT10
			hRet = g_pD3DDevice8->SetTextureStageState(Stage, D3DTSS_BUMPENVMAT10, Value);
            break;
        case 26:    // X_D3DTSS_BUMPENVLSCALE
			hRet = g_pD3DDevice8->SetTextureStageState(Stage, D3DTSS_BUMPENVLSCALE, Value);
            break;
    }

	DEBUG_D3DRESULT(hRet, "g_pD3DDevice8->SetTextureStageState");
}

// ******************************************************************
// * patch: D3DDevice_SetRenderState_FrontFace
// ******************************************************************
VOID WINAPI EMUPATCH(D3DDevice_SetRenderState_FrontFace)
(
    DWORD Value
)
{
	FUNC_EXPORTS

	LOG_FUNC_ONE_ARG(Value);

	LOG_NOT_SUPPORTED();
}

// ******************************************************************
// * patch: D3DDevice_SetRenderState_LogicOp
// ******************************************************************
VOID WINAPI EMUPATCH(D3DDevice_SetRenderState_LogicOp)
(
    DWORD Value
)
{
	FUNC_EXPORTS

	LOG_FUNC_ONE_ARG(Value);

	LOG_NOT_SUPPORTED();
}

// ******************************************************************
// * patch: D3DDevice_SetRenderState_NormalizeNormals
// ******************************************************************
VOID WINAPI EMUPATCH(D3DDevice_SetRenderState_NormalizeNormals)
(
    DWORD Value
)
{
	FUNC_EXPORTS

	LOG_FUNC_ONE_ARG(Value);

    HRESULT hRet = g_pD3DDevice8->SetRenderState(D3DRS_NORMALIZENORMALS, Value);
	DEBUG_D3DRESULT(hRet, "g_pD3DDevice8->SetRenderState");
}

// ******************************************************************
// * patch: D3DDevice_SetRenderState_TextureFactor
// ******************************************************************
VOID WINAPI EMUPATCH(D3DDevice_SetRenderState_TextureFactor)
(
    DWORD Value
)
{
	FUNC_EXPORTS

	LOG_FUNC_ONE_ARG(Value);

	HRESULT hRet = g_pD3DDevice8->SetRenderState(D3DRS_TEXTUREFACTOR, Value);
	DEBUG_D3DRESULT(hRet, "g_pD3DDevice8->SetRenderState");
}

// ******************************************************************
// * patch: D3DDevice_SetRenderState_ZBias
// ******************************************************************
VOID WINAPI EMUPATCH(D3DDevice_SetRenderState_ZBias)
(
    DWORD Value
)
{
	FUNC_EXPORTS

	LOG_FUNC_ONE_ARG(Value);

	HRESULT hRet = g_pD3DDevice8->SetRenderState(D3DRS_ZBIAS, Value);
	DEBUG_D3DRESULT(hRet, "g_pD3DDevice8->SetRenderState");
}

// ******************************************************************
// * patch: D3DDevice_SetRenderState_EdgeAntiAlias
// ******************************************************************
VOID WINAPI EMUPATCH(D3DDevice_SetRenderState_EdgeAntiAlias)
(
    DWORD Value
)
{
	FUNC_EXPORTS

	LOG_FUNC_ONE_ARG(Value);

//  TODO: Analyze performance and compatibility (undefined behavior on PC with triangles or points)
//  HRESULT hRet = g_pD3DDevice8->SetRenderState(D3DRS_EDGEANTIALIAS, Value);
//	DEBUG_D3DRESULT(hRet, "g_pD3DDevice8->SetRenderState");

    LOG_UNIMPLEMENTED();	
}

// ******************************************************************
// * patch: D3DDevice_SetRenderState_FillMode
// ******************************************************************
VOID WINAPI EMUPATCH(D3DDevice_SetRenderState_FillMode)
(
    DWORD Value
)
{
	FUNC_EXPORTS

	LOG_FUNC_ONE_ARG(Value);

    DWORD dwFillMode;

	g_CurrentFillMode = EmuXB2PC_D3DFILLMODE(Value);

    if(g_iWireframe == 0)
        dwFillMode = g_CurrentFillMode;
    else if(g_iWireframe == 1)
        dwFillMode = D3DFILL_WIREFRAME;
    else
        dwFillMode = D3DFILL_POINT;

	HRESULT hRet = g_pD3DDevice8->SetRenderState(D3DRS_FILLMODE, dwFillMode);
	DEBUG_D3DRESULT(hRet, "g_pD3DDevice8->SetRenderState");
}

// ******************************************************************
// * patch: D3DDevice_SetRenderState_FogColor
// ******************************************************************
VOID WINAPI EMUPATCH(D3DDevice_SetRenderState_FogColor)
(
    DWORD Value
)
{
	FUNC_EXPORTS

	LOG_FUNC_ONE_ARG(Value);

	HRESULT hRet = g_pD3DDevice8->SetRenderState(D3DRS_FOGCOLOR, Value);
	DEBUG_D3DRESULT(hRet, "g_pD3DDevice8->SetRenderState");
}

// ******************************************************************
// * patch: D3DDevice_SetRenderState_Dxt1NoiseEnable
// ******************************************************************
VOID WINAPI EMUPATCH(D3DDevice_SetRenderState_Dxt1NoiseEnable)
(
    DWORD Value
)
{
	FUNC_EXPORTS

	LOG_FUNC_ONE_ARG(Value);

	LOG_UNIMPLEMENTED();
}

// ******************************************************************
// * patch: D3DDevice_SetRenderState_Simple
// ******************************************************************
VOID __fastcall EMUPATCH(D3DDevice_SetRenderState_Simple)
(
    DWORD Method,
    DWORD Value
)
{
	FUNC_EXPORTS

	LOG_FUNC_BEGIN
		LOG_FUNC_ARG(Method)
		LOG_FUNC_ARG(Value)
		LOG_FUNC_END;

    int State = -1;

    // Todo: make this faster and more elegant
    for(int v=0;v<174;v++)
    {
        if(EmuD3DRenderStateSimpleEncoded[v] == Method)
        {
            State = v;
            break;
        }
    }

    if(State == -1)
        EmuWarning("RenderState_Simple(0x%.08X, 0x%.08X) is unsupported!", Method, Value);
    else
    {
        switch(State)
        {
            case D3DRS_COLORWRITEENABLE:
            {
                DWORD OrigValue = Value;

                Value = 0;

                if(OrigValue & (1L<<16))
                    Value |= D3DCOLORWRITEENABLE_RED;
                if(OrigValue & (1L<<8))
                    Value |= D3DCOLORWRITEENABLE_GREEN;
                if(OrigValue & (1L<<0))
                    Value |= D3DCOLORWRITEENABLE_BLUE;
                if(OrigValue & (1L<<24))
                    Value |= D3DCOLORWRITEENABLE_ALPHA;

                DbgPrintf("D3DRS_COLORWRITEENABLE := 0x%.08X\n", Value);
            }
            break;

            case D3DRS_SHADEMODE:
                Value = EmuXB2PC_D3DSHADEMODE(Value);
                DbgPrintf("D3DRS_SHADEMODE := 0x%.08X\n", Value);
                break;

            case D3DRS_BLENDOP:
                Value = EmuXB2PC_D3DBLENDOP(Value);
                DbgPrintf("D3DRS_BLENDOP := 0x%.08X\n", Value);
                break;

            case D3DRS_SRCBLEND:
                Value = EmuXB2PC_D3DBLEND(Value);
                DbgPrintf("D3DRS_SRCBLEND := 0x%.08X\n", Value);
                break;

            case D3DRS_DESTBLEND:
                Value = EmuXB2PC_D3DBLEND(Value);
                DbgPrintf("D3DRS_DESTBLEND := 0x%.08X\n", Value);
                break;

            case D3DRS_ZFUNC:
                Value = EmuXB2PC_D3DCMPFUNC(Value);
                DbgPrintf("D3DRS_ZFUNC := 0x%.08X\n", Value);
                break;

            case D3DRS_ALPHAFUNC:
                Value = EmuXB2PC_D3DCMPFUNC(Value);
                DbgPrintf("D3DRS_ALPHAFUNC := 0x%.08X\n", Value);
                break;

            case D3DRS_ALPHATESTENABLE:
                DbgPrintf("D3DRS_ALPHATESTENABLE := 0x%.08X\n", Value);
                break;

            case D3DRS_ALPHABLENDENABLE:
                DbgPrintf("D3DRS_ALPHABLENDENABLE := 0x%.08X\n", Value);
                break;

            case D3DRS_ALPHAREF:
                DbgPrintf("D3DRS_ALPHAREF := %lf\n", DWtoF(Value));
                break;

            case D3DRS_ZWRITEENABLE:
                DbgPrintf("D3DRS_ZWRITEENABLE := 0x%.08X\n", Value);
                break;

            case D3DRS_DITHERENABLE:
                DbgPrintf("D3DRS_DITHERENABLE := 0x%.08X\n", Value);
                break;

			case D3DRS_STENCILZFAIL:
				Value = EmuXB2PC_D3DSTENCILOP(Value);
				DbgPrintf("D3DRS_STENCILZFAIL := 0x%.08X\n", Value);
				break;

			case D3DRS_STENCILPASS:
				Value = EmuXB2PC_D3DSTENCILOP(Value);
				DbgPrintf("D3DRS_STENCILPASS := 0x%.08X\n", Value);
				break;

			case D3DRS_STENCILFUNC:
				Value = EmuXB2PC_D3DCMPFUNC(Value);
				DbgPrintf("D3DRS_STENCILFUNC := 0x%.08X\n", Value);
				break;

			case D3DRS_STENCILREF:
				DbgPrintf("D3DRS_STENCILREF := 0x%.08X\n", Value);
				break;

			case D3DRS_STENCILMASK:
				DbgPrintf("D3DRS_STENCILMASK := 0x%.08X\n", Value);
				break;

			case D3DRS_STENCILWRITEMASK:
				DbgPrintf("D3DRS_STENCILWRITEMASK := 0x%.08X\n", Value);
				break;

            default:
                CxbxKrnlCleanup("Unsupported RenderState (0x%.08X)", State);
                break;
        };

        // TODO: verify these params as you add support for them!
        HRESULT hRet = g_pD3DDevice8->SetRenderState((D3DRENDERSTATETYPE)State, Value);
		DEBUG_D3DRESULT(hRet, "g_pD3DDevice8->SetRenderState");
	}
}

// ******************************************************************
// * patch: D3DDevice_SetRenderState_VertexBlend
// ******************************************************************
VOID WINAPI EMUPATCH(D3DDevice_SetRenderState_VertexBlend)
(
    DWORD Value
)
{
	FUNC_EXPORTS

	LOG_FUNC_ONE_ARG(Value);

    // convert from Xbox direct3d to PC direct3d enumeration
    if(Value <= 1)
        Value = Value;
    else if(Value == 3)
        Value = 2;
    else if(Value == 5)
        Value = 3;
    else
        CxbxKrnlCleanup("Unsupported D3DVERTEXBLENDFLAGS (%d)", Value);

	HRESULT hRet = g_pD3DDevice8->SetRenderState(D3DRS_VERTEXBLEND, Value);
	DEBUG_D3DRESULT(hRet, "g_pD3DDevice8->SetRenderState");
}

// ******************************************************************
// * patch: D3DDevice_SetRenderState_PSTextureModes
// ******************************************************************
VOID WINAPI EMUPATCH(D3DDevice_SetRenderState_PSTextureModes)
(
    DWORD Value
)
{
	FUNC_EXPORTS

	LOG_FUNC_ONE_ARG(Value);

    // TODO: do something..

	LOG_UNIMPLEMENTED();
}

// ******************************************************************
// * patch: D3DDevice_SetRenderState_CullMode
// ******************************************************************
VOID WINAPI EMUPATCH(D3DDevice_SetRenderState_CullMode)
(
    DWORD Value
)
{
	FUNC_EXPORTS

	LOG_FUNC_ONE_ARG(Value);

    // convert from Xbox D3D to PC D3D enumeration
    // TODO: XDK-Specific Tables? So far they are the same
    switch(Value)
    {
        case X_D3DCULL_NONE:
            Value = D3DCULL_NONE;
            break;
        case X_D3DCULL_CW:
            Value = D3DCULL_CW;
            break;
        case X_D3DCULL_CCW:
            Value = D3DCULL_CCW;
            break;
        default:
            CxbxKrnlCleanup("EmuD3DDevice_SetRenderState_CullMode: Unknown Cullmode (%d)", Value);
    }

	HRESULT hRet = g_pD3DDevice8->SetRenderState(D3DRS_CULLMODE, Value);
	DEBUG_D3DRESULT(hRet, "g_pD3DDevice8->SetRenderState");
}

// ******************************************************************
// * patch: D3DDevice_SetRenderState_LineWidth
// ******************************************************************
VOID WINAPI EMUPATCH(D3DDevice_SetRenderState_LineWidth)
(
    DWORD Value
)
{
	FUNC_EXPORTS

	LOG_FUNC_ONE_ARG(Value);

    // TODO: Convert to PC format??
//    g_pD3DDevice8->SetRenderState(D3DRS_LINEPATTERN, Value);
	LOG_NOT_SUPPORTED();
}

// ******************************************************************
// * patch: D3DDevice_SetRenderState_StencilFail
// ******************************************************************
VOID WINAPI EMUPATCH(D3DDevice_SetRenderState_StencilFail)
(
    DWORD Value
)
{
	FUNC_EXPORTS

	LOG_FUNC_ONE_ARG(Value);

	HRESULT hRet = g_pD3DDevice8->SetRenderState(D3DRS_STENCILFAIL, Value);
	DEBUG_D3DRESULT(hRet, "g_pD3DDevice8->SetRenderState");
}

// ******************************************************************
// * patch: D3DDevice_SetRenderState_OcclusionCullEnable
// ******************************************************************
VOID WINAPI EMUPATCH(D3DDevice_SetRenderState_OcclusionCullEnable)
(
    DWORD Value
)
{
	FUNC_EXPORTS

	LOG_FUNC_ONE_ARG(Value);

	LOG_NOT_SUPPORTED();
}

// ******************************************************************
// * patch: D3DDevice_SetRenderState_StencilCullEnable
// ******************************************************************
VOID WINAPI EMUPATCH(D3DDevice_SetRenderState_StencilCullEnable)
(
    DWORD Value
)
{
	FUNC_EXPORTS

	LOG_FUNC_ONE_ARG(Value);

	LOG_NOT_SUPPORTED();
}

// ******************************************************************
// * patch: D3DDevice_SetRenderState_RopZCmpAlwaysRead
// ******************************************************************
VOID WINAPI EMUPATCH(D3DDevice_SetRenderState_RopZCmpAlwaysRead)
(
    DWORD Value
)
{
	FUNC_EXPORTS

	LOG_FUNC_ONE_ARG(Value);

	LOG_NOT_SUPPORTED();
}

// ******************************************************************
// * patch: D3DDevice_SetRenderState_RopZRead
// ******************************************************************
VOID WINAPI EMUPATCH(D3DDevice_SetRenderState_RopZRead)
(
    DWORD Value
)
{
	FUNC_EXPORTS

	LOG_FUNC_ONE_ARG(Value);

	LOG_NOT_SUPPORTED();
}

// ******************************************************************
// * patch: D3DDevice_SetRenderState_DoNotCullUncompressed
// ******************************************************************
VOID WINAPI EMUPATCH(D3DDevice_SetRenderState_DoNotCullUncompressed)
(
    DWORD Value
)
{
	FUNC_EXPORTS

	LOG_FUNC_ONE_ARG(Value);

	LOG_NOT_SUPPORTED();
}

// ******************************************************************
// * patch: D3DDevice_SetRenderState_ZEnable
// ******************************************************************
VOID WINAPI EMUPATCH(D3DDevice_SetRenderState_ZEnable)
(
    DWORD Value
)
{
	FUNC_EXPORTS

	LOG_FUNC_ONE_ARG(Value);

	HRESULT hRet = g_pD3DDevice8->SetRenderState(D3DRS_ZENABLE, Value);
	DEBUG_D3DRESULT(hRet, "g_pD3DDevice8->SetRenderState");
}

// ******************************************************************
// * patch: D3DDevice_SetRenderState_StencilEnable
// ******************************************************************
VOID WINAPI EMUPATCH(D3DDevice_SetRenderState_StencilEnable)
(
    DWORD Value
)
{
	FUNC_EXPORTS

	LOG_FUNC_ONE_ARG(Value);

	HRESULT hRet = g_pD3DDevice8->SetRenderState(D3DRS_STENCILENABLE, Value);
	DEBUG_D3DRESULT(hRet, "g_pD3DDevice8->SetRenderState");
}

// ******************************************************************
// * patch: D3DDevice_SetRenderState_MultiSampleAntiAlias
// ******************************************************************
VOID WINAPI EMUPATCH(D3DDevice_SetRenderState_MultiSampleAntiAlias)
(
    DWORD Value
)
{
	FUNC_EXPORTS

	LOG_FUNC_ONE_ARG(Value);

	HRESULT hRet = g_pD3DDevice8->SetRenderState(D3DRS_MULTISAMPLEANTIALIAS, Value);
	DEBUG_D3DRESULT(hRet, "g_pD3DDevice8->SetRenderState");
}

// ******************************************************************
// * patch: D3DDevice_SetRenderState_MultiSampleMask
// ******************************************************************
VOID WINAPI EMUPATCH(D3DDevice_SetRenderState_MultiSampleMask)
(
    DWORD Value
)
{
	FUNC_EXPORTS

	LOG_FUNC_ONE_ARG(Value);

	HRESULT hRet = g_pD3DDevice8->SetRenderState(D3DRS_MULTISAMPLEMASK, Value);
	DEBUG_D3DRESULT(hRet, "g_pD3DDevice8->SetRenderState");
}

// ******************************************************************
// * patch: D3DDevice_SetRenderState_MultiSampleMode
// ******************************************************************
VOID WINAPI EMUPATCH(D3DDevice_SetRenderState_MultiSampleMode)
(
    DWORD Value
)
{
	FUNC_EXPORTS

	LOG_FUNC_ONE_ARG(Value);

	LOG_NOT_SUPPORTED();
}

// ******************************************************************
// * patch: D3DDevice_SetRenderState_MultiSampleRenderTargetMode
// ******************************************************************
VOID WINAPI EMUPATCH(D3DDevice_SetRenderState_MultiSampleRenderTargetMode)
(
    DWORD Value
)
{
	FUNC_EXPORTS

	LOG_FUNC_ONE_ARG(Value);

	LOG_NOT_SUPPORTED();
}

// ******************************************************************
// * patch: D3DDevice_SetRenderState_ShadowFunc
// ******************************************************************
VOID WINAPI EMUPATCH(D3DDevice_SetRenderState_ShadowFunc)
(
    DWORD Value
)
{
	FUNC_EXPORTS

	LOG_FUNC_ONE_ARG(Value);

    // ShadowFunc reflects the following Xbox-only extension
    //
    // typedef enum _D3DRENDERSTATETYPE {
    //   ...
    //   D3DRS_SHADOWFUNC = 156, // D3DCMPFUNC
    //   ...
    // } D3DRENDERSTATETYPE;
    //
    // Value is a member of the D3DCMPFUNC enumeration that 
    // specifies what function to use with a shadow buffer. 
    // The default value is D3DCMP_NEVER. 

    // EmuXB2PC_D3DCMPFUNC(Value);

    // this warning just gets annoying
    // LOG_UNIMPLEMENTED();	
}

// ******************************************************************
// * patch: D3DDevice_SetRenderState_YuvEnable
// ******************************************************************
VOID WINAPI EMUPATCH(D3DDevice_SetRenderState_YuvEnable)
(
    BOOL Enable
)
{
	FUNC_EXPORTS

	LOG_FUNC_ONE_ARG(Enable);

    // HACK: Display YUV surface by using an overlay.
    if(Enable != g_fYuvEnabled)
    {
        g_fYuvEnabled = Enable;

        EmuWarning("EmuD3DDevice_SetRenderState_YuvEnable using overlay!");
        
		EMUPATCH(D3DDevice_EnableOverlay)(g_fYuvEnabled);
    }
}

// ******************************************************************
// * patch: D3DDevice_SetTransform
// ******************************************************************
VOID WINAPI EMUPATCH(D3DDevice_SetTransform)
(
    D3DTRANSFORMSTATETYPE State,
    CONST D3DMATRIX      *pMatrix
)
{
	FUNC_EXPORTS

	LOG_FUNC_BEGIN
		LOG_FUNC_ARG(State)
		LOG_FUNC_ARG(pMatrix)
		LOG_FUNC_END;

    /*
    printf("pMatrix (%d)\n", State);
    printf("{\n");
    printf("    %.08f,%.08f,%.08f,%.08f\n", pMatrix->_11, pMatrix->_12, pMatrix->_13, pMatrix->_14);
    printf("    %.08f,%.08f,%.08f,%.08f\n", pMatrix->_21, pMatrix->_22, pMatrix->_23, pMatrix->_24);
    printf("    %.08f,%.08f,%.08f,%.08f\n", pMatrix->_31, pMatrix->_32, pMatrix->_33, pMatrix->_34);
    printf("    %.08f,%.08f,%.08f,%.08f\n", pMatrix->_41, pMatrix->_42, pMatrix->_43, pMatrix->_44);
    printf("}\n");

    if(State == 6 && (pMatrix->_11 == 1.0f) && (pMatrix->_22 == 1.0f) && (pMatrix->_33 == 1.0f) && (pMatrix->_44 == 1.0f))
    {
        g_bSkipPush = TRUE;
        printf("SkipPush ON\n");
    }
    else
    {
        g_bSkipPush = FALSE;
        printf("SkipPush OFF\n");
    }
    */

    State = EmuXB2PC_D3DTS(State);

    HRESULT hRet = g_pD3DDevice8->SetTransform(State, pMatrix);
	DEBUG_D3DRESULT(hRet, "g_pD3DDevice8->SetTransform");    
}

// ******************************************************************
// * patch: D3DDevice_GetTransform
// ******************************************************************
VOID WINAPI EMUPATCH(D3DDevice_GetTransform)
(
    D3DTRANSFORMSTATETYPE State,
    D3DMATRIX            *pMatrix
)
{
	FUNC_EXPORTS

	LOG_FUNC_BEGIN
		LOG_FUNC_ARG(State)
		LOG_FUNC_ARG(pMatrix)
		LOG_FUNC_END;

    State = EmuXB2PC_D3DTS(State);

    HRESULT hRet = g_pD3DDevice8->GetTransform(State, pMatrix);
	DEBUG_D3DRESULT(hRet, "g_pD3DDevice8->GetTransform");    
}

// ******************************************************************
// * patch: Lock2DSurface
// ******************************************************************
VOID WINAPI EMUPATCH(Lock2DSurface)
(
	X_D3DPixelContainer *pPixelContainer,
	D3DCUBEMAP_FACES     FaceType,
	UINT                 Level,
	D3DLOCKED_RECT      *pLockedRect,
	RECT                *pRect,
	DWORD                Flags
	)
{
	FUNC_EXPORTS

	LOG_FUNC_BEGIN
		LOG_FUNC_ARG(pPixelContainer)
		LOG_FUNC_ARG(FaceType)
		LOG_FUNC_ARG(Level)
		LOG_FUNC_ARG(pLockedRect)
		LOG_FUNC_ARG(pRect)
		LOG_FUNC_ARG(Flags)
		LOG_FUNC_END;


	// Pass through to the Xbox implementation of this function
	XB_trampoline(VOID, WINAPI, Lock2DSurface, (X_D3DPixelContainer*, D3DCUBEMAP_FACES, UINT, D3DLOCKED_RECT*, RECT*, DWORD));

	XB_Lock2DSurface(pPixelContainer, FaceType, Level, pLockedRect, pRect, Flags);

	// Mark the resource as modified
	ForceResourceRehash(pPixelContainer);
}


// ******************************************************************
// * patch: Lock3DSurface
// ******************************************************************
VOID WINAPI EMUPATCH(Lock3DSurface)
(
	X_D3DPixelContainer *pPixelContainer,
	UINT				Level,
	D3DLOCKED_BOX		*pLockedVolume,
	D3DBOX				*pBox,
	DWORD				Flags
	)
{
	FUNC_EXPORTS

	LOG_FUNC_BEGIN
		LOG_FUNC_ARG(pPixelContainer)
		LOG_FUNC_ARG(Level)
		LOG_FUNC_ARG(pLockedVolume)
		LOG_FUNC_ARG(pBox)
		LOG_FUNC_ARG(Flags)
		LOG_FUNC_END;

	// Pass through to the Xbox implementation of this function
	XB_trampoline(VOID, WINAPI, Lock3DSurface, (X_D3DPixelContainer*, UINT, D3DLOCKED_BOX*, D3DBOX*, DWORD));

	XB_Lock3DSurface(pPixelContainer, Level, pLockedVolume, pBox, Flags);

	// Mark the resource as modified
	ForceResourceRehash(pPixelContainer);
}

// ******************************************************************
// * patch: D3DDevice_SetStreamSource
// ******************************************************************
VOID WINAPI EMUPATCH(D3DDevice_SetStreamSource)
(
    UINT                StreamNumber,
    X_D3DVertexBuffer  *pStreamData,
    UINT                Stride
)
{
	FUNC_EXPORTS

	LOG_FUNC_BEGIN
		LOG_FUNC_ARG(StreamNumber)
		LOG_FUNC_ARG(pStreamData)
		LOG_FUNC_ARG(Stride)
		LOG_FUNC_END;

	// Forward to Xbox implementation
	// This should stop us having to patch GetStreamSource!
	XB_trampoline(VOID, WINAPI, D3DDevice_SetStreamSource, (UINT, X_D3DVertexBuffer*, UINT));
	XB_D3DDevice_SetStreamSource(StreamNumber, pStreamData, Stride);

	if (StreamNumber < 16) {
		g_D3DStreams[StreamNumber] = pStreamData;
		g_D3DStreamStrides[StreamNumber] = Stride;
	}
}


// ******************************************************************
// * patch: IDirect3DVertexBuffer8_Lock
// ******************************************************************
VOID WINAPI EMUPATCH(D3DVertexBuffer_Lock)
(
    X_D3DVertexBuffer  *pVertexBuffer,
    UINT                OffsetToLock,
    UINT                SizeToLock,
    BYTE              **ppbData,
    DWORD               Flags
)
{
	FUNC_EXPORTS

	LOG_FUNC_BEGIN
		LOG_FUNC_ARG(pVertexBuffer)
		LOG_FUNC_ARG(OffsetToLock)
		LOG_FUNC_ARG(SizeToLock)
		LOG_FUNC_ARG(ppbData)
		LOG_FUNC_ARG(Flags)
		LOG_FUNC_END;


	// Pass through to the Xbox implementation of this function
	XB_trampoline(VOID, WINAPI, D3DVertexBuffer_Lock, (X_D3DVertexBuffer*, UINT, UINT, BYTE**, DWORD));

	XB_D3DVertexBuffer_Lock(pVertexBuffer, OffsetToLock, SizeToLock, ppbData, Flags);

	// Mark the resource as modified
	ForceResourceRehash(pVertexBuffer);
}

// ******************************************************************
// * patch: IDirect3DVertexBuffer8_Lock2
// ******************************************************************
BYTE* WINAPI EMUPATCH(D3DVertexBuffer_Lock2)
(
    X_D3DVertexBuffer  *pVertexBuffer,
    DWORD               Flags
)
{
	FUNC_EXPORTS

	LOG_FUNC_BEGIN
		LOG_FUNC_ARG(pVertexBuffer)
		LOG_FUNC_ARG(Flags)
		LOG_FUNC_END;

	// Pass through to the Xbox implementation of this function
	XB_trampoline(BYTE*, WINAPI, D3DVertexBuffer_Lock2, (X_D3DVertexBuffer*, DWORD));

	BYTE* pRet = XB_D3DVertexBuffer_Lock2(pVertexBuffer, Flags);

	// Mark the resource as modified
	ForceResourceRehash(pVertexBuffer);

	RETURN(pRet);
}

// ******************************************************************
// * patch: D3DDevice_SetVertexShader
// ******************************************************************
VOID WINAPI EMUPATCH(D3DDevice_SetVertexShader)
(
    DWORD Handle
)
{
	FUNC_EXPORTS

	LOG_FUNC_ONE_ARG(Handle);

    HRESULT hRet = D3D_OK;

    g_CurrentVertexShader = Handle;

    // Store viewport offset and scale in constant registers 58 (c-38) and
    // 59 (c-37) used for screen space transformation.
    if(g_VertexShaderConstantMode != X_D3DSCM_NORESERVEDCONSTANTS)
    {
        // TODO: Proper solution.
        static float vScale[] = { (2.0f / 640), (-2.0f / 480), 0.0f, 0.0f };
        static float vOffset[] = { -1.0f, 1.0f, 0.0f, 1.0f };

        g_pD3DDevice8->SetVertexShaderConstant(58, vScale, 1);
        g_pD3DDevice8->SetVertexShaderConstant(59, vOffset, 1);
    }

    DWORD RealHandle;
    if(VshHandleIsVertexShader(Handle))
    {
        RealHandle = ((VERTEX_SHADER *)(VshHandleGetVertexShader(Handle))->Handle)->Handle;
    }
    else
    {
        RealHandle = Handle;
		RealHandle &= ~D3DFVF_XYZ;
		RealHandle &= ~D3DFVF_XYZRHW;
		RealHandle &= ~D3DFVF_XYZB1;
		RealHandle &= ~D3DFVF_XYZB2;
		RealHandle &= ~D3DFVF_XYZB3;
		RealHandle &= ~D3DFVF_XYZB4;
		RealHandle &= ~D3DFVF_DIFFUSE;
		RealHandle &= ~D3DFVF_NORMAL;
		RealHandle &= ~D3DFVF_SPECULAR;
		RealHandle &= 0x00FF;
		if( RealHandle != 0 )
			EmuWarning("EmuD3DDevice_SetVertexShader Handle = 0x%.08X", RealHandle );
		RealHandle = Handle;
    }

	hRet = g_pD3DDevice8->SetVertexShader(RealHandle);
	DEBUG_D3DRESULT(hRet, "g_pD3DDevice8->SetVertexShader");    
}

void EmuUpdateActiveTextureStages()
{
	LOG_INIT;

	for (int i = 0; i < TEXTURE_STAGES; i++)
	{
		X_D3DBaseTexture *pBaseTexture = EmuD3DActiveTexture[i];
		if (pBaseTexture == nullptr) {
			HRESULT hRet = g_pD3DDevice8->SetTexture(i, NULL);
			DEBUG_D3DRESULT(hRet, "g_pD3DDevice8->SetTexture");
			continue;
		}

		IDirect3DTexture8 *pHostTexture = GetHostTexture(pBaseTexture);

		if (pHostTexture != nullptr) {
			HRESULT hRet = g_pD3DDevice8->SetTexture(i, pHostTexture);
			DEBUG_D3DRESULT(hRet, "g_pD3DDevice8->SetTexture");
		}
	}
}

void CxbxUpdateNativeD3DResources()
{
	EmuUnswizzleTextureStages(); 
	EmuUpdateActiveTextureStages();
	EmuUpdateDeferredStates();
/* TODO : Port these :
	DxbxUpdateActiveVertexShader();
	DxbxUpdateActiveTextures();
	DxbxUpdateActivePixelShader();
	DxbxUpdateDeferredStates(); // BeginPush sample shows us that this must come *after* texture update!
	DxbxUpdateActiveVertexBufferStreams();
	DxbxUpdateActiveRenderTarget();
*/
}

// ******************************************************************
// * patch: D3DDevice_DrawVertices
// ******************************************************************
VOID WINAPI EMUPATCH(D3DDevice_DrawVertices)
(
    X_D3DPRIMITIVETYPE PrimitiveType,
    UINT               StartVertex,
    UINT               VertexCount
)
{
	FUNC_EXPORTS

	LOG_FUNC_BEGIN
		LOG_FUNC_ARG(PrimitiveType)
		LOG_FUNC_ARG(StartVertex)
		LOG_FUNC_ARG(VertexCount)
		LOG_FUNC_END;

	// Dxbx Note : In DrawVertices and DrawIndexedVertices, PrimitiveType may not be D3DPT_POLYGON

	CxbxUpdateNativeD3DResources();

	VertexPatchDesc VPDesc;

    VPDesc.PrimitiveType = PrimitiveType;
    VPDesc.dwVertexCount = VertexCount;
    VPDesc.dwOffset = StartVertex;
    VPDesc.pVertexStreamZeroData = 0;
    VPDesc.uiVertexStreamZeroStride = 0;
    VPDesc.hVertexShader = g_CurrentVertexShader;

    VertexPatcher VertPatch;

    bool bPatched = VertPatch.Apply(&VPDesc, NULL);

    if(IsValidCurrentShader())
    {
        #ifdef _DEBUG_TRACK_VB
        if(!g_bVBSkipStream)
        {
        #endif

        HRESULT hRet = g_pD3DDevice8->DrawPrimitive
        (
            EmuXB2PC_D3DPrimitiveType(VPDesc.PrimitiveType),
            StartVertex,
            VPDesc.dwPrimitiveCount
        );
		DEBUG_D3DRESULT(hRet, "g_pD3DDevice8->DrawPrimitive");

		g_dwPrimPerFrame += VPDesc.dwPrimitiveCount;

        #ifdef _DEBUG_TRACK_VB
        }
        #endif
    }

    VertPatch.Restore();

	// Execute callback procedure
	if( g_CallbackType == X_D3DCALLBACK_WRITE )
	{
		if( g_pCallback )
		{
			
			g_pCallback(g_CallbackParam);

			// TODO: Reset pointer?
		}
	}
}

// ******************************************************************
// * patch: D3DDevice_DrawVerticesUP
// ******************************************************************
VOID WINAPI EMUPATCH(D3DDevice_DrawVerticesUP)
(
    X_D3DPRIMITIVETYPE  PrimitiveType,
    UINT                VertexCount,
    CONST PVOID         pVertexStreamZeroData,
    UINT                VertexStreamZeroStride
)
{
	FUNC_EXPORTS

	LOG_FUNC_BEGIN
		LOG_FUNC_ARG(PrimitiveType)
		LOG_FUNC_ARG(VertexCount)
		LOG_FUNC_ARG(pVertexStreamZeroData)
		LOG_FUNC_ARG(VertexStreamZeroStride)
		LOG_FUNC_END;

	CxbxUpdateNativeD3DResources();

/*#if 0
	// HACK: Phantom Crash...
//	if( (*(DWORD*)&pVertexStreamZeroData) == 0x81FC3080 || (*(DWORD*)&pVertexStreamZeroData) == 0x8191E080 )
	if( ((*(DWORD*)&pVertexStreamZeroData) & 0xFF000FFF) == 0x81000080 )
	{
//		EmuWarning( "Invalid vertex data! (0x%.08X)", pVertexStreamZeroData );
		EmuWarning( "Phantom Crash hack applied!" );
		return;
	}
#else
	return;
#endif*/

    VertexPatchDesc VPDesc;

    VPDesc.PrimitiveType = PrimitiveType;
    VPDesc.dwVertexCount = VertexCount;
    VPDesc.dwOffset = 0;
    VPDesc.pVertexStreamZeroData = pVertexStreamZeroData;
    VPDesc.uiVertexStreamZeroStride = VertexStreamZeroStride;
    VPDesc.hVertexShader = g_CurrentVertexShader;

    VertexPatcher VertPatch;

    bool bPatched = VertPatch.Apply(&VPDesc, NULL);

    if (IsValidCurrentShader())
    {
        #ifdef _DEBUG_TRACK_VB
        if(!g_bVBSkipStream)
        #endif
        {
			HRESULT hRet = g_pD3DDevice8->DrawPrimitiveUP
			(
				EmuXB2PC_D3DPrimitiveType(VPDesc.PrimitiveType),
				VPDesc.dwPrimitiveCount,
				VPDesc.pVertexStreamZeroData,
				VPDesc.uiVertexStreamZeroStride
			);
			DEBUG_D3DRESULT(hRet, "g_pD3DDevice8->DrawPrimitiveUP");

			g_dwPrimPerFrame += VPDesc.dwPrimitiveCount;
        }
    }

    VertPatch.Restore();

	// Execute callback procedure
	if( g_CallbackType == X_D3DCALLBACK_WRITE )
	{
		if( g_pCallback )
		{
			
			g_pCallback(g_CallbackParam);
			

			// TODO: Reset pointer?
		}
	}
}

#define VERTICES_PER_QUAD 4
#define TRIANGLES_PER_QUAD 2

// ******************************************************************
// * patch: D3DDevice_DrawIndexedVertices
// ******************************************************************
VOID WINAPI EMUPATCH(D3DDevice_DrawIndexedVertices)
(
    X_D3DPRIMITIVETYPE  PrimitiveType,
    UINT                VertexCount,
    CONST PWORD         pIndexData
)
{
	FUNC_EXPORTS

	// Note : In gamepad.xbe, the gamepad is drawn by D3DDevice_DrawIndexedVertices

	LOG_FUNC_BEGIN
		LOG_FUNC_ARG(PrimitiveType)
		LOG_FUNC_ARG(VertexCount)
		LOG_FUNC_ARG(pIndexData)
		LOG_FUNC_END;

	// Dxbx Note : In DrawVertices and DrawIndexedVertices, PrimitiveType may not be D3DPT_POLYGON
	CxbxUpdateNativeD3DResources();
	CxbxUpdateActiveIndexBuffer(pIndexData, VertexCount);

    VertexPatchDesc VPDesc;

    VPDesc.PrimitiveType = PrimitiveType;
    VPDesc.dwVertexCount = VertexCount;
    VPDesc.dwOffset = 0;
    VPDesc.pVertexStreamZeroData = 0;
    VPDesc.uiVertexStreamZeroStride = 0;
    VPDesc.hVertexShader = g_CurrentVertexShader;
	VPDesc.pIndexData = pIndexData;

    VertexPatcher VertPatch;
	bool FatalError = false;
    VertPatch.Apply(&VPDesc, &FatalError);

	UINT uiStartIndex = 0;
	UINT uiNumVertices = VertexCount;

    if(IsValidCurrentShader() && !FatalError)
    {
		VertexCount = VPDesc.dwVertexCount; // Dxbx addition : Use the new VertexCount

		if (VPDesc.PrimitiveType == X_D3DPT_QUADLIST)
		{
			// Indexed quadlist can be drawn using unpatched indexes via multiple draws of 2 'strip' triangles :
			// 4 vertices are just enough for two triangles (a fan starts with 3 vertices for 1 triangle,
			// and adds 1 triangle via 1 additional vertex)
			// This is slower (because of call-overhead) but doesn't require any index buffer patching at all!
			// Note : XDK samples reaching this case are : DisplacementMap, Ripple
			while ((int)VertexCount >= VERTICES_PER_QUAD)
			{
				HRESULT hRet = g_pD3DDevice8->DrawIndexedPrimitive
				(
					D3DPT_TRIANGLEFAN, // Draw a triangle-fan instead of a quad
					//{ $IFDEF DXBX_USE_D3D9 } {BaseVertexIndex = }0, { $ENDIF }
					/* MinVertexIndex = */0,
					/* NumVertices = */VERTICES_PER_QUAD, // Use all 4 vertices of 1 quad
					uiStartIndex,
					/* primCount = */TRIANGLES_PER_QUAD // Draw 2 triangles with that
				);
				DEBUG_D3DRESULT(hRet, "g_pD3DDevice8->DrawIndexedPrimitive(X_D3DPT_QUADLIST)");

				uiStartIndex += VERTICES_PER_QUAD;
				VertexCount -= VERTICES_PER_QUAD;
			}
		}
		else
		{
			HRESULT hRet;
			// Other primitives than X_D3DPT_QUADLIST can be drawn normally :
			hRet = g_pD3DDevice8->DrawIndexedPrimitive(
				EmuXB2PC_D3DPrimitiveType(VPDesc.PrimitiveType),
				/* MinVertexIndex = */0,
				/* NumVertices = */uiNumVertices, // TODO : g_EmuD3DActiveStreamSizes[0], // Note : ATI drivers are especially picky about this -
				// NumVertices should be the span of covered vertices in the active vertex buffer (TODO : Is stream 0 correct?)
				uiStartIndex,
				VPDesc.dwPrimitiveCount);
			DEBUG_D3DRESULT(hRet, "g_pD3DDevice8->DrawIndexedPrimitive");

			if (VPDesc.PrimitiveType == X_D3DPT_LINELOOP)
			{
				EmuWarning("Unsupported PrimitiveType! (%d)", (DWORD)PrimitiveType);
				//CxbxKrnlCleanup("XTL_EmuD3DDevice_DrawIndexedVertices : X_D3DPT_LINELOOP not unsupported yet!");
				// TODO : Close line-loops using a final single line, drawn from the end to the start vertex
			}
		}

		g_dwPrimPerFrame += VPDesc.dwPrimitiveCount;
    }

    VertPatch.Restore();

	g_pD3DDevice8->SetIndices(NULL, 0);

	// Execute callback procedure
	if( g_CallbackType == X_D3DCALLBACK_WRITE )
	{
		if( g_pCallback )
		{
			
			g_pCallback(g_CallbackParam);
			

			// TODO: Reset pointer?
		}
	}

//#endif
}

// ******************************************************************
// * patch: D3DDevice_DrawIndexedVerticesUP
// ******************************************************************
VOID WINAPI EMUPATCH(D3DDevice_DrawIndexedVerticesUP)
(
    X_D3DPRIMITIVETYPE  PrimitiveType,
    UINT                VertexCount,
    CONST PVOID         pIndexData,
    CONST PVOID         pVertexStreamZeroData,
    UINT                VertexStreamZeroStride
)
{
	FUNC_EXPORTS

	LOG_FUNC_BEGIN
		LOG_FUNC_ARG(PrimitiveType)
		LOG_FUNC_ARG(VertexCount)
		LOG_FUNC_ARG(pIndexData)
		LOG_FUNC_ARG(pVertexStreamZeroData)
		LOG_FUNC_ARG(VertexStreamZeroStride)
		LOG_FUNC_END;


	CxbxUpdateNativeD3DResources();

    if( (PrimitiveType == X_D3DPT_LINELOOP) || (PrimitiveType == X_D3DPT_QUADLIST) )
        EmuWarning("Unsupported PrimitiveType! (%d)", (DWORD)PrimitiveType);

    VertexPatchDesc VPDesc;

    VPDesc.PrimitiveType = PrimitiveType;
    VPDesc.dwVertexCount = VertexCount;
    VPDesc.dwOffset = 0;
    VPDesc.pVertexStreamZeroData = pVertexStreamZeroData;
    VPDesc.uiVertexStreamZeroStride = VertexStreamZeroStride;
    VPDesc.hVertexShader = g_CurrentVertexShader;
	VPDesc.pIndexData = (PWORD)pIndexData;

    VertexPatcher VertPatch;

    bool bPatched = VertPatch.Apply(&VPDesc, NULL);

    #ifdef _DEBUG_TRACK_VB
    if(!g_bVBSkipStream)
    {
    #endif

    if (IsValidCurrentShader())
    {
        HRESULT hRet = g_pD3DDevice8->DrawIndexedPrimitiveUP
        (
            EmuXB2PC_D3DPrimitiveType(VPDesc.PrimitiveType), 0, VPDesc.dwVertexCount, VPDesc.dwPrimitiveCount, pIndexData, D3DFMT_INDEX16, VPDesc.pVertexStreamZeroData, VPDesc.uiVertexStreamZeroStride
        );
		DEBUG_D3DRESULT(hRet, "g_pD3DDevice8->DrawIndexedPrimitiveUP");

		g_dwPrimPerFrame += VPDesc.dwPrimitiveCount;
    }

    #ifdef _DEBUG_TRACK_VB
    }
    #endif

    VertPatch.Restore();

	// Execute callback procedure
	if( g_CallbackType == X_D3DCALLBACK_WRITE )
	{
		if( g_pCallback )
		{
			
			g_pCallback(g_CallbackParam);
			
			// TODO: Reset pointer?
		}
	}
}

// ******************************************************************
// * patch: D3DDevice_GetLight
// ******************************************************************
VOID WINAPI EMUPATCH(D3DDevice_GetLight)
(
    DWORD            Index,
    D3DLIGHT8       *pLight
)
{
	FUNC_EXPORTS

	LOG_FUNC_BEGIN
		LOG_FUNC_ARG(Index)
		LOG_FUNC_ARG(pLight)
		LOG_FUNC_END;

    HRESULT hRet = g_pD3DDevice8->GetLight(Index, pLight);
	DEBUG_D3DRESULT(hRet, "g_pD3DDevice8->GetLight");    
}

// ******************************************************************
// * patch: D3DDevice_SetLight
// ******************************************************************
HRESULT WINAPI EMUPATCH(D3DDevice_SetLight)
(
    DWORD            Index,
    CONST D3DLIGHT8 *pLight
)
{
	FUNC_EXPORTS

	LOG_FUNC_BEGIN
		LOG_FUNC_ARG(Index)
		LOG_FUNC_ARG(pLight)
		LOG_FUNC_END;

    HRESULT hRet = g_pD3DDevice8->SetLight(Index, pLight);
	DEBUG_D3DRESULT(hRet, "g_pD3DDevice8->SetLight");    

    return hRet;
}

// ******************************************************************
// * patch: D3DDevice_SetMaterial
// ******************************************************************
VOID WINAPI EMUPATCH(D3DDevice_SetMaterial)
(
    CONST D3DMATERIAL8 *pMaterial
)
{
	FUNC_EXPORTS

	LOG_FUNC_ONE_ARG(pMaterial);

    HRESULT hRet = g_pD3DDevice8->SetMaterial(pMaterial);
	DEBUG_D3DRESULT(hRet, "g_pD3DDevice8->SetMaterial");
}

// ******************************************************************
// * patch: D3DDevice_LightEnable
// ******************************************************************
HRESULT WINAPI EMUPATCH(D3DDevice_LightEnable)
(
    DWORD            Index,
    BOOL             bEnable
)
{
	FUNC_EXPORTS

	LOG_FUNC_BEGIN
		LOG_FUNC_ARG(Index)
		LOG_FUNC_ARG(bEnable)
		LOG_FUNC_END;

    HRESULT hRet = g_pD3DDevice8->LightEnable(Index, bEnable);
	DEBUG_D3DRESULT(hRet, "g_pD3DDevice8->LightEnable");    

    return hRet;
}

// ******************************************************************
// * patch: D3DDevice_SetRenderTarget
// ******************************************************************
VOID WINAPI EMUPATCH(D3DDevice_SetRenderTarget)
(
    X_D3DSurface    *pRenderTarget,
    X_D3DSurface    *pNewZStencil
)
{
	FUNC_EXPORTS

	LOG_FUNC_BEGIN
		LOG_FUNC_ARG(pRenderTarget)
		LOG_FUNC_ARG(pNewZStencil)
		LOG_FUNC_END;

    IDirect3DSurface8 *pPCRenderTarget = nullptr;
    IDirect3DSurface8 *pPCNewZStencil  = nullptr;

    if(pRenderTarget != NULL)
    {
		g_pCachedRenderTarget = pRenderTarget;

		if(GetHostSurface(pRenderTarget) != nullptr)
		{
			pPCRenderTarget = GetHostSurface(pRenderTarget);
		}
		else
		{
			pPCRenderTarget = GetHostSurface(g_pCachedRenderTarget);
		}
    }

    if(pNewZStencil != NULL)
    {
		g_pCachedDepthStencil = pNewZStencil;
        if(GetHostSurface(pNewZStencil) != nullptr)
        {
            pPCNewZStencil = GetHostSurface(pNewZStencil);
        }
        else
        {
            pPCNewZStencil = GetHostSurface(g_pCachedDepthStencil);
        }
    }

    // TODO: Follow that stencil!
    HRESULT hRet = g_pD3DDevice8->SetRenderTarget(pPCRenderTarget, pPCNewZStencil);
	DEBUG_D3DRESULT(hRet, "g_pD3DDevice8->SetRenderTarget");
}

// ******************************************************************
// * patch: D3DDevice_SetPalette
// ******************************************************************
VOID WINAPI EMUPATCH(D3DDevice_SetPalette)
(
    DWORD         Stage,
    X_D3DPalette *pPalette
)
{
	FUNC_EXPORTS

	LOG_FUNC_BEGIN
		LOG_FUNC_ARG(Stage)
		LOG_FUNC_ARG(pPalette)
		LOG_FUNC_END;

	//    g_pD3DDevice9->SetPaletteEntries(Stage?, (PALETTEENTRY*)pPalette->Data);
	//    g_pD3DDevice9->SetCurrentTexturePalette(Stage, Stage);

	if (Stage < TEXTURE_STAGES)
		// Cache palette data and size
		g_pCurrentPalette[Stage] = GetDataFromXboxResource(pPalette);
}

// ******************************************************************
// * patch: D3DDevice_SetFlickerFilter
// ******************************************************************
void WINAPI EMUPATCH(D3DDevice_SetFlickerFilter)
(
    DWORD         Filter
)
{
	FUNC_EXPORTS

	LOG_FUNC_ONE_ARG(Filter);

	LOG_IGNORED();
}

// ******************************************************************
// * patch: D3DDevice_SetSoftDisplayFilter
// ******************************************************************
void WINAPI EMUPATCH(D3DDevice_SetSoftDisplayFilter)
(
    BOOL Enable
)
{
	FUNC_EXPORTS

	LOG_FUNC_ONE_ARG(Enable);

	LOG_IGNORED();
}

// ******************************************************************
// * patch: IDirect3DPalette8_Lock
// ******************************************************************
VOID WINAPI EMUPATCH(D3DPalette_Lock)
(
    X_D3DPalette   *pThis,
    D3DCOLOR      **ppColors,
    DWORD           Flags
)
{
	FUNC_EXPORTS

	LOG_FORWARD("D3DPalette_Lock2");

	HRESULT hRet = D3D_OK;

	if( pThis )
		*ppColors = EMUPATCH(D3DPalette_Lock2)(pThis, Flags);
	else
	{
		EmuWarning( "EmuIDirect3DPalette8_Lock: pThis == NULL!" );
		hRet = E_FAIL;
	}
}

// ******************************************************************
// * patch: IDirect3DPalette8_Lock2
// ******************************************************************
D3DCOLOR * WINAPI EMUPATCH(D3DPalette_Lock2)
(
    X_D3DPalette   *pThis,
    DWORD           Flags
)
{
	FUNC_EXPORTS

	LOG_FUNC_BEGIN
		LOG_FUNC_ARG(pThis)
		LOG_FUNC_ARG(Flags)
		LOG_FUNC_END;

    // If X_D3DLOCK_READONLY and X_D3DLOCK_NOOVERWRITE bitflags not set
    if( !(Flags & (X_D3DLOCK_READONLY | X_D3DLOCK_NOOVERWRITE)) )
    {
		EMUPATCH(D3DResource_BlockUntilNotBusy)(pThis);
    }

    D3DCOLOR *pColors = (D3DCOLOR*)pThis->Data;

    

    return pColors;
}

// ******************************************************************
// * patch: D3DDevice_GetVertexShaderSize
// ******************************************************************
VOID WINAPI EMUPATCH(D3DDevice_GetVertexShaderSize)
(
    DWORD Handle,
    UINT* pSize
)
{
	FUNC_EXPORTS

	LOG_FUNC_BEGIN
		LOG_FUNC_ARG(Handle)
		LOG_FUNC_ARG(pSize)
		LOG_FUNC_END;

    if(pSize  && VshHandleIsVertexShader(Handle))
    {
        X_D3DVertexShader *pD3DVertexShader = (X_D3DVertexShader *)(Handle & 0x7FFFFFFF);
        VERTEX_SHADER *pVertexShader = (VERTEX_SHADER *)pD3DVertexShader->Handle;
        *pSize = pVertexShader->Size;
    }
    else if(pSize)
    {
        *pSize = 0;
    }

    
}

// ******************************************************************
// * patch: D3DDevice_DeleteVertexShader
// ******************************************************************
VOID WINAPI EMUPATCH(D3DDevice_DeleteVertexShader)
(
    DWORD Handle
)
{
	FUNC_EXPORTS

	LOG_FUNC_ONE_ARG(Handle);

    DWORD RealHandle = 0;

    if(VshHandleIsVertexShader(Handle))
    {
        X_D3DVertexShader *pD3DVertexShader = (X_D3DVertexShader *)(Handle & 0x7FFFFFFF);
        VERTEX_SHADER *pVertexShader = (VERTEX_SHADER *)pD3DVertexShader->Handle;

        RealHandle = pVertexShader->Handle;
		g_VMManager.Deallocate((VAddr)pVertexShader->pDeclaration);

        if(pVertexShader->pFunction)
        {
			g_VMManager.Deallocate((VAddr)pVertexShader->pFunction);
        }

        FreeVertexDynamicPatch(pVertexShader);

		g_VMManager.Deallocate((VAddr)pVertexShader);
        g_VMManager.Deallocate((VAddr)pD3DVertexShader);
    }

    HRESULT hRet = g_pD3DDevice8->DeleteVertexShader(RealHandle);
	DEBUG_D3DRESULT(hRet, "g_pD3DDevice8->DeleteVertexShader");    
}

// ******************************************************************
// * patch: D3DDevice_SelectVertexShaderDirect
// ******************************************************************
VOID WINAPI EMUPATCH(D3DDevice_SelectVertexShaderDirect)
(
    X_VERTEXATTRIBUTEFORMAT *pVAF,
    DWORD                    Address
)
{
	FUNC_EXPORTS

	LOG_FUNC_BEGIN
		LOG_FUNC_ARG(pVAF)
		LOG_FUNC_ARG(Address)
		LOG_FUNC_END;

    LOG_UNIMPLEMENTED(); 
}

// ******************************************************************
// * patch: D3DDevice_GetShaderConstantMode
// ******************************************************************
VOID WINAPI EMUPATCH(D3DDevice_GetShaderConstantMode)
(
    DWORD *pMode
)
{
	FUNC_EXPORTS

	LOG_FUNC_ONE_ARG(pMode);
        
    if(pMode)
    {
        *pMode = g_VertexShaderConstantMode;
    }
}

// ******************************************************************
// * patch: D3DDevice_GetVertexShader
// ******************************************************************
VOID WINAPI EMUPATCH(D3DDevice_GetVertexShader)
(
    DWORD *pHandle
)
{
	FUNC_EXPORTS

	LOG_FUNC_ONE_ARG(pHandle);

    if(pHandle)
    {
        (*pHandle) = g_CurrentVertexShader;
    }
}

// ******************************************************************
// * patch: D3DDevice_GetVertexShaderConstant
// ******************************************************************
VOID WINAPI EMUPATCH(D3DDevice_GetVertexShaderConstant)
(
    INT   Register,
    void  *pConstantData,
    DWORD ConstantCount
)
{
	FUNC_EXPORTS

	LOG_FUNC_BEGIN
		LOG_FUNC_ARG(Register)
		LOG_FUNC_ARG(pConstantData)
		LOG_FUNC_ARG(ConstantCount)
		LOG_FUNC_END;

    HRESULT hRet = g_pD3DDevice8->GetVertexShaderConstant
    (
        Register + 96,
        pConstantData,
        ConstantCount
    );

	DEBUG_D3DRESULT(hRet, "g_pD3DDevice8->GetVertexShaderConstant");    
}

// ******************************************************************
// * patch: D3DDevice_SetVertexShaderInputDirect
// ******************************************************************
VOID WINAPI EMUPATCH(D3DDevice_SetVertexShaderInputDirect)
(
    X_VERTEXATTRIBUTEFORMAT *pVAF,
    UINT                     StreamCount,
    X_STREAMINPUT           *pStreamInputs
)
{
	FUNC_EXPORTS

	LOG_FUNC_BEGIN
		LOG_FUNC_ARG(pVAF)
		LOG_FUNC_ARG(StreamCount)
		LOG_FUNC_ARG(pStreamInputs)
		LOG_FUNC_END;

    LOG_UNIMPLEMENTED(); 
}

// ******************************************************************
// * patch: D3DDevice_GetVertexShaderInput
// ******************************************************************
HRESULT WINAPI EMUPATCH(D3DDevice_GetVertexShaderInput)
(
    DWORD              *pHandle,
    UINT               *pStreamCount,
    X_STREAMINPUT      *pStreamInputs
)
{
	FUNC_EXPORTS

	LOG_FUNC_BEGIN
		LOG_FUNC_ARG(pHandle)
		LOG_FUNC_ARG(pStreamCount)
		LOG_FUNC_ARG(pStreamInputs)
		LOG_FUNC_END;

    LOG_UNIMPLEMENTED(); 

    return 0;
}

// ******************************************************************
// * patch: D3DDevice_SetVertexShaderInput
// ******************************************************************
VOID WINAPI EMUPATCH(D3DDevice_SetVertexShaderInput)
(
    DWORD              Handle,
    UINT               StreamCount,
    X_STREAMINPUT     *pStreamInputs
)
{
	FUNC_EXPORTS

	LOG_FUNC_BEGIN
		LOG_FUNC_ARG(Handle)
		LOG_FUNC_ARG(StreamCount)
		LOG_FUNC_ARG(pStreamInputs)
		LOG_FUNC_END;

    LOG_UNIMPLEMENTED(); 

	return;
}


// ******************************************************************
// * patch: D3DDevice_RunVertexStateShader
// ******************************************************************
VOID WINAPI EMUPATCH(D3DDevice_RunVertexStateShader)
(
    DWORD Address,
    CONST FLOAT *pData
)
{
	FUNC_EXPORTS

	LOG_FUNC_BEGIN
		LOG_FUNC_ARG(Address)
		LOG_FUNC_ARG(pData)
		LOG_FUNC_END;

    LOG_UNIMPLEMENTED(); 
}

// Maps pFunction defintions to pre-compiled shaders
// to reduce the speed impact of LoadVertexShaderProgram
typedef uint64_t load_shader_program_key_t;
std::map<load_shader_program_key_t, DWORD> g_LoadVertexShaderProgramCache;

// ******************************************************************
// * patch: D3DDevice_LoadVertexShaderProgram
// ******************************************************************
VOID WINAPI EMUPATCH(D3DDevice_LoadVertexShaderProgram)
(
    CONST DWORD *pFunction,
    DWORD        Address
)
{
	FUNC_EXPORTS

	LOG_FUNC_BEGIN
		LOG_FUNC_ARG(pFunction)
		LOG_FUNC_ARG(Address)
		LOG_FUNC_END;

	DWORD hCurrentShader = g_CurrentVertexShader;


	load_shader_program_key_t shaderCacheKey = ((load_shader_program_key_t)hCurrentShader << 32) | (DWORD)pFunction;

	// If the shader key was located in the cache, use the cached shader
	// TODO: When do we clear the cache? In this approach, shaders are
	// never freed...
	auto it = g_LoadVertexShaderProgramCache.find(shaderCacheKey);
	if (it != g_LoadVertexShaderProgramCache.end()) {
		EMUPATCH(D3DDevice_LoadVertexShader)(it->second, Address);
		EMUPATCH(D3DDevice_SelectVertexShader)(it->second, Address);
		return;
	}

	if( VshHandleIsVertexShader(hCurrentShader) )
    {
		DWORD hNewShader = 0;
        X_D3DVertexShader *pD3DVertexShader = (X_D3DVertexShader *)(hCurrentShader & 0x7FFFFFFF);
        VERTEX_SHADER *pVertexShader = (VERTEX_SHADER *)pD3DVertexShader->Handle;

		// Save the contents of the existing vertex shader program
		DWORD* pDeclaration = (DWORD*) malloc( pVertexShader->DeclarationSize );
		memmove( pDeclaration, pVertexShader->pDeclaration, pVertexShader->DeclarationSize );

		// Create a new vertex shader with the new 
		HRESULT hr = EMUPATCH(D3DDevice_CreateVertexShader)( pDeclaration, pFunction, &hNewShader, 0 );
		free(pDeclaration);
		if( FAILED( hr ) )
			CxbxKrnlCleanup( "Error creating new vertex shader!" );

		EMUPATCH(D3DDevice_LoadVertexShader)(hNewShader, Address);
		EMUPATCH(D3DDevice_SelectVertexShader)(hNewShader, Address);

		g_LoadVertexShaderProgramCache[shaderCacheKey] = hNewShader;

		EmuWarning("Vertex Shader Cache Size: %d", g_LoadVertexShaderProgramCache.size());
    }
}

// ******************************************************************
// * patch: D3DDevice_GetVertexShaderType
// ******************************************************************
VOID WINAPI EMUPATCH(D3DDevice_GetVertexShaderType)
(
    DWORD  Handle,
    DWORD *pType
)
{
	FUNC_EXPORTS

	LOG_FUNC_BEGIN
		LOG_FUNC_ARG(Handle)
		LOG_FUNC_ARG(pType)
		LOG_FUNC_END;

    if(pType && VshHandleIsVertexShader(Handle))
    {
        *pType = ((VERTEX_SHADER *)(VshHandleGetVertexShader(Handle))->Handle)->Type;
    }
}

// ******************************************************************
// * patch: D3DDevice_GetVertexShaderDeclaration
// ******************************************************************
HRESULT WINAPI EMUPATCH(D3DDevice_GetVertexShaderDeclaration)
(
    DWORD  Handle,
    PVOID  pData,
    DWORD *pSizeOfData
)
{
	FUNC_EXPORTS

	LOG_FUNC_BEGIN
		LOG_FUNC_ARG(Handle)
		LOG_FUNC_ARG(pData)
		LOG_FUNC_ARG(pSizeOfData)
		LOG_FUNC_END;

    HRESULT hRet = D3DERR_INVALIDCALL;

    if(pSizeOfData && VshHandleIsVertexShader(Handle))
    {
        VERTEX_SHADER *pVertexShader = (VERTEX_SHADER *)(VshHandleGetVertexShader(Handle))->Handle;
        if(*pSizeOfData < pVertexShader->DeclarationSize || !pData)
        {
            *pSizeOfData = pVertexShader->DeclarationSize;

            hRet = !pData ? D3D_OK : D3DERR_MOREDATA;
        }
        else
        {
            memcpy(pData, pVertexShader->pDeclaration, pVertexShader->DeclarationSize);
            hRet = D3D_OK;
        }
    }

    
    return hRet;
}

// ******************************************************************
// * patch: D3DDevice_GetVertexShaderFunction
// ******************************************************************
HRESULT WINAPI EMUPATCH(D3DDevice_GetVertexShaderFunction)
(
    DWORD  Handle,
    PVOID *pData,
    DWORD *pSizeOfData
)
{
	FUNC_EXPORTS

	LOG_FUNC_BEGIN
		LOG_FUNC_ARG(Handle)
		LOG_FUNC_ARG(pData)
		LOG_FUNC_ARG(pSizeOfData)
		LOG_FUNC_END;

    HRESULT hRet = D3DERR_INVALIDCALL;

    if(pSizeOfData && VshHandleIsVertexShader(Handle))
    {
        VERTEX_SHADER *pVertexShader = (VERTEX_SHADER *)(VshHandleGetVertexShader(Handle))->Handle;
        if(*pSizeOfData < pVertexShader->FunctionSize || !pData)
        {
            *pSizeOfData = pVertexShader->FunctionSize;

            hRet = !pData ? D3D_OK : D3DERR_MOREDATA;
        }
        else
        {
            memcpy(pData, pVertexShader->pFunction, pVertexShader->FunctionSize);
            hRet = D3D_OK;
        }
    }

    
    return hRet;
}

// ******************************************************************
// * patch: D3DDevice_SetDepthClipPlanes
// ******************************************************************
HRESULT WINAPI EMUPATCH(D3DDevice_SetDepthClipPlanes)
(
    FLOAT Near,
    FLOAT Far,
    DWORD Flags
)
{
	FUNC_EXPORTS

	LOG_FUNC_BEGIN
		LOG_FUNC_ARG(Near)
		LOG_FUNC_ARG(Far)
		LOG_FUNC_ARG(Flags)
		LOG_FUNC_END;

    HRESULT hRet = D3D_OK;

    switch(Flags) // Member of X_D3DSET_DEPTH_CLIP_PLANES_FLAGS enum
    {
        case X_D3DSDCP_SET_VERTEXPROGRAM_PLANES:
        {
            // Sets the depth-clipping planes used whenever vertex shader programs are active
            // TODO

            // pDevice->fNear = Near
            // pDevice->fFar  = Far
        }
        break;

        case X_D3DSDCP_SET_FIXEDFUNCTION_PLANES:
        {
            // Sets the depth-clipping planes used whenever the fixed-function pipeline is in use. 
            // TODO

            // pDevice->fNear = Near
            // pDevice->fFar  = Far
        }
        break;

        case X_D3DSDCP_USE_DEFAULT_VERTEXPROGRAM_PLANES:
        {
            // Causes Direct3D to disregard the depth-clipping planes set when using X_D3DSDCP_SET_VERTEXPROGRAM_PLANE. 
            // Direct3D will resume using its own internally calculated clip planes when vertex shader programs are active. 
            // TODO
        }
        break;

        case X_D3DSDCP_USE_DEFAULT_FIXEDFUNCTION_PLANES:
        {
            // Causes Direct3D to disregard the depth-clipping planes set when using X_D3DSDCP_SET_FIXEDFUNCTION_PLANES. 
            // Direct3D will resume using its own internally calculated clip planes when the fixed-function pipeline is active.
            // TODO
        }
        break;

        default:
            EmuWarning("Unknown SetDepthClipPlanes Flags provided");;
    }

    // TODO

    

    return hRet;
}

// ******************************************************************
// * patch: D3DDevice_InsertFence
// ******************************************************************
DWORD WINAPI EMUPATCH(D3DDevice_InsertFence)()
{
	FUNC_EXPORTS

	LOG_FUNC();

    // TODO: Actually implement this
    DWORD dwRet = 0x8000BEEF;

	LOG_UNIMPLEMENTED();

    return dwRet;
}

// ******************************************************************
// * patch: D3DDevice_IsFencePending
// ******************************************************************
BOOL WINAPI EMUPATCH(D3DDevice_IsFencePending)
(
    DWORD Fence
)
{
	FUNC_EXPORTS

	LOG_FUNC_ONE_ARG(Fence);

	// TODO: Implement
	LOG_UNIMPLEMENTED();

	return FALSE;
}

// ******************************************************************
// * patch: D3DDevice_BlockOnFence
// ******************************************************************
VOID WINAPI EMUPATCH(D3DDevice_BlockOnFence)
(
    DWORD Fence
)
{
	FUNC_EXPORTS

	LOG_FUNC_ONE_ARG(Fence);

    // TODO: Implement
	LOG_UNIMPLEMENTED();
}

// ******************************************************************
// * patch: IDirect3DResource8_BlockUntilNotBusy
// ******************************************************************
VOID WINAPI EMUPATCH(D3DResource_BlockUntilNotBusy)
(
    X_D3DResource *pThis
)
{
	FUNC_EXPORTS

	LOG_FUNC_ONE_ARG(pThis);

    // TODO: Implement
	LOG_UNIMPLEMENTED();
}

// ******************************************************************
// * patch: D3DDevice_SetScissors
// ******************************************************************
VOID WINAPI EMUPATCH(D3DDevice_SetScissors)
(
    DWORD          Count,
    BOOL           Exclusive,
    CONST D3DRECT  *pRects
)
{
	FUNC_EXPORTS

	LOG_FUNC_BEGIN
		LOG_FUNC_ARG(Count)
		LOG_FUNC_ARG(Exclusive)
		LOG_FUNC_ARG(pRects)
		LOG_FUNC_END;

    // TODO: Implement
	LOG_UNIMPLEMENTED();
}

// ******************************************************************
// * patch: D3DDevice_SetScreenSpaceOffset
// ******************************************************************
VOID WINAPI EMUPATCH(D3DDevice_SetScreenSpaceOffset)
(
    FLOAT x,
    FLOAT y
)
{
	FUNC_EXPORTS

	LOG_FUNC_BEGIN
		LOG_FUNC_ARG(x)
		LOG_FUNC_ARG(y)
		LOG_FUNC_END;

    EmuWarning("EmuD3DDevice_SetScreenSpaceOffset ignored");
}

// ******************************************************************
// * patch: D3DDevice_SetPixelShaderProgram
// ******************************************************************
VOID WINAPI EMUPATCH(D3DDevice_SetPixelShaderProgram)
(
	X_D3DPIXELSHADERDEF* pPSDef
)
{
	FUNC_EXPORTS

	LOG_FUNC_ONE_ARG(pPSDef);

	// This is not strictly correct, but close enough
	// On real hardware, SetPixelShaderProgram is identical to
	// SetPixelShader, but it doesn't require a Handle
	DWORD dwHandle = 0;
	EMUPATCH(D3DDevice_CreatePixelShader)(pPSDef, &dwHandle);
	EMUPATCH(D3DDevice_SetPixelShader)( dwHandle );
}

// ******************************************************************
// * patch: IDirect3DDevice_CreateStateBlock
// ******************************************************************
HRESULT WINAPI EMUPATCH(D3DDevice_CreateStateBlock)
(
	D3DSTATEBLOCKTYPE Type,
	DWORD			  *pToken
)
{
	FUNC_EXPORTS

	LOG_FUNC_BEGIN
		LOG_FUNC_ARG(Type)
		LOG_FUNC_ARG(pToken)
		LOG_FUNC_END;

	// blueshogun96 10/1/07
	// I'm assuming this is the same as the PC version...

	HRESULT hRet = g_pD3DDevice8->CreateStateBlock( Type, pToken );
	DEBUG_D3DRESULT(hRet, "g_pD3DDevice8->CreateStateBlock");

	return hRet;
}

// ******************************************************************
// * patch: D3DDevice_InsertCallback
// ******************************************************************
VOID WINAPI EMUPATCH(D3DDevice_InsertCallback)
(
	X_D3DCALLBACKTYPE	Type,
	X_D3DCALLBACK		pCallback,
	DWORD				Context
)
{
	FUNC_EXPORTS

	LOG_FUNC_BEGIN
		LOG_FUNC_ARG(Type)
		LOG_FUNC_ARG(pCallback)
		LOG_FUNC_ARG(Context)
		LOG_FUNC_END;

	// TODO: Implement
	g_pCallback = (D3DCALLBACK) pCallback;
	g_CallbackType = Type;
	g_CallbackParam = Context;

	LOG_INCOMPLETE();
}

// ******************************************************************
// * patch: D3DDevice_DrawRectPatch
// ******************************************************************
HRESULT WINAPI EMUPATCH(D3DDevice_DrawRectPatch)
(
	UINT					Handle,
	CONST FLOAT				*pNumSegs,
	CONST D3DRECTPATCH_INFO *pRectPatchInfo
)
{
	FUNC_EXPORTS

	LOG_FUNC_BEGIN
		LOG_FUNC_ARG(Handle)
		LOG_FUNC_ARG(pNumSegs)
		LOG_FUNC_ARG(pRectPatchInfo)
		LOG_FUNC_END;

	HRESULT hRet = g_pD3DDevice8->DrawRectPatch( Handle, pNumSegs, pRectPatchInfo );
	DEBUG_D3DRESULT(hRet, "g_pD3DDevice8->DrawRectPatch");

	return hRet;
}

#pragma warning(disable:4244)
// ******************************************************************
// * patch: D3DDevice_GetProjectionViewportMatrix
// ******************************************************************
VOID WINAPI EMUPATCH(D3DDevice_GetProjectionViewportMatrix)
(
	D3DXMATRIX *pProjectionViewport
)
{
	FUNC_EXPORTS

	LOG_FUNC_ONE_ARG(pProjectionViewport);

	// blueshogun96 1/25/10
	// It's been almost 3 years, but I think this is a better 
	// implementation.  Still probably not right, but better
	// then before.

	HRESULT hRet;
	D3DXMATRIX Out, mtxProjection, mtxViewport;
	D3DVIEWPORT8 Viewport;

	// Get current viewport
	hRet = g_pD3DDevice8->GetViewport(&Viewport);
	DEBUG_D3DRESULT(hRet, "g_pD3DDevice8->GetViewport - Unable to get viewport!");

	// Get current projection matrix
	hRet = g_pD3DDevice8->GetTransform(D3DTS_PROJECTION, &mtxProjection);
	DEBUG_D3DRESULT(hRet, "g_pD3DDevice8->GetTransform - Unable to get projection matrix!");

	// Clear the destination matrix
	::ZeroMemory(&Out, sizeof(D3DMATRIX));

	// Create the Viewport matrix manually
	// Direct3D8 doesn't give me everything I need in a viewport structure
	// (one thing I REALLY HATE!) so some constants will have to be used
	// instead.

	float ClipWidth = 2.0f;
	float ClipHeight = 2.0f;
	float ClipX = -1.0f;
	float ClipY = 1.0f;
	float Width = DWtoF(Viewport.Width);
	float Height = DWtoF(Viewport.Height);

	D3DXMatrixIdentity(&mtxViewport);
	mtxViewport._11 = Width / ClipWidth;
	mtxViewport._22 = -(Height / ClipHeight);
	mtxViewport._41 = -(ClipX * mtxViewport._11);
	mtxViewport._42 = -(ClipY * mtxViewport._22);

	// Multiply projection and viewport matrix together
	Out = mtxProjection * mtxViewport;

	*pProjectionViewport = Out;

//	__asm int 3;
}
#pragma warning(default:4244)

// ******************************************************************
// * patch: D3DDevice_GetTexture2
// ******************************************************************
X_D3DBaseTexture* WINAPI EMUPATCH(D3DDevice_GetTexture2)(DWORD Stage)
{
	FUNC_EXPORTS

	LOG_FUNC_ONE_ARG(Stage);
	
	// Get the active texture from this stage
	X_D3DBaseTexture* pRet = EmuD3DActiveTexture[Stage];

	if (pRet) {
		pRet->Common++; // EMUPATCH(D3DResource_AddRef)(pRet);
	}

	return pRet;
}

// ******************************************************************
// * patch: D3DDevice_GetTexture
// ******************************************************************
VOID WINAPI EMUPATCH(D3DDevice_GetTexture)
(
	DWORD           Stage,
	X_D3DBaseTexture  **pTexture
)
{
	FUNC_EXPORTS

	LOG_FORWARD("D3DDevice_GetTexture2");

	*pTexture = EMUPATCH(D3DDevice_GetTexture2)(Stage);
}

// ******************************************************************
// * patch: D3DDevice_SetStateVB (D3D::CDevice::SetStateVB)
// ******************************************************************
VOID WINAPI EMUPATCH(D3DDevice_SetStateVB)( ULONG Unknown1 )
{
	FUNC_EXPORTS

	LOG_FUNC_ONE_ARG(Unknown1);

	// TODO: Anything?
//	__asm int 3;

	LOG_UNIMPLEMENTED();	
}

// ******************************************************************
// * patch: D3DDevice_SetStateUP (D3D::CDevice::SetStateUP)
// ******************************************************************
VOID WINAPI EMUPATCH(D3DDevice_SetStateUP)()
{
	FUNC_EXPORTS

	LOG_FUNC();

	LOG_UNIMPLEMENTED();

	// TODO: Anything?
//	__asm int 3;
	
}

// ******************************************************************
// * patch: D3DDevice_SetStipple
// ******************************************************************
void WINAPI EMUPATCH(D3DDevice_SetStipple)( DWORD* pPattern )
{
	FUNC_EXPORTS

	LOG_FUNC_ONE_ARG(pPattern);

	// We need an OpenGL port... badly

	LOG_IGNORED();
}

// ******************************************************************
// * patch: D3DDevice_SetSwapCallback
// ******************************************************************
void WINAPI EMUPATCH(D3DDevice_SetSwapCallback)
(
	D3DSWAPCALLBACK		pCallback
)
{
	FUNC_EXPORTS

	LOG_FUNC_ONE_ARG(pCallback);

    g_pSwapCallback = pCallback;
}

// ******************************************************************
// * patch: D3DDevice_PersistDisplay
// ******************************************************************
HRESULT WINAPI EMUPATCH(D3DDevice_PersistDisplay)()
{
	FUNC_EXPORTS

	LOG_FUNC();

	HRESULT hRet = D3D_OK;

	LOG_UNIMPLEMENTED();

	// TODO: This function simply saves a copy of the display to a surface and persists it in contiguous memory
	// This function, if ever required, can be implemented as the following
	// 1. Check for an existing persisted surface via AvGetSavedDataAddress, free it if necessary
	// 2. Create an Xbox format surface with the same size and format as active display
	// 3. Copy the host framebuffer to the xbox surface, converting format if necessary
	// 4. Set the display mode via AvSetDisplayMode to the same format as the persisted surface,
	//    passing the ->Data pointer of the xbox surface as the framebuffer pointer.
	// 5. Use MmPersistContigousMemory to persist the surface data across reboot
	// 6. Call AvSetSavedDataAddress, passing the xbox surface data pointer


	return hRet;
}

// ******************************************************************
// * patch: D3DDevice_PrimeVertexCache
// ******************************************************************
VOID WINAPI EMUPATCH(D3DDevice_PrimeVertexCache)
(
	UINT  VertexCount,
	WORD *pIndexData
)
{
	FUNC_EXPORTS

	LOG_FUNC_BEGIN
		LOG_FUNC_ARG(VertexCount)
		LOG_FUNC_ARG(pIndexData)
		LOG_FUNC_END;

	// TODO: Implement
	LOG_UNIMPLEMENTED();
}

// ******************************************************************
// * patch: D3DDevice_SetRenderState_SampleAlpha
// ******************************************************************
HRESULT WINAPI EMUPATCH(D3DDevice_SetRenderState_SampleAlpha)
(
	DWORD dwSampleAlpha
)
{
	FUNC_EXPORTS

	LOG_FUNC_ONE_ARG(dwSampleAlpha);

	// TODO: Implement?

	LOG_UNIMPLEMENTED();

		

	return D3D_OK;
}

// ******************************************************************
// * patch: D3DDevice_SetRenderState_Deferred
// ******************************************************************
VOID __fastcall EMUPATCH(D3DDevice_SetRenderState_Deferred)
(
	DWORD State,
	DWORD Value
)
{
	FUNC_EXPORTS

	LOG_FUNC_BEGIN
		LOG_FUNC_ARG(State)
		LOG_FUNC_ARG(Value)
		LOG_FUNC_END;

	// TODO: HACK: Technically, this function doesn't need to be emulated.
	// The location of EmuD3DDeferredRenderState for 3911 isn't correct and at
	// the time of writing, I don't understand how to fix it.  Until then, 
	// I'm going to implement this in a reckless manner.  When the offset for
	// EmuD3DDeferredRenderState is fixed for 3911, this function should be
	// obsolete!

	if( State > 81 && State < 116 )
		EmuD3DDeferredRenderState[State-82] = Value;
	else
		CxbxKrnlCleanup("Unknown Deferred RenderState! (%d)\n", State);

	/*
	XDK 3911 Deferred RenderState values
	D3DRS_FOGENABLE                 = 82,   // TRUE to enable fog blending 
    D3DRS_FOGTABLEMODE              = 83,   // D3DFOGMODE 
    D3DRS_FOGSTART                  = 84,   // float fog start (for both vertex and pixel fog) 
    D3DRS_FOGEND                    = 85,   // float fog end      
    D3DRS_FOGDENSITY                = 86,   // float fog density  
    D3DRS_RANGEFOGENABLE            = 87,   // TRUE to enable range-based fog 
    D3DRS_WRAP0                     = 88,   // D3DWRAP* flags (D3DWRAP_U, D3DWRAPCOORD_0, etc.) for 1st texture coord.
    D3DRS_WRAP1                     = 89,   // D3DWRAP* flags (D3DWRAP_U, D3DWRAPCOORD_0, etc.) for 2nd texture coord. 
    D3DRS_WRAP2                     = 90,   // D3DWRAP* flags (D3DWRAP_U, D3DWRAPCOORD_0, etc.) for 3rd texture coord. 
    D3DRS_WRAP3                     = 91,   // D3DWRAP* flags (D3DWRAP_U, D3DWRAPCOORD_0, etc.) for 4th texture coord. 
    D3DRS_LIGHTING                  = 92,   // TRUE to enable lighting
    D3DRS_SPECULARENABLE            = 93,   // TRUE to enable specular 
    D3DRS_LOCALVIEWER               = 94,   // TRUE to enable camera-relative specular highlights
    D3DRS_COLORVERTEX               = 95,   // TRUE to enable per-vertex color
    D3DRS_BACKSPECULARMATERIALSOURCE= 96,   // D3DMATERIALCOLORSOURCE (Xbox extension)
    D3DRS_BACKDIFFUSEMATERIALSOURCE = 97,   // D3DMATERIALCOLORSOURCE (Xbox extension)
    D3DRS_BACKAMBIENTMATERIALSOURCE = 98,   // D3DMATERIALCOLORSOURCE (Xbox extension)
    D3DRS_BACKEMISSIVEMATERIALSOURCE= 99,   // D3DMATERIALCOLORSOURCE (Xbox extension)
    D3DRS_SPECULARMATERIALSOURCE    = 100,  // D3DMATERIALCOLORSOURCE 
    D3DRS_DIFFUSEMATERIALSOURCE     = 101,  // D3DMATERIALCOLORSOURCE 
    D3DRS_AMBIENTMATERIALSOURCE     = 102,  // D3DMATERIALCOLORSOURCE 
    D3DRS_EMISSIVEMATERIALSOURCE    = 103,  // D3DMATERIALCOLORSOURCE 
    D3DRS_BACKAMBIENT               = 104,  // D3DCOLOR (Xbox extension)
    D3DRS_AMBIENT                   = 105,  // D3DCOLOR 
    D3DRS_POINTSIZE                 = 106,  // float point size 
    D3DRS_POINTSIZE_MIN             = 107,  // float point size min threshold 
    D3DRS_POINTSPRITEENABLE         = 108,  // TRUE to enable point sprites
    D3DRS_POINTSCALEENABLE          = 109,  // TRUE to enable point size scaling
    D3DRS_POINTSCALE_A              = 110,  // float point attenuation A value 
    D3DRS_POINTSCALE_B              = 111,  // float point attenuation B value 
    D3DRS_POINTSCALE_C              = 112,  // float point attenuation C value 
    D3DRS_POINTSIZE_MAX             = 113,  // float point size max threshold 
    D3DRS_PATCHEDGESTYLE            = 114,  // D3DPATCHEDGESTYLE
    D3DRS_PATCHSEGMENTS             = 115,  // DWORD number of segments per edge when drawing patches
	*/

		
}

// ******************************************************************
// * patch: D3DDevice_DeleteStateBlock
// ******************************************************************
VOID WINAPI EMUPATCH(D3DDevice_DeleteStateBlock)
(
	DWORD Token
)
{
	FUNC_EXPORTS

	LOG_FUNC_ONE_ARG(Token);

	HRESULT hRet = g_pD3DDevice8->DeleteStateBlock(Token);
	DEBUG_D3DRESULT(hRet, "g_pD3DDevice8->DeleteStateBlock");
}


// ******************************************************************
// * patch: D3DDevice_SetModelView
// ******************************************************************
VOID WINAPI EMUPATCH(D3DDevice_SetModelView)
(
	CONST D3DMATRIX *pModelView, 
	CONST D3DMATRIX *pInverseModelView, 
	CONST D3DMATRIX *pComposite
)
{
	FUNC_EXPORTS

	LOG_FUNC_BEGIN
		LOG_FUNC_ARG(pModelView)
		LOG_FUNC_ARG(pInverseModelView)
		LOG_FUNC_ARG(pComposite)
		LOG_FUNC_END;

	// TODO: Implement
	LOG_UNIMPLEMENTED();
}

// ******************************************************************
// * patch: D3DDevice_FlushVertexCache
// ******************************************************************
void WINAPI EMUPATCH(D3DDevice_FlushVertexCache)()
{
	FUNC_EXPORTS

	LOG_FUNC();

	LOG_UNIMPLEMENTED();
}

// ******************************************************************
// * patch: D3DDevice_GetModelView
// ******************************************************************
HRESULT WINAPI EMUPATCH(D3DDevice_GetModelView)(D3DXMATRIX* pModelView)
{
	FUNC_EXPORTS

	LOG_FUNC_ONE_ARG(pModelView);

	D3DXMATRIX mtxWorld, mtxView;

	// I hope this is right
	g_pD3DDevice8->GetTransform( D3DTS_WORLD, &mtxWorld );
	g_pD3DDevice8->GetTransform( D3DTS_VIEW, &mtxView );

	*pModelView = mtxWorld * mtxView;

	return D3D_OK;
}

// ******************************************************************
// * patch: D3DDevice_SetBackMaterial
// ******************************************************************
VOID WINAPI EMUPATCH(D3DDevice_SetBackMaterial)(D3DMATERIAL8* pMaterial)
{
	FUNC_EXPORTS

	LOG_FUNC_ONE_ARG(pMaterial);

	LOG_NOT_SUPPORTED();
}

DWORD PushBuffer[64 * 1024 / sizeof(DWORD)];

// ******************************************************************
// * patch: D3D_SetCommonDebugRegisters
// ******************************************************************
void WINAPI EMUPATCH(D3D_SetCommonDebugRegisters)()
{
	FUNC_EXPORTS

	LOG_FUNC();

	// NOTE: I added this because I was too lazy to deal with emulating certain render
	// states that use it.  

	LOG_UNIMPLEMENTED();

}

// ******************************************************************
// * patch: D3D_BlockOnTime
// ******************************************************************
void WINAPI EMUPATCH(D3D_BlockOnTime)( DWORD Unknown1, int Unknown2 )
{
	FUNC_EXPORTS

	LOG_FUNC_BEGIN
		LOG_FUNC_ARG(Unknown1)
		LOG_FUNC_ARG(Unknown2)
		LOG_FUNC_END;

	// NOTE: This function is not meant to be emulated.  Just use it to find out
	// the function that is calling it, and emulate that instead!!!  If necessary,
	// create an XRef...

	//__asm int 3;

	LOG_UNIMPLEMENTED();
}

// ******************************************************************
// * patch: D3D_BlockOnResource
// ******************************************************************
void WINAPI EMUPATCH(D3D_BlockOnResource)( X_D3DResource* pResource )
{
	FUNC_EXPORTS

	LOG_FUNC_ONE_ARG(pResource);

	// TODO: Implement
	// NOTE: Azurik appears to call this directly from numerous points
	LOG_UNIMPLEMENTED();
		
}

// ******************************************************************
// * patch: D3DDevice_GetPixelShader
// ******************************************************************
VOID WINAPI EMUPATCH(D3DDevice_GetPixelShader)
(
	DWORD  Name,
	DWORD* pHandle
)
{
	FUNC_EXPORTS

	LOG_FUNC_BEGIN
		LOG_FUNC_ARG(Name)
		LOG_FUNC_ARG(pHandle)
		LOG_FUNC_END;

	// TODO: This implementation is very wrong, but better than nothing.
	*pHandle = g_dwCurrentPixelShader;
}

// ******************************************************************
// * patch: D3DDevice_SetRenderTargetFast
// ******************************************************************
VOID WINAPI EMUPATCH(D3DDevice_SetRenderTargetFast)
(
    X_D3DSurface	*pRenderTarget,
    X_D3DSurface	*pNewZStencil,
    DWORD			Flags
)
{
	FUNC_EXPORTS

	LOG_FORWARD("D3DDevice_SetRenderTarget");

	// Redirect to the standard version.
	
	EMUPATCH(D3DDevice_SetRenderTarget)(pRenderTarget, pNewZStencil);
}

// ******************************************************************
// * patch: D3DDevice_GetScissors
// ******************************************************************
VOID WINAPI EMUPATCH(D3DDevice_GetScissors)
(
	DWORD	*pCount, 
	BOOL	*pExclusive, 
	D3DRECT *pRects
)
{
	FUNC_EXPORTS

	LOG_FUNC_BEGIN
		LOG_FUNC_ARG(pCount)
		LOG_FUNC_ARG(pExclusive)
		LOG_FUNC_ARG(pRects)
		LOG_FUNC_END;

    // TODO: Save a copy of each scissor rect in case this function is called
	// in conjunction with D3DDevice::SetScissors. So far, only Outrun2 uses
	// this function. For now, just return the values within the current
	// viewport.

	D3DVIEWPORT8 vp;

	HRESULT hRet = g_pD3DDevice8->GetViewport( &vp );
	DEBUG_D3DRESULT(hRet, "g_pD3DDevice8->GetViewport");

	pRects->x1 = pRects->y1 = 0;
	pRects->x2 = vp.Width;
	pRects->y2 = vp.Height;

	pExclusive[0] = FALSE;
}

// ******************************************************************
// * patch: D3DDevice_GetBackMaterial
// ******************************************************************
VOID WINAPI EMUPATCH(D3DDevice_GetBackMaterial)(D3DMATERIAL8* pMaterial)
{
	FUNC_EXPORTS

	LOG_FUNC_ONE_ARG(pMaterial);

	LOG_NOT_SUPPORTED();

	HRESULT hRet = D3D_OK;

	// TODO: HACK: This is wrong, but better than nothing, right?
	if (pMaterial)
	{
		HRESULT hRet = g_pD3DDevice8->GetMaterial(pMaterial);
		DEBUG_D3DRESULT(hRet, "g_pD3DDevice8->GetMaterial");
	}

	if (FAILED(hRet))
	{
		EmuWarning("We're lying about getting a back material!");
		hRet = D3D_OK;
	}
}

// ******************************************************************
// * patch: D3D::LazySetPointParams
// ******************************************************************
void WINAPI EMUPATCH(D3D_LazySetPointParams)
(
	void* Device
)
{
	FUNC_EXPORTS

	LOG_FUNC_ONE_ARG(Device);

	LOG_UNIMPLEMENTED();
}

// ******************************************************************
// * patch: D3DDevice_GetMaterial
// ******************************************************************
VOID WINAPI EMUPATCH(D3DDevice_GetMaterial)
(
	D3DMATERIAL8* pMaterial
)
{
	FUNC_EXPORTS

	LOG_FUNC_ONE_ARG(pMaterial);

	HRESULT hRet = D3D_OK;

	if (pMaterial)
	{
		HRESULT hRet = g_pD3DDevice8->GetMaterial(pMaterial);
		DEBUG_D3DRESULT(hRet, "g_pD3DDevice8->GetMaterial");
	}

	if(FAILED(hRet))
    {
		EmuWarning("We're lying about getting a material!");
        hRet = D3D_OK;
    }
}

// ******************************************************************
// * update the current milliseconds per frame
// ******************************************************************

static void UpdateCurrentMSpFAndFPS() {
	if (g_EmuShared) {
		static float currentFPSVal = 30;

		float currentMSpFVal = (float)(1000.0 / (currentFPSVal == 0 ? 0.001 : currentFPSVal));
		g_EmuShared->SetCurrentMSpF(&currentMSpFVal);

		currentFPSVal = (float)(g_Frames*0.5 + currentFPSVal*0.5);
		g_EmuShared->SetCurrentFPS(&currentFPSVal);
	}
}
