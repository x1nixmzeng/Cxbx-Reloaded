// ******************************************************************
// *
// * proj : OpenXDK
// *
// * desc : Open Source XBox Development Kit
// *
// * file : ex.h
// *
// * note : XBox Kernel *Executive* Declarations
// *
// ******************************************************************
#ifndef XBOXKRNL_EX_H
#define XBOXKRNL_EX_H

// ******************************************************************
// * 0x000C - ExAcquireReadWriteLockExclusive()
// ******************************************************************
XBSYSAPI EXPORTNUM(12) VOID NTAPI ExAcquireReadWriteLockExclusive
(
	IN PERWLOCK ReadWriteLock
);

// ******************************************************************
// * 0x000D - ExAcquireReadWriteLockShared()
// ******************************************************************
XBSYSAPI EXPORTNUM(13) VOID NTAPI ExAcquireReadWriteLockShared
(
	IN PERWLOCK ReadWriteLock
);

// ******************************************************************
// * 0x000E - ExAllocatePool()
// ******************************************************************
XBSYSAPI EXPORTNUM(14) PVOID NTAPI ExAllocatePool
(
    IN SIZE_T NumberOfBytes
);

// ******************************************************************
// * 0x000F - ExAllocatePoolWithTag()
// ******************************************************************
XBSYSAPI EXPORTNUM(15) PVOID NTAPI ExAllocatePoolWithTag
(
    IN SIZE_T NumberOfBytes,
    IN ULONG Tag
);

// ******************************************************************
// * 0x0010 - ExEventObjectType
// ******************************************************************
XBSYSAPI EXPORTNUM(16) OBJECT_TYPE ExEventObjectType;

// ******************************************************************
// * 0x0011 - ExFreePool()
// ******************************************************************
XBSYSAPI EXPORTNUM(17) VOID NTAPI ExFreePool
(
    IN PVOID P
);

// ******************************************************************
// * 0x0012 - ExInitializeReadWriteLock()
// ******************************************************************
XBSYSAPI EXPORTNUM(18) VOID NTAPI ExInitializeReadWriteLock
(
	IN PERWLOCK ReadWriteLock
);

// ******************************************************************
// * 0x0013 - ExInterlockedAddLargeInteger()
// ******************************************************************
XBSYSAPI EXPORTNUM(19) LARGE_INTEGER NTAPI ExInterlockedAddLargeInteger
(
    IN PLARGE_INTEGER Addend,
    IN LARGE_INTEGER Increment
);

// ******************************************************************
// * 0x0014 - ExInterlockedAddLargeStatistic()
// ******************************************************************
XBSYSAPI EXPORTNUM(20) VOID FASTCALL ExInterlockedAddLargeStatistic
(
    IN PLARGE_INTEGER Addend,
    IN ULONG Increment
);

// ******************************************************************
// * 0x0015 - ExInterlockedCompareExchange64()
// ******************************************************************
XBSYSAPI EXPORTNUM(21) LONGLONG FASTCALL ExInterlockedCompareExchange64
(
    IN PLONGLONG Destination,
    IN PLONGLONG Exchange,
    IN PLONGLONG Comperand
);

// ******************************************************************
// * 0x0016 - ExMutantObjectType
// ******************************************************************
XBSYSAPI EXPORTNUM(22) OBJECT_TYPE ExMutantObjectType;

// ******************************************************************
// * 0x0017 - ExQueryPoolBlockSize()
// ******************************************************************
XBSYSAPI EXPORTNUM(23) ULONG NTAPI ExQueryPoolBlockSize
(
    IN PVOID PoolBlock
);

// ******************************************************************
// * 0x0018 - ExQueryNonVolatileSetting()
// ******************************************************************
XBSYSAPI EXPORTNUM(24) NTSTATUS NTAPI ExQueryNonVolatileSetting
(
    IN  ULONG   ValueIndex,
    OUT PULONG  Type,
    OUT PVOID   Value,
    IN  ULONG   ValueLength,
    OUT PULONG  ResultLength
);

// ******************************************************************
// * 0x0019 - ExReadWriteRefurbInfo()
// ******************************************************************
XBSYSAPI EXPORTNUM(25) NTSTATUS NTAPI ExReadWriteRefurbInfo
(
    OUT PXBOX_REFURB_INFO   RefurbInfo,
    IN  ULONG               ValueLength,
    IN  BOOLEAN             DoWrite
);

// ******************************************************************
// * 0x001A - ExRaiseException()
// ******************************************************************
XBSYSAPI EXPORTNUM(26) VOID NTAPI ExRaiseException
(
	IN PEXCEPTION_RECORD ExceptionRecord
);

// ******************************************************************
// * 0x001B - ExRaiseStatus()
// ******************************************************************
XBSYSAPI EXPORTNUM(27) VOID NTAPI ExRaiseStatus
(
	IN NTSTATUS Status
);

// ******************************************************************
// * 0x001C - ExReleaseReadWriteLock()
// ******************************************************************
XBSYSAPI EXPORTNUM(28) VOID NTAPI ExReleaseReadWriteLock
(
	IN PERWLOCK ReadWriteLock
);

// ******************************************************************
// * 0x001D - ExSaveNonVolatileSetting()
// ******************************************************************
XBSYSAPI EXPORTNUM(29) NTSTATUS NTAPI ExSaveNonVolatileSetting
(
    IN ULONG ValueIndex,
    IN ULONG Type,
    IN PVOID Value,
    IN ULONG ValueLength
);

// ******************************************************************
// * 0x001E - ExSemaphoreObjectType
// ******************************************************************
XBSYSAPI EXPORTNUM(30) OBJECT_TYPE ExSemaphoreObjectType;

// ******************************************************************
// * 0x001F - ExTimerObjectType
// ******************************************************************
XBSYSAPI EXPORTNUM(31) OBJECT_TYPE ExTimerObjectType;

// ******************************************************************
// * 0x0020 - ExfInterlockedInsertHeadList()
// ******************************************************************
XBSYSAPI EXPORTNUM(32) PLIST_ENTRY FASTCALL ExfInterlockedInsertHeadList
(
	IN PLIST_ENTRY ListHead,
	IN PLIST_ENTRY ListEntry
);

// ******************************************************************
// * 0x0021 - ExfInterlockedInsertTailList()
// ******************************************************************
XBSYSAPI EXPORTNUM(33) PLIST_ENTRY FASTCALL ExfInterlockedInsertTailList
(
	IN PLIST_ENTRY ListHead,
	IN PLIST_ENTRY ListEntry
);

XBSYSAPI EXPORTNUM(34) PLIST_ENTRY FASTCALL ExfInterlockedRemoveHeadList
(
	IN PLIST_ENTRY ListHead,
	IN PKSPIN_LOCK Lock
);

XBSYSAPI EXPORTNUM(51) LONG FASTCALL KRNL(InterlockedCompareExchange)
(
	IN OUT volatile PLONG Destination,
	IN LONG  Exchange,
	IN LONG  Comparand
);

XBSYSAPI EXPORTNUM(52) LONG FASTCALL KRNL(InterlockedDecrement)
(
	IN OUT PLONG Addend
);

XBSYSAPI EXPORTNUM(53) LONG FASTCALL KRNL(InterlockedIncrement)
(
	IN OUT PLONG Addend
);

XBSYSAPI EXPORTNUM(54) LONG FASTCALL KRNL(InterlockedExchange)
(
	IN volatile PLONG Destination,
	IN LONG Value
);

XBSYSAPI EXPORTNUM(55) LONG FASTCALL KRNL(InterlockedExchangeAdd)
(
	IN volatile PLONG Addend,
	IN LONG	Value
);

// Dxbx Note : The Xbox1 SINGLE_LIST strucures are the same as in WinNT
XBSYSAPI EXPORTNUM(56) SINGLE_LIST_ENTRY * FASTCALL KRNL(InterlockedFlushSList)
(
	IN PSLIST_HEADER ListHead
);

XBSYSAPI EXPORTNUM(57) SLIST_ENTRY * FASTCALL KRNL(InterlockedPopEntrySList)
(
	IN PSLIST_HEADER ListHead
);

XBSYSAPI EXPORTNUM(58) SLIST_ENTRY * FASTCALL KRNL(InterlockedPushEntrySList)
(
	IN PSLIST_HEADER ListHead,
	IN PSLIST_ENTRY ListEntry
);

#endif


