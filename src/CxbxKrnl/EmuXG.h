// ******************************************************************
// *
// *    .,-:::::    .,::      .::::::::.    .,::      .:
// *  ,;;;'````'    `;;;,  .,;;  ;;;'';;'   `;;;,  .,;;
// *  [[[             '[[,,[['   [[[__[[\.    '[[,,[['
// *  $$$              Y$$$P     $$""""Y$$     Y$$$P
// *  `88bo,__,o,    oP"``"Yo,  _88o,,od8P   oP"``"Yo,
// *    "YUMMMMMP",m"       "Mm,""YUMMMP" ,m"       "Mm,
// *
// *   Cxbx->Win32->CxbxKrnl->EmuXG.h
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
#ifndef EMUXG_H
#define EMUXG_H

#include "XDK.h"

namespace Xbox
{

typedef struct _XGPOINT3D
{
    XDK::DWORD u;
    XDK::DWORD v;
    XDK::DWORD w;
}
XGPOINT3D;

// ******************************************************************
// * patch: XGIsSwizzledFormat
// ******************************************************************
XDK::PVOID WINAPI EMUPATCH(XGIsSwizzledFormat)
(
    X_D3DFORMAT     Format
);

#if 0 // Leave unpatched
// ******************************************************************
// * patch: XGSwizzleRect
// ******************************************************************
VOID WINAPI EMUPATCH(XGSwizzleRect)
(
    LPCVOID       pSource,
    DWORD         Pitch,
    LPCRECT       pRect,
    LPVOID        pDest,
    DWORD         Width,
    DWORD         Height,
    CONST LPPOINT pPoint,
    DWORD         BytesPerPixel
);
#endif

// ******************************************************************
// * patch: XGSwizzleBox
// ******************************************************************
VOID WINAPI EMUPATCH(XGSwizzleBox)
(
    LPCVOID          pSource,
    XDK::DWORD            RowPitch,
    XDK::DWORD            SlicePitch,
    CONST Native::D3DBOX    *pBox,
    XDK::LPVOID           pDest,
    XDK::DWORD            Width,
    XDK::DWORD            Height,
    XDK::DWORD            Depth,
    CONST XGPOINT3D *pPoint,
	XDK::DWORD            BytesPerPixel
);

// ******************************************************************
// * patch: XGWriteSurfaceOrTextureToXPR
// ******************************************************************
XDK::HRESULT WINAPI EMUPATCH(XGWriteSurfaceOrTextureToXPR)
( 
	Native::LPVOID			pResource,
	const char*		cPath,
	XDK::BOOL			bWriteSurfaceAsTexture
);

// ******************************************************************
// * patch: XGSetTextureHeader
// ******************************************************************
VOID	WINAPI EMUPATCH(XGSetTextureHeader)
(
	XDK::UINT			Width,
	XDK::UINT			Height,
	XDK::UINT			Levels,
	XDK::DWORD			Usage,
	X_D3DFORMAT		Format,
	Native::D3DPOOL			Pool,
	X_D3DTexture*	pTexture,
	XDK::UINT			Data,
	XDK::UINT			Pitch
);

// ******************************************************************
// * patch: XFONT_OpenBitmapFontFromMemory 
// ******************************************************************
HRESULT WINAPI EMUPATCH(XFONT_OpenBitmapFontFromMemory) 
(
	CONST void		*pFontData,
	unsigned		uFontDataSize,
	void			**ppFont
);

} // Xbox

#endif
