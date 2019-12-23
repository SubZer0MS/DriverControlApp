#include "CDriverControl.h"
#include <tchar.h>
#include "Utils.h"
#include <strsafe.h>

CDriverControl::~CDriverControl()
{
	UnloadDrv();

	m_lpFilePath = NULL;
	m_lpServiceName = NULL;
	m_lpDisplayName = NULL;
	m_dwStartType = NULL;
	m_hService = NULL;
	m_loaded = FALSE;
	m_started = FALSE;
}

DWORD CDriverControl::Initialize()
{
	if (m_hService != NULL)
	{
		return DRV_SUCCESS;
	}

	DWORD status = DRV_SUCCESS;

	SC_HANDLE hSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_CREATE_SERVICE);
	if (hSCManager == NULL)
	{
		utils::PrintError(_T(__FUNCTION__), _T("OpenSCManager"));
		status = DRV_ERROR_SCMANAGER;
		goto CleanUp;
	}

	m_hService = CreateService(
		hSCManager,
		m_lpServiceName,
		m_lpDisplayName,
		SERVICE_ALL_ACCESS,
		SERVICE_KERNEL_DRIVER,
		m_dwStartType,
		SERVICE_ERROR_IGNORE,
		m_lpFilePath,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL
	);

	if (m_hService == NULL)
	{
		m_hService = OpenService(hSCManager, m_lpServiceName, SERVICE_ALL_ACCESS);
		if (m_hService == NULL)
		{
			utils::PrintError(_T(__FUNCTION__), _T("OpenService"));
			status = DRV_ERROR_CREATE;
			goto CleanUp;
		}

		if (!DeleteService(m_hService))
		{
			utils::PrintError(_T(__FUNCTION__), _T("DeleteService"));
			status = DRV_ERROR_CREATE;
			goto CleanUp;
		}

		CloseServiceHandle(m_hService);

		m_hService = CreateService(
			hSCManager,
			m_lpServiceName,
			m_lpDisplayName,
			SERVICE_ALL_ACCESS,
			SERVICE_KERNEL_DRIVER,
			m_dwStartType,
			SERVICE_ERROR_IGNORE,
			m_lpFilePath,
			NULL,
			NULL,
			NULL,
			NULL,
			NULL
		);
		if (m_hService == NULL)
		{
			utils::PrintError(_T(__FUNCTION__), _T("CreateService"));
			status = DRV_ERROR_CREATE;
			goto CleanUp;
		}
	}


CleanUp:

	if (hSCManager)
	{
		CloseServiceHandle(hSCManager);
	}

	return status;
}

DWORD CDriverControl::CreateDrv()
{
	if (IsLoaded())
	{
		return DRV_SUCCESS;
	}

	DWORD status = DRV_SUCCESS;

	status = Initialize();
	if (status != DRV_SUCCESS)
	{
		goto CleanUp;
	}

	m_loaded = TRUE;


CleanUp:

	return status;
}

DWORD CDriverControl::StartDrv()
{
	if (!IsLoaded())
	{
		return DRV_NOT_CREATE;
	}

	if (IsStarted())
	{
		return DRV_SUCCESS;
	}

	DWORD status = DRV_SUCCESS;

	status = Initialize();
	if (status != DRV_SUCCESS)
	{
		goto CleanUp;
	}

	LPCTSTR pszFormat;
	int strLen;
	LPTSTR szDestination;

	if (StartService(m_hService, 0, NULL) == NULL)
	{
		utils::PrintError(_T(__FUNCTION__), _T("StartService"));
		status = DRV_ERROR_START;
		goto CleanUp;
	}

	pszFormat = _T("\\\\.\\%s");

	strLen = (lstrlen(m_lpServiceName) + lstrlen(pszFormat) + 1) * sizeof(TCHAR);
	szDestination = (LPTSTR)LocalAlloc(LMEM_ZEROINIT, strLen);

	StringCchPrintf(szDestination, strLen, pszFormat, m_lpServiceName);

	m_hDriver = CreateFile(
		szDestination,
		GENERIC_READ | GENERIC_WRITE,
		NULL,
		NULL,
		OPEN_EXISTING,
		NULL,
		NULL
	);

	LocalFree(szDestination);

	if (m_hDriver == INVALID_HANDLE_VALUE)
	{
		utils::PrintError(_T(__FUNCTION__), _T("CreateFile"));
		status = DRV_ERROR_CONNECT;
		goto CleanUp;
	}

	m_started = TRUE;


CleanUp:

	return status;
}

DWORD CDriverControl::StopDrv()
{
	if (!IsStarted())
	{
		return DRV_SUCCESS;
	}

	SERVICE_STATUS ss;
	DWORD status = DRV_SUCCESS;

	status = Initialize();
	if (status != DRV_SUCCESS)
	{
		goto CleanUp;
	}

	if (!ControlService(m_hService, SERVICE_CONTROL_STOP, &ss))
	{
		utils::PrintError(_T(__FUNCTION__), _T("ControlService"));
		status = DRV_ERROR_STOP;
		goto CleanUp;
	}

	m_started = FALSE;


CleanUp:

	return status;
}

DWORD CDriverControl::UnloadDrv()
{
	if (!IsLoaded())
	{
		return DRV_SUCCESS;
	}
	if (IsStarted() && StopDrv() != DRV_SUCCESS)
	{
		return DRV_ERROR_UNLOAD;
	}

	DWORD status = DRV_SUCCESS;

	CloseHandle(m_hDriver);

	if (!DeleteService(m_hService))
	{
		utils::PrintError(_T(__FUNCTION__), _T("DeleteService"));
		status = DRV_ERROR_UNLOAD;
		goto CleanUp;
	}

	CloseServiceHandle(m_hService);

	if (!DeleteFile(m_lpFilePath))
	{
		// this isn't considered fatal, but we should notify the user
		utils::PrintError(_T(__FUNCTION__), _T("DeleteFile"));
	}

	m_loaded = FALSE;


CleanUp:

	if (m_hService)
	{
		CloseServiceHandle(m_hService);
	}

	return status;
}

inline BOOL CDriverControl::IsLoaded()
{
	return m_loaded;
}

inline BOOL CDriverControl::IsStarted()
{
	return m_started;
}

DWORD CDriverControl::DoAllocatePool(IN ULONG type, IN ULONGLONG size, IN LPCWSTR szBuffer)
{
	DWORD status = DRV_SUCCESS;

	if (!IsLoaded() || !IsStarted())
	{
		return DRV_ERROR_OPEN;
	}

	size_t sCharsConverted;
	char pChars[TAG_LENGTH + 1];

	// Conversion from UNICODE TO ASCII (in case this was compiled as UNICODE)
	wcstombs_s(&sCharsConverted, pChars, TAG_LENGTH + 1, szBuffer, TAG_LENGTH + 1);

	POOL_INFO_STRUCT poolInfo;
	poolInfo.Size = size;
	poolInfo.Tag = *(ULONG*)pChars;
	poolInfo.Type = type;


	TCHAR outBuffer[MAX_PATH];
	DWORD bytesRead = NULL;

	if (!DeviceIoControl(
		m_hDriver,
		IOCTL_APPKRNL_ALLOCATE_POOL,
		&poolInfo,
		sizeof(poolInfo),
		&outBuffer,
		MAX_PATH * sizeof(TCHAR),
		&bytesRead,
		NULL
	))
	{
		status = E_FAIL;
		utils::PrintError(_T(__FUNCTION__), _T("DeviceIoControl"));
	}

	return status;
}

DWORD CDriverControl::DoFreePool(IN ULONG type, IN LPCWSTR szBuffer)
{
	DWORD status = DRV_SUCCESS;

	if (!IsLoaded() || !IsStarted())
	{
		return DRV_ERROR_OPEN;
	}

	size_t sCharsConverted;
	char pChars[TAG_LENGTH + 1];

	// Conversion from UNICODE TO ASCII (in case this was compiled as UNICODE)
	wcstombs_s(&sCharsConverted, pChars, TAG_LENGTH + 1, szBuffer, TAG_LENGTH + 1);

	POOL_INFO_STRUCT poolInfo;
	poolInfo.Size = NULL;
	poolInfo.Tag = *(ULONG*)pChars;
	poolInfo.Type = type;

	TCHAR outBuffer[MAX_PATH];
	DWORD bytesRead = NULL;

	if (!DeviceIoControl(
		m_hDriver,
		IOCTL_APPKRNL_FREE_POOL,
		&poolInfo,
		sizeof(poolInfo),
		&outBuffer,
		sizeof(outBuffer),
		&bytesRead,
		NULL
	))
	{
		status = E_FAIL;
		utils::PrintError(_T(__FUNCTION__), _T("DeviceIoControl"));
	}

	return status;
}
