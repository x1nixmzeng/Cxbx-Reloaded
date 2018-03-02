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
// *   Cxbx->Win32->Mutex.cpp
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
#include "Mutex.h"

// ******************************************************************
// * Constructor
// ******************************************************************
Mutex::Mutex()
{
    Native::InterlockedExchange(&m_MutexLock, 0);
    Native::InterlockedExchange(&m_OwnerProcess, 0);
    Native::InterlockedExchange(&m_OwnerThread, 0);
    Native::InterlockedExchange(&m_LockCount, 0);
}

// ******************************************************************
// * Lock
// ******************************************************************
void Mutex::Lock()
{
    Native::LONG _CurrentProcessId = (Native::LONG)Native::GetCurrentProcessId();
    Native::LONG _CurrentThreadId = (Native::LONG) Native::GetCurrentThreadId();
    while(true)
    {
        // Grab the lock, letting us look at the variables
#if (_MSC_VER < 1300)  // We are not using VC++.NET
		while (Native::InterlockedCompareExchange((Native::LPVOID*)&m_MutexLock, (Native::LPVOID)1, (Native::LPVOID)0))
#else
		while (Native::InterlockedCompareExchange((Native::LPLONG)&m_MutexLock, (Native::LONG)1, (Native::LONG)0))
#endif
		{
			Native::Sleep(1);
		}

        // Are we the the new owner?
        if (!m_OwnerProcess)
        {
            // Take ownership
            Native::InterlockedExchange(&m_OwnerProcess, _CurrentProcessId);
            Native::InterlockedExchange(&m_OwnerThread, _CurrentThreadId);
            Native::InterlockedExchange(&m_LockCount, 1);

            // Unlock the mutex itself
			Native::InterlockedExchange(&m_MutexLock, 0);

            return;
        }

        // If a different process owns this mutex right now, unlock
        // the mutex lock and wait.  The reading need not be
        // interlocked.
        if ((m_OwnerProcess != _CurrentProcessId) ||
            (m_OwnerThread  != _CurrentThreadId))
        {
            // Unlock the mutex itself
			Native::InterlockedExchange(&m_MutexLock, 0);

            // Wait and try again
			// TODO : Improve performance replacing Sleep(1) with YieldProcessor and perhaps an optional SpinLock
			Native::Sleep(1);
            continue;
        }

        // The mutex was already locked, but by us.  Just increment
        // the lock count and unlock the mutex itself.
        Native::InterlockedIncrement(&m_LockCount);
        Native::InterlockedExchange(&m_MutexLock, 0);

        return;
    }
}

// ******************************************************************
// * Unlock
// ******************************************************************
void Mutex::Unlock()
{
    // Grab the lock, letting us look at the variables
#if (_MSC_VER < 1300)  // We are not using VC++.NET
	while (Native::InterlockedCompareExchange((Native::LPVOID*)&m_MutexLock, (Native::LPVOID)1, (Native::LPVOID)0))
#else
	while (Native::InterlockedCompareExchange((Native::LPLONG)&m_MutexLock, (Native::LONG)1, (Native::LONG)0))
#endif
	{
		Native::Sleep(1);
	}

    // Decrement the lock count
    if (!Native::InterlockedDecrement(&m_LockCount))
    {
        // Mark the mutex as now unused
        Native::InterlockedExchange(&m_OwnerProcess, 0);
        Native::InterlockedExchange(&m_OwnerThread, 0);
    }

    // Unlock the mutex itself
	Native::InterlockedExchange(&m_MutexLock, 0);
}
