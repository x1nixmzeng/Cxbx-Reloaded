// ******************************************************************
// *
// *    .,-:::::    .,::      .::::::::.    .,::      .:
// *  ,;;;'````'    `;;;,  .,;;  ;;;'';;'   `;;;,  .,;;
// *  [[[             '[[,,[['   [[[__[[\.    '[[,,[['
// *  $$$              Y$$$P     $$""""Y$$     Y$$$P
// *  `88bo,__,o,    oP"``"Yo,  _88o,,od8P   oP"``"Yo,
// *    "YUMMMMMP",m"       "Mm,""YUMMMP" ,m"       "Mm,
// *
// *   Cxbx->Win32->CxbxKrnl->EmuNtDll.h
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
#ifndef EMUNTDLL_H
#define EMUNTDLL_H

#include "XDK.h"

namespace Xbox
{
#define DIRECTORY_QUERY 1
#define DIRECTORY_TRAVERSE 2
#define DIRECTORY_CREATE_OBJECT 4
#define DIRECTORY_CREATE_SUBDIRECTORY 8

// ******************************************************************
// * KPROCESSOR_MODE
// ******************************************************************
typedef XDK::CCHAR KPROCESSOR_MODE;

// ******************************************************************
// * KWAIT_REASON
// ******************************************************************
typedef enum _KWAIT_REASON {
	Executive,
	FreePage,
	PageIn,
	PoolAllocation,
	DelayExecution,
	Suspended,
	UserRequest,
	WrExecutive,
	WrFreePage,
	WrPageIn,
	WrPoolAllocation,
	WrDelayExecution,
	WrSuspended,
	WrUserRequest,
	WrEventPair,
	WrQueue,
	WrLpcReceive,
	WrLpcReply,
	WrVirtualMemory,
	WrPageOut,
	WrRendezvous,
	Spare2,
	Spare3,
	Spare4,
	Spare5,
	WrCalloutStack,
	WrKernel,
	WrResource,
	WrPushLock,
	WrMutex,
	WrQuantumEnd,
	WrDispatchInt,
	WrPreempted,
	WrYieldExecution,
	WrFastMutex,
	WrGuardedMutex,
	WrRundown,
	MaximumWaitReason
} KWAIT_REASON;

// ******************************************************************
// * MODE
// ******************************************************************
typedef enum _MODE
{
    KernelMode,
    UserMode,
    MaximumMode
}
MODE;

// ******************************************************************
// * LARGE_INTEGER
// ******************************************************************
typedef union _LARGE_INTEGER
{
	struct {
		XDK::DWORD LowPart;
		XDK::LONG  HighPart;
	};
	struct {
		XDK::DWORD LowPart;
		XDK::LONG  HighPart;
	} u;
	XDK::LONGLONG QuadPart;
}
LARGE_INTEGER, *PLARGE_INTEGER;

// ******************************************************************
// * LDT_ENTRY
// ******************************************************************
typedef struct _LDT_ENTRY
{
    XDK::WORD    LimitLow;
    XDK::WORD    BaseLow;

    union
    {
        struct
        {
            XDK::BYTE    BaseMid;
            XDK::BYTE    Flags1;     // Declare as bytes to avoid alignment
            XDK::BYTE    Flags2;     // Problems.
            XDK::BYTE    BaseHi;
        }
        Bytes;

        struct
        {
            XDK::DWORD   BaseMid : 8;
            XDK::DWORD   Type : 5;
            XDK::DWORD   Dpl : 2;
            XDK::DWORD   Pres : 1;
            XDK::DWORD   LimitHi : 4;
            XDK::DWORD   Sys : 1;
            XDK::DWORD   Reserved_0 : 1;
            XDK::DWORD   Default_Big : 1;
            XDK::DWORD   Granularity : 1;
            XDK::DWORD   BaseHi : 8;
        }
        Bits;

    }
    HighWord;
}
LDT_ENTRY, *PLDT_ENTRY;

// ******************************************************************
// * STRING
// ******************************************************************
typedef struct _STRING
{
	XDK::USHORT  Length;
    XDK::USHORT  MaximumLength;
    XDK::PCHAR   Buffer;
}
STRING, ANSI_STRING, *PSTRING, *PANSI_STRING;

// ******************************************************************
// * RTL_HEAP_DEFINITION
// ******************************************************************
typedef struct _RTL_HEAP_DEFINITION
{
    XDK::ULONG   Length;
    XDK::ULONG   Unknown1;
    XDK::ULONG   Unknown2;
    XDK::ULONG   Unknown3;
    XDK::ULONG   Unknown4;
    XDK::ULONG   Unknown5;
    XDK::ULONG   Unknown6;
    XDK::ULONG   Unknown7;
    XDK::ULONG   Unknown8;
    XDK::ULONG   Unknown9;
    XDK::ULONG   Unknown10;
    XDK::ULONG   Unknown11;
}
RTL_HEAP_DEFINITION, *PRTL_HEAP_DEFINITION;

// ******************************************************************
// * RTL_CRITICAL_SECTION
// ******************************************************************
typedef struct _RTL_CRITICAL_SECTION
{
    XDK::DWORD               Unknown[4];                                     // 0x00
    XDK::LONG                LockCount;                                      // 0x10
    XDK::LONG                RecursionCount;                                 // 0x14
    XDK::HANDLE              OwningThread;                                   // 0x18
    XDK::HANDLE              LockSemaphore;
    XDK::DWORD               Reserved;
}
RTL_CRITICAL_SECTION, *PRTL_CRITICAL_SECTION;

// ******************************************************************
// * Valid values for the Attributes field
// ******************************************************************
#define OBJ_INHERIT             0x00000002L
#define OBJ_PERMANENT           0x00000010L
#define OBJ_EXCLUSIVE           0x00000020L
#define OBJ_CASE_INSENSITIVE    0x00000040L
#define OBJ_OPENIF              0x00000080L
#define OBJ_OPENLINK            0x00000100L
#define OBJ_KERNEL_HANDLE       0x00000200L
#define OBJ_VALID_ATTRIBUTES    0x000003F2L

// ******************************************************************
// * UNICODE_STRING
// ******************************************************************
typedef struct _UNICODE_STRING
{
	XDK::USHORT Length;
	XDK::USHORT MaximumLength;
#ifdef MIDL_PASS
    [size_is(MaximumLength / 2), length_is((Length) / 2) ] USHORT * Buffer;
#else // MIDL_PASS
	XDK::PWSTR  Buffer;
#endif // MIDL_PASS
}
UNICODE_STRING, *PUNICODE_STRING;

typedef const UNICODE_STRING *PCUNICODE_STRING;

#define UNICODE_NULL ((WCHAR)0) // winnt

// ******************************************************************
// * OBJECT_ATTRIBUTES
// ******************************************************************
typedef struct _OBJECT_ATTRIBUTES
{
    XDK::ULONG           Length;
    XDK::HANDLE          RootDirectory;
    XDK::PUNICODE_STRING ObjectName;
    XDK::ULONG           Attributes;
    XDK::PVOID           SecurityDescriptor;        // Points to type SECURITY_DESCRIPTOR
    XDK::PVOID           SecurityQualityOfService;  // Points to type SECURITY_QUALITY_OF_SERVICE
}
OBJECT_ATTRIBUTES, *POBJECT_ATTRIBUTES;

// ******************************************************************
// * InitializeObjectAttributes
// *
// * VOID
// * InitializeObjectAttributes(
// *     OUT POBJECT_ATTRIBUTES p,
// *     IN PUNICODE_STRING n,
// *     IN XDK::ULONG a,
// *     IN XDK::HANDLE r,
// *     IN PSECURITY_DESCRIPTOR s
// *     )
// *
// ******************************************************************
#define InitializeObjectAttributes( p, n, a, r, s )     \
{                                                       \
    (p)->Length = sizeof( XDK::OBJECT_ATTRIBUTES );   \
    (p)->RootDirectory = r;                             \
    (p)->Attributes = a;                                \
    (p)->ObjectName = n;                                \
    (p)->SecurityDescriptor = s;                        \
    (p)->SecurityQualityOfService = NULL;               \
}

// ******************************************************************
// * IO_STATUS_BLOCK
// ******************************************************************
typedef struct _IO_STATUS_BLOCK
{
    union
    {
        XDK::NTSTATUS Status;
        XDK::PVOID    Pointer;
    }
    u1;

	XDK::ULONG_PTR Information;
}
IO_STATUS_BLOCK, *PIO_STATUS_BLOCK;

// ******************************************************************
// * IO_APC_ROUTINE
// ******************************************************************
typedef VOID (NTAPI *PIO_APC_ROUTINE)
(
    IN XDK::PVOID            ApcContext,
    IN PIO_STATUS_BLOCK IoStatusBlock,
    IN XDK::ULONG            Reserved
);

// ******************************************************************
// * MEMORY_BASIC_INFORMATION
// ******************************************************************
typedef struct _MEMORY_BASIC_INFORMATION
{
    XDK::PVOID   BaseAddress;
    XDK::PVOID   AllocationBase;
    XDK::DWORD   AllocationProtect;
    XDK::SIZE_T  RegionSize;
    XDK::DWORD   State;
    XDK::DWORD   Protect;
    XDK::DWORD   Type;
}
MEMORY_BASIC_INFORMATION, *PMEMORY_BASIC_INFORMATION;

// ******************************************************************
// * MEMORY_INFORMATION_CLASS
// ******************************************************************
typedef enum _MEMORY_INFORMATION_CLASS
{
    MemoryBasicInformation
}
MEMORY_INFORMATION_CLASS;

// ******************************************************************
// * MUTANT_INFORMATION_CLASS
// ******************************************************************
typedef enum _MUTANT_INFORMATION_CLASS {
	MutantBasicInformation
} MUTANT_INFORMATION_CLASS, *PMUTANT_INFORMATION_CLASS;

// ******************************************************************
// * SEMAPHORE_INFORMATION_CLASS
// ******************************************************************
typedef enum _SEMAPHORE_INFORMATION_CLASS {
	SemaphoreBasicInformation
} SEMAPHORE_INFORMATION_CLASS, *PSEMAPHORE_INFORMATION_CLASS;

// ******************************************************************
// * EVENT_TYPE
// ******************************************************************
typedef enum _EVENT_TYPE
{
    NotificationEvent = 0,
    SynchronizationEvent
}
EVENT_TYPE;

// ******************************************************************
// * EVENT_INFORMATION_CLASS
// ******************************************************************
typedef enum _EVENT_INFORMATION_CLASS {
	EventBasicInformation
} EVENT_INFORMATION_CLASS, *PEVENT_INFORMATION_CLASS;

// ******************************************************************
// * TIMER_TYPE
// ******************************************************************
typedef enum _TIMER_TYPE
{
	NotificationTimer,
	SynchronizationTimer
}
TIMER_TYPE;

// ******************************************************************
// * TIMER_INFORMATION_CLASS
// ******************************************************************
typedef enum _TIMER_INFORMATION_CLASS
{
	TimerBasicInformation
}
TIMER_INFORMATION_CLASS, *PTIMER_INFORMATION_CLASS;

// ******************************************************************
// * TIMER_BASIC_INFORMATION
// ******************************************************************
typedef struct _TIMER_BASIC_INFORMATION
{
	XDK::LARGE_INTEGER TimeRemaining;
	XDK::BOOLEAN SignalState;
}
TIMER_BASIC_INFORMATION, *PTIMER_BASIC_INFORMATION;

// ******************************************************************
// * PTIMER_APC_ROUTINE
// ******************************************************************
typedef VOID(NTAPI * PTIMER_APC_ROUTINE)
(
	IN XDK::PVOID TimerContext,
	IN XDK::ULONG TimerLowValue,
	IN XDK::LONG TimerHighValue
);

// ******************************************************************
// * OBJECT_WAIT_TYPE
// ******************************************************************
typedef enum _OBJECT_WAIT_TYPE
{
    WaitAllObject,
    WaitAnyObject
}
OBJECT_WAIT_TYPE;

// ******************************************************************
// * CREATE_FILE_TYPE
// ******************************************************************
typedef enum _CREATE_FILE_TYPE
{
    CreateFileTypeNone,
    CreateFileTypeNamedPipe,
    CreateFileTypeMailslot
}
CREATE_FILE_TYPE;

typedef struct _FILE_FS_LABEL_INFORMATION {
	XDK::ULONG VolumeLabelLength;
	XDK::WCHAR VolumeLabel[1];
} FILE_FS_LABEL_INFORMATION, *PFILE_FS_LABEL_INFORMATION;

typedef struct _FILE_FS_VOLUME_INFORMATION {
	XDK::LARGE_INTEGER VolumeCreationTime;
	XDK::ULONG VolumeSerialNumber;
	XDK::ULONG VolumeLabelLength;
	XDK::BOOLEAN SupportsObjects;
	XDK::WCHAR VolumeLabel[1];
} FILE_FS_VOLUME_INFORMATION, *PFILE_FS_VOLUME_INFORMATION;

typedef struct _FILE_FS_SIZE_INFORMATION {
	XDK::LARGE_INTEGER TotalAllocationUnits;
	XDK::LARGE_INTEGER AvailableAllocationUnits;
	XDK::ULONG SectorsPerAllocationUnit;
	XDK::ULONG BytesPerSector;
} FILE_FS_SIZE_INFORMATION, *PFILE_FS_SIZE_INFORMATION;

typedef struct _FILE_FS_FULL_SIZE_INFORMATION {
	XDK::LARGE_INTEGER TotalAllocationUnits;
	XDK::LARGE_INTEGER CallerAvailableAllocationUnits;
	XDK::LARGE_INTEGER ActualAvailableAllocationUnits;
	XDK::ULONG SectorsPerAllocationUnit;
	XDK::ULONG BytesPerSector;
} FILE_FS_FULL_SIZE_INFORMATION, *PFILE_FS_FULL_SIZE_INFORMATION;

typedef struct _FILE_FS_OBJECTID_INFORMATION {
	XDK::UCHAR ObjectId[16];
	XDK::UCHAR ExtendedInfo[48];
} FILE_FS_OBJECTID_INFORMATION, *PFILE_FS_OBJECTID_INFORMATION;

typedef struct _FILE_FS_DEVICE_INFORMATION {                    
	XDK::DWORD DeviceType;                                     
	XDK::ULONG Characteristics;                                      
} FILE_FS_DEVICE_INFORMATION, *PFILE_FS_DEVICE_INFORMATION;     
															
typedef struct _FILE_FS_ATTRIBUTE_INFORMATION {
	XDK::ULONG FileSystemAttributes;
	XDK::LONG MaximumComponentNameLength;
	XDK::ULONG FileSystemNameLength;
	XDK::WCHAR FileSystemName[1];
} FILE_FS_ATTRIBUTE_INFORMATION, *PFILE_FS_ATTRIBUTE_INFORMATION;

// ******************************************************************
// * FILE_INFORMATION_CLASS
// ******************************************************************
typedef enum _FILE_INFORMATION_CLASS
{
	FileDirectoryInformation=1,
	FileFullDirectoryInformation,
	FileBothDirectoryInformation,
	FileBasicInformation,
	FileStandardInformation,
	FileInternalInformation,
	FileEaInformation,
	FileAccessInformation,
	FileNameInformation,
	FileRenameInformation,
	FileLinkInformation,
	FileNamesInformation,
	FileDispositionInformation,
	FilePositionInformation,
	FileFullEaInformation,
	FileModeInformation,
	FileAlignmentInformation,
	FileAllInformation,
	FileAllocationInformation,
	FileEndOfFileInformation,
	FileAlternateNameInformation,
	FileStreamInformation,
	FilePipeInformation,
	FilePipeLocalInformation,
	FilePipeRemoteInformation,
	FileMailslotQueryInformation,
	FileMailslotSetInformation,
	FileCompressionInformation,
	FileCopyOnWriteInformation,
	FileCompletionInformation,
	FileMoveClusterInformation,
	FileQuotaInformation,
	FileReparsePointInformation,
	FileNetworkOpenInformation,
	FileObjectIdInformation,
	FileTrackingInformation,
	FileOleDirectoryInformation,
	FileContentIndexInformation,
	FileInheritContentIndexInformation,
	FileOleInformation,
	FileMaximumInformation
}
FILE_INFORMATION_CLASS, *PFILE_INFORMATION_CLASS;

// ******************************************************************
// * FS_INFORMATION_CLASS
// ******************************************************************
typedef enum _FS_INFORMATION_CLASS
{
    FileFsVolumeInformation       = 1,
    FileFsLabelInformation,      // 2
    FileFsSizeInformation,       // 3
    FileFsDeviceInformation,     // 4
    FileFsAttributeInformation,  // 5
    FileFsControlInformation,    // 6
    FileFsFullSizeInformation,   // 7
    FileFsObjectIdInformation,   // 8
    FileFsMaximumInformation
}
FS_INFORMATION_CLASS, *PFS_INFORMATION_CLASS;

// ******************************************************************
// * FILE_DIRECTORY_INFORMATION
// ******************************************************************
typedef struct _FILE_DIRECTORY_INFORMATION
{
	XDK::ULONG           NextEntryOffset;
	XDK::ULONG           FileIndex;
	XDK::LARGE_INTEGER   CreationTime;
	XDK::LARGE_INTEGER   LastAccessTime;
	XDK::LARGE_INTEGER   LastWriteTime;
	XDK::LARGE_INTEGER   ChangeTime;
	XDK::LARGE_INTEGER   EndOfFile;
	XDK::LARGE_INTEGER   AllocationSize;
	XDK::ULONG           FileAttributes;
	XDK::ULONG           FileNameLength;
	XDK::WCHAR           FileName[1];        // Offset: 0x40
}
FILE_DIRECTORY_INFORMATION;

// ******************************************************************
// * FILE_RENAME_INFORMATION
// ******************************************************************
typedef struct _FILE_RENAME_INFORMATION
{
	XDK::BOOLEAN         ReplaceIfExists;
	XDK::HANDLE          RootDirectory;
	XDK::ULONG           FileNameLength;
	XDK::WCHAR           FileName[1];
}
FILE_RENAME_INFORMATION, *PFILE_RENAME_INFORMATION;

// ******************************************************************
// * FILE_LINK_INFORMATION
// ******************************************************************
typedef struct _FILE_LINK_INFORMATION {
	XDK::BOOLEAN         ReplaceIfExists;
	XDK::HANDLE          RootDirectory;
	XDK::ULONG           FileNameLength;
	XDK::WCHAR           FileName[1];
} FILE_LINK_INFORMATION, *PFILE_LINK_INFORMATION;

// ******************************************************************
// * FILE_NETWORK_OPEN_INFORMATION
// ******************************************************************
typedef struct _FILE_NETWORK_OPEN_INFORMATION {
	XDK::LARGE_INTEGER   CreationTime;
	XDK::LARGE_INTEGER   LastAccessTime;
	XDK::LARGE_INTEGER   LastWriteTime;
	XDK::LARGE_INTEGER   ChangeTime;
	XDK::LARGE_INTEGER   AllocationSize;
	XDK::LARGE_INTEGER   EndOfFile;
	XDK::ULONG           FileAttributes;
	XDK::ULONG           Reserved;
} FILE_NETWORK_OPEN_INFORMATION, *PFILE_NETWORK_OPEN_INFORMATION;

// ******************************************************************
// * FILE_BASIC_INFORMATION
// ******************************************************************
typedef struct _FILE_BASIC_INFORMATION {
	XDK::LARGE_INTEGER   CreationTime;
	XDK::LARGE_INTEGER   LastAccessTime;
	XDK::LARGE_INTEGER   LastWriteTime;
	XDK::LARGE_INTEGER   ChangeTime;
	XDK::ULONG           FileAttributes;
} FILE_BASIC_INFORMATION, *PFILE_BASIC_INFORMATION;

// ******************************************************************
// * FILE_STANDARD_INFORMATION
// ******************************************************************
typedef struct _FILE_STANDARD_INFORMATION {
	XDK::LARGE_INTEGER   AllocationSize;
	XDK::LARGE_INTEGER   EndOfFile;
	XDK::ULONG           NumberOfLinks;
	XDK::BOOLEAN         DeletePending;
	XDK::BOOLEAN         Directory;
} FILE_STANDARD_INFORMATION, *PFILE_STANDARD_INFORMATION;

// ******************************************************************
// * FILE_INTERNAL_INFORMATION
// ******************************************************************
typedef struct _FILE_INTERNAL_INFORMATION {
	XDK::LARGE_INTEGER   IndexNumber;
} FILE_INTERNAL_INFORMATION, *PFILE_INTERNAL_INFORMATION;

// ******************************************************************
// * FILE_EA_INFORMATION
// ******************************************************************
typedef struct _FILE_EA_INFORMATION {
	XDK::ULONG           EaSize;
} FILE_EA_INFORMATION, *PFILE_EA_INFORMATION;

// ******************************************************************
// * FILE_ACCESS_INFORMATION
// ******************************************************************
typedef struct _FILE_ACCESS_INFORMATION {
	XDK::ACCESS_MASK     AccessFlags;
} FILE_ACCESS_INFORMATION, *PFILE_ACCESS_INFORMATION;

// ******************************************************************
// * FILE_POSITION_INFORMATION
// ******************************************************************
typedef struct _FILE_POSITION_INFORMATION {
	XDK::LARGE_INTEGER   CurrentByteOffset;
} FILE_POSITION_INFORMATION, *PFILE_POSITION_INFORMATION;

// ******************************************************************
// * FILE_MODE_INFORMATION
// ******************************************************************
typedef struct _FILE_MODE_INFORMATION {
	XDK::ULONG           Mode;
} FILE_MODE_INFORMATION, *PFILE_MODE_INFORMATION;

// ******************************************************************
// * FILE_ALIGNMENT_INFORMATION
// ******************************************************************
typedef struct _FILE_ALIGNMENT_INFORMATION {
	XDK::ULONG           AlignmentRequirement;
} FILE_ALIGNMENT_INFORMATION, *PFILE_ALIGNMENT_INFORMATION;

// ******************************************************************
// * FILE_NAME_INFORMATION
// ******************************************************************
typedef struct _FILE_NAME_INFORMATION {
	XDK::ULONG           FileNameLength;
	XDK::WCHAR           FileName[1];
} FILE_NAME_INFORMATION, *PFILE_NAME_INFORMATION;

// ******************************************************************
// * FILE_ALL_INFORMATION
// ******************************************************************
typedef struct _FILE_ALL_INFORMATION {
	XDK::FILE_BASIC_INFORMATION     BasicInformation;
	XDK::FILE_STANDARD_INFORMATION  StandardInformation;
	XDK::FILE_INTERNAL_INFORMATION  InternalInformation;
	XDK::FILE_EA_INFORMATION        EaInformation;
	XDK::FILE_ACCESS_INFORMATION    AccessInformation;
	XDK::FILE_POSITION_INFORMATION  PositionInformation;
	XDK::FILE_MODE_INFORMATION      ModeInformation;
	XDK::FILE_ALIGNMENT_INFORMATION AlignmentInformation;
	XDK::FILE_NAME_INFORMATION      NameInformation;
} FILE_ALL_INFORMATION, *PFILE_ALL_INFORMATION;

// ******************************************************************
// * TIME_FIELDS
// ******************************************************************
typedef struct _TIME_FIELDS
{
    XDK::USHORT  Year;
    XDK::USHORT  Month;
    XDK::USHORT  Day;
    XDK::USHORT  Hour;
    XDK::USHORT  Minute;
    XDK::USHORT  Second;
    XDK::USHORT  Millisecond;
    XDK::USHORT  Weekday;
}
TIME_FIELDS, *PTIME_FIELDS;

// ******************************************************************
// * KSYSTEM_TIME
// ******************************************************************
typedef struct _KSYSTEM_TIME {
	XDK::ULONG LowPart;
	XDK::LONG High1Time;
	XDK::LONG High2Time;
} KSYSTEM_TIME, *PKSYSTEM_TIME;

// ******************************************************************
// * KUSER_SHARED_DATA
// ******************************************************************
typedef struct _KUSER_SHARED_DATA
{
	/* Current low 32-bit of tick count and tick count multiplier.
	* N.B. The tick count is updated each time the clock ticks.
	*/
	XDK::ULONG VOLATILE TickCountLow;
	Native::UINT32 TickCountMultiplier;

	/* Current 64-bit interrupt time in 100ns units. */
	XDK::KSYSTEM_TIME VOLATILE InterruptTime;

	/* Current 64-bit system time in 100ns units. */
	XDK::KSYSTEM_TIME VOLATILE SystemTime;

	/* Current 64-bit time zone bias. */
	XDK::KSYSTEM_TIME VOLATILE TimeZoneBias;
}
KUSER_SHARED_DATA, *PKUSER_SHARED_DATA;
// This is only the top of the actual definition. For the complete version,
// see http://processhacker.sourceforge.net/doc/ntexapi_8h_source.html

// ******************************************************************
// * GENERIC_MAPPING
// ******************************************************************
typedef struct _GENERIC_MAPPING
{
	XDK::ACCESS_MASK GenericRead;
	XDK::ACCESS_MASK GenericWrite;
	XDK::ACCESS_MASK GenericExecute;
	XDK::ACCESS_MASK GenericAll;
}
GENERIC_MAPPING, *PGENERIC_MAPPING;

// ******************************************************************
// * KeDelayExecutionThread
// ******************************************************************
typedef XDK::NTSTATUS (NTAPI *FPTR_KeDelayExecutionThread)
(
    IN XDK::KPROCESSOR_MODE  WaitMode,
    IN XDK::BOOLEAN          Alertable,
    IN XDK::PLARGE_INTEGER   Interval
);

// ******************************************************************
// * KeQuerySystemTime
// ******************************************************************
typedef VOID (NTAPI *FPTR_KeQuerySystemTime)
(
	XDK::PLARGE_INTEGER CurrentTime
);

// ******************************************************************
// * RtlCreateHeap
// ******************************************************************
typedef XDK::PVOID (NTAPI *FPTR_RtlCreateHeap)
(
    IN XDK::ULONG   Flags,
    IN XDK::PVOID   Base OPTIONAL,
    IN XDK::ULONG   Reserve OPTIONAL,
    IN XDK::ULONG   Commit,
    IN XDK::PVOID   Lock OPTIONAL,
    IN XDK::PVOID   RtlHeapParams OPTIONAL
);

// ******************************************************************
// * RtlAllocateHeap
// ******************************************************************
typedef XDK::PVOID (NTAPI *FPTR_RtlAllocateHeap)
(
    IN XDK::HANDLE hHeap,
    IN XDK::DWORD  dwFlags,
    IN XDK::SIZE_T dwBytes
);

// ******************************************************************
// * RtlFreeHeap
// ******************************************************************
typedef XDK::BOOL (NTAPI *FPTR_RtlFreeHeap)
(
    IN XDK::HANDLE hHeap,
    IN XDK::DWORD  dwFlags,
    IN XDK::PVOID  lpMem
);

// ******************************************************************
// * RtlReAllocateHeap
// ******************************************************************
typedef XDK::PVOID (NTAPI *FPTR_RtlReAllocateHeap)
(
    IN XDK::HANDLE hHeap,
    IN XDK::DWORD  dwFlags,
    IN XDK::PVOID  lpMem,
    IN XDK::DWORD  dwBytes
);

// ******************************************************************
// * RtlSizeHeap
// ******************************************************************
typedef XDK::SIZE_T (NTAPI *FPTR_RtlSizeHeap)
(
    IN XDK::HANDLE hHeap,
    IN XDK::DWORD  dwFlags,
    IN XDK::PVOID  lpMem
);

// ******************************************************************
// * RtlMapGenericMask
// ******************************************************************
typedef VOID (NTAPI *FPTR_RtlMapGenericMask)
(
	IN OUT XDK::PACCESS_MASK     AccessMask,
	IN     XDK::PGENERIC_MAPPING GenericMapping
);

// ******************************************************************
// * RtlNtStatusToDosError
// ******************************************************************
typedef XDK::ULONG (NTAPI *FPTR_RtlNtStatusToDosError)
(
    IN XDK::NTSTATUS Status
);

// ******************************************************************
// * RtlTimeToTimeFields
// ******************************************************************
typedef XDK::BOOLEAN (NTAPI *FPTR_RtlTimeFieldsToTime)
(
    IN  XDK::PTIME_FIELDS    TimeFields,
    OUT XDK::PLARGE_INTEGER  Time
);

// ******************************************************************
// * RtlTimeToTimeFields
// ******************************************************************
typedef VOID (NTAPI *FPTR_RtlTimeToTimeFields)
(
    IN  XDK::PLARGE_INTEGER  Time,
    OUT PTIME_FIELDS    TimeFields
);

// ******************************************************************
// * RtlTryEnterCriticalSection
// ******************************************************************
typedef XDK::BOOL (NTAPI *FPTR_RtlTryEnterCriticalSection)
(
    IN XDK::PRTL_CRITICAL_SECTION CriticalSection
);

// ******************************************************************
// * RtlUlongByteSwap
// ******************************************************************
typedef XDK::ULONG (FASTCALL *FPTR_RtlUlongByteSwap)
(
	IN XDK::ULONG Source
);

// ******************************************************************
// * RtlUnicodeStringToInteger
// ******************************************************************
typedef XDK::NTSTATUS (NTAPI *FPTR_RtlUnicodeStringToInteger)
(
	IN  PCUNICODE_STRING String,
	IN  XDK::ULONG            Base OPTIONAL,
	OUT XDK::PULONG           Value
);

// ******************************************************************
// * RtlUnicodeToMultiByteN
// ******************************************************************
typedef XDK::NTSTATUS (NTAPI *FPTR_RtlUnicodeToMultiByteN)
(
	OUT XDK::PCHAR  MultiByteString,
	IN  XDK::ULONG  MaxBytesInMultiByteString,
	OUT XDK::PULONG BytesInMultiByteString OPTIONAL,
	IN  Native::PCWCH  UnicodeString,
	IN  XDK::ULONG  BytesInUnicodeString
);

// ******************************************************************
// * RtlUnicodeToMultiByteSize
// ******************************************************************
typedef XDK::NTSTATUS (NTAPI *FPTR_RtlUnicodeToMultiByteSize)
(
	OUT XDK::PULONG BytesInMultiByteString,
	IN  Native::PWCH   UnicodeString,
	IN  XDK::ULONG  BytesInUnicodeString
);

// ******************************************************************
// * RtlUpcaseUnicodeChar
// ******************************************************************
typedef XDK::WCHAR (NTAPI *FPTR_RtlUpcaseUnicodeChar)
(
	IN XDK::WCHAR    SourceCharacter
);

// ******************************************************************
// * RtlUpcaseUnicodeString
// ******************************************************************
typedef XDK::NTSTATUS (NTAPI *FPTR_RtlUpcaseUnicodeString)
(
	OUT XDK::PUNICODE_STRING DestinationString,
	IN XDK::PUNICODE_STRING SourceString,
	IN XDK::BOOLEAN AllocateDestinationString
);

// ******************************************************************
// * RtlUpcaseUnicodeToMultiByteN
// ******************************************************************
typedef XDK::NTSTATUS (NTAPI *FPTR_RtlUpcaseUnicodeToMultiByteN)
(
	OUT XDK::PCHAR  MultiByteString,
	IN  XDK::ULONG  MaxBytesInMultiByteString,
	OUT XDK::PULONG BytesInMultiByteString OPTIONAL,
	IN  Native::PCWCH  UnicodeString,
	IN  XDK::ULONG  BytesInUnicodeString
);

// ******************************************************************
// * RtlUpperString
// ******************************************************************
typedef VOID (NTAPI *FPTR_RtlUpperString)
(
	OUT PSTRING DestinationString,
	IN const STRING  *SourceString
);

// ******************************************************************
// * RtlUshortByteSwap
// ******************************************************************
typedef XDK::USHORT (FASTCALL *FPTR_RtlUshortByteSwap)
(
	IN XDK::USHORT Source
);

// ******************************************************************
// * RtlCompareMemory
// ******************************************************************
typedef XDK::BOOL (NTAPI *FPTR_RtlCompareMemory)
(
	IN const VOID   *Source1,
	IN const VOID   *Source2,
	IN       XDK::SIZE_T Length
);

// ******************************************************************
// * RtlCompareMemoryUlong
// ******************************************************************
typedef XDK::BOOL(NTAPI *FPTR_RtlCompareMemoryUlong)
(
	IN 	VOID *Source,
	IN XDK::SIZE_T Length,
	IN XDK::ULONG Pattern
);

// ******************************************************************
// * RtlInitAnsiString
// ******************************************************************
typedef VOID (NTAPI *FPTR_RtlInitAnsiString)
(
  IN OUT PANSI_STRING   DestinationString,
  IN     XDK::PCSZ           SourceString
);

// ******************************************************************
// * RtlInitUnicodeString
// ******************************************************************
typedef VOID (NTAPI *FPTR_RtlInitUnicodeString)
(
  IN OUT PUNICODE_STRING DestinationString,
  IN     XDK::PCWSTR          SourceString
);

// ******************************************************************
// * RtlAnsiStringToUnicodeString
// ******************************************************************
typedef XDK::NTSTATUS (NTAPI *FPTR_RtlAnsiStringToUnicodeString)
(
    IN OUT PUNICODE_STRING DestinationString,
    IN     PANSI_STRING    SourceString,
    IN     XDK::BOOLEAN         AllocateDestinationString
);

// ******************************************************************
// * RtlAppendStringToString
// ******************************************************************
typedef XDK::NTSTATUS(NTAPI *FPTR_RtlAppendStringToString)
(
	IN OUT PSTRING    Destination,
	IN     PSTRING    Source
);

// ******************************************************************
// * RtlAppendUnicodeStringToString
// ******************************************************************
typedef XDK::NTSTATUS(NTAPI *FPTR_RtlAppendUnicodeStringToString)
(
	IN OUT PUNICODE_STRING  Destination,
	IN     PUNICODE_STRING  Source
);

// ******************************************************************
// * RtlAppendUnicodeToString
// ******************************************************************
typedef XDK::NTSTATUS(NTAPI *FPTR_RtlAppendUnicodeToString)
(
	IN OUT PUNICODE_STRING  Destination,
	IN     XDK::PCWSTR  Source
);

// ******************************************************************
// * RtlCharToInteger
// ******************************************************************
typedef XDK::NTSTATUS(NTAPI *FPTR_RtlCharToInteger)
(
	IN     XDK::PCSZ   String,
	IN     XDK::ULONG  Base OPTIONAL,
	OUT    XDK::PULONG Value
);

// ******************************************************************
// * RtlCompareString
// ******************************************************************
typedef XDK::LONG (NTAPI *FPTR_RtlCompareString)
(
	IN	const STRING  *String1,
	IN	const STRING  *String2,
	IN	      XDK::BOOLEAN CaseInSensitive
);

// ******************************************************************
// * RtlCompareUnicodeString
// ******************************************************************
typedef XDK::LONG (NTAPI *FPTR_RtlCompareUnicodeString)
(
	IN	PCUNICODE_STRING  String1,
	IN	PCUNICODE_STRING  String2,
	IN	XDK::BOOLEAN CaseInSensitive
);

// ******************************************************************
// * RtlCopyString
// ******************************************************************
typedef XDK::LONG (NTAPI *FPTR_RtlCopyString)
(
	OUT PSTRING DestinationString,
	IN const STRING  *SourceString OPTIONAL
);

// ******************************************************************
// * RtlCopyUnicodeString
// ******************************************************************
typedef XDK::LONG (NTAPI *FPTR_RtlCopyUnicodeString)
(
	OUT PUNICODE_STRING DestinationString,
	IN PUNICODE_STRING SourceString OPTIONAL
);

// ******************************************************************
// * RtlCreateUnicodeString
// ******************************************************************
typedef XDK::BOOLEAN (NTAPI *FPTR_RtlCreateUnicodeString)
(
	OUT PUNICODE_STRING DestinationString,
	IN XDK::PCWSTR           SourceString
);

// ******************************************************************
// * RtlDowncaseUnicodeChar
// ******************************************************************
typedef XDK::WCHAR (NTAPI *FPTR_RtlDowncaseUnicodeChar)
(
	IN XDK::WCHAR    SourceCharacter
);

// ******************************************************************
// * RtlDowncaseUnicodeString
// ******************************************************************
typedef XDK::NTSTATUS (NTAPI *FPTR_RtlDowncaseUnicodeString)
(
	OUT PUNICODE_STRING DestinationString,
	IN PUNICODE_STRING SourceString,
	IN XDK::BOOLEAN AllocateDestinationString
);

// ******************************************************************
// * RtlUnicodeStringToAnsiString
// ******************************************************************
typedef XDK::NTSTATUS (NTAPI *FPTR_RtlUnicodeStringToAnsiString)
(
    IN OUT PANSI_STRING    DestinationString,
    IN     PUNICODE_STRING SourceString,
    IN     XDK::BOOLEAN         AllocateDestinationString
);

// ******************************************************************
// * RtlFreeAnsiString
// ******************************************************************
typedef VOID (NTAPI *FPTR_RtlFreeAnsiString)
(
  IN OUT PANSI_STRING   AnsiString
);

// ******************************************************************
// * RtlFreeUnicodeString
// ******************************************************************
typedef VOID(NTAPI *FPTR_RtlFreeUnicodeString)
(
	IN OUT PUNICODE_STRING   UnicodeString
);

// ******************************************************************
// * RtlEqualString
// ******************************************************************
typedef XDK::BOOLEAN (NTAPI *FPTR_RtlEqualString)
(
	IN PSTRING			String1,
	IN PSTRING			String2,
	IN XDK::BOOLEAN			CaseSensitive
);

// ******************************************************************
// * RtlEqualUnicodeString
// ******************************************************************
typedef XDK::BOOLEAN (NTAPI *FPTR_RtlEqualUnicodeString)
(
	IN PUNICODE_STRING String1,
	IN PUNICODE_STRING String2,
	IN XDK::BOOLEAN CaseSensitive
);

// ******************************************************************
// * RtlExtendedIntegerMultiply
// ******************************************************************
typedef XDK::LARGE_INTEGER (NTAPI *FPTR_RtlExtendedIntegerMultiply)
(
	IN XDK::LARGE_INTEGER Multiplicand,
	IN XDK::LONG          Multiplier
);

// ******************************************************************
// * RtlExtendedLargeIntegerDivide
// ******************************************************************
typedef XDK::LARGE_INTEGER (NTAPI *FPTR_RtlExtendedLargeIntegerDivide)
(
	IN XDK::LARGE_INTEGER Dividend,
	IN XDK::ULONG         Divisor,
	OUT XDK::PULONG		 Remainder
);

// ******************************************************************
// * RtlExtendedMagicDivide
// ******************************************************************
typedef XDK::LARGE_INTEGER (NTAPI *FPTR_RtlExtendedMagicDivide)
(
	IN XDK::LARGE_INTEGER Dividend,
	IN XDK::LARGE_INTEGER MagicDivisor,
	IN XDK::CCHAR		 ShiftCount
);

// ******************************************************************
// * RtlFillMemory
// ******************************************************************
typedef VOID (NTAPI *FPTR_RtlFillMemory)
(
	OUT VOID UNALIGNED *Destination,
	IN  XDK::SIZE_T         Length,
	IN	XDK::UCHAR          Fill
);

// ******************************************************************
// * RtlFillMemoryUlong
// ******************************************************************
typedef VOID (NTAPI *FPTR_RtlFillMemoryUlong)
(
	OUT XDK::PVOID  Destination,
	IN  XDK::SIZE_T Length,
	IN	XDK::ULONG  Pattern
); 

// ******************************************************************
// * RtlIntegerToChar
// ******************************************************************
typedef XDK::NTSTATUS (NTAPI *FPTR_RtlIntegerToChar)
(
	IN XDK::ULONG Value,
	IN XDK::ULONG Base,
	IN XDK::ULONG Length,
	IN XDK::PCHAR Str
);

// ******************************************************************
// * RtlIntegerToUnicodeString
// ******************************************************************
typedef XDK::NTSTATUS (NTAPI *FPTR_RtlIntegerToUnicodeString)
(
	IN	XDK::ULONG           Value,
	IN	XDK::ULONG           Base OPTIONAL,
	IN OUT	PUNICODE_STRING String
);

// ******************************************************************
// * RtlMultiByteToUnicodeN
// ******************************************************************
typedef XDK::NTSTATUS (NTAPI *FPTR_RtlMultiByteToUnicodeN)
(
	OUT Native::PWCH   UnicodeString,
	IN  XDK::ULONG  MaxBytesInUnicodeString,
	OUT XDK::PULONG BytesInUnicodeString OPTIONAL,
	IN  const XDK::CHAR   *MultiByteString,
	IN  XDK::ULONG  BytesInMultiByteString
);

// ******************************************************************
// * RtlMultiByteToUnicodeSize
// ******************************************************************
typedef XDK::NTSTATUS (NTAPI *FPTR_RtlMultiByteToUnicodeSize)
(
	OUT       XDK::PULONG BytesInUnicodeString,
	IN  const XDK::CHAR   *MultiByteString,
	IN        XDK::ULONG  BytesInMultiByteString
);

// ******************************************************************
// * NtDelayExecution
// ******************************************************************
typedef XDK::NTSTATUS (NTAPI *FPTR_NtDelayExecution)
(
    IN XDK::BOOLEAN          Alertable,
    IN XDK::PLARGE_INTEGER   DelayInterval
);

// ******************************************************************
// * NtDuplicateObject
// ******************************************************************
typedef XDK::NTSTATUS (NTAPI *FPTR_NtDuplicateObject)
(
    IN  XDK::HANDLE          SourceProcessHandle,
    IN  XDK::HANDLE          SourceHandle,
    IN  XDK::HANDLE          TargetProcessHandle,
    OUT XDK::PHANDLE         TargetHandle,
    IN  XDK::ACCESS_MASK     DesiredAccess OPTIONAL,
    IN  XDK::ULONG           Attributes,
    IN  XDK::ULONG           Options
);

#define DUPLICATE_SAME_ATTRIBUTES   0x00000004

// ******************************************************************
// * NtFlushBuffersFile
// ******************************************************************
typedef XDK::NTSTATUS (NTAPI *FPTR_NtFlushBuffersFile)
(
    IN  XDK::HANDLE              FileHandle,
    OUT XDK::PIO_STATUS_BLOCK    IoStatusBlock
);

// ******************************************************************
// * NtSetLdtEntries
// ******************************************************************
typedef XDK::NTSTATUS (NTAPI *FPTR_NtSetLdtEntries)
(
    IN XDK::USHORT       Selector1,
    IN LDT_ENTRY    Descriptor1,
    IN XDK::USHORT       Selector2,
    IN LDT_ENTRY    Descriptor2
);

// ******************************************************************
// * NtAllocateVirtualMemory
// ******************************************************************
typedef XDK::NTSTATUS (NTAPI *FPTR_NtAllocateVirtualMemory)
(
    IN XDK::HANDLE               ProcessHandle,
    IN OUT XDK::PVOID            *BaseAddress,
    IN XDK::ULONG                ZeroBits,
    IN OUT XDK::PULONG           RegionSize,
    IN XDK::ULONG                AllocationType,
    IN XDK::ULONG                Protect
);

// ******************************************************************
// * NtFreeVirtualMemory
// ******************************************************************
typedef XDK::NTSTATUS (NTAPI *FPTR_NtFreeVirtualMemory)
(
    IN XDK::HANDLE               ProcessHandle,
    IN XDK::PVOID               *BaseAddress,
    IN OUT XDK::PULONG           RegionSize,
    IN XDK::ULONG                FreeType
);

// ******************************************************************
// * NtQueryVirtualMemory
// ******************************************************************
typedef XDK::NTSTATUS (NTAPI *FPTR_NtQueryVirtualMemory)
(
    IN  XDK::HANDLE                      ProcessHandle,
    IN  XDK::PVOID                       BaseAddress,
    IN  MEMORY_INFORMATION_CLASS    MemoryInformationClass,
    OUT XDK::PVOID                       Buffer,
    IN  XDK::ULONG                       Length,
    OUT XDK::PULONG                      ResultLength OPTIONAL
);

// ******************************************************************
// * RtlInitializeCriticalSection
// ******************************************************************
typedef VOID (NTAPI *FPTR_RtlInitializeCriticalSection)
(
    IN PRTL_CRITICAL_SECTION CriticalSection
);

// ******************************************************************
// * RtlEnterCriticalSection
// ******************************************************************
typedef VOID (NTAPI *FPTR_RtlEnterCriticalSection)
(
    IN PRTL_CRITICAL_SECTION CriticalSection
);

// ******************************************************************
// * RtlLeaveCriticalSection
// ******************************************************************
typedef VOID (NTAPI *FPTR_RtlLeaveCriticalSection)
(
    IN PRTL_CRITICAL_SECTION CriticalSection
);

// ******************************************************************
// * NtWaitForSingleObject
// ******************************************************************
typedef XDK::NTSTATUS (NTAPI *FPTR_NtWaitForSingleObject)
(
    IN XDK::HANDLE               ObjectHandle,
    IN XDK::BOOLEAN              Alertable,
    IN XDK::PLARGE_INTEGER       TimeOut
);

// ******************************************************************
// * NtWaitForMultipleObjects
// ******************************************************************
typedef XDK::NTSTATUS (NTAPI *FPTR_NtWaitForMultipleObjects)
(
    IN XDK::ULONG                ObjectCount,
    IN XDK::PHANDLE              ObjectsArray,
    IN OBJECT_WAIT_TYPE     WaitType,
    IN XDK::BOOLEAN              Alertable,
    IN XDK::PLARGE_INTEGER       TimeOut OPTIONAL
);

// ******************************************************************
// * NtCreateEvent
// ******************************************************************
typedef XDK::NTSTATUS (NTAPI *FPTR_NtCreateEvent)
(
    OUT XDK::PHANDLE             EventHandle,
    IN  XDK::ACCESS_MASK         DesiredAccess,
	IN  XDK::POBJECT_ATTRIBUTES  ObjectAttributes OPTIONAL,
    IN  XDK::EVENT_TYPE          EventType,
    IN  XDK::BOOLEAN             InitialState
);

// ******************************************************************
// * NtQueryEvent
// ******************************************************************
typedef XDK::NTSTATUS (NTAPI *FPTR_NtQueryEvent)
(
	IN XDK::HANDLE EventHandle,
	IN EVENT_INFORMATION_CLASS EventInformationClass,
	OUT XDK::PVOID EventInformation,
	IN XDK::ULONG EventInformationLength,
	OUT XDK::PULONG ReturnLength OPTIONAL
);

// ******************************************************************
// * NtPulseEvent
// ******************************************************************
typedef XDK::NTSTATUS(NTAPI *FPTR_NtPulseEvent)
(
	IN XDK::HANDLE	EventHandle,
	OUT XDK::PLONG	PreviousState OPTIONAL
);

// ******************************************************************
// * NtCreateMutant
// ******************************************************************
typedef XDK::NTSTATUS (NTAPI *FPTR_NtCreateMutant)
(
    OUT XDK::PHANDLE             MutantHandle,
    IN  XDK::ACCESS_MASK         DesiredAccess,
    IN  XDK::POBJECT_ATTRIBUTES  ObjectAttributes OPTIONAL,
    IN  XDK::BOOLEAN             InitialOwner
);

// ******************************************************************
// * NtQueryMutant
// ******************************************************************
typedef XDK::NTSTATUS (NTAPI *FPTR_NtQueryMutant)
(
	IN XDK::HANDLE MutantHandle,
	IN MUTANT_INFORMATION_CLASS MutantInformationClass,
	OUT XDK::PVOID MutantInformation,
	IN XDK::ULONG MutantInformationLength,
	OUT XDK::PULONG ReturnLength OPTIONAL
);

// ******************************************************************
// * NtReleaseMutant
// ******************************************************************
typedef XDK::NTSTATUS (NTAPI *FPTR_NtReleaseMutant)
(
    IN  XDK::HANDLE              MutantHandle,
    OUT XDK::PLONG               PreviousCount OPTIONAL
);

// ******************************************************************
// * NtCreateSemaphore
// ******************************************************************
typedef XDK::NTSTATUS (NTAPI *FPTR_NtCreateSemaphore)
(
    OUT XDK::PHANDLE             SemaphoreHandle,
    IN  XDK::ACCESS_MASK         DesiredAccess,
    IN  POBJECT_ATTRIBUTES  ObjectAttributes OPTIONAL,
    IN  XDK::ULONG               InitialCount,
    IN  XDK::ULONG               MaximumCount
);

// ******************************************************************
// * NtQuerySemaphore
// ******************************************************************
typedef XDK::NTSTATUS(NTAPI *FPTR_NtQuerySemaphore)
(
	IN XDK::HANDLE SemaphoreHandle,
	IN SEMAPHORE_INFORMATION_CLASS SemaphoreInformationClass,
	OUT XDK::PVOID SemaphoreInformation,
	IN XDK::ULONG SemaphoreInformationLength,
	OUT XDK::PULONG ReturnLength OPTIONAL
);

// ******************************************************************
// * NtReleaseSemaphore
// ******************************************************************
typedef XDK::NTSTATUS (NTAPI *FPTR_NtReleaseSemaphore)
(
    IN  XDK::HANDLE              SemaphoreHandle,
    IN  XDK::ULONG               ReleaseCount,
    OUT XDK::PULONG              PreviousCount OPTIONAL
);

// ******************************************************************
// * NtCreateTimer
// ******************************************************************
typedef XDK::NTSTATUS(NTAPI *FPTR_NtCreateTimer)
(
	OUT XDK::PHANDLE             TimerHandle,
	IN  XDK::ACCESS_MASK         DesiredAccess,
	IN  POBJECT_ATTRIBUTES  ObjectAttributes OPTIONAL,
	IN  TIMER_TYPE          TimerType
);

// ******************************************************************
// * SetTimer
// ******************************************************************
typedef XDK::NTSTATUS(NTAPI *FPTR_NtSetTimer)
(
	IN XDK::HANDLE 	TimerHandle,
	IN XDK::PLARGE_INTEGER 	DueTime,
	IN PTIMER_APC_ROUTINE TimerApcRoutine 	OPTIONAL,
	IN XDK::PVOID TimerContext 	OPTIONAL,
	IN XDK::BOOLEAN 	WakeTimer,
	IN XDK::LONG Period 	OPTIONAL,
	OUT XDK::PBOOLEAN PreviousState 	OPTIONAL
);

// ******************************************************************
// * QueryTimer
// ******************************************************************
typedef XDK::NTSTATUS(NTAPI *FPTR_NtQueryTimer)
(
	IN XDK::HANDLE 	TimerHandle,
	IN TIMER_INFORMATION_CLASS 	TimerInformationClass,
	OUT XDK::PVOID 	TimerInformation,
	IN XDK::ULONG 	TimerInformationLength,
	OUT XDK::PULONG ReturnLength 	OPTIONAL
);

// ******************************************************************
// * NtCancelTimer
// ******************************************************************
typedef XDK::NTSTATUS(NTAPI *FPTR_NtCancelTimer)
(
	IN XDK::HANDLE TimerHandle,
	OUT XDK::PBOOLEAN CurrentState OPTIONAL
);

// ******************************************************************
// * NtCreateFile
// ******************************************************************
typedef XDK::NTSTATUS (NTAPI *FPTR_NtCreateFile)
(
    OUT XDK::PHANDLE             FileHandle,
    IN  XDK::ACCESS_MASK         DesiredAccess,
    IN  POBJECT_ATTRIBUTES  ObjectAttributes,
    OUT PIO_STATUS_BLOCK    IoStatusBlock,
    IN  XDK::PLARGE_INTEGER      AllocationSize OPTIONAL,
    IN  XDK::ULONG               FileAttributes,
    IN  XDK::ULONG               ShareAccess,
    IN  XDK::ULONG               CreateDisposition,
    IN  XDK::ULONG               CreateOptions,
    IN  XDK::PVOID               EaBuffer OPTIONAL,
    IN  XDK::ULONG               EaLength
);

// ******************************************************************
// * NtDeleteFile
// ******************************************************************
typedef XDK::NTSTATUS(NTAPI *FPTR_NtDeleteFile)
(
	IN  POBJECT_ATTRIBUTES  ObjectAttributes
);

// ******************************************************************
// * NtCreateDirectoryObject
// ******************************************************************
typedef XDK::NTSTATUS(NTAPI *FPTR_NtCreateDirectoryObject)
(
	OUT XDK::PHANDLE            DirectoryHandle,
	IN  XDK::ACCESS_MASK        DesiredAccess,
	IN  POBJECT_ATTRIBUTES ObjectAttributes
);


// ******************************************************************
// * NtClearEvent
// ******************************************************************
typedef XDK::NTSTATUS (NTAPI *FPTR_NtClearEvent)
(
    IN XDK::HANDLE               EventHandle
);

// ******************************************************************
// * NtClose
// ******************************************************************
typedef XDK::NTSTATUS (NTAPI *FPTR_NtClose)
(
    IN XDK::HANDLE               Handle
);

// ******************************************************************
// * NtOpenFile
// ******************************************************************
typedef XDK::NTSTATUS (NTAPI *FPTR_NtOpenFile)
(
    OUT XDK::PHANDLE             FileHandle,
    IN  XDK::ACCESS_MASK         DesiredAccess,
    IN  POBJECT_ATTRIBUTES  ObjectAttributes,
    OUT PIO_STATUS_BLOCK    IoStatusBlock,
    IN  XDK::ULONG               ShareAccess,
    IN  XDK::ULONG               OpenOptions
);

// ******************************************************************
// * NtReadFile
// ******************************************************************
typedef XDK::NTSTATUS (NTAPI *FPTR_NtReadFile)
(
    IN  XDK::HANDLE          FileHandle,            // TODO: correct paramters
    IN  XDK::HANDLE          Event OPTIONAL,
    IN  XDK::PVOID           ApcRoutine OPTIONAL,
    IN  XDK::PVOID           ApcContext,
    OUT XDK::PVOID           IoStatusBlock,
    OUT XDK::PVOID           Buffer,
    IN  XDK::ULONG           Length,
    IN  XDK::PLARGE_INTEGER  ByteOffset OPTIONAL,
    IN  XDK::PULONG          Key OPTIONAL
);

// ******************************************************************
// * NtWriteFile
// ******************************************************************
typedef XDK::NTSTATUS (NTAPI *FPTR_NtWriteFile)
(
    IN  XDK::HANDLE          FileHandle,            // TODO: correct paramters
    IN  XDK::HANDLE          Event OPTIONAL,
    IN  XDK::PVOID           ApcRoutine OPTIONAL,
    IN  XDK::PVOID           ApcContext,
    OUT XDK::PVOID           IoStatusBlock,
    OUT XDK::PVOID           Buffer,
    IN  XDK::ULONG           Length,
    IN  XDK::PLARGE_INTEGER  ByteOffset OPTIONAL,
    IN  XDK::PULONG          Key OPTIONAL
);

// ******************************************************************
// * NtYieldExecution
// ******************************************************************
typedef VOID (NTAPI *FPTR_NtYieldExecution)();

// ******************************************************************
// * NtSetInformationFile
// ******************************************************************
typedef XDK::NTSTATUS (NTAPI *FPTR_NtSetInformationFile)
(
    IN  XDK::HANDLE  FileHandle,            // TODO: correct paramters
    OUT XDK::PVOID   IoStatusBlock,
    IN  XDK::PVOID   FileInformation,
    IN  XDK::ULONG   Length,
    IN  XDK::ULONG   FileInformationClass
);

// ******************************************************************
// * NtSuspendThread
// ******************************************************************
typedef XDK::NTSTATUS (NTAPI *FPTR_NtSuspendThread)
(
    IN  XDK::HANDLE  ThreadHandle,
    OUT XDK::PULONG  PreviousSuspendCount OPTIONAL
);

// ******************************************************************
// * NtResumeThread
// ******************************************************************
typedef XDK::NTSTATUS (NTAPI *FPTR_NtResumeThread)
(
    IN  XDK::HANDLE  ThreadHandle,
    OUT XDK::PULONG  SuspendCount OPTIONAL
);

// ******************************************************************
// * NtSetEvent
// ******************************************************************
typedef XDK::NTSTATUS (NTAPI *FPTR_NtSetEvent)
(
    IN  XDK::HANDLE  EventHandle,
    OUT XDK::PLONG   PreviousState OPTIONAL
);

// ******************************************************************
// * NtQueryDirectoryFile
// ******************************************************************
typedef XDK::NTSTATUS (NTAPI *FPTR_NtQueryDirectoryFile)
(
    IN  XDK::HANDLE                  FileHandle,
    IN  XDK::HANDLE                  Event OPTIONAL,
    IN  PIO_APC_ROUTINE         ApcRoutine OPTIONAL,
    IN  XDK::PVOID                   ApcContext OPTIONAL,
    OUT PIO_STATUS_BLOCK        IoStatusBlock,
    OUT XDK::PVOID                   FileInformation,
    IN  XDK::ULONG                   Length,
    IN  FILE_INFORMATION_CLASS  FileInformationClass,
    IN  XDK::BOOLEAN                 ReturnSingleEntry,
    IN  PUNICODE_STRING         FileMask OPTIONAL,
    IN  XDK::BOOLEAN                 RestartScan
);

// ******************************************************************
// * NtQueryFullAttributesFile
// ******************************************************************
typedef XDK::NTSTATUS (NTAPI *FPTR_NtQueryFullAttributesFile)
(
    IN  POBJECT_ATTRIBUTES          ObjectAttributes,
    OUT PFILE_NETWORK_OPEN_INFORMATION  Attributes
);

// ******************************************************************
// * NtQueryInformationFile
// ******************************************************************
typedef XDK::NTSTATUS (NTAPI *FPTR_NtQueryInformationFile)
(
    IN  XDK::HANDLE                      FileHandle,
    OUT XDK::PIO_STATUS_BLOCK            IoStatusBlock,
    OUT XDK::PVOID                       FileInformation,
    IN  XDK::ULONG                       Length,
    IN  XDK::FILE_INFORMATION_CLASS      FileInfo
);

// ******************************************************************
// * NtQueryVolumeInformationFile
// ******************************************************************
typedef XDK::NTSTATUS (NTAPI *FPTR_NtQueryVolumeInformationFile)
(
    IN  XDK::HANDLE                      FileHandle,
    OUT PIO_STATUS_BLOCK            IoStatusBlock,
    OUT PFILE_FS_SIZE_INFORMATION   FileInformation,
    IN  XDK::ULONG                       Length,
    IN  FS_INFORMATION_CLASS        FsInformationClass
);

// ******************************************************************
// * IoCreateFile
// ******************************************************************
typedef XDK::NTSTATUS (NTAPI *FPTR_IoCreateFile)
(
    OUT XDK::PHANDLE             FileHandle,
    IN  XDK::ACCESS_MASK         DesiredAccess,
    IN  XDK::POBJECT_ATTRIBUTES  ObjectAttributes,
    OUT XDK::PIO_STATUS_BLOCK    IoStatusBlock,
    IN  XDK::PLARGE_INTEGER      AllocationSize OPTIONAL,
    IN  XDK::ULONG               FileAttributes,
    IN  XDK::ULONG               ShareAccess,
    IN  XDK::ULONG               Disposition,
    IN  XDK::ULONG               CreateOptions,
    IN  XDK::PVOID               EaBuffer OPTIONAL,
    IN  XDK::ULONG               EaLength,
    IN  CREATE_FILE_TYPE    CreateFileType,
    IN  XDK::PVOID               ExtraCreateParameters OPTIONAL,
    IN  XDK::ULONG               Options
);

// ******************************************************************
// * NtQueueApcThread
// ******************************************************************
typedef XDK::NTSTATUS (NTAPI *FPTR_NtQueueApcThread)
(
	IN XDK::HANDLE               ThreadHandle,
	IN XDK::PIO_APC_ROUTINE      ApcRoutine,
	IN XDK::PVOID                ApcRoutineContext OPTIONAL,
	IN XDK::PIO_STATUS_BLOCK     ApcStatusBlock OPTIONAL,
	IN XDK::ULONG                ApcReserved OPTIONAL 
);

// ******************************************************************
// * RtlDestroyHeap
// ******************************************************************
typedef XDK::PVOID (NTAPI *FPTR_RtlDestroyHeap)
(
    IN XDK::PVOID				HeapHandle
);

// ******************************************************************
// * NtOpenSymbolicLinkObject
// ******************************************************************
typedef XDK::NTSTATUS(NTAPI *FPTR_NtOpenSymbolicLinkObject)
(
	OUT XDK::PHANDLE				pHandle,
	IN  XDK::ACCESS_MASK			DesiredAccess,
	IN  POBJECT_ATTRIBUTES	ObjectAttributes
);

// ******************************************************************
// * NtDeviceIoControlFile
// ******************************************************************
typedef XDK::NTSTATUS(NTAPI *FPTR_NtDeviceIoControlFile)
(
	IN  XDK::HANDLE				FileHandle,
	IN  XDK::HANDLE				Event OPTIONAL,
	IN  XDK::PIO_APC_ROUTINE		ApcRoutine OPTIONAL,
	IN  XDK::PVOID				ApcContext OPTIONAL,
	OUT XDK::PIO_STATUS_BLOCK	IoStatusBlock,
	IN  XDK::ULONG				IoControlCode,
	IN  XDK::PVOID				InputBuffer OPTIONAL,
	IN  XDK::ULONG				InputBufferLength,
	OUT XDK::PVOID				OutputBuffer OPTIONAL,
	IN  XDK::ULONG				OutputBufferLength
);

// ******************************************************************
// * NtFsControlFile
// ******************************************************************
typedef XDK::NTSTATUS(NTAPI *FPTR_NtFsControlFile)
(
	IN  XDK::HANDLE				FileHandle,
	IN  XDK::HANDLE				Event OPTIONAL,
	IN  XDK::PIO_APC_ROUTINE		ApcRoutine OPTIONAL,
	IN  XDK::PVOID				ApcContext OPTIONAL,
	OUT XDK::PIO_STATUS_BLOCK	IoStatusBlock,
	IN  XDK::ULONG				FsControlCode,
	IN  XDK::PVOID				InputBuffer OPTIONAL,
	IN  XDK::ULONG				InputBufferLength,
	OUT XDK::PVOID				OutputBuffer OPTIONAL,
	IN  XDK::ULONG				OutputBufferLength
);

// ******************************************************************
// * NtCreateTimer
// ******************************************************************
typedef XDK::NTSTATUS(NTAPI *FPTR_NtCreateTimer)
(
	OUT XDK::PHANDLE				TimerHandle,
	IN  XDK::ACCESS_MASK			DesiredAccess,
	IN  XDK::POBJECT_ATTRIBUTES	ObjectAttributes OPTIONAL,
	IN  XDK::TIMER_TYPE			TimerType
);

// ******************************************************************
// * NtSetTimer
// ******************************************************************
typedef XDK::NTSTATUS(NTAPI *FPTR_NtSetTimer)
(
	IN  XDK::HANDLE				TimerHandle,
	IN  XDK::PLARGE_INTEGER		DueTime,
	IN  XDK::PTIMER_APC_ROUTINE	TimerApcRoutine OPTIONAL,
	IN  XDK::PVOID				TimerContext OPTIONAL,
	IN  XDK::BOOLEAN				ResumeTimer,
	IN  XDK::LONG				Period OPTIONAL,
	OUT XDK::PBOOLEAN			PreviousState OPTIONAL
);

// ******************************************************************
// * NtCancelTimer
// ******************************************************************
typedef XDK::NTSTATUS(NTAPI *FPTR_NtCancelTimer)
(
	IN  XDK::HANDLE				TimerHandle,
	OUT XDK::PBOOLEAN			CurrentState OPTIONAL
);

// ******************************************************************
// * Exported API
// ******************************************************************
#define EXTERN(API)  extern FPTR_##API API

// Note : Keep EXTERN's sorted, to ease future sync's (and compares with IMPORT's):
/*
EXTERN(InterlockedCompareExchange);
EXTERN(InterlockedDecrement);
EXTERN(InterlockedExchange);
EXTERN(InterlockedExchangeAdd);
EXTERN(InterlockedFlushSList);
EXTERN(InterlockedIncrement);
EXTERN(InterlockedPopEntrySList);
EXTERN(InterlockedPushEntrySList);
*/
EXTERN(NtAllocateVirtualMemory);
EXTERN(NtCancelTimer);
EXTERN(NtClearEvent);
EXTERN(NtClose);
EXTERN(NtCreateDirectoryObject);
EXTERN(NtCreateEvent);
EXTERN(NtCreateFile);
EXTERN(NtCreateMutant);
EXTERN(NtCreateSemaphore);
EXTERN(NtCreateTimer);
EXTERN(NtDelayExecution);
EXTERN(NtDeleteFile);
EXTERN(NtDeviceIoControlFile);
EXTERN(NtDuplicateObject);
EXTERN(NtFlushBuffersFile);
EXTERN(NtFreeVirtualMemory);
EXTERN(NtFsControlFile);
EXTERN(NtOpenSymbolicLinkObject);
EXTERN(NtPulseEvent);
EXTERN(NtQueryDirectoryFile);
EXTERN(NtQueryEvent);
EXTERN(NtQueryFullAttributesFile);
EXTERN(NtQueryInformationFile);
EXTERN(NtQueryMutant);
EXTERN(NtQuerySemaphore);
EXTERN(NtQueryTimer);
EXTERN(NtQueryVirtualMemory);
EXTERN(NtQueryVolumeInformationFile);
EXTERN(NtQueueApcThread);
EXTERN(NtReadFile);
EXTERN(NtReleaseMutant);
EXTERN(NtReleaseSemaphore);
EXTERN(NtResumeThread);
EXTERN(NtSetEvent);
EXTERN(NtSetInformationFile);
EXTERN(NtSetLdtEntries);
EXTERN(NtSetTimer);
EXTERN(NtSuspendThread);
EXTERN(NtWaitForMultipleObjects);
EXTERN(NtWaitForSingleObject);
EXTERN(NtWriteFile);
EXTERN(NtYieldExecution);
EXTERN(RtlAllocateHeap);
EXTERN(RtlAnsiStringToUnicodeString);
EXTERN(RtlAppendStringToString);
EXTERN(RtlAppendUnicodeStringToString);
EXTERN(RtlAppendUnicodeToString);
EXTERN(RtlCharToInteger);
EXTERN(RtlCompareMemory);
EXTERN(RtlCompareMemoryUlong);
EXTERN(RtlCompareString);
EXTERN(RtlCompareUnicodeString);
EXTERN(RtlCopyString);
EXTERN(RtlCopyUnicodeString);
EXTERN(RtlCreateHeap);
EXTERN(RtlCreateUnicodeString);
EXTERN(RtlDestroyHeap);
EXTERN(RtlDowncaseUnicodeChar);
EXTERN(RtlDowncaseUnicodeString);
EXTERN(RtlEnterCriticalSection);
EXTERN(RtlEqualString);
EXTERN(RtlEqualUnicodeString);
EXTERN(RtlExtendedIntegerMultiply);
EXTERN(RtlExtendedLargeIntegerDivide);
EXTERN(RtlExtendedMagicDivide);
EXTERN(RtlFillMemory);
EXTERN(RtlFillMemoryUlong);
EXTERN(RtlFreeAnsiString);
EXTERN(RtlFreeHeap);
EXTERN(RtlFreeUnicodeString);
EXTERN(RtlIntegerToChar);
EXTERN(RtlIntegerToUnicodeString);
EXTERN(RtlInitAnsiString);
EXTERN(RtlInitializeCriticalSection);
EXTERN(RtlInitUnicodeString);
EXTERN(RtlLeaveCriticalSection);
EXTERN(RtlMapGenericMask);
EXTERN(RtlMultiByteToUnicodeN);
EXTERN(RtlMultiByteToUnicodeSize);
EXTERN(RtlNtStatusToDosError);
EXTERN(RtlReAllocateHeap);
EXTERN(RtlSizeHeap);
EXTERN(RtlTimeFieldsToTime);
EXTERN(RtlTimeToTimeFields);
EXTERN(RtlTryEnterCriticalSection);
EXTERN(RtlUlongByteSwap);
EXTERN(RtlUnicodeStringToAnsiString);
EXTERN(RtlUnicodeStringToInteger);
EXTERN(RtlUnicodeToMultiByteN);
EXTERN(RtlUnicodeToMultiByteSize);
EXTERN(RtlUpcaseUnicodeChar);
EXTERN(RtlUpcaseUnicodeString);
EXTERN(RtlUpcaseUnicodeToMultiByteN);
EXTERN(RtlUpperString);
EXTERN(RtlUshortByteSwap);

} // NativeTypes

#endif
