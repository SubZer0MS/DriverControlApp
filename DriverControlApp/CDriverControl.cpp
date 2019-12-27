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

	STARTUPINFO si;
	PROCESS_INFORMATION pi;
	ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);
	ZeroMemory(&pi, sizeof(pi));
	WCHAR commandLine[MAX_PATH];

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

		CloseServiceHandle(m_hService);

		// if the program is started again after it closed and unloaded the driver and deleted the service,
		// depending on whether SCM has already been refreshed somehow (using sc.exe query) for example,
		// then it might be that the service is currently still just marked for deletion and if we try to delete
		// it again, it will just say that it has already been marged for deletion
		// we need to do a query process from an external process or else while doing it, we will still have a handle open
		// we'll simply use sc.exe

		StringCchPrintf(commandLine, MAX_PATH, _T("sc.exe query %s"), m_lpServiceName);

		if (!CreateProcess(
			NULL,			// Process name
			commandLine,	// Command line
			NULL,			// Process handle not inheritable
			NULL,			// Thread handle not inheritable
			FALSE,			// Set handle inheritance to FALSE
			NULL,			// No creation flags
			NULL,			// Use parent's environment block
			NULL,			// Use parent's starting directory 
			&si,			// Pointer to STARTUPINFO structure
			&pi				// Pointer to PROCESS_INFORMATION structure
		))
		{
			utils::PrintError(_T(__FUNCTION__), _T("CreateProcess"));
			status = DRV_ERROR_CREATE;
			goto CleanUp;
		}

		// Wait until child process exits.
		WaitForSingleObject(pi.hProcess, INFINITE);

		// Close process and thread handles. 
		CloseHandle(pi.hProcess);
		CloseHandle(pi.hThread);
		
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

	if (!StartService(m_hService, 0, NULL))
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
	if (StopDrv() != DRV_SUCCESS)
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
		m_hService = NULL;
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
