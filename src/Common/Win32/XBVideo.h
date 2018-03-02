// ******************************************************************
// *
// *    .,-:::::    .,::      .::::::::.    .,::      .:
// *  ,;;;'````'    `;;;,  .,;;  ;;;'';;'   `;;;,  .,;;
// *  [[[             '[[,,[['   [[[__[[\.    '[[,,[['
// *  $$$              Y$$$P     $$""""Y$$     Y$$$P
// *  `88bo,__,o,    oP"``"Yo,  _88o,,od8P   oP"``"Yo,
// *    "YUMMMMMP",m"       "Mm,""YUMMMP" ,m"       "Mm,
// *
// *   Cxbx->Win32->Cxbx->XBVideo.h
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
#ifndef XBVIDEO_H
#define XBVIDEO_H

#include "Cxbx.h"
#include "Common/Error.h"
#include "Mutex.h"

// ******************************************************************
// * class: XBVideo
// ******************************************************************
class XBVideo : public Error
{
    public:
        // ******************************************************************
        // * Initialization
        // ******************************************************************
        XBVideo();
       ~XBVideo();

        // ******************************************************************
        // * Registry Load/Save
        // ******************************************************************
        void Load(const char *szRegistryKey);
        void Save(const char *szRegistryKey);

        // ******************************************************************
        // * SetDirect3DDevice
        // ******************************************************************
        void  SetDirect3DDevice(Native::DWORD dwDirect3DDevice) { m_dwDirect3DDevice = dwDirect3DDevice; }
		Native::DWORD GetDirect3DDevice() const { return m_dwDirect3DDevice; }

        // ******************************************************************
        // * SetDisplayAdapter
        // ******************************************************************
        void  SetDisplayAdapter(Native::DWORD dwDisplayAdapter) { m_dwDisplayAdapter = dwDisplayAdapter; }
		Native::DWORD GetDisplayAdapter() const { return m_dwDisplayAdapter; }

        // ******************************************************************
        // * SetVideoResolution
        // ******************************************************************
        void SetVideoResolution(const char *szBuffer) { strcpy(m_szVideoResolution, szBuffer); }
        const char *GetVideoResolution(){ return m_szVideoResolution; }

        // ******************************************************************
        // * Fullscreen Toggling
        // ******************************************************************
        void SetFullscreen(Native::BOOL bFullscreen) { m_bFullscreen = bFullscreen; }
		Native::BOOL GetFullscreen() const { return m_bFullscreen; }

        // ******************************************************************
        // * VSync Toggling
        // ******************************************************************
        void SetVSync(Native::BOOL bVSync) { m_bVSync = bVSync; }
		Native::BOOL GetVSync() const { return m_bVSync; }

		// ******************************************************************
		// * Hardware YUV Toggling
		// ******************************************************************
		void SetHardwareYUV(Native::BOOL bHardwareYUV) { m_bHardwareYUV = bHardwareYUV; }
		Native::BOOL GetHardwareYUV() const { return m_bHardwareYUV; }

    private:
        // ******************************************************************
        // * Configuration
        // ******************************************************************
        char  m_szVideoResolution[100];
        Native::DWORD m_dwDisplayAdapter;
        Native::DWORD m_dwDirect3DDevice;
        Native::BOOL  m_bVSync;
        Native::BOOL  m_bFullscreen;
        Native::BOOL  m_bHardwareYUV;
};

#endif
