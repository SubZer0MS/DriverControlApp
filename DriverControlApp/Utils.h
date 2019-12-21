#pragma once

#include <Windows.h>

namespace utils
{
	BOOL ExtractAndGetDriverFullPathName(OUT TCHAR& lpFilePathFull);
	VOID PrintError(CONST LPCTSTR&, CONST LPCTSTR&);
	VOID PrintInfo(CONST LPCTSTR, ...);
	void RedirectIOToConsole();
#ifndef _UNICODE
	LPSTR* CommandLineToArgvA(LPSTR, int*);
#endif
}
