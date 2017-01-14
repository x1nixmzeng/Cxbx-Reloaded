// ******************************************************************
// *
// * proj : OpenXDK
// *
// * desc : Open Source XBox Development Kit
// *
// * file : dbg.h
// *
// * note : XBox Kernel *Debug* Declarations
// *
// ******************************************************************
#ifndef XBOXKRNL_DBG_H
#define XBOXKRNL_DBG_H

// ******************************************************************
// * 0x0005 - DbgBreakPoint()
// ******************************************************************
XBSYSAPI EXPORTNUM(5) VOID NTAPI DbgBreakPoint();

// ******************************************************************
// * 0x0006 - DbgBreakPointWithStatus()
// ******************************************************************
XBSYSAPI EXPORTNUM(6) VOID NTAPI DbgBreakPointWithStatus
(
	IN ULONG Status
);

// ******************************************************************
// * 0x0007 - DbgLoadImageSymbols()
// ******************************************************************
XBSYSAPI EXPORTNUM(7) NTSTATUS NTAPI DbgLoadImageSymbols
(
	IN PSTRING FileName,
	IN PVOID ImageBase,
	IN ULONG_PTR ProcessId
);

// ******************************************************************
// * 0x0008 - DbgPrint()
// ******************************************************************
XBSYSAPI EXPORTNUM(8) ULONG _cdecl DbgPrint
(
	IN PCHAR  Format,
    ...
);

// ******************************************************************
// * 0x000A - DbgPrompt()
// ******************************************************************
XBSYSAPI EXPORTNUM(10) ULONG NTAPI DbgPrompt
(
	IN PCH Prompt,
	OUT PCH Response,
	IN ULONG MaximumResponseLength
);

// ******************************************************************
// * 0x000B - DbgUnLoadImageSymbols()
// ******************************************************************
XBSYSAPI EXPORTNUM(11) VOID NTAPI DbgUnLoadImageSymbols
(
    IN PSTRING FileName,
    IN PVOID ImageBase,
    IN ULONG_PTR ProcessId
);

#endif




