// ******************************************************************
// *
// * proj : OpenXDK
// *
// * desc : Open Source XBox Development Kit
// *
// * file : hal.h
// *
// * note : XBox Kernel *Hardware Abstraction Layer* Declarations
// *
// ******************************************************************
#ifndef XBOXKRNL_HAL_H
#define XBOXKRNL_HAL_H

// ******************************************************************
// * 0x0009 - HalReadSMCTrayState()
// ******************************************************************
XBSYSAPI EXPORTNUM(9) NTSTATUS NTAPI HalReadSMCTrayState
(
    OUT PULONG TrayState,
    OUT PULONG TrayStateChangeCount OPTIONAL
);

// ******************************************************************
// * 0x0026 - HalClearSoftwareInterrupt()
// ******************************************************************
XBSYSAPI EXPORTNUM(38) VOID FASTCALL HalClearSoftwareInterrupt
(
    IN KIRQL RequestIrql
);

// ******************************************************************
// * 0x0027 - HalDisableSystemInterrupt()
// ******************************************************************
XBSYSAPI EXPORTNUM(39) VOID NTAPI HalDisableSystemInterrupt
(
    IN ULONG BusInterruptLevel
);

// ******************************************************************
// * 0x0028 - HalDiskCachePartitionCount
// ******************************************************************
XBSYSAPI EXPORTNUM(40) ULONG HalDiskCachePartitionCount;

// ******************************************************************
// * 0x0029 - HalDiskModelNumber
// ******************************************************************
XBSYSAPI EXPORTNUM(41) PANSI_STRING HalDiskModelNumber;

// ******************************************************************
// * 0x002A - HalDiskSerialNumber
// ******************************************************************
XBSYSAPI EXPORTNUM(42) PANSI_STRING HalDiskSerialNumber;

// ******************************************************************
// * 0x002B - HalEnableSystemInterrupt()
// ******************************************************************
XBSYSAPI EXPORTNUM(43) VOID NTAPI HalEnableSystemInterrupt
(
    IN ULONG BusInterruptLevel,
    IN KINTERRUPT_MODE InterruptMode
);

// ******************************************************************
// * 0x002C - HalGetInterruptVector()
// ******************************************************************
XBSYSAPI EXPORTNUM(44) ULONG  NTAPI HalGetInterruptVector
(
    IN ULONG BusInterruptLevel,
    OUT PKIRQL Irql
);

// ******************************************************************
// * 0x002D - HalReadSMBusValue()
// ******************************************************************
XBSYSAPI EXPORTNUM(45) NTSTATUS NTAPI HalReadSMBusValue
(
    IN  UCHAR   SlaveAddress,
    IN  UCHAR   CommandCode,
    IN  BOOLEAN ReadWordValue,
    OUT PULONG  DataValue
);

// ******************************************************************
// * 0x002E - HalReadWritePCISpace()
// ******************************************************************
XBSYSAPI EXPORTNUM(46) VOID NTAPI HalReadWritePCISpace
(
    IN ULONG    BusNumber,
    IN ULONG    SlotNumber,
    IN ULONG    RegisterNumber,
    IN PVOID    Buffer,
    IN ULONG    Length,
    IN BOOLEAN  WritePCISpace
);

typedef VOID (*PHAL_SHUTDOWN_NOTIFICATION)(
    IN struct _HAL_SHUTDOWN_REGISTRATION *ShutdownRegistration
);

typedef struct {
    PHAL_SHUTDOWN_NOTIFICATION  NotificationRoutine;
    LONG                        Priority;
    LIST_ENTRY                  ListEntry;
} HAL_SHUTDOWN_REGISTRATION, *PHAL_SHUTDOWN_REGISTRATION;

// ******************************************************************
// * 0x002F - HalRegisterShutdownNotification()
// ******************************************************************
XBSYSAPI EXPORTNUM(47) VOID NTAPI HalRegisterShutdownNotification
(
    IN PHAL_SHUTDOWN_REGISTRATION ShutdownRegistration,
    IN BOOLEAN Register
);

// ******************************************************************
// * 0x0030 - HalRequestSoftwareInterrupt()
// ******************************************************************
XBSYSAPI EXPORTNUM(46) VOID FASTCALL HalRequestSoftwareInterrupt
(
	IN KIRQL Request
);

// ******************************************************************
// * 0x0031 - HalReturnToFirmware()
// ******************************************************************
XBSYSAPI EXPORTNUM(49) VOID DECLSPEC_NORETURN HalReturnToFirmware
(
    IN RETURN_FIRMWARE Routine
);

// ******************************************************************
// * 0x0032 - HalWriteSMBusValue()
// ******************************************************************
XBSYSAPI EXPORTNUM(50) NTSTATUS NTAPI HalWriteSMBusValue
(
    IN UCHAR    SlaveAddress,
    IN UCHAR    CommandCode,
    IN BOOLEAN  WriteWordValue,
    IN ULONG    DataValue
);

// ******************************************************************
// * READ_PORT_BUFFER_UCHAR
// ******************************************************************
XBSYSAPI EXPORTNUM(329) VOID NTAPI READ_PORT_BUFFER_UCHAR
(
    IN PUCHAR Port,
    IN PUCHAR Buffer,
    IN ULONG  Count
);

// ******************************************************************
// * READ_PORT_BUFFER_USHORT
// ******************************************************************
XBSYSAPI EXPORTNUM(330) VOID NTAPI READ_PORT_BUFFER_USHORT
(
    IN PUSHORT Port,
    IN PUSHORT Buffer,
    IN ULONG   Count
);

// ******************************************************************
// * READ_PORT_BUFFER_ULONG
// ******************************************************************
XBSYSAPI EXPORTNUM(331) VOID NTAPI READ_PORT_BUFFER_ULONG
(
    IN PULONG Port,
    IN PULONG Buffer,
    IN ULONG  Count
);

// ******************************************************************
// * WRITE_PORT_BUFFER_UCHAR
// ******************************************************************
XBSYSAPI EXPORTNUM(332) VOID NTAPI WRITE_PORT_BUFFER_UCHAR
(
    IN PUCHAR Port,
    IN PUCHAR Buffer,
    IN ULONG  Count
);

// ******************************************************************
// * WRITE_PORT_BUFFER_USHORT
// ******************************************************************
XBSYSAPI EXPORTNUM(333) VOID NTAPI WRITE_PORT_BUFFER_USHORT
(
    IN PUSHORT Port,
    IN PUSHORT Buffer,
    IN ULONG   Count
);

// ******************************************************************
// * WRITE_PORT_BUFFER_ULONG
// ******************************************************************
XBSYSAPI EXPORTNUM(334) VOID NTAPI WRITE_PORT_BUFFER_ULONG
(
    IN PULONG Port,
    IN PULONG Buffer,
    IN ULONG  Count
);

// ******************************************************************
// * HalBootSMCVideoMode
// ******************************************************************
XBSYSAPI EXPORTNUM(356) DWORD HalBootSMCVideoMode;

XBSYSAPI EXPORTNUM(358) BOOLEAN NTAPI HalIsResetOrShutdownPending
(
);

XBSYSAPI EXPORTNUM(360) NTSTATUS NTAPI HalInitiateShutdown
(
);

XBSYSAPI EXPORTNUM(365) VOID NTAPI HalEnableSecureTrayEject
(
);

XBSYSAPI EXPORTNUM(366) NTSTATUS NTAPI HalWriteSMCScratchRegister
(
	IN DWORD ScratchRegister
);

#endif


