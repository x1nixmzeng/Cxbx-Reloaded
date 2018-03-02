// ******************************************************************
// *
// *    .,-:::::    .,::      .::::::::.    .,::      .:
// *  ,;;;'````'    `;;;,  .,;;  ;;;'';;'   `;;;,  .,;;
// *  [[[             '[[,,[['   [[[__[[\.    '[[,,[['
// *  $$$              Y$$$P     $$""""Y$$     Y$$$P
// *  `88bo,__,o,    oP"``"Yo,  _88o,,od8P   oP"``"Yo,
// *    "YUMMMMMP",m"       "Mm,""YUMMMP" ,m"       "Mm,
// *
// *   Common->XDK.h
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

#pragma once

namespace XDK
{
#include <xboxkrnl/xboxkrnl.h>

	// The following has moved from EmuXapi.h

	// ******************************************************************
	// * calling conventions
	// ******************************************************************
#define WINAPI              __stdcall

	// ******************************************************************
	// * Basic types
	// ******************************************************************
	typedef char                CHAR, CCHAR;
	typedef short               SHORT, CSHORT;
	typedef long                LONG;
	typedef unsigned char       UCHAR;
	typedef unsigned char       BYTE;
	typedef unsigned char       BOOLEAN;
	typedef unsigned short      USHORT;
	typedef unsigned short      WORD;
	typedef unsigned long       ULONG;
	//typedef unsigned long       DWORD;
	typedef unsigned long       SIZE_T, *PSIZE_T;
	typedef unsigned long       ACCESS_MASK, *PACCESS_MASK;
	typedef unsigned long       PHYSICAL_ADDRESS;
	typedef long                INT_PTR;
	typedef unsigned __int64    ULONGLONG;

	// xapi
	typedef int                 BOOL;
	typedef int                 INT;
	typedef unsigned int        UINT;
	typedef unsigned int        *PUINT;
	typedef LONG                HRESULT;

	// ******************************************************************
	// * Pointer types
	// ******************************************************************
	typedef CHAR               *PCHAR;
	typedef CHAR               *PCSZ;
	typedef BYTE               *PBYTE;
	typedef BOOLEAN            *PBOOLEAN;
	typedef UCHAR              *PUCHAR;
	typedef USHORT             *PUSHORT;
	typedef unsigned long              *PULONG;
	typedef DWORD              *LPDWORD;
	typedef ACCESS_MASK        *PACCESS_MASK;

	// Already declared!
	//typedef long               *LONG_PTR;
	//typedef unsigned long              *ULONG_PTR;

	typedef INT_PTR            *PINT_PTR;

#ifndef VOID
#  define VOID void
#endif

	typedef VOID               *PVOID, *LPVOID;
	typedef void               *HANDLE;
	typedef HANDLE             *PHANDLE;

	// ******************************************************************
	// * LPSECURITY_ATTRIBUTES
	// ******************************************************************
	typedef void* LPSECURITY_ATTRIBUTES;

	// ******************************************************************
	// * PTHREAD_START_ROUTINE / LPTHREAD_START_ROUTINE
	// ******************************************************************
	typedef DWORD (WINAPI *PTHREAD_START_ROUTINE)
	(
		LPVOID lpThreadParameter
	);

	typedef PTHREAD_START_ROUTINE LPTHREAD_START_ROUTINE;

	// ******************************************************************
	// * macro: EMUPATCH - marker on patches on original Xbox functions
	// ******************************************************************
#define EMUPATCH(Name) EmuPatch_##Name

	// ******************************************************************
	// * documentation purposes only
	// * Kept in the Xbox:: namespace for sanity
	// ******************************************************************
#define EXPORTNUM(a)
#define UNALIGNED
#define OPTIONAL
#define IN
#define OUT

}
