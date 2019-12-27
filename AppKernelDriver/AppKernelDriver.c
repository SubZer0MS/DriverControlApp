#include "AppKernelDriver.h"

//-----------------------------------------------------------------------------
//
// Global
//
//-----------------------------------------------------------------------------

static ULONG refCount;

//-----------------------------------------------------------------------------
//
// DriverEntry / Dispatch / Unload
//
//-----------------------------------------------------------------------------

NTSTATUS
DriverEntry(
	IN PDRIVER_OBJECT DriverObject,
	IN PUNICODE_STRING RegistryPath
)
{
	UNREFERENCED_PARAMETER(RegistryPath);

	NTSTATUS		status;
	UNICODE_STRING  ntDeviceName;
	UNICODE_STRING  win32DeviceName;
	PDEVICE_OBJECT  deviceObject = NULL;

	RtlInitUnicodeString(&ntDeviceName, NT_DEVICE_NAME);

	status = IoCreateDevice(
		DriverObject,				// Our Driver Object
		0,							// We don't use a device extension
		&ntDeviceName,				// Device name 
		APPKRNL_TYPE,				// Device type
		FILE_DEVICE_SECURE_OPEN,	// Device characteristics
		FALSE,						// Not an exclusive device
		&deviceObject
	);

	if (!NT_SUCCESS(status))
	{
		refCount = (ULONG)-1;
		return status;
	}
	else
	{
		refCount = 0;
	}

	// Initialize the driver object with this driver's entry points.
	DriverObject->MajorFunction[IRP_MJ_CREATE] = Dispatch;
	DriverObject->MajorFunction[IRP_MJ_CLOSE] = Dispatch;
	DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = Dispatch;
	DriverObject->DriverUnload = Unload;

	// Initialize a Unicode String containing the Win32 name for our device.
	RtlInitUnicodeString(&win32DeviceName, DOS_DEVICE_NAME);

	// Create a symbolic link between our device name and the Win32 name
	status = IoCreateSymbolicLink(&win32DeviceName, &ntDeviceName);

	if (!NT_SUCCESS(status))
	{
		// Delete everything that this routine has allocated.
		IoDeleteDevice(deviceObject);
	}

	return status;
}

NTSTATUS
Dispatch(
	IN	PDEVICE_OBJECT pDO,
	IN	PIRP pIrp
)
{
	UNREFERENCED_PARAMETER(pDO);

	PIO_STACK_LOCATION pIrpStack;
	NTSTATUS status = STATUS_NOT_IMPLEMENTED;

	//  Initialize the irp info field.
	//	  This is used to return the number of bytes transfered.
	pIrp->IoStatus.Information = 0;
	pIrpStack = IoGetCurrentIrpStackLocation(pIrp);

	if (pIrpStack == NULL)
	{
		status = NO_MORE_IRP_STACK_LOCATIONS;
		goto CleanUp;
	}

	// Dispatch based on major fcn code.
	switch (pIrpStack->MajorFunction)
	{
	case IRP_MJ_CREATE:
		if (refCount != (ULONG)-1)
		{
			refCount++;
		}

		status = STATUS_SUCCESS;
		break;

	case IRP_MJ_CLOSE:
		if (refCount != (ULONG)-1)
		{
			refCount--;
		}

		status = STATUS_SUCCESS;
		break;

	case IRP_MJ_DEVICE_CONTROL:
		//  Dispatch on IOCTL
		switch (pIrpStack->Parameters.DeviceIoControl.IoControlCode)
		{
		case IOCTL_APPKRNL_ALLOCATE_POOL:
			status = DoAllocatePoolWithTag(
				pIrp->AssociatedIrp.SystemBuffer,
				pIrpStack->Parameters.DeviceIoControl.InputBufferLength,
				pIrp->AssociatedIrp.SystemBuffer,
				pIrpStack->Parameters.DeviceIoControl.OutputBufferLength,
				(ULONG*)&pIrp->IoStatus.Information
			);
			break;

		case IOCTL_APPKRNL_FREE_POOL:
			status = DoFreePoolWithTag(
				pIrp->AssociatedIrp.SystemBuffer,
				pIrpStack->Parameters.DeviceIoControl.InputBufferLength,
				pIrp->AssociatedIrp.SystemBuffer,
				pIrpStack->Parameters.DeviceIoControl.OutputBufferLength,
				(ULONG*)&pIrp->IoStatus.Information
			);
			break;
		}
		break;
	}


CleanUp:

	// We're done with I/O request.  Record the status of the I/O action.
	pIrp->IoStatus.Status = status;

	// Don't boost priority when returning since this took little time.
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);

	return status;
}

VOID
Unload(
	IN PDRIVER_OBJECT DriverObject
)
{
	PDEVICE_OBJECT deviceObject = DriverObject->DeviceObject;
	UNICODE_STRING win32NameString;

	PAGED_CODE();

	// Create counted string version of our Win32 device name.
	RtlInitUnicodeString(&win32NameString, DOS_DEVICE_NAME);

	// Delete the link from our device name to a name in the Win32 namespace.
	IoDeleteSymbolicLink(&win32NameString);

	if (deviceObject != NULL)
	{
		IoDeleteDevice(deviceObject);
	}
}

//-----------------------------------------------------------------------------
//
// Work Routines
//
//-----------------------------------------------------------------------------

NTSTATUS DoAllocatePoolWithTag(
	PVOID lpInBuffer,
	ULONG nInBufferSize,
	PVOID lpOutBuffer,
	ULONG nOutBufferSize,
	PULONG lpBytesReturned
)
{
	if (nInBufferSize != sizeof(POOL_INFO_STRUCT) ||
		lpInBuffer == NULL)
	{
		*lpBytesReturned = 0;
		return STATUS_UNSUCCESSFUL;
	}

	__try
	{
		PULONG_PTR pBuffer;
		PULONG_PTR pNext;
		PPOOL_INFO_STRUCT pPoolInfo = (PPOOL_INFO_STRUCT)lpInBuffer;

		pBuffer = (PULONG_PTR)ExAllocatePoolWithTag(
			pPoolInfo->Type == 0 ? NonPagedPool : PagedPool,
			(SIZE_T)pPoolInfo->Size,
			pPoolInfo->Tag
		);

		if (pBuffer)
		{
			if (pPoolInfo->Type == PagedPool)
			{
				pNext = PagedLeakedPoolHead;
				*pBuffer = (ULONG_PTR)pNext;
				PagedLeakedPoolHead = pBuffer;
			}
			else
			{
				pNext = NonPagedLeakedPoolHead;
				*pBuffer = (ULONG_PTR)pNext;
				NonPagedLeakedPoolHead = pBuffer;
			}
		}
		
		// I don't use this, but I'm keeping it here as an example of
		// how to send data back - it's usually something more complex like a stucture
		PWCHAR returnData = L"This is a message back from the driver!";
		size_t bytesCount;
		RtlStringCbLengthW(returnData, NTSTRSAFE_MAX_CCH, &bytesCount);
		
		memcpy(lpOutBuffer, &returnData, bytesCount);
		nOutBufferSize = (ULONG)bytesCount;
		*lpBytesReturned = (ULONG)bytesCount;

		return STATUS_SUCCESS;
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		*lpBytesReturned = 0;
		return STATUS_UNSUCCESSFUL;
	}
}

NTSTATUS DoFreePoolWithTag(
	PVOID lpInBuffer,
	ULONG nInBufferSize,
	PVOID lpOutBuffer,
	ULONG nOutBufferSize,
	PULONG lpBytesReturned
)
{
	if (nInBufferSize != sizeof(POOL_INFO_STRUCT) ||
		lpInBuffer == NULL)
	{
		*lpBytesReturned = 0;
		return STATUS_UNSUCCESSFUL;
	}

	__try
	{
		ULONG_PTR pNext;
		PPOOL_INFO_STRUCT pPoolInfo = (PPOOL_INFO_STRUCT)lpInBuffer;

		if (pPoolInfo->Type == PagedPool)
		{
			while (PagedLeakedPoolHead)
			{
				pNext = (ULONG_PTR)*PagedLeakedPoolHead;
				ExFreePoolWithTag(PagedLeakedPoolHead, pPoolInfo->Tag);
				PagedLeakedPoolHead = (PULONG_PTR)pNext;
			}
		}
		else
		{
			while (NonPagedLeakedPoolHead)
			{
				pNext = *NonPagedLeakedPoolHead;
				ExFreePoolWithTag(NonPagedLeakedPoolHead, pPoolInfo->Tag);
				NonPagedLeakedPoolHead = (PULONG_PTR)pNext;
			}
		}

		// I don't use this, but I'm keeping it here as an example of
		// how to send data back - it's usually something more complex like a stucture
		PWCHAR returnData = L"This is a message back from the driver!";
		size_t bytesCount;
		RtlStringCbLengthW(returnData, NTSTRSAFE_MAX_CCH, &bytesCount);

		memcpy((LPTSTR)lpOutBuffer, &returnData, bytesCount);
		nOutBufferSize = (ULONG)bytesCount;
		*lpBytesReturned = (ULONG)bytesCount;

		return STATUS_SUCCESS;
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		*lpBytesReturned = 0;
		return STATUS_UNSUCCESSFUL;
	}
}
