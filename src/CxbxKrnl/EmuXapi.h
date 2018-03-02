// ******************************************************************
// *
// *    .,-:::::    .,::      .::::::::.    .,::      .:
// *  ,;;;'````'    `;;;,  .,;;  ;;;'';;'   `;;;,  .,;;
// *  [[[             '[[,,[['   [[[__[[\.    '[[,,[['
// *  $$$              Y$$$P     $$""""Y$$     Y$$$P
// *  `88bo,__,o,    oP"``"Yo,  _88o,,od8P   oP"``"Yo,
// *    "YUMMMMMP",m"       "Mm,""YUMMMP" ,m"       "Mm,
// *
// *   Cxbx->Win32->CxbxKrnl->EmuXapi.h
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
#ifndef EMUXAPI_H
#define EMUXAPI_H

//// Beware of this namespace choice as DirectX8 is both Xbox AND Native
//namespace Native
//{
//#include <dxerr8.h>
//}

#include "XDK.h"

namespace Xbox
{
// ******************************************************************
// * documentation purposes only
// * Kept in the Xbox:: namespace for sanity
// ******************************************************************
#define EXPORTNUM(a)
#define UNALIGNED
#define OPTIONAL
#define IN
#define OUT

// ******************************************************************
// * LPSECURITY_ATTRIBUTES
// ******************************************************************
typedef void* LPSECURITY_ATTRIBUTES;

// ******************************************************************
// * PTHREAD_START_ROUTINE / LPTHREAD_START_ROUTINE
// ******************************************************************
typedef XDK::DWORD (WINAPI *PTHREAD_START_ROUTINE)
(
    XDK::LPVOID lpThreadParameter
);
typedef PTHREAD_START_ROUTINE LPTHREAD_START_ROUTINE;

// ******************************************************************
// * XINPUT_POLLING_PARAMETERS
// ******************************************************************
typedef struct _XINPUT_POLLING_PARAMETERS
{
    XDK::BYTE       fAutoPoll        : 1;
    XDK::BYTE       fInterruptOut    : 1;
    XDK::BYTE       ReservedMBZ1     : 6;
    XDK::BYTE       bInputInterval;
    XDK::BYTE       bOutputInterval;
    XDK::BYTE       ReservedMBZ2;
}
XINPUT_POLLING_PARAMETERS, *PXINPUT_POLLING_PARAMETERS;

// ******************************************************************
// * POLLING_PARAMETERS_HANDLE
// ******************************************************************
typedef struct _POLLING_PARAMETERS_HANDLE
{
    XINPUT_POLLING_PARAMETERS *pPollingParameters;

	XDK::DWORD dwPort;
}
POLLING_PARAMETERS_HANDLE, *PPOLLING_PARAMETERS_HANDLE;

// ******************************************************************
// * XPP_DEVICE_TYPE
// ******************************************************************
typedef struct _XPP_DEVICE_TYPE
{
	XDK::ULONG CurrentConnected;
	XDK::ULONG ChangeConnected;
	XDK::ULONG PreviousConnected;
}
XPP_DEVICE_TYPE, *PXPP_DEVICE_TYPE;

// ******************************************************************
// * XID_TYPE_INFORMATION
// ******************************************************************
typedef struct _XID_TYPE_INFORMATION
{
	XDK::UCHAR				ucType;
	XDK::UCHAR				ucUnknown[3];
	PXPP_DEVICE_TYPE    XppType;
	XDK::DWORD				dwUnknown[5];
} XID_TYPE_INFORMATION, *PXID_TYPE_INFORMATION;

// ******************************************************************
// * XDEVICE_PREALLOC_TYPE
// ******************************************************************
typedef struct _XDEVICE_PREALLOC_TYPE 
{
    PXPP_DEVICE_TYPE DeviceType;
	XDK::DWORD            dwPreallocCount;
} 
XDEVICE_PREALLOC_TYPE, *PXDEVICE_PREALLOC_TYPE;

// ******************************************************************
// * XINPUT_GAMEPAD
// ******************************************************************
typedef struct _XINPUT_GAMEPAD
{
    XDK::WORD    wButtons;
    XDK::BYTE    bAnalogButtons[8];
    XDK::SHORT   sThumbLX;
    XDK::SHORT   sThumbLY;
    XDK::SHORT   sThumbRX;
    XDK::SHORT   sThumbRY;
}
XINPUT_GAMEPAD, *PXINPUT_GAMEPAD;

// ******************************************************************
// * XINPUT_RUMBLE
// ******************************************************************
typedef struct _XINPUT_RUMBLE
{
    XDK::WORD   wLeftMotorSpeed;
    XDK::WORD   wRightMotorSpeed;
}
XINPUT_RUMBLE, *PXINPUT_RUMBLE;

// ******************************************************************
// * XINPUT_CAPABILITIES
// ******************************************************************
typedef struct _XINPUT_CAPABILITIES
{
    XDK::BYTE SubType;
    XDK::WORD Reserved;

    union
    {
        XINPUT_GAMEPAD Gamepad;
    }
    In;

    union
    {
        XINPUT_RUMBLE Rumble;
    }
    Out;
}
XINPUT_CAPABILITIES, *PXINPUT_CAPABILITIES;

// ******************************************************************
// * Device SubTypes
// ******************************************************************
#define XINPUT_DEVSUBTYPE_GC_GAMEPAD              0x01
#define XINPUT_DEVSUBTYPE_GC_GAMEPAD_ALT          0x02
#define XINPUT_DEVSUBTYPE_GC_WHEEL                0x10
#define XINPUT_DEVSUBTYPE_GC_ARCADE_STICK         0x20
#define XINPUT_DEVSUBTYPE_GC_DIGITAL_ARCADE_STICK 0x21
#define XINPUT_DEVSUBTYPE_GC_FLIGHT_STICK         0x30
#define XINPUT_DEVSUBTYPE_GC_SNOWBOARD            0x40

// ******************************************************************
// * XINPUT_STATE
// ******************************************************************
typedef struct _XINPUT_STATE
{
	XDK::DWORD dwPacketNumber;

    union
    {
        XINPUT_GAMEPAD Gamepad;
    };
}
XINPUT_STATE, *PXINPUT_STATE;

// ******************************************************************
// * offsets into analog button array
// ******************************************************************
#define XB_XINPUT_GAMEPAD_A                0
#define XB_XINPUT_GAMEPAD_B                1
#define XB_XINPUT_GAMEPAD_X                2
#define XB_XINPUT_GAMEPAD_Y                3
#define XB_XINPUT_GAMEPAD_BLACK            4
#define XB_XINPUT_GAMEPAD_WHITE            5
#define XB_XINPUT_GAMEPAD_LEFT_TRIGGER     6
#define XB_XINPUT_GAMEPAD_RIGHT_TRIGGER    7

// ******************************************************************
// * masks for digital buttons
// ******************************************************************
#define XB_XINPUT_GAMEPAD_DPAD_UP          0x00000001
#define XB_XINPUT_GAMEPAD_DPAD_DOWN        0x00000002
#define XB_XINPUT_GAMEPAD_DPAD_LEFT        0x00000004
#define XB_XINPUT_GAMEPAD_DPAD_RIGHT       0x00000008
#define XB_XINPUT_GAMEPAD_START            0x00000010
#define XB_XINPUT_GAMEPAD_BACK             0x00000020
#define XB_XINPUT_GAMEPAD_LEFT_THUMB       0x00000040
#define XB_XINPUT_GAMEPAD_RIGHT_THUMB      0x00000080

// ******************************************************************
// * XINPUT_FEEDBACK_HEADER
// ******************************************************************
typedef struct _XINPUT_FEEDBACK_HEADER
{
    XDK::DWORD           dwStatus;
    XDK::HANDLE OPTIONAL hEvent;
    XDK::BYTE            Reserved[58];
}
XINPUT_FEEDBACK_HEADER, *PXINPUT_FEEDBACK_HEADER;

// ******************************************************************
// * XINPUT_FEEDBACK
// ******************************************************************
typedef struct _XINPUT_FEEDBACK
{
    XINPUT_FEEDBACK_HEADER Header;

    union
    {
        XINPUT_RUMBLE Rumble;
    };
}
XINPUT_FEEDBACK, *PXINPUT_FEEDBACK;

// ******************************************************************
// * XBGAMEPAD
// ******************************************************************
struct XBGAMEPAD : public XINPUT_GAMEPAD
{
	Native::FLOAT      fX1;
	Native::FLOAT      fY1;
	Native::FLOAT      fX2;
	Native::FLOAT      fY2;
	XDK::WORD       wLastButtons;
	XDK::BOOL       bLastAnalogButtons[8];
	XDK::WORD       wPressedButtons;
	XDK::BOOL       bPressedAnalogButtons[8];
	XINPUT_RUMBLE	Rumble;
	XINPUT_FEEDBACK	Feedback;
	XINPUT_CAPABILITIES caps;
	XDK::HANDLE     hDevice;
	XDK::BOOL       bInserted;
	XDK::BOOL       bRemoved;
};

// ******************************************************************
// * RTL_HEAP_PARAMETERS
// ******************************************************************
typedef struct _RTL_HEAP_PARAMETERS
{
    XDK::ULONG   Length;
    XDK::BYTE    Unknown[0x2C];
}
RTL_HEAP_PARAMETERS;

// ******************************************************************
// * XTHREAD_NOTIFY_PROC
// ******************************************************************
typedef VOID (WINAPI *XTHREAD_NOTIFY_PROC)(XDK::BOOL fCreate);

// ******************************************************************
// * XTHREAD_NOTIFICATION
// ******************************************************************
typedef struct _XTHREAD_NOTIFICATION
{
    XDK::LIST_ENTRY          Reserved;
    XTHREAD_NOTIFY_PROC pfnNotifyRoutine;
}
XTHREAD_NOTIFICATION, *PXTHREAD_NOTIFICATION;

// ******************************************************************
// * XCALCSIG_SIGNATURE
// ******************************************************************
#define XCALCSIG_SIGNATURE_SIZE         20

typedef struct _XCALCSIG_SIGNATURE
{
    Native::BYTE    Signature[XCALCSIG_SIGNATURE_SIZE];
}
XCALCSIG_SIGNATURE, *PXCALCSIG_SIGNATURE;

#define XCALCSIG_FLAG_NON_ROAMABLE  (0x00000001)

// ******************************************************************
// * LAUNCH_DATA
// ******************************************************************
#define MAX_LAUNCH_DATA_SIZE	1024 * 3

typedef struct _LAUNCH_DATA 
{
	XDK::BYTE	Data[MAX_LAUNCH_DATA_SIZE];
} 
LAUNCH_DATA, *PLAUNCH_DATA;

#define LDT_TITLE                 0
#define LDT_FROM_DASHBOARD        2
#define LDT_FROM_DEBUGGER_CMDLINE 3
#define LDT_FROM_UPDATE           4

// ******************************************************************
// * XGetDeviceEnumerationStatus flags
// ******************************************************************
#define XDEVICE_ENUMERATION_IDLE 0
#define XDEVICE_ENUMERATION_BUSY 1

// ******************************************************************
// * macro: EMUPATCH - marker on patches on original Xbox functions
// ******************************************************************
#define EMUPATCH(Name) EmuPatch_##Name

// ******************************************************************
// * patch: XFormatUtilityDrive
// ******************************************************************
XDK::BOOL WINAPI EMUPATCH(XFormatUtilityDrive)();

#if 0 // Handled by ExQueryNonVolatileSetting(XC_MAX_OS) returning XBOX_USER_SETTINGS
// ******************************************************************
// * patch: GetTimeZoneInformation
// ******************************************************************
DWORD WINAPI EMUPATCH(GetTimeZoneInformation)
(
    OUT LPTIME_ZONE_INFORMATION lpTimeZoneInformation
);
#endif

// ******************************************************************
// * patch: XMountUtilityDrive
// ******************************************************************
XDK::BOOL WINAPI EMUPATCH(XMountUtilityDrive)
(
	XDK::BOOL    fFormatClean
);

// ******************************************************************
// * patch: XInitDevices
// ******************************************************************
VOID WINAPI EMUPATCH(XInitDevices)
(
	XDK::DWORD					dwPreallocTypeCount,
	PXDEVICE_PREALLOC_TYPE	PreallocTypes
);

// ******************************************************************
// * patch: XGetDevices
// ******************************************************************
XDK::DWORD WINAPI EMUPATCH(XGetDevices)
(
    XPP_DEVICE_TYPE *DeviceType
);

// ******************************************************************
// * patch: XGetDeviceChanges
// ******************************************************************
XDK::BOOL WINAPI EMUPATCH(XGetDeviceChanges)
(
    PXPP_DEVICE_TYPE DeviceType,
    XDK::PDWORD           pdwInsertions,
    XDK::PDWORD           pdwRemovals
);

// ******************************************************************
// * patch: XInputOpen
// ******************************************************************
XDK::HANDLE WINAPI EMUPATCH(XInputOpen)
(
    IN PXPP_DEVICE_TYPE             DeviceType,
    IN XDK::DWORD                        dwPort,
    IN XDK::DWORD                        dwSlot,
    IN PXINPUT_POLLING_PARAMETERS   pPollingParameters OPTIONAL
);

// ******************************************************************
// * patch: XInputClose
// ******************************************************************
VOID WINAPI EMUPATCH(XInputClose)
(
    IN XDK::HANDLE hDevice
);

// ******************************************************************
// * patch: XInputPoll
// ******************************************************************
XDK::DWORD WINAPI EMUPATCH(XInputPoll)
(
    IN XDK::HANDLE Device
);

// ******************************************************************
// * patch: XInputGetCapabilities
// ******************************************************************
XDK::DWORD WINAPI EMUPATCH(XInputGetCapabilities)
(
    IN  XDK::HANDLE               hDevice,
    OUT PXINPUT_CAPABILITIES pCapabilities
);

// ******************************************************************
// * patch: XInputGetState
// ******************************************************************
XDK::DWORD WINAPI EMUPATCH(XInputGetState)
(
    IN  XDK::HANDLE         hDevice,
    OUT PXINPUT_STATE  pState
);

// ******************************************************************
// * patch: XInputSetState
// ******************************************************************
XDK::DWORD WINAPI EMUPATCH(XInputSetState)
(
    IN     XDK::HANDLE           hDevice,
    IN OUT PXINPUT_FEEDBACK pFeedback
);


// ******************************************************************
// * patch: CreateMutex
// ******************************************************************
XDK::HANDLE WINAPI EMUPATCH(CreateMutex)
(
    LPSECURITY_ATTRIBUTES   lpMutexAttributes,
	XDK::BOOL                    bInitialOwner,
	Native::LPCSTR                  lpName
);

// ******************************************************************
// * patch: CloseHandle
// ******************************************************************
XDK::BOOL WINAPI EMUPATCH(CloseHandle)
(
	XDK::HANDLE hObject
);

// ******************************************************************
// * patch: SetThreadPriority
// ******************************************************************
XDK::BOOL WINAPI EMUPATCH(SetThreadPriority)
(
	XDK::HANDLE  hThread,
    int     nPriority
);

// ******************************************************************
// * patch: GetThreadPriority
// ******************************************************************
int WINAPI EMUPATCH(GetThreadPriority)
(
	XDK::HANDLE  hThread
);

// ******************************************************************
// * patch: SetThreadPriorityBoost
// ******************************************************************
XDK::BOOL WINAPI EMUPATCH(SetThreadPriorityBoost)
(
	XDK::HANDLE  hThread,
	XDK::BOOL    DisablePriorityBoost
);

// ******************************************************************
// * patch: GetExitCodeThread
// ******************************************************************
XDK::BOOL WINAPI EMUPATCH(GetExitCodeThread)
(
	XDK::HANDLE  hThread,
	XDK::LPDWORD lpExitCode
);

// ******************************************************************
// * patch: XapiThreadStartup
// ******************************************************************
VOID WINAPI EMUPATCH(XapiThreadStartup)
(
    XDK::DWORD dwDummy1,
    XDK::DWORD dwDummy2
);

/* Too High Level!
// ******************************************************************
// * patch: XapiSetupPerTitleDriveLetters
// ******************************************************************
NTSTATUS CDECL XapiSetupPerTitleDriveLetters(DWORD dwTitleId, LPCWSTR wszTitleName);
*/

// ******************************************************************
// * patch: XRegisterThreadNotifyRoutine
// ******************************************************************
VOID WINAPI EMUPATCH(XRegisterThreadNotifyRoutine)
(
    PXTHREAD_NOTIFICATION   pThreadNotification,
	XDK::BOOL                    fRegister
);

#if 0 // patch disabled
// ******************************************************************
// * patch: CreateFiber
// ******************************************************************
LPVOID WINAPI EMUPATCH(CreateFiber)
(
	DWORD					dwStackSize,
	LPFIBER_START_ROUTINE	lpStartRoutine,
	LPVOID					lpParameter
);
#endif

#if 0 // patch disabled
// ******************************************************************
// * patch: DeleteFiber
// ******************************************************************
VOID WINAPI EMUPATCH(DeleteFiber)
(
	LPVOID lpFiber
);
#endif

#if 0 // patch disabled
// ******************************************************************
// * patch: SwitchToFiber
// ******************************************************************
VOID WINAPI EMUPATCH(SwitchToFiber)
(
	LPVOID lpFiber 
);
#endif

#if 0 // patch disabled
// ******************************************************************
// * patch: ConvertThreadToFiber
// ******************************************************************
LPVOID WINAPI EMUPATCH(ConvertThreadToFiber)
(
	LPVOID lpParameter
);
#endif

#if 0 // patch disabled
// ******************************************************************
// * patch: XapiFiberStartup
// ******************************************************************
VOID WINAPI EMUPATCH(XapiFiberStartup)(DWORD dwDummy);
#endif

// ******************************************************************
// * patch: QueueUserAPC
// ******************************************************************
XDK::DWORD WINAPI EMUPATCH(QueueUserAPC)
(
	Native::PAPCFUNC	pfnAPC,
	XDK::HANDLE		hThread,
	XDK::DWORD   	dwData
);

#if 0 // Handled by WaitForSingleObject
// ******************************************************************
// * patch: GetOverlappedResult
// ******************************************************************
BOOL WINAPI EMUPATCH(GetOverlappedResult)
(
	HANDLE			hFile,
	LPOVERLAPPED	lpOverlapped,
	LPDWORD			lpNumberOfBytesTransferred,
	BOOL			bWait
);
#endif

// ******************************************************************
// * patch: XLaunchNewImageA
// ******************************************************************
XDK::DWORD WINAPI EMUPATCH(XLaunchNewImageA)
(
	Native::LPCSTR			lpTitlePath,
	PLAUNCH_DATA	pLaunchData
);

#if 0 // patch disabled
// ******************************************************************
// * patch: XGetLaunchInfo
// ******************************************************************
DWORD WINAPI EMUPATCH(XGetLaunchInfo)
(
	PDWORD			pdwLaunchDataType,
	PLAUNCH_DATA	pLaunchData
);
#endif

// ******************************************************************
// * patch: XSetProcessQuantumLength
// ******************************************************************
VOID WINAPI EMUPATCH(XSetProcessQuantumLength)
(
	XDK::DWORD dwMilliseconds
);

// ******************************************************************
// * patch: SignalObjectAndWait
// ******************************************************************
XDK::DWORD WINAPI EMUPATCH(SignalObjectAndWait)
(
	XDK::HANDLE	hObjectToSignal,
	XDK::HANDLE	hObjectToWaitOn,
	XDK::DWORD	dwMilliseconds,
	XDK::BOOL	bAlertable
);

// ******************************************************************
// * patch: timeSetEvent
// ******************************************************************
Native::MMRESULT WINAPI EMUPATCH(timeSetEvent)
(
	XDK::UINT			uDelay,
	XDK::UINT			uResolution,
	Native::LPTIMECALLBACK	fptc,
	XDK::DWORD			dwUser,
	XDK::UINT			fuEvent
);

// ******************************************************************
// * patch: timeKillEvent
// ******************************************************************
Native::MMRESULT WINAPI EMUPATCH(timeKillEvent)
(
	XDK::UINT uTimerID
);

// ******************************************************************
// * patch: RaiseException
// ******************************************************************
VOID WINAPI EMUPATCH(RaiseException)
(
	XDK::DWORD			dwExceptionCode,       // exception code
	XDK::DWORD			dwExceptionFlags,      // continuable exception flag
	XDK::DWORD			nNumberOfArguments,    // number of arguments
	CONST XDK::ULONG_PTR *lpArguments		   // array of arguments
);

// ******************************************************************
// * patch: lstrcmpiW
// ******************************************************************
int WINAPI EMUPATCH(lstrcmpiW)
(
	Native::LPCWSTR lpString1,
	Native::LPCWSTR lpString2
);

// ******************************************************************
// * patch: XMountMUA
// ******************************************************************
XDK::DWORD WINAPI EMUPATCH(XMountMUA)
(
	XDK::DWORD dwPort,                  
	XDK::DWORD dwSlot,                  
	XDK::PCHAR pchDrive               
);

// ******************************************************************
// * patch: XMountMURootA
// ******************************************************************
XDK::DWORD WINAPI EMUPATCH(XMountMURootA)
(
	XDK::DWORD dwPort,                  
	XDK::DWORD dwSlot,                  
	XDK::PCHAR pchDrive               
);

// ******************************************************************
// * patch: XMountAlternateTitleA
// ******************************************************************
XDK::DWORD WINAPI EMUPATCH(XMountAlternateTitleA)
(
	Native::LPCSTR		lpRootPath,
	XDK::DWORD		dwAltTitleId,               
	XDK::PCHAR		pchDrive               
);

// ******************************************************************
// * patch: XUnmountAlternateTitleA
// ******************************************************************
XDK::DWORD WINAPI EMUPATCH(XUnmountAlternateTitleA)(XDK::CHAR chDrive);

// ******************************************************************
// * patch: MoveFileA
// ******************************************************************
XDK::BOOL WINAPI EMUPATCH(MoveFileA)
(
    Native::LPCSTR lpExistingFileName,
	Native::LPCSTR lpNewFileName
);

// ******************************************************************
// * patch: XGetDeviceEnumerationStatus
// ******************************************************************
XDK::DWORD WINAPI EMUPATCH(XGetDeviceEnumerationStatus)();

// ******************************************************************
// * patch: SwitchToThread
// ******************************************************************
XDK::BOOL WINAPI EMUPATCH(SwitchToThread)();

// ******************************************************************
// * patch: XInputGetDeviceDescription
// ******************************************************************
XDK::DWORD WINAPI EMUPATCH(XInputGetDeviceDescription)
(
    XDK::HANDLE	hDevice,
    XDK::PVOID	pDescription
);

// ******************************************************************
// * patch: ReadFileEx
// ******************************************************************
XDK::BOOL WINAPI EMUPATCH(ReadFileEx)
(
	XDK::HANDLE hFile,                                       // handle to file
	XDK::LPVOID lpBuffer,                                    // data buffer
	XDK::DWORD nNumberOfBytesToRead,                         // number of bytes to read
	Native::LPOVERLAPPED lpOverlapped,                          // offset
	Native::LPOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine // completion routine
);

// ******************************************************************
// * patch: WriteFileEx
// ******************************************************************
XDK::BOOL WINAPI EMUPATCH(WriteFileEx)
(
	XDK::HANDLE hFile,                                       // handle to output file
	Native::LPCVOID lpBuffer,                                   // data buffer
	XDK::DWORD nNumberOfBytesToWrite,                        // number of bytes to write
	Native::LPOVERLAPPED lpOverlapped,                          // overlapped buffer
	Native::LPOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine // completion routine
);

// ******************************************************************
// * patch: OutputDebugStringA
// ******************************************************************
VOID WINAPI EMUPATCH(OutputDebugStringA)
(
	IN Native::LPCSTR lpOutputString
);

// s+
/* not necessary?
// ******************************************************************
// * patch: XCalculateSignatureBegin
// ******************************************************************
HANDLE WINAPI EMUPATCH(XCalculateSignatureBegin)
(
    DWORD dwFlags
);

// ******************************************************************
// * patch: XCalculateSignatureBegin
// ******************************************************************
HANDLE WINAPI EMUPATCH(XCalculateSignatureBeginEx)
(
    DWORD dwFlags,
    DWORD dwAltTitleId
);

// ******************************************************************
// * patch: XCalculateSignatureUpdate
// ******************************************************************
DWORD WINAPI EMUPATCH(XCalculateSignatureUpdate)
(
  HANDLE        hCalcSig,
  const BYTE    *pbData,
  ULONG         cbData
);

// ******************************************************************
// * patch: XCalculateSignatureEnd
// ******************************************************************
DWORD WINAPI EMUPATCH(XCalculateSignatureEnd)
(
  HANDLE                hCalcSig,
  PXCALCSIG_SIGNATURE   pSignature
);
//*/
// +s


} // ~XBL
#endif
