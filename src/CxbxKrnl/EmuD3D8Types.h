// ******************************************************************
// *
// *    .,-:::::    .,::      .::::::::.    .,::      .:
// *  ,;;;'````'    `;;;,  .,;;  ;;;'';;'   `;;;,  .,;;
// *  [[[             '[[,,[['   [[[__[[\.    '[[,,[['
// *  $$$              Y$$$P     $$""""Y$$     Y$$$P
// *  `88bo,__,o,    oP"``"Yo,  _88o,,od8P   oP"``"Yo,
// *    "YUMMMMMP",m"       "Mm,""YUMMMP" ,m"       "Mm,
// *
// *   Cxbx->Win32->CxbxKrnl->EmuD3D8Types.h
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
#ifndef EMUD3D8TYPES_H
#define EMUD3D8TYPES_H

namespace Native
{
	// include direct3d 8x headers
	#define DIRECT3D_VERSION 0x0800
	#include <d3d8.h>
	#include <d3dx8tex.h>
	#include <d3d8types.h>
} // Native


#include "XDK.h"

namespace Xbox
{

// TODO: fill out these enumeration tables for convienance
typedef XDK::DWORD X_D3DBLENDOP;
typedef XDK::DWORD X_D3DBLEND;
typedef XDK::DWORD X_D3DCMPFUNC;
typedef XDK::DWORD X_D3DFILLMODE;
typedef XDK::DWORD X_D3DSHADEMODE;
typedef XDK::DWORD X_D3DSTENCILOP;
typedef XDK::DWORD X_D3DTEXTURESTAGESTATETYPE;
typedef XDK::PVOID X_D3DCALLBACK;

typedef enum _X_D3DCULL
{
	X_D3DCULL_NONE = 0,
	X_D3DCULL_CW = 0x900,
	X_D3DCULL_CCW = 0x901,
	X_D3DCULL_FORCE_DWORD = 0x7fffffff
}
X_D3DCULL;

typedef enum _X_D3DFORMAT
{
/*
	Xbox1 D3DFORMAT notes
	---------------------

	The Xbox1 D3DFORMAT type consists of 4 different format categories :
	1. Swizzled (improves data locality, incompatible with native Direct3D)
	2. Compressed (DXT compression, giving 4:1 reduction on 4x4 pixel blocks)
	3. Linear (compatible with native Direct3D)
	4. Depth (Fixed or Floating point, stored Linear or Swizzled)

	Requirements\Format      Swizzled  Compressed  Linear  Depth   Notes

	Power-of-two required ?  YES       YES         NO      NO
	Mipmap supported ?       YES       YES         NO      YES     Linear has MipmapLevels = 1
	CubeMaps supported ?     YES       YES         NO      NO      Cubemaps have 6 faces
	Supports volumes ?       YES       YES         NO      NO      Volumes have 3 dimensions, Textures have 2
	Can be a rendertarget ?  YES       YES         YES     LINEAR  Depth buffers can only be rendered to if stored Linear

	Implications :
	- CubeMaps must be square
	- Volumes cannot be cube mapped and vice versa

	Maximum dimensions :
	2D : 4096 x 4096 (12 mipmap levels)
	3D : 512 x 512 x 512 (9 mipmap levels)

*/

	// Xbox D3DFORMAT types :
	// See http://wiki.beyondunreal.com/Legacy:Texture_Format

	// Swizzled Formats

	X_D3DFMT_L8 = 0x00,
	X_D3DFMT_AL8 = 0x01,
	X_D3DFMT_A1R5G5B5 = 0x02,
	X_D3DFMT_X1R5G5B5 = 0x03,
	X_D3DFMT_A4R4G4B4 = 0x04,
	X_D3DFMT_R5G6B5 = 0x05,
	X_D3DFMT_A8R8G8B8 = 0x06,
	X_D3DFMT_X8R8G8B8 = 0x07,
	X_D3DFMT_X8L8V8U8 = 0x07, // Alias

	X_D3DFMT_P8 = 0x0b, // 8-bit Palletized

	X_D3DFMT_A8 = 0x19,
	X_D3DFMT_A8L8 = 0x1a,
	X_D3DFMT_R6G5B5 = 0x27,
	X_D3DFMT_L6V5U5 = 0x27, // Alias

	X_D3DFMT_G8B8 = 0x28,
	X_D3DFMT_V8U8 = 0x28, // Alias

	X_D3DFMT_R8B8 = 0x29,
	X_D3DFMT_D24S8 = 0x2a,
	X_D3DFMT_F24S8 = 0x2b,
	X_D3DFMT_D16 = 0x2c,
	X_D3DFMT_D16_LOCKABLE = 0x2c, // Alias

	X_D3DFMT_F16 = 0x2d,
	X_D3DFMT_L16 = 0x32,
	X_D3DFMT_V16U16 = 0x33,
	X_D3DFMT_R5G5B5A1 = 0x38,
	X_D3DFMT_R4G4B4A4 = 0x39,
	X_D3DFMT_A8B8G8R8 = 0x3A,
	X_D3DFMT_Q8W8V8U8 = 0x3A, // Alias

	X_D3DFMT_B8G8R8A8 = 0x3B,
	X_D3DFMT_R8G8B8A8 = 0x3C,

	// YUV Formats

	X_D3DFMT_YUY2 = 0x24,
	X_D3DFMT_UYVY = 0x25,

	// Compressed Formats

	X_D3DFMT_DXT1 = 0x0C, // opaque/one-bit alpha
	X_D3DFMT_DXT2 = 0x0E, // Alias for X_D3DFMT_DXT3
	X_D3DFMT_DXT3 = 0x0E, // linear alpha
	X_D3DFMT_DXT4 = 0x0F, // Alias for X_D3DFMT_DXT5
	X_D3DFMT_DXT5 = 0x0F, // interpolated alpha

	// Linear Formats

	X_D3DFMT_LIN_A1R5G5B5 = 0x10,
	X_D3DFMT_LIN_R5G6B5 = 0x11,
	X_D3DFMT_LIN_A8R8G8B8 = 0x12,
	X_D3DFMT_LIN_L8 = 0x13,
	X_D3DFMT_LIN_R8B8 = 0x16,
	X_D3DFMT_LIN_G8B8 = 0x17,
	X_D3DFMT_LIN_V8U8 = 0x17, // Alias

	X_D3DFMT_LIN_AL8 = 0x1b,
	X_D3DFMT_LIN_X1R5G5B5 = 0x1c,
	X_D3DFMT_LIN_A4R4G4B4 = 0x1d,
	X_D3DFMT_LIN_X8R8G8B8 = 0x1e,
	X_D3DFMT_LIN_X8L8V8U8 = 0x1e, // Alias

	X_D3DFMT_LIN_A8 = 0x1f,
	X_D3DFMT_LIN_A8L8 = 0x20,
	X_D3DFMT_LIN_D24S8 = 0x2E,
	X_D3DFMT_LIN_F24S8 = 0x2f,
	X_D3DFMT_LIN_D16 = 0x30,
	X_D3DFMT_LIN_F16 = 0x31,
	X_D3DFMT_LIN_L16 = 0x35,
	X_D3DFMT_LIN_V16U16 = 0x36,
	X_D3DFMT_LIN_R6G5B5 = 0x37,
	X_D3DFMT_LIN_L6V5U5 = 0x37, // Alias

	X_D3DFMT_LIN_R5G5B5A1 = 0x3D,
	X_D3DFMT_LIN_R4G4B4A4 = 0x3e,
	X_D3DFMT_LIN_A8B8G8R8 = 0x3f,
	X_D3DFMT_LIN_B8G8R8A8 = 0x40,
	X_D3DFMT_LIN_R8G8B8A8 = 0x41,

	X_D3DFMT_VERTEXDATA = 0x64,

	X_D3DFMT_INDEX16 = 101/*=D3DFMT_INDEX16*/, // Dxbx addition : Not an Xbox format, used internally

	X_D3DFMT_UNKNOWN = 0xFFFFFFFF - 3,  // Unique declaration to make overloads possible
}
X_D3DFORMAT, *PX_D3DFORMAT;

// Primitives supported by draw-primitive API
typedef enum _X_D3DPRIMITIVETYPE
{
    X_D3DPT_NONE = 0, // Dxbx addition

    X_D3DPT_POINTLIST             = 1,
    X_D3DPT_LINELIST              = 2,
    X_D3DPT_LINELOOP              = 3,    // Xbox only
    X_D3DPT_LINESTRIP             = 4,
    X_D3DPT_TRIANGLELIST          = 5,
    X_D3DPT_TRIANGLESTRIP         = 6,
    X_D3DPT_TRIANGLEFAN           = 7,
    X_D3DPT_QUADLIST              = 8,    // Xbox only
    X_D3DPT_QUADSTRIP             = 9,    // Xbox only
    X_D3DPT_POLYGON               = 10,   // Xbox only

    X_D3DPT_MAX                   = 11,
    X_D3DPT_INVALID               = 0x7fffffff, /* force 32-bit size enum */
}
X_D3DPRIMITIVETYPE;

typedef enum _X_D3DRESOURCETYPE
{
    X_D3DRTYPE_NONE               =  0,
    X_D3DRTYPE_SURFACE            =  1, // = D3DRESOURCETYPE.D3DRTYPE_SURFACE
    X_D3DRTYPE_VOLUME             =  2, // = D3DRESOURCETYPE.D3DRTYPE_VOLUME
    X_D3DRTYPE_TEXTURE            =  3, // = D3DRESOURCETYPE.D3DRTYPE_TEXTURE
    X_D3DRTYPE_VOLUMETEXTURE      =  4, // = D3DRESOURCETYPE.D3DRTYPE_VOLUMETEXTURE
    X_D3DRTYPE_CUBETEXTURE        =  5, // = D3DRESOURCETYPE.D3DRTYPE_CUBETEXTURE
    X_D3DRTYPE_VERTEXBUFFER       =  6, // = D3DRESOURCETYPE.D3DRTYPE_VERTEXBUFFER
    X_D3DRTYPE_INDEXBUFFER        =  7, // = D3DRESOURCETYPE.D3DRTYPE_INDEXBUFFER
    X_D3DRTYPE_PUSHBUFFER         =  8,
    X_D3DRTYPE_PALETTE            =  9,
    X_D3DRTYPE_FIXUP              =  10,

    X_D3DRTYPE_FORCE_DWORD        = 0x7fffffff
}
X_D3DRESOURCETYPE;

// D3DUSAGE values (all but the Xbox extensions match the PC versions) :
#define X_D3DUSAGE_RENDERTARGET           0x00000001
#define X_D3DUSAGE_DEPTHSTENCIL           0x00000002
// for Vertex/Index buffers
#define X_D3DUSAGE_WRITEONLY              0x00000008
#define X_D3DUSAGE_POINTS                 0x00000040
#define X_D3DUSAGE_RTPATCHES              0x00000080
#define X_D3DUSAGE_DYNAMIC                0x00000200
// for CreateVertexShader
#define X_D3DUSAGE_PERSISTENTDIFFUSE      0x00000400L   // Xbox-only
#define X_D3DUSAGE_PERSISTENTSPECULAR     0x00000800L   // Xbox-only
#define X_D3DUSAGE_PERSISTENTBACKDIFFUSE  0x00001000L   // Xbox-only
#define X_D3DUSAGE_PERSISTENTBACKSPECULAR 0x00002000L   // Xbox-only
// for CreateTexture/CreateImageSurface
#define X_D3DUSAGE_BORDERSOURCE_COLOR     0x00000000L   // Xbox-only
#define X_D3DUSAGE_BORDERSOURCE_TEXTURE   0x00010000L   // Xbox-only

#define X_D3D_RENDER_MEMORY_ALIGNMENT     64

#define X_D3DSURFACE_ALIGNMENT            X_D3D_RENDER_MEMORY_ALIGNMENT
#define X_D3DTEXTURE_ALIGNMENT            (2 * X_D3D_RENDER_MEMORY_ALIGNMENT)
#define X_D3DTEXTURE_CUBEFACE_ALIGNMENT   (2 * X_D3D_RENDER_MEMORY_ALIGNMENT)
#define X_D3DTEXTURE_PITCH_ALIGNMENT      X_D3D_RENDER_MEMORY_ALIGNMENT
#define X_D3DTEXTURE_PITCH_MIN            X_D3DTEXTURE_PITCH_ALIGNMENT

#define TEXTURE_STAGES 4 // X_D3DTS_STAGECOUNT

typedef enum _X_D3DSET_DEPTH_CLIP_PLANES_FLAGS
{
    X_D3DSDCP_SET_VERTEXPROGRAM_PLANES         = 1,
    X_D3DSDCP_SET_FIXEDFUNCTION_PLANES         = 2,
    X_D3DSDCP_USE_DEFAULT_VERTEXPROGRAM_PLANES = 3,
    X_D3DSDCP_USE_DEFAULT_FIXEDFUNCTION_PLANES = 4,
} 
X_D3DSET_DEPTH_CLIP_PLANES_FLAGS;

#define X_D3DPRESENTFLAG_INTERLACED               0x00000020

typedef struct _X_D3DDISPLAYMODE
{
    XDK::UINT        Width;
    XDK::UINT        Height;
    XDK::UINT        RefreshRate;
    XDK::DWORD       Flags;
    X_D3DFORMAT Format;
}
X_D3DDISPLAYMODE;

typedef struct _X_D3DVERTEXBUFFER_DESC
{
	X_D3DFORMAT           Format;
	X_D3DRESOURCETYPE     Type;
}
X_D3DVERTEXBUFFER_DESC;

typedef struct _X_D3DINDEXBUFFER_DESC
{
	X_D3DFORMAT           Format;
	X_D3DRESOURCETYPE     Type;
}
X_D3DINDEXBUFFER_DESC;

typedef struct _X_D3DSURFACE_DESC
{
    X_D3DFORMAT         Format;
    X_D3DRESOURCETYPE   Type;
    XDK::DWORD               Usage;
    XDK::UINT                Size;
    Native::D3DMULTISAMPLE_TYPE MultiSampleType;
    XDK::UINT                Width;
    XDK::UINT                Height;
}
X_D3DSURFACE_DESC;

typedef struct _X_D3DPRESENT_PARAMETERS
{
    XDK::UINT                BackBufferWidth;
    XDK::UINT                BackBufferHeight;
    X_D3DFORMAT         BackBufferFormat;
	XDK::UINT                BackBufferCount;

	Native::D3DMULTISAMPLE_TYPE MultiSampleType;

	Native::D3DSWAPEFFECT       SwapEffect;
	Native::HWND                hDeviceWindow;
    XDK::BOOL                Windowed;
    XDK::BOOL                EnableAutoDepthStencil;
    X_D3DFORMAT         AutoDepthStencilFormat;
	XDK::DWORD               Flags;

	XDK::UINT                FullScreen_RefreshRateInHz;
	XDK::UINT                FullScreen_PresentationInterval;
    // The Windows DirectX8 variant ends here
    // This check guarantees identical layout, compared to Direct3D8._D3DPRESENT_PARAMETERS_:
    // assert(offsetof(X_D3DPRESENT_PARAMETERS, BufferSurfaces) == sizeof(_D3DPRESENT_PARAMETERS_));
    Native::IDirect3DSurface8  *BufferSurfaces[3];
    Native::IDirect3DSurface8  *DepthStencilSurface;
}
X_D3DPRESENT_PARAMETERS;

typedef struct _X_D3DGAMMARAMP
{
    XDK::BYTE    red[256];
    XDK::BYTE    green[256];
    XDK::BYTE    blue[256];
}
X_D3DGAMMARAMP;

#define X_PIXELSHADER_FAKE_HANDLE 0xDEADBEEF

struct X_D3DVertexShader
{
    union
    {
        XDK::DWORD   UnknownA;
        XDK::DWORD   Handle;
    };

    XDK::DWORD UnknownB;
    XDK::DWORD Flags;
    XDK::DWORD UnknownC[0x59];
};

typedef struct _X_D3DPIXELSHADERDEF	// <- blueshogun 10/1/07
{
   XDK::DWORD    PSAlphaInputs[8];          // X_D3DRS_PSALPHAINPUTS0..X_D3DRS_PSALPHAINPUTS7 : Alpha inputs for each stage
   XDK::DWORD    PSFinalCombinerInputsABCD; // X_D3DRS_PSFINALCOMBINERINPUTSABCD : Final combiner inputs
   XDK::DWORD    PSFinalCombinerInputsEFG;  // X_D3DRS_PSFINALCOMBINERINPUTSEFG : Final combiner inputs (continued)
   XDK::DWORD    PSConstant0[8];            // X_D3DRS_PSCONSTANT0_0..X_D3DRS_PSCONSTANT0_7 : C0 for each stage
   XDK::DWORD    PSConstant1[8];            // X_D3DRS_PSCONSTANT1_0..X_D3DRS_PSCONSTANT1_7 : C1 for each stage
   XDK::DWORD    PSAlphaOutputs[8];         // X_D3DRS_PSALPHAOUTPUTS0..X_D3DRS_PSALPHAOUTPUTS7 : Alpha output for each stage
   XDK::DWORD    PSRGBInputs[8];            // X_D3DRS_PSRGBINPUTS0..X_D3DRS_PSRGBINPUTS7 : RGB inputs for each stage
   XDK::DWORD    PSCompareMode;             // X_D3DRS_PSCOMPAREMODE : Compare modes for clipplane texture mode
   XDK::DWORD    PSFinalCombinerConstant0;  // X_D3DRS_PSFINALCOMBINERCONSTANT0 : C0 in final combiner
   XDK::DWORD    PSFinalCombinerConstant1;  // X_D3DRS_PSFINALCOMBINERCONSTANT1 : C1 in final combiner
   XDK::DWORD    PSRGBOutputs[8];           // X_D3DRS_PSRGBOUTPUTS0..X_D3DRS_PSRGBOUTPUTS7 : Stage 0 RGB outputs
   XDK::DWORD    PSCombinerCount;           // X_D3DRS_PSCOMBINERCOUNT : Active combiner count (Stages 0-7)
   XDK::DWORD    PSTextureModes;            // X_D3DRS_PSTEXTUREMODES: Texture addressing modes
   XDK::DWORD    PSDotMapping;              // X_D3DRS_PSDOTMAPPING : Input mapping for dot product modes
   XDK::DWORD    PSInputTexture;            // X_D3DRS_PSINPUTTEXTURE : Texture source for some texture modes

    // These last three DWORDs are used to define how Direct3D8 pixel shader constants map to the constant
    // registers in each combiner stage. They are used by the Direct3D run-time software but not by the hardware.
   XDK::DWORD    PSC0Mapping;               // Mapping of c0 regs to D3D constants
   XDK::DWORD    PSC1Mapping;               // Mapping of c1 regs to D3D constants
   XDK::DWORD    PSFinalCombinerConstants;  // Final combiner constant mapping
}
X_D3DPIXELSHADERDEF;

// These structures are used by Cxbx, not by the Xbox!!!
typedef struct _PixelShader_ 
{
	//IDirect3DPixelShader9* pShader;
	//ID3DXConstantTable *pConstantTable;
	XDK::DWORD Handle;

	XDK::BOOL bBumpEnvMap;

	// constants
	XDK::DWORD PSRealC0[8];
	XDK::DWORD PSRealC1[8];
	XDK::DWORD PSRealFC0;
	XDK::DWORD PSRealFC1;

	XDK::BOOL bConstantsInitialized;
	XDK::BOOL bConstantsChanged;

	XDK::DWORD dwStatus;
	X_D3DPIXELSHADERDEF	PSDef;

	XDK::DWORD dwStageMap[TEXTURE_STAGES];

}
PIXEL_SHADER;

typedef struct _STREAM_DYNAMIC_PATCH_
{
    XDK::BOOL  NeedPatch;       // This is to know whether it's data which must be patched
    XDK::DWORD ConvertedStride;
    XDK::DWORD NbrTypes;        // Number of the stream data types
    XDK::UINT  *pTypes;         // The stream data types (xbox)
	XDK::UINT  *pSizes;         // The stream data sizes (pc)
}
STREAM_DYNAMIC_PATCH;

typedef struct _VERTEX_DYNAMIC_PATCH_
{
	XDK::UINT                         NbrStreams; // The number of streams the vertex shader uses
    STREAM_DYNAMIC_PATCH        *pStreamPatches;
}
VERTEX_DYNAMIC_PATCH;

typedef struct _VERTEX_SHADER
{
	XDK::DWORD Handle;

    // These are the parameters given by the XBE,
    // we save them to be able to return them when necessary.
    XDK::UINT                  Size;
    XDK::DWORD                *pDeclaration;
    XDK::DWORD                 DeclarationSize;
    XDK::DWORD                *pFunction;
    XDK::DWORD                 FunctionSize;
    XDK::DWORD                 Type;
    XDK::DWORD                 Status;

    // Needed for dynamic stream patching
    VERTEX_DYNAMIC_PATCH  VertexDynamicPatch;
}
VERTEX_SHADER;

struct X_D3DResource
{
    XDK::DWORD Common;
	XDK::DWORD Data;
	XDK::DWORD Lock;
};

// D3D resource "common" masks
#define X_D3DCOMMON_REFCOUNT_MASK      0x0000FFFF
#define X_D3DCOMMON_TYPE_MASK          0x00070000
#define X_D3DCOMMON_TYPE_SHIFT         16
#define X_D3DCOMMON_TYPE_VERTEXBUFFER  0x00000000
#define X_D3DCOMMON_TYPE_INDEXBUFFER   0x00010000
#define X_D3DCOMMON_TYPE_PUSHBUFFER    0x00020000
#define X_D3DCOMMON_TYPE_PALETTE       0x00030000
#define X_D3DCOMMON_TYPE_TEXTURE       0x00040000
#define X_D3DCOMMON_TYPE_SURFACE       0x00050000 // Also covers Volume resources
#define X_D3DCOMMON_TYPE_FIXUP         0x00060000
#define X_D3DCOMMON_INTREFCOUNT_MASK   0x00780000
#define X_D3DCOMMON_INTREFCOUNT_SHIFT  19
#define X_D3DCOMMON_INTREFCOUNT_1      (1 << X_D3DCOMMON_INTREFCOUNT_SHIFT) // Dxbx addition
#define X_D3DCOMMON_VIDEOMEMORY        0x00800000 // Not used.
#define X_D3DCOMMON_D3DCREATED         0x01000000
#define X_D3DCOMMON_ISLOCKED           0x02000010 // Surface is currently locked (potential unswizzle candidate)
#define X_D3DCOMMON_UNUSED_MASK        0xFE000000
#define X_D3DCOMMON_UNUSED_SHIFT       25

// d3d palette common
#define X_D3DPALETTE_COMMON_PALETTESIZE_MASK       0xC0000000
#define X_D3DPALETTE_COMMON_PALETTESIZE_SHIFT      30

// special resource data flags (must set _SPECIAL *AND* specific flag(s))
#define X_D3DRESOURCE_DATA_FLAG_SPECIAL 0xFFFF0000
#define X_D3DRESOURCE_DATA_BACK_BUFFER (X_D3DRESOURCE_DATA_FLAG_SPECIAL | 0x00000001)
#define X_D3DRESOURCE_DATA_YUV_SURFACE (X_D3DRESOURCE_DATA_FLAG_SPECIAL | 0x00000002)
#define X_D3DRESOURCE_DATA_RENDER_TARGET (X_D3DRESOURCE_DATA_FLAG_SPECIAL | 0x00000004)
#define X_D3DRESOURCE_DATA_DEPTH_STENCIL (X_D3DRESOURCE_DATA_FLAG_SPECIAL | 0x00000008)
#define X_D3DRESOURCE_DATA_SURFACE_LEVEL (X_D3DRESOURCE_DATA_FLAG_SPECIAL | 0x00000020) // was 0xB00BBABE

// special resource lock flags
#define X_D3DRESOURCE_LOCK_FLAG_NOSIZE  0xEFFFFFFF
#define X_D3DRESOURCE_LOCK_PALETTE 0x8000BEEF

// Lock flags
#define X_D3DLOCK_NOFLUSH               0x00000010 // Xbox extension
#define X_D3DLOCK_NOOVERWRITE           0x00000020
#define X_D3DLOCK_TILED                 0x00000040 // Xbox extension
#define X_D3DLOCK_READONLY              0x00000080


const int X_D3DMULTISAMPLE_NONE = 0x0011;
const int X_D3DMULTISAMPLE_2_SAMPLES_MULTISAMPLE_LINEAR = 0x1021;
const int X_D3DMULTISAMPLE_2_SAMPLES_MULTISAMPLE_QUINCUNX = 0x1121;
const int X_D3DMULTISAMPLE_2_SAMPLES_SUPERSAMPLE_HORIZONTAL_LINEAR = 0x2021;
const int X_D3DMULTISAMPLE_2_SAMPLES_SUPERSAMPLE_VERTICAL_LINEAR = 0x2012;
const int X_D3DMULTISAMPLE_4_SAMPLES_MULTISAMPLE_LINEAR = 0x1022;
const int X_D3DMULTISAMPLE_4_SAMPLES_MULTISAMPLE_GAUSSIAN = 0x1222;
const int X_D3DMULTISAMPLE_4_SAMPLES_SUPERSAMPLE_LINEAR = 0x2022;
const int X_D3DMULTISAMPLE_4_SAMPLES_SUPERSAMPLE_GAUSSIAN = 0x2222;
const int X_D3DMULTISAMPLE_9_SAMPLES_MULTISAMPLE_GAUSSIAN = 0x1233;
const int X_D3DMULTISAMPLE_9_SAMPLES_SUPERSAMPLE_GAUSSIAN = 0x2233;

struct X_D3DVertexBuffer : public X_D3DResource
{

};
struct X_D3DIndexBuffer : public X_D3DResource
{

};

struct X_D3DPushBuffer : public X_D3DResource
{
    XDK::ULONG Size;
    XDK::ULONG AllocationSize;
};

struct X_D3DFixup : public X_D3DResource
{
    XDK::ULONG Run;
    XDK::ULONG Next;
    XDK::ULONG Size;
};

struct X_D3DPalette : public X_D3DResource
{
};

typedef enum _X_D3DPALETTESIZE
{
    D3DPALETTE_256              = 0,
    D3DPALETTE_128              = 1,
    D3DPALETTE_64               = 2,
    D3DPALETTE_32               = 3,
    D3DPALETTE_MAX              = 4,
    D3DPALETTE_FORCE_DWORD      = 0x7fffffff, /* force 32-bit size enum */
}
X_D3DPALETTESIZE;

struct X_D3DPixelContainer : public X_D3DResource
{
    XDK::DWORD		Format; // Format information about the texture.
    XDK::DWORD       Size; // Size of a non power-of-2 texture, must be zero otherwise
};

// pixel container "format" masks
#define X_D3DFORMAT_RESERVED1_MASK        0x00000003      // Must be zero
#define X_D3DFORMAT_DMACHANNEL_MASK       0x00000003
#define X_D3DFORMAT_DMACHANNEL_A          0x00000001      // DMA channel A - the default for all system memory
#define X_D3DFORMAT_DMACHANNEL_B          0x00000002      // DMA channel B - unused
#define X_D3DFORMAT_CUBEMAP               0x00000004      // Set if the texture if a cube map
#define X_D3DFORMAT_BORDERSOURCE_COLOR    0x00000008      // If set, uses D3DTSS_BORDERCOLOR as a border
#define X_D3DFORMAT_DIMENSION_MASK        0x000000F0      // # of dimensions, must be 2 or 3
#define X_D3DFORMAT_DIMENSION_SHIFT       4
#define X_D3DFORMAT_FORMAT_MASK           0x0000FF00      // D3DFORMAT - See X_D3DFMT_* above
#define X_D3DFORMAT_FORMAT_SHIFT          8
#define X_D3DFORMAT_MIPMAP_MASK           0x000F0000      // # mipmap levels (always 1 for surfaces)
#define X_D3DFORMAT_MIPMAP_SHIFT          16
#define X_D3DFORMAT_USIZE_MASK            0x00F00000      // Log 2 of the U size of the base texture (only set for swizzled or compressed)
#define X_D3DFORMAT_USIZE_SHIFT           20
#define X_D3DFORMAT_VSIZE_MASK            0x0F000000      // Log 2 of the V size of the base texture (only set for swizzled or compressed)
#define X_D3DFORMAT_VSIZE_SHIFT           24
#define X_D3DFORMAT_PSIZE_MASK            0xF0000000      // Log 2 of the P size of the base texture (only set for swizzled or compressed)
#define X_D3DFORMAT_PSIZE_SHIFT           28

// pixel container "size" masks
// The layout of the size field, used for non swizzled or compressed textures.
//
// The Size field of a container will be zero if the texture is swizzled or compressed.
// It is guarenteed to be non-zero otherwise because either the height/width will be
// greater than one or the pitch adjust will be nonzero because the minimum texture
// pitch is 8 bytes.
#define X_D3DSIZE_WIDTH_MASK              0x00000FFF   // Width of the texture - 1, in texels
//#define X_D3DSIZE_WIDTH_SHIFT           0
#define X_D3DSIZE_HEIGHT_MASK             0x00FFF000   // Height of the texture - 1, in texels
#define X_D3DSIZE_HEIGHT_SHIFT            12
#define X_D3DSIZE_PITCH_MASK              0xFF000000   // Pitch / 64 - 1
#define X_D3DSIZE_PITCH_SHIFT             24


struct X_D3DBaseTexture : public X_D3DPixelContainer
{

};

struct X_D3DTexture : public X_D3DBaseTexture
{

};

struct X_D3DVolumeTexture : public X_D3DBaseTexture
{

};

struct X_D3DCubeTexture : public X_D3DBaseTexture
{

};

struct X_D3DSurface : public X_D3DPixelContainer
{
	X_D3DBaseTexture *Parent;
};

struct X_D3DTILE
{
    XDK::DWORD   Flags;
    XDK::PVOID   pMemory;
    XDK::DWORD   Size;
    XDK::DWORD   Pitch;
    XDK::DWORD   ZStartTag;
    XDK::DWORD   ZOffset;
};

typedef enum _X_D3DCALLBACKTYPE	// blueshogun96 10/1/07
{
	X_D3DCALLBACK_READ		= 0, // Fixed PatrickvL 10/7/22
	X_D3DCALLBACK_WRITE		= 1
}
X_D3DCALLBACKTYPE;

typedef enum _X_D3DFIELDTYPE
{
    X_D3DFIELD_ODD            = 1,
    X_D3DFIELD_EVEN           = 2,
    X_D3DFIELD_PROGRESSIVE    = 3,
    X_D3DFIELD_FORCE_DWORD    = 0x7fffffff
}
X_D3DFIELDTYPE;

typedef struct _X_D3DFIELD_STATUS
{
    X_D3DFIELDTYPE Field;
	XDK::UINT           VBlankCount;
}
X_D3DFIELD_STATUS;

typedef struct _D3DVBLANKDATA
{
    XDK::DWORD           VBlank;
    XDK::DWORD           Swap;
    XDK::DWORD           Flags;
}
D3DVBLANKDATA;

typedef struct _D3DSWAPDATA 
{
    XDK::DWORD           Swap;
    XDK::DWORD           SwapVBlank;
    XDK::DWORD           MissedVBlanks;
    XDK::DWORD           TimeUntilSwapVBlank;
    XDK::DWORD           TimeBetweenSwapVBlanks;
} 
D3DSWAPDATA;

// D3DVBLANKCALLBACK
typedef void (__cdecl * D3DVBLANKCALLBACK)(D3DVBLANKDATA *pData);

// D3DSWAPCALLBACK
typedef void (__cdecl * D3DSWAPCALLBACK)(D3DSWAPDATA *pData);

// D3DCALLBACK
typedef void (__cdecl * D3DCALLBACK)(XDK::DWORD Context);

// X_D3DRENDERSTATETYPE values
typedef enum _X_D3DRENDERSTATETYPE {

	// Dxbx note : These declarations are from XDK version 5933, the most recent and complete version.
	// Older versions are slightly different (some members are missing), so we use a mapping table to
	// cater for the differences (see DxbxBuildRenderStateMappingTable). This enables to ignore these
	// version-differences in the rest of our code (unless it matters somehow); We write via indirection :
	//   *EmuMappedD3DRenderState[X_D3DRENDERSTATETYPE] = Value;
	//
	// And we read via the same mapping (do note, that missing elements all point to the same dummy) :
	//   Result = *EmuMappedD3DRenderState[X_D3DRENDERSTATETYPE];

	// Dxbx note : The PS* render states map 1-on-1 to the X_D3DPIXELSHADERDEF record,
	// SetPixelShader actually pushes the definition into these render state slots.
	// See DxbxUpdateActivePixelShader for how this is employed.

	// The set starts out with "pixel-shader" render states (all Xbox extensions) :
	X_D3DRS_PSALPHAINPUTS0 = 0,
	X_D3DRS_PSALPHAINPUTS1 = 1,
	X_D3DRS_PSALPHAINPUTS2 = 2,
	X_D3DRS_PSALPHAINPUTS3 = 3,
	X_D3DRS_PSALPHAINPUTS4 = 4,
	X_D3DRS_PSALPHAINPUTS5 = 5,
	X_D3DRS_PSALPHAINPUTS6 = 6,
	X_D3DRS_PSALPHAINPUTS7 = 7,
	X_D3DRS_PSFINALCOMBINERINPUTSABCD = 8,
	X_D3DRS_PSFINALCOMBINERINPUTSEFG = 9,
	X_D3DRS_PSCONSTANT0_0 = 10,
	X_D3DRS_PSCONSTANT0_1 = 11,
	X_D3DRS_PSCONSTANT0_2 = 12,
	X_D3DRS_PSCONSTANT0_3 = 13,
	X_D3DRS_PSCONSTANT0_4 = 14,
	X_D3DRS_PSCONSTANT0_5 = 15,
	X_D3DRS_PSCONSTANT0_6 = 16,
	X_D3DRS_PSCONSTANT0_7 = 17,
	X_D3DRS_PSCONSTANT1_0 = 18,
	X_D3DRS_PSCONSTANT1_1 = 19,
	X_D3DRS_PSCONSTANT1_2 = 20,
	X_D3DRS_PSCONSTANT1_3 = 21,
	X_D3DRS_PSCONSTANT1_4 = 22,
	X_D3DRS_PSCONSTANT1_5 = 23,
	X_D3DRS_PSCONSTANT1_6 = 24,
	X_D3DRS_PSCONSTANT1_7 = 25,
	X_D3DRS_PSALPHAOUTPUTS0 = 26,
	X_D3DRS_PSALPHAOUTPUTS1 = 27,
	X_D3DRS_PSALPHAOUTPUTS2 = 28,
	X_D3DRS_PSALPHAOUTPUTS3 = 29,
	X_D3DRS_PSALPHAOUTPUTS4 = 30,
	X_D3DRS_PSALPHAOUTPUTS5 = 31,
	X_D3DRS_PSALPHAOUTPUTS6 = 32,
	X_D3DRS_PSALPHAOUTPUTS7 = 33,
	X_D3DRS_PSRGBINPUTS0 = 34,
	X_D3DRS_PSRGBINPUTS1 = 35,
	X_D3DRS_PSRGBINPUTS2 = 36,
	X_D3DRS_PSRGBINPUTS3 = 37,
	X_D3DRS_PSRGBINPUTS4 = 38,
	X_D3DRS_PSRGBINPUTS5 = 39,
	X_D3DRS_PSRGBINPUTS6 = 40,
	X_D3DRS_PSRGBINPUTS7 = 41,
	X_D3DRS_PSCOMPAREMODE = 42,
	X_D3DRS_PSFINALCOMBINERCONSTANT0 = 43,
	X_D3DRS_PSFINALCOMBINERCONSTANT1 = 44,
	X_D3DRS_PSRGBOUTPUTS0 = 45,
	X_D3DRS_PSRGBOUTPUTS1 = 46,
	X_D3DRS_PSRGBOUTPUTS2 = 47,
	X_D3DRS_PSRGBOUTPUTS3 = 48,
	X_D3DRS_PSRGBOUTPUTS4 = 49,
	X_D3DRS_PSRGBOUTPUTS5 = 50,
	X_D3DRS_PSRGBOUTPUTS6 = 51,
	X_D3DRS_PSRGBOUTPUTS7 = 52,
	X_D3DRS_PSCOMBINERCOUNT = 53,
	X_D3DRS_PS_RESERVED = 54, // Dxbx note : This takes the slot of X_D3DPIXELSHADERDEF.PSTextureModes, set by D3DDevice_SetRenderState_LogicOp?
	X_D3DRS_PSDOTMAPPING = 55,
	X_D3DRS_PSINPUTTEXTURE = 56,
	// End of "pixel-shader" render states, continuing with "simple" render states :
	X_D3DRS_ZFUNC = 57, // D3DCMPFUNC
	X_D3DRS_ALPHAFUNC = 58, // D3DCMPFUNC
	X_D3DRS_ALPHABLENDENABLE = 59, // TRUE to enable alpha blending
	X_D3DRS_ALPHATESTENABLE = 60, // TRUE to enable alpha tests
	X_D3DRS_ALPHAREF = 61, // BYTE
	X_D3DRS_SRCBLEND = 62, // D3DBLEND
	X_D3DRS_DESTBLEND = 63, // D3DBLEND
	X_D3DRS_ZWRITEENABLE = 64, // TRUE to enable Z writes
	X_D3DRS_DITHERENABLE = 65, // TRUE to enable dithering
	X_D3DRS_SHADEMODE = 66, // D3DSHADEMODE
	X_D3DRS_COLORWRITEENABLE = 67, // D3DCOLORWRITEENABLE_ALPHA, etc. per-channel write enable
	X_D3DRS_STENCILZFAIL = 68, // D3DSTENCILOP to do if stencil test passes and Z test fails
	X_D3DRS_STENCILPASS = 69, // D3DSTENCILOP to do if both stencil and Z tests pass
	X_D3DRS_STENCILFUNC = 70, // D3DCMPFUNC
	X_D3DRS_STENCILREF = 71, // BYTE reference value used in stencil test
	X_D3DRS_STENCILMASK = 72, // BYTE mask value used in stencil test
	X_D3DRS_STENCILWRITEMASK = 73, // BYTE write mask applied to values written to stencil buffer
	X_D3DRS_BLENDOP = 74, // D3DBLENDOP setting
	X_D3DRS_BLENDCOLOR = 75, // D3DCOLOR for D3DBLEND_CONSTANTCOLOR (Xbox ext.)
	X_D3DRS_SWATHWIDTH = 76, // D3DSWATHWIDTH (Xbox ext.)
	X_D3DRS_POLYGONOFFSETZSLOPESCALE = 77, // float Z factor for shadow maps (Xbox ext.)
	X_D3DRS_POLYGONOFFSETZOFFSET = 78, // Xbox ext.
	X_D3DRS_POINTOFFSETENABLE = 79, // Xbox ext.
	X_D3DRS_WIREFRAMEOFFSETENABLE = 80, // Xbox ext.
	X_D3DRS_SOLIDOFFSETENABLE = 81, // Xbox ext.
	X_D3DRS_DEPTHCLIPCONTROL = 82, // [4627+] Xbox ext.
	X_D3DRS_STIPPLEENABLE = 83, // [4627+] Xbox ext.
	X_D3DRS_SIMPLE_UNUSED8 = 84, // [4627+]
	X_D3DRS_SIMPLE_UNUSED7 = 85, // [4627+]
	X_D3DRS_SIMPLE_UNUSED6 = 86, // [4627+]
	X_D3DRS_SIMPLE_UNUSED5 = 87, // [4627+]
	X_D3DRS_SIMPLE_UNUSED4 = 88, // [4627+]
	X_D3DRS_SIMPLE_UNUSED3 = 89, // [4627+]
	X_D3DRS_SIMPLE_UNUSED2 = 90, // [4627+]
	X_D3DRS_SIMPLE_UNUSED1 = 91, // [4627+]
	// End of "simple" render states, continuing with "deferred" render states :
	X_D3DRS_FOGENABLE = 92,
	X_D3DRS_FOGTABLEMODE = 93,
	X_D3DRS_FOGSTART = 94,
	X_D3DRS_FOGEND = 95,
	X_D3DRS_FOGDENSITY = 96,
	X_D3DRS_RANGEFOGENABLE = 97,
	X_D3DRS_WRAP0 = 98,
	X_D3DRS_WRAP1 = 99,
	X_D3DRS_WRAP2 = 100, // Dxbx addition
	X_D3DRS_WRAP3 = 101, // Dxbx addition
	X_D3DRS_LIGHTING = 102,
	X_D3DRS_SPECULARENABLE = 103,
	X_D3DRS_LOCALVIEWER = 104, // Dxbx addition
	X_D3DRS_COLORVERTEX = 105,
	X_D3DRS_BACKSPECULARMATERIALSOURCE = 106, // Xbox ext. nsp.
	X_D3DRS_BACKDIFFUSEMATERIALSOURCE = 107, // Xbox ext. nsp.
	X_D3DRS_BACKAMBIENTMATERIALSOURCE = 108, // Xbox ext. nsp.
	X_D3DRS_BACKEMISSIVEMATERIALSOURCE = 109, // Xbox ext. nsp.
	X_D3DRS_SPECULARMATERIALSOURCE = 110,
	X_D3DRS_DIFFUSEMATERIALSOURCE = 111,
	X_D3DRS_AMBIENTMATERIALSOURCE = 112,
	X_D3DRS_EMISSIVEMATERIALSOURCE = 113,
	X_D3DRS_BACKAMBIENT = 114, // Xbox ext. nsp.
	X_D3DRS_AMBIENT = 115,
	X_D3DRS_POINTSIZE = 116,
	X_D3DRS_POINTSIZE_MIN = 117,
	X_D3DRS_POINTSPRITEENABLE = 118,
	X_D3DRS_POINTSCALEENABLE = 119,
	X_D3DRS_POINTSCALE_A = 120,
	X_D3DRS_POINTSCALE_B = 121,
	X_D3DRS_POINTSCALE_C = 122,
	X_D3DRS_POINTSIZE_MAX = 123,
	X_D3DRS_PATCHEDGESTYLE = 124, // Dxbx addition
	X_D3DRS_PATCHSEGMENTS = 125,
	X_D3DRS_SWAPFILTER = 126, // [4361+] Xbox ext. nsp. D3DTEXF_LINEAR etc. filter to use for Swap
	X_D3DRS_PRESENTATIONINTERVAL = 127, // [4627+] Xbox ext. nsp.
	X_D3DRS_DEFERRED_UNUSED8 = 128, // [4627+]
	X_D3DRS_DEFERRED_UNUSED7 = 129, // [4627+]
	X_D3DRS_DEFERRED_UNUSED6 = 130, // [4627+]
	X_D3DRS_DEFERRED_UNUSED5 = 131, // [4627+]
	X_D3DRS_DEFERRED_UNUSED4 = 132, // [4627+]
	X_D3DRS_DEFERRED_UNUSED3 = 133, // [4627+]
	X_D3DRS_DEFERRED_UNUSED2 = 134, // [4627+]
	X_D3DRS_DEFERRED_UNUSED1 = 135, // [4627+]
	// End of "deferred" render states, continuing with "complex" render states :
	X_D3DRS_PSTEXTUREMODES = 136, // Xbox ext.
	X_D3DRS_VERTEXBLEND = 137,
	X_D3DRS_FOGCOLOR = 138,
	X_D3DRS_FILLMODE = 139,
	X_D3DRS_BACKFILLMODE = 140, // Dxbx addition : Xbox ext. nsp.
	X_D3DRS_TWOSIDEDLIGHTING = 141, // Dxbx addition : Xbox ext. nsp.
	X_D3DRS_NORMALIZENORMALS = 142,
	X_D3DRS_ZENABLE = 143,
	X_D3DRS_STENCILENABLE = 144,
	X_D3DRS_STENCILFAIL = 145,
	X_D3DRS_FRONTFACE = 146, // Dxbx addition : Xbox ext. nsp.
	X_D3DRS_CULLMODE = 147,
	X_D3DRS_TEXTUREFACTOR = 148,
	X_D3DRS_ZBIAS = 149,
	X_D3DRS_LOGICOP = 150, // Xbox ext.
	X_D3DRS_EDGEANTIALIAS = 151, // Dxbx note : No Xbox ext. (according to Direct3D8) !
	X_D3DRS_MULTISAMPLEANTIALIAS = 152,
	X_D3DRS_MULTISAMPLEMASK = 153,
	X_D3DRS_MULTISAMPLETYPE = 154, // [-3911] Xbox ext. \_ aliasses  D3DMULTISAMPLE_TYPE
	X_D3DRS_MULTISAMPLEMODE = 154, // [4361+] Xbox ext. /            D3DMULTISAMPLEMODE for the backbuffer
	X_D3DRS_MULTISAMPLERENDERTARGETMODE = 155, // [4361+] Xbox ext.
	X_D3DRS_SHADOWFUNC = 156, // D3DCMPFUNC (Xbox extension)
	X_D3DRS_LINEWIDTH = 157, // Xbox ext.
	X_D3DRS_SAMPLEALPHA = 158, // Xbox ext.
	X_D3DRS_DXT1NOISEENABLE = 159, // Xbox ext.
	X_D3DRS_YUVENABLE = 160, // [3911+] Xbox ext.
	X_D3DRS_OCCLUSIONCULLENABLE = 161, // [3911+] Xbox ext.
	X_D3DRS_STENCILCULLENABLE = 162, // [3911+] Xbox ext.
	X_D3DRS_ROPZCMPALWAYSREAD = 163, // [3911+] Xbox ext.
	X_D3DRS_ROPZREAD = 164, // [3911+] Xbox ext.
	X_D3DRS_DONOTCULLUNCOMPRESSED = 165, // [3911+] Xbox ext.
	// End of "complex" render states.
	X_D3DRS_UNK = 0x7fffffff // deferred render state "unknown" flag
} X_D3DRENDERSTATETYPE;

// Render state boundaries :

#define X_D3DRS_PS_FIRST X_D3DRS_PSALPHAINPUTS0
#define X_D3DRS_PS_LAST X_D3DRS_PSINPUTTEXTURE

#define X_D3DRS_SIMPLE_FIRST X_D3DRS_ZFUNC
#define X_D3DRS_SIMPLE_LAST X_D3DRS_SIMPLE_UNUSED1

#define X_D3DRS_DEFERRED_FIRST X_D3DRS_FOGENABLE
#define X_D3DRS_DEFERRED_LAST X_D3DRS_DEFERRED_UNUSED1

#define X_D3DRS_COMPLEX_FIRST X_D3DRS_PSTEXTUREMODES
#define X_D3DRS_COMPLEX_LAST X_D3DRS_DONOTCULLUNCOMPRESSED

#define X_D3DRS_FIRST X_D3DRS_PS_FIRST
#define X_D3DRS_LAST X_D3DRS_COMPLEX_LAST

// X_D3DWRAP values :
constexpr XDK::DWORD X_D3DWRAP_U = 0x00000010;
constexpr XDK::DWORD X_D3DWRAP_V = 0x00001000;
constexpr XDK::DWORD X_D3DWRAP_W = 0x00100000;

// X_D3DTEXTURESTAGESTATETYPE values :
// Dxbx note : See DxbxFromOldVersion_D3DTSS(), as these might need correction for older SDK versions!
// The set starts out with "deferred" texture states :
constexpr XDK::DWORD X_D3DTSS_ADDRESSU = 0;
constexpr XDK::DWORD X_D3DTSS_ADDRESSV = 1;
constexpr XDK::DWORD X_D3DTSS_ADDRESSW = 2;
constexpr XDK::DWORD X_D3DTSS_MAGFILTER = 3;
constexpr XDK::DWORD X_D3DTSS_MINFILTER = 4;
constexpr XDK::DWORD X_D3DTSS_MIPFILTER = 5;
constexpr XDK::DWORD X_D3DTSS_MIPMAPLODBIAS = 6;
constexpr XDK::DWORD X_D3DTSS_MAXMIPLEVEL = 7;
constexpr XDK::DWORD X_D3DTSS_MAXANISOTROPY = 8;
constexpr XDK::DWORD X_D3DTSS_COLORKEYOP = 9; // Xbox ext.
constexpr XDK::DWORD X_D3DTSS_COLORSIGN = 10; // Xbox ext.
constexpr XDK::DWORD X_D3DTSS_ALPHAKILL = 11; // Xbox ext.
constexpr XDK::DWORD X_D3DTSS_COLOROP = 12;
constexpr XDK::DWORD X_D3DTSS_COLORARG0 = 13;
constexpr XDK::DWORD X_D3DTSS_COLORARG1 = 14;
constexpr XDK::DWORD X_D3DTSS_COLORARG2 = 15;
constexpr XDK::DWORD X_D3DTSS_ALPHAOP = 16;
constexpr XDK::DWORD X_D3DTSS_ALPHAARG0 = 17;
constexpr XDK::DWORD X_D3DTSS_ALPHAARG1 = 18;
constexpr XDK::DWORD X_D3DTSS_ALPHAARG2 = 19;
constexpr XDK::DWORD X_D3DTSS_RESULTARG = 20;
constexpr XDK::DWORD X_D3DTSS_TEXTURETRANSFORMFLAGS = 21;
// End of "deferred" texture states, continuing with the rest :
constexpr XDK::DWORD X_D3DTSS_BUMPENVMAT00 = 22;
constexpr XDK::DWORD X_D3DTSS_BUMPENVMAT01 = 23;
constexpr XDK::DWORD X_D3DTSS_BUMPENVMAT11 = 24;
constexpr XDK::DWORD X_D3DTSS_BUMPENVMAT10 = 25;
constexpr XDK::DWORD X_D3DTSS_BUMPENVLSCALE = 26;
constexpr XDK::DWORD X_D3DTSS_BUMPENVLOFFSET = 27;
constexpr XDK::DWORD X_D3DTSS_TEXCOORDINDEX = 28;
constexpr XDK::DWORD X_D3DTSS_BORDERCOLOR = 29;
constexpr XDK::DWORD X_D3DTSS_COLORKEYCOLOR = 30; // Xbox ext.
constexpr XDK::DWORD X_D3DTSS_UNSUPPORTED = 31; // Note : Somehow, this one comes through D3DDevice_SetTextureStageStateNotInline sometimes
// End of texture states.

// Texture state boundaries :

constexpr XDK::DWORD X_D3DTSS_DEFERRED_FIRST = X_D3DTSS_ADDRESSU;
constexpr XDK::DWORD X_D3DTSS_DEFERRED_LAST = X_D3DTSS_TEXTURETRANSFORMFLAGS;

constexpr XDK::DWORD X_D3DTSS_FIRST = X_D3DTSS_ADDRESSU;
constexpr XDK::DWORD X_D3DTSS_LAST = X_D3DTSS_COLORKEYCOLOR;

constexpr XDK::DWORD X_D3DTS_STAGECOUNT = 4; // Dxbx addition
constexpr XDK::DWORD X_D3DTS_STAGESIZE = 32; // Dxbx addition

constexpr XDK::DWORD X_PSH_COMBINECOUNT = 8; // Dxbx addition
constexpr XDK::DWORD X_PSH_CONSTANTCOUNT = 8; // Dxbx addition

// X_D3DTEXTUREOP values :
constexpr XDK::DWORD X_D3DTOP_DISABLE = 1;
constexpr XDK::DWORD X_D3DTOP_SELECTARG1 = 2;
constexpr XDK::DWORD X_D3DTOP_SELECTARG2 = 3;
constexpr XDK::DWORD X_D3DTOP_MODULATE = 4;
constexpr XDK::DWORD X_D3DTOP_MODULATE2X = 5;
constexpr XDK::DWORD X_D3DTOP_MODULATE4X = 6;
constexpr XDK::DWORD X_D3DTOP_ADD = 7;
constexpr XDK::DWORD X_D3DTOP_ADDSIGNED = 8;
constexpr XDK::DWORD X_D3DTOP_ADDSIGNED2X = 9;
constexpr XDK::DWORD X_D3DTOP_SUBTRACT = 10;
constexpr XDK::DWORD X_D3DTOP_ADDSMOOTH = 11;
constexpr XDK::DWORD X_D3DTOP_BLENDDIFFUSEALPHA = 12;
constexpr XDK::DWORD X_D3DTOP_BLENDCURRENTALPHA = 13;
constexpr XDK::DWORD X_D3DTOP_BLENDTEXTUREALPHA = 14;
constexpr XDK::DWORD X_D3DTOP_BLENDFACTORALPHA = 15;
constexpr XDK::DWORD X_D3DTOP_BLENDTEXTUREALPHAPM = 16;
constexpr XDK::DWORD X_D3DTOP_PREMODULATE = 17;
constexpr XDK::DWORD X_D3DTOP_MODULATEALPHA_ADDCOLOR = 18;
constexpr XDK::DWORD X_D3DTOP_MODULATECOLOR_ADDALPHA = 19;
constexpr XDK::DWORD X_D3DTOP_MODULATEINVALPHA_ADDCOLOR = 20;
constexpr XDK::DWORD X_D3DTOP_MODULATEINVCOLOR_ADDALPHA = 21;
constexpr XDK::DWORD X_D3DTOP_DOTPRODUCT3 = 22;
constexpr XDK::DWORD X_D3DTOP_MULTIPLYADD = 23;
constexpr XDK::DWORD X_D3DTOP_LERP = 24;
constexpr XDK::DWORD X_D3DTOP_BUMPENVMAP = 25;
constexpr XDK::DWORD X_D3DTOP_BUMPENVMAPLUMINANCE = 26;

// X_D3DTEXTUREADDRESS values :
constexpr XDK::DWORD X_D3DTADDRESS_WRAP = 1;
constexpr XDK::DWORD X_D3DTADDRESS_MIRROR = 2;
constexpr XDK::DWORD X_D3DTADDRESS_CLAMP = 3;
constexpr XDK::DWORD X_D3DTADDRESS_BORDER = 4;
constexpr XDK::DWORD X_D3DTADDRESS_CLAMPTOEDGE = 5;

// X_D3DCLEAR values :
constexpr XDK::DWORD X_D3DCLEAR_ZBUFFER = 0x00000001;
constexpr XDK::DWORD X_D3DCLEAR_STENCIL = 0x00000002;
constexpr XDK::DWORD X_D3DCLEAR_TARGET_R = 0x00000010;  // Clear target surface R component (Xbox ext.)
constexpr XDK::DWORD X_D3DCLEAR_TARGET_G = 0x00000020;  // Clear target surface G component (Xbox ext.)
constexpr XDK::DWORD X_D3DCLEAR_TARGET_B = 0x00000040;  // Clear target surface B component (Xbox ext.)
constexpr XDK::DWORD X_D3DCLEAR_TARGET_A = 0x00000080;  // Clear target surface A component (Xbox ext.)
constexpr XDK::DWORD X_D3DCLEAR_TARGET = X_D3DCLEAR_TARGET_R | X_D3DCLEAR_TARGET_G | X_D3DCLEAR_TARGET_B | X_D3DCLEAR_TARGET_A;

// X_D3DCOLORWRITEENABLE values :
constexpr XDK::DWORD X_D3DCOLORWRITEENABLE_RED = (1 << 16);
constexpr XDK::DWORD X_D3DCOLORWRITEENABLE_GREEN = (1 << 8);
constexpr XDK::DWORD X_D3DCOLORWRITEENABLE_BLUE = (1 << 0);
constexpr XDK::DWORD X_D3DCOLORWRITEENABLE_ALPHA = (1 << 24);
constexpr XDK::DWORD X_D3DCOLORWRITEENABLE_ALL = 0x01010101; // Xbox ext.

// deferred texture stage state "unknown" flag
#define X_D3DTSS_UNK 0x7fffffff

typedef XDK::DWORD X_VERTEXSHADERCONSTANTMODE;

#define X_D3DSCM_96CONSTANTS                  0x00 // Enables constants 0..95
#define X_D3DSCM_192CONSTANTS                 0x01 // Enables constants -96..-1 on top of 0..95
#define X_D3DSCM_192CONSTANTSANDFIXEDPIPELINE 0x02 // Unsupported?
#define X_D3DSCM_NORESERVEDCONSTANTS          0x10  // Do not reserve constant -38 and -37

#define X_D3DSCM_RESERVED_CONSTANT1 -38 // Becomes 58 after correction, contains Scale v
#define X_D3DSCM_RESERVED_CONSTANT2 -37 // Becomes 59 after correction, contains Offset

#define X_D3DSCM_CORRECTION 96 // Add 96 to arrive at the range 0..191 (instead of 96..95)
#define X_D3DVS_CONSTREG_COUNT 192

// Vertex shader types
#define X_VST_NORMAL                  1
#define X_VST_READWRITE               2
#define X_VST_STATE                   3

// ******************************************************************
// * X_VERTEXSHADERINPUT
// ******************************************************************
typedef struct _X_VERTEXSHADERINPUT
{
    XDK::DWORD IndexOfStream;
    XDK::DWORD Offset;
    XDK::DWORD Format;
    XDK::BYTE  TesselationType;
    XDK::BYTE  TesselationSource;
}
X_VERTEXSHADERINPUT;

// ******************************************************************
// * X_VERTEXATTRIBUTEFORMAT
// ******************************************************************
typedef struct _X_VERTEXATTRIBUTEFORMAT
{
    X_VERTEXSHADERINPUT pVertexShaderInput[16];
}
X_VERTEXATTRIBUTEFORMAT;

// ******************************************************************
// * X_STREAMINPUT
// ******************************************************************
typedef struct _X_STREAMINPUT
{
    X_D3DVertexBuffer  *VertexBuffer;
    XDK::UINT                Stride;
    XDK::UINT                Offset;
} X_STREAMINPUT;

// vertex shader input registers for fixed function vertex shader

//          Name                   Register number      D3DFVF
const int X_D3DVSDE_POSITION     = 0; // Corresponds to D3DFVF_XYZ
const int X_D3DVSDE_BLENDWEIGHT  = 1; // Corresponds to D3DFVF_XYZRHW
const int X_D3DVSDE_NORMAL       = 2; // Corresponds to D3DFVF_NORMAL
const int X_D3DVSDE_DIFFUSE      = 3; // Corresponds to D3DFVF_DIFFUSE
const int X_D3DVSDE_SPECULAR     = 4; // Corresponds to D3DFVF_SPECULAR
const int X_D3DVSDE_FOG          = 5; // Xbox extension
const int X_D3DVSDE_POINTSIZE    = 6; // Dxbx addition
const int X_D3DVSDE_BACKDIFFUSE  = 7; // Xbox extension
const int X_D3DVSDE_BACKSPECULAR = 8; // Xbox extension
const int X_D3DVSDE_TEXCOORD0    = 9; // "Corresponds to D3DFVF_TEX0" says the docs, but 0 means no textures, so probably D3DFVF_TEX1!
const int X_D3DVSDE_TEXCOORD1    = 10; // Corresponds to D3DFVF_TEX{above}+1
const int X_D3DVSDE_TEXCOORD2    = 11; // Corresponds to D3DFVF_TEX{above}+2
const int X_D3DVSDE_TEXCOORD3    = 12; // Corresponds to D3DFVF_TEX{above}+3
const int X_D3DVSDE_VERTEX       = 0xFFFFFFFF; // Xbox extension for Begin/End drawing (data is a D3DVSDT_FLOAT4)

//typedef X_D3DVSDE = X_D3DVSDE_POSITION..High(DWORD)-2; // Unique declaration to make overloads possible;

  // bit declarations for _Type fields
const int X_D3DVSDT_FLOAT1      = 0x12; // 1D float expanded to (value, 0.0, 0.0, 1.0)
const int X_D3DVSDT_FLOAT2      = 0x22; // 2D float expanded to (value, value, 0.0, 1.0)
const int X_D3DVSDT_FLOAT3      = 0x32; // 3D float expanded to (value, value, value, 1.0) In double word format this is ARGB, or in byte ordering it would be B, G, R, A.
const int X_D3DVSDT_FLOAT4      = 0x42; // 4D float
const int X_D3DVSDT_D3DCOLOR    = 0x40; // 4D packed unsigned bytes mapped to 0.0 to 1.0 range
//const int X_D3DVSDT_UBYTE4      = 0x05; // 4D unsigned byte   Dxbx note : Not supported on Xbox ?
const int X_D3DVSDT_SHORT2      = 0x25; // 2D signed short expanded to (value, value, 0.0, 1.0)
const int X_D3DVSDT_SHORT4      = 0x45; // 4D signed short

  //  Xbox only declarations :
const int X_D3DVSDT_NORMSHORT1  = 0x11; // xbox ext. 1D signed, normalized short expanded to (value, 0.0, 0.0, 1.0). Signed, normalized shorts map from -1.0 to 1.0.
const int X_D3DVSDT_NORMSHORT2  = 0x21; // xbox ext. 2D signed, normalized short expanded to (value, value, 0.0, 1.0). Signed, normalized shorts map from -1.0 to 1.0.
const int X_D3DVSDT_NORMSHORT3  = 0x31; // xbox ext. 3D signed, normalized short expanded to (value, value, value, 1.0). Signed, normalized shorts map from -1.0 to 1.0.
const int X_D3DVSDT_NORMSHORT4  = 0x41; // xbox ext. 4D signed, normalized short expanded to (value, value, value, value). Signed, normalized shorts map from -1.0 to 1.0.
const int X_D3DVSDT_NORMPACKED3 = 0x16; // xbox ext. Three signed, normalized components packed in 32-bits. (11,11,10). Each component ranges from -1.0 to 1.0. Expanded to (value, value, value, 1.0).
const int X_D3DVSDT_SHORT1      = 0x15; // xbox ext. 1D signed short expanded to (value, 0., 0., 1). Signed shorts map to the range [-32768, 32767].
const int X_D3DVSDT_SHORT3      = 0x35; // xbox ext. 3D signed short expanded to (value, value, value, 1). Signed shorts map to the range [-32768, 32767].
const int X_D3DVSDT_PBYTE1      = 0x14; // xbox ext. 1D packed byte expanded to (value, 0., 0., 1). Packed bytes map to the range [0, 1].
const int X_D3DVSDT_PBYTE2      = 0x24; // xbox ext. 2D packed byte expanded to (value, value, 0., 1). Packed bytes map to the range [0, 1].
const int X_D3DVSDT_PBYTE3      = 0x34; // xbox ext. 3D packed byte expanded to (value, value, value, 1). Packed bytes map to the range [0, 1].
const int X_D3DVSDT_PBYTE4      = 0x44; // xbox ext. 4D packed byte expanded to (value, value, value, value). Packed bytes map to the range [0, 1].
const int X_D3DVSDT_FLOAT2H     = 0x72; // xbox ext. 3D float that expands to (value, value, 0.0, value). Useful for projective texture coordinates.
const int X_D3DVSDT_NONE        = 0x02; // xbox ext. nsp

const int MAX_NBR_STREAMS = 16;

#define X_D3DVSD_TOKENTYPESHIFT   29
#define X_D3DVSD_TOKENTYPEMASK    (7 << X_D3DVSD_TOKENTYPESHIFT)

#define X_D3DVSD_STREAMNUMBERSHIFT 0
#define X_D3DVSD_STREAMNUMBERMASK (0xF << X_D3DVSD_STREAMNUMBERSHIFT)

#define X_D3DVSD_DATALOADTYPESHIFT 28
#define X_D3DVSD_DATALOADTYPEMASK (0x1 << X_D3DVSD_DATALOADTYPESHIFT)

#define X_D3DVSD_DATATYPESHIFT 16
#define X_D3DVSD_DATATYPEMASK (0xFF << X_D3DVSD_DATATYPESHIFT)

#define X_D3DVSD_SKIPCOUNTSHIFT 16
#define X_D3DVSD_SKIPCOUNTMASK (0xF << X_D3DVSD_SKIPCOUNTSHIFT)

#define X_D3DVSD_VERTEXREGSHIFT 0
#define X_D3DVSD_VERTEXREGMASK (0x1F << X_D3DVSD_VERTEXREGSHIFT)

#define X_D3DVSD_VERTEXREGINSHIFT 20
#define X_D3DVSD_VERTEXREGINMASK (0xF << X_D3DVSD_VERTEXREGINSHIFT)

#define X_D3DVSD_CONSTCOUNTSHIFT 25
#define X_D3DVSD_CONSTCOUNTMASK (0xF << X_D3DVSD_CONSTCOUNTSHIFT)

#define X_D3DVSD_CONSTADDRESSSHIFT 0
#define X_D3DVSD_CONSTADDRESSMASK (0xFF << X_D3DVSD_CONSTADDRESSSHIFT)

#define X_D3DVSD_CONSTRSSHIFT 16
#define X_D3DVSD_CONSTRSMASK (0x1FFF << X_D3DVSD_CONSTRSSHIFT)

#define X_D3DVSD_EXTCOUNTSHIFT 24
#define X_D3DVSD_EXTCOUNTMASK (0x1F << X_D3DVSD_EXTCOUNTSHIFT)

#define X_D3DVSD_EXTINFOSHIFT 0
#define X_D3DVSD_EXTINFOMASK (0xFFFFFF << X_D3DVSD_EXTINFOSHIFT)

} // Xbox

#endif
