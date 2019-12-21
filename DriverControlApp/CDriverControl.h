#pragma once

#include <Windows.h>

#define APPKRNL_TYPE 40000

#define NT_DEVICE_NAME	L"\\Device\\AppKernelDriver"
#define DOS_DEVICE_NAME	L"\\DosDevices\\AppKernelDriver"

#define IOCTL_APPKRNL_GET_DRIVER_VERSION \
	CTL_CODE(APPKRNL_TYPE, 0x800, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_APPKRNL_ALLOCATE_POOL \
	CTL_CODE(APPKRNL_TYPE, 0x801, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_APPKRNL_FREE_POOL \
	CTL_CODE(APPKRNL_TYPE, 0x802, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define TAG_LENGTH			4

#define DRV_SUCCESS				0
#define DRV_NOT_CREATE		1
#define DRV_NOT_START		2
#define DRV_NOT_INIT		3
#define DRV_ERROR_SCMANAGER	4
#define DRV_ERROR_CREATE	5
#define DRV_ERROR_START		6
#define DRV_ERROR_OPEN		7
#define DRV_ERROR_STOP		8
#define DRV_ERROR_UNLOAD	9
#define DRV_ERROR_CONNECT	10

typedef struct _POOL_INFO_STRUCT
{
	ULONG Type;
	ULONGLONG Size;
	ULONG Tag;
} POOL_INFO_STRUCT, *PPOOL_INFO_STRUCT;

class CDriverControl
{
public:

	CDriverControl(
		LPCTSTR lpFilePath,
		LPCTSTR lpServiceName,
		LPCTSTR lpDisplayName,
		DWORD dwStartType
	) :
		m_lpFilePath(lpFilePath),
		m_lpServiceName(lpServiceName),
		m_lpDisplayName(lpDisplayName),
		m_dwStartType(dwStartType),
		m_hService(NULL),
		m_hDriver(INVALID_HANDLE_VALUE),
		m_loaded(FALSE),
		m_started(FALSE)
	{ }

	~CDriverControl();

	DWORD CreateDrv();
	DWORD StartDrv();
	DWORD StopDrv();
	DWORD UnloadDrv();

	DWORD DoAllocatePool(IN ULONG, IN ULONGLONG, IN LPCWSTR);
	DWORD DoFreePool(IN ULONG, IN LPCWSTR);

private:

	LPCTSTR m_lpFilePath;
	LPCTSTR m_lpServiceName;
	LPCTSTR m_lpDisplayName;
	DWORD m_dwStartType;
	SC_HANDLE m_hService;
	BOOL m_loaded;
	BOOL m_started;

	HANDLE m_hDriver;

	DWORD Initialize();
	inline BOOL IsLoaded();
	inline BOOL IsStarted();

};