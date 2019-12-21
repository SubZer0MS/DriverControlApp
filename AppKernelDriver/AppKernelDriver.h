#pragma once

#include <ntddk.h>
#include <devioctl.h>
#include <stddef.h>
#include <ntstrsafe.h>

#define APPKRNL_TYPE 40000

//-----------------------------------------------------------------------------
//
// Version Information
//
//-----------------------------------------------------------------------------

#define APPKRNL_DRIVER_ID				_T("AppKernelDriver")

#define APPKRNL_DRIVER_MAJOR_VERSION	1
#define APPKRNL_DRIVER_MINOR_VERSION	0
#define APPKRNL_DRIVER_REVISION			0
#define APPKRNL_DRIVER_RELESE			0

#define APPKRNL_DRIVER_VERSION \
	((APPKRNL_DRIVER_MAJOR_VERSION << 24) | (APPKRNL_DRIVER_MINOR_VERSION << 16) \
	| (APPKRNL_DRIVER_REVISION << 8) | APPKRNL_DRIVER_RELESE)

//-----------------------------------------------------------------------------
//
// Device Name
//
//-----------------------------------------------------------------------------

#define NT_DEVICE_NAME	L"\\Device\\AppKernelDriver"
#define DOS_DEVICE_NAME	L"\\DosDevices\\AppKernelDriver"

//-----------------------------------------------------------------------------
//
// The IOCTL function codes from 0x800 to 0xFFF are for customer use.
//
//-----------------------------------------------------------------------------
#define IOCTL_APPKRNL_GET_DRIVER_VERSION \
	CTL_CODE(APPKRNL_TYPE, 0x800, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_APPKRNL_ALLOCATE_POOL \
	CTL_CODE(APPKRNL_TYPE, 0x801, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_APPKRNL_FREE_POOL \
	CTL_CODE(APPKRNL_TYPE, 0x802, METHOD_BUFFERED, FILE_ANY_ACCESS)

//-----------------------------------------------------------------------------
//
// Typedef Struct
//
//-----------------------------------------------------------------------------

typedef struct _POOL_INFO_STRUCT
{
	ULONG Type;
	ULONGLONG Size;
	ULONG Tag;
} POOL_INFO_STRUCT, *PPOOL_INFO_STRUCT;

//-----------------------------------------------------------------------------
//
// Globals
//
//-----------------------------------------------------------------------------

PULONG_PTR PagedLeakedPoolHead = NULL;
PULONG_PTR NonPagedLeakedPoolHead = NULL;

//-----------------------------------------------------------------------------
//
// Function Prototypes
//
//-----------------------------------------------------------------------------

/*
Return Value:
STATUS_SUCCESS if the driver initialized correctly, otherwise an error
indicating the reason for failure.
*/
NTSTATUS DriverEntry(
	IN PDRIVER_OBJECT DriverObject,
	IN PUNICODE_STRING RegistryPath
);

/*++
Routine Description:
This routine is the dispatch handler for the driver.  It is responsible
for processing the IRPs.
Arguments:
pDO - Pointer to device object.
pIrp - Pointer to the current IRP.
Return Value:
STATUS_SUCCESS if the IRP was processed successfully, otherwise an erroror
indicating the reason for failure.
--*/
NTSTATUS Dispatch(
	IN PDEVICE_OBJECT pDO,
	IN PIRP pIrp
);

/*++
Routine Description:
This routine is called by the I/O system to unload the driver.
Any resources previously allocated must be freed.
Arguments:
DriverObject - a pointer to the object that represents our driver.
Return Value:
None
--*/
VOID Unload(
	IN PDRIVER_OBJECT DriverObject
);

//-----------------------------------------------------------------------------
//
// Function Prototypes for Control Code
//
//-----------------------------------------------------------------------------

NTSTATUS DoAllocatePoolWithTag(
	PVOID lpInBuffer,
	ULONG nInBufferSize,
	PVOID lpOutBuffer,
	ULONG nOutBufferSize,
	PULONG lpBytesReturned
);

NTSTATUS DoFreePoolWithTag(
	PVOID lpInBuffer,
	ULONG nInBufferSize,
	PVOID lpOutBuffer,
	ULONG nOutBufferSize,
	PULONG lpBytesReturned
);
