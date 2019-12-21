#include "Utils.h"
#include <strsafe.h>
#include <tchar.h>
#include "resource.h"
#include <stdio.h>
#include <fcntl.h>
#include <io.h>
#include <iostream>
#include <fstream>

namespace utils
{
	BOOL ExtractAndGetDriverFullPathName(OUT TCHAR& lpFilePathFull)
	{
		DWORD dwRetVal = NULL;
		TCHAR lpTempPathBuffer[MAX_PATH];
		HRSRC hRes = NULL;
		HGLOBAL hResourceLoaded = INVALID_HANDLE_VALUE;
		LPVOID lpResLock;
		DWORD fileSize = NULL;
		HANDLE hFile = INVALID_HANDLE_VALUE;

		LPCTSTR pszFormat = _T("%s\\%s");

		dwRetVal = GetTempPath(MAX_PATH, lpTempPathBuffer);
		if (dwRetVal > MAX_PATH || (dwRetVal == 0))
		{
			PrintError(_T(__FUNCTION__), _T("GetTempPath"));
			goto CleanUp;
		}

		hRes = FindResource(
			NULL,
			MAKEINTRESOURCE(IDR_BINARY_DRIVER_FILE),
			_T("BINARY")
		);

		if (hRes == NULL)
		{
			PrintError(_T(__FUNCTION__), _T("FindResource"));
			goto CleanUp;
		}

		hResourceLoaded = LoadResource(NULL, hRes);
		if (hResourceLoaded == NULL)
		{
			PrintError(_T(__FUNCTION__), _T("LoadResource"));
			goto CleanUp;
		}

		lpResLock = LockResource(hResourceLoaded);
		if (lpResLock == NULL)
		{
			PrintError(_T(__FUNCTION__), _T("LockResource"));
			goto CleanUp;
		}

		fileSize = SizeofResource(NULL, hRes);
		if (fileSize == NULL)
		{
			PrintError(_T(__FUNCTION__), _T("SizeofResource"));
			goto CleanUp;
		}

		StringCchPrintf(&lpFilePathFull, MAX_PATH, pszFormat, lpTempPathBuffer, _T("AppKernelDriver.sys"));

		hFile = CreateFile(
			&lpFilePathFull,
			GENERIC_WRITE,
			NULL,
			NULL,
			CREATE_ALWAYS,
			FILE_ATTRIBUTE_NORMAL,
			NULL
		);

		if (hFile == INVALID_HANDLE_VALUE)
		{
			PrintError(_T(__FUNCTION__), _T("CreateFile"));
			goto CleanUp;
		}

		dwRetVal = WriteFile(hFile, lpResLock, fileSize, NULL, NULL);
		if (dwRetVal == FALSE)
		{
			PrintError(_T(__FUNCTION__), _T("WriteFile"));
			goto CleanUp;
		}

		CloseHandle(hFile);

		return ERROR_SUCCESS;


	CleanUp:

		if (hFile != INVALID_HANDLE_VALUE)
		{
			CloseHandle(hFile);
		}

		return NULL;
	}

	VOID PrintError(CONST LPCTSTR& errDesc, CONST LPCTSTR& errDesc2)
	{
		LPVOID lpMsgBuf;
		LPVOID lpDisplayBuf;
		DWORD dw = GetLastError();

		FormatMessage(
			FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL,
			dw,
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			(LPTSTR)&lpMsgBuf,
			0, NULL
		);

		lpDisplayBuf = (LPVOID)LocalAlloc(
			LMEM_ZEROINIT,
			(lstrlen((LPCTSTR)lpMsgBuf) + lstrlen((LPCTSTR)errDesc) + lstrlen((LPCTSTR)errDesc2) + 40) * sizeof(TCHAR)
		);

		StringCchPrintf(
			(LPTSTR)lpDisplayBuf,
			LocalSize(lpDisplayBuf) / sizeof(TCHAR),
			_T("%s => %s failed with error %d: %s"),
			errDesc,
			errDesc2,
			dw,
			lpMsgBuf
		);

		MessageBox(NULL, (LPCTSTR)lpDisplayBuf, _T("ERROR"), MB_OK);

		LocalFree(lpMsgBuf);
		LocalFree(lpDisplayBuf);
	}

	VOID PrintInfo(CONST LPCTSTR pMessage, ...)
	{
		LPWSTR pBuffer = NULL;

		va_list args = NULL;
		va_start(args, pMessage);

		FormatMessage(
			FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ALLOCATE_BUFFER,
			pMessage,
			0,
			0,
			(LPWSTR)&pBuffer,
			0,
			&args);

		va_end(args);

		MessageBox(NULL, (LPCTSTR)pBuffer, _T("INFO"), MB_OK);

		LocalFree(pBuffer);
	}

	void RedirectIOToConsole()
	{

		//Create a console for this application
		AllocConsole();

		HANDLE ConsoleOutput = GetStdHandle(STD_OUTPUT_HANDLE);
		int SystemOutput = _open_osfhandle(intptr_t(ConsoleOutput), _O_TEXT);
		FILE* COutputHandle = _fdopen(SystemOutput, "w");

		HANDLE ConsoleError = GetStdHandle(STD_ERROR_HANDLE);
		int SystemError = _open_osfhandle(intptr_t(ConsoleError), _O_TEXT);
		FILE* CErrorHandle = _fdopen(SystemError, "w");

		HANDLE ConsoleInput = GetStdHandle(STD_INPUT_HANDLE);
		int SystemInput = _open_osfhandle(intptr_t(ConsoleInput), _O_TEXT);
		FILE* CInputHandle = _fdopen(SystemInput, "r");

		std::ios::sync_with_stdio(true);

		freopen_s(&CInputHandle, "CONIN$", "r", stdin);
		freopen_s(&COutputHandle, "CONOUT$", "w", stdout);
		freopen_s(&CErrorHandle, "CONOUT$", "w", stderr);

		std::wcout.clear();
		std::cout.clear();
		std::wcerr.clear();
		std::cerr.clear();
		std::wcin.clear();
		std::cin.clear();
	}

#ifndef _UNICODE
	LPSTR* CommandLineToArgvA(LPSTR CmdLine, int* _argc)
	{
		PCHAR* argv;
		PCHAR  _argv;
		ULONG   len;
		ULONG   argc;
		CHAR   a;
		ULONG   i, j;

		BOOLEAN  in_QM;
		BOOLEAN  in_TEXT;
		BOOLEAN  in_SPACE;

		len = strlen(CmdLine);
		i = ((len + 2) / 2) * sizeof(PVOID) + sizeof(PVOID);

		argv = (PCHAR*)GlobalAlloc(GMEM_FIXED,
			i + (len + 2) * sizeof(CHAR));

		_argv = (PCHAR)(((PUCHAR)argv) + i);

		argc = 0;
		argv[argc] = _argv;
		in_QM = FALSE;
		in_TEXT = FALSE;
		in_SPACE = TRUE;
		i = 0;
		j = 0;

		while (a = CmdLine[i]) {
			if (in_QM) {
				if (a == '\"') {
					in_QM = FALSE;
				}
				else {
					_argv[j] = a;
					j++;
				}
			}
			else {
				switch (a) {
				case '\"':
					in_QM = TRUE;
					in_TEXT = TRUE;
					if (in_SPACE) {
						argv[argc] = _argv + j;
						argc++;
					}
					in_SPACE = FALSE;
					break;
				case ' ':
				case '\t':
				case '\n':
				case '\r':
					if (in_TEXT) {
						_argv[j] = '\0';
						j++;
					}
					in_TEXT = FALSE;
					in_SPACE = TRUE;
					break;
				default:
					in_TEXT = TRUE;
					if (in_SPACE) {
						argv[argc] = _argv + j;
						argc++;
					}
					_argv[j] = a;
					j++;
					in_SPACE = FALSE;
					break;
				}
			}
			i++;
		}
		_argv[j] = '\0';
		argv[argc] = NULL;

		(*_argc) = argc;
		return argv;
	}
#endif
}
