// ******************************************************************
// *
// *    .,-:::::    .,::      .::::::::.    .,::      .:
// *  ,;;;'````'    `;;;,  .,;;  ;;;'';;'   `;;;,  .,;;
// *  [[[             '[[,,[['   [[[__[[\.    '[[,,[['
// *  $$$              Y$$$P     $$""""Y$$     Y$$$P
// *  `88bo,__,o,    oP"``"Yo,  _88o,,od8P   oP"``"Yo,
// *    "YUMMMMMP",m"       "Mm,""YUMMMP" ,m"       "Mm,
// *
// *   Cxbx->Win32->Threads.cpp
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

namespace Native
{
#include <windows.h>
}

#include "Threads.h"

// Exception structure and method from:
// https://msdn.microsoft.com/en-us/library/xcb2z8hs.aspx

const Native::DWORD MS_VC_EXCEPTION = 0x406D1388;

#pragma pack(push,8)  
typedef struct tagTHREADNAME_INFO
{
	Native::DWORD dwType; // Must be 0x1000.  
	Native::LPCSTR szName; // Pointer to name (in user addr space).  
	Native::DWORD dwThreadID; // Thread ID (-1=caller thread).  
	Native::DWORD dwFlags; // Reserved for future use, must be zero.  
} THREADNAME_INFO;
#pragma pack(pop)  

void SetThreadName(Native::DWORD dwThreadID, const char* szThreadName)
{
	if (!Native::IsDebuggerPresent())
		return;

	THREADNAME_INFO info;
	info.dwType = 0x1000;
	info.szName = szThreadName;
	info.dwThreadID = dwThreadID;
	info.dwFlags = 0;
#pragma warning(push)  
#pragma warning(disable: 6320 6322)  
	__try {
		Native::RaiseException(MS_VC_EXCEPTION, 0, sizeof(info) / sizeof(Native::ULONG_PTR), (ULONG_PTR*)&info);
	}
	__except (EXCEPTION_EXECUTE_HANDLER) {
	}
#pragma warning(pop)  
}

void SetCurrentThreadName(const char* szThreadName)
{
	SetThreadName(Native::GetCurrentThreadId(), szThreadName);
}
