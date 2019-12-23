#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <sstream>
#include <Windows.h>

#ifdef _UNICODE
using tstring = std::wstring;
using tsstream = std::wstringstream;
#else
using tstring = std::string;
using tsstream = std::stringstream;
#endif

typedef struct _Arg
{
	_Arg() :
		m_hasValue(false),
		m_parsed(false)
	{}

	tstring m_sArg;
	tstring m_lArg;
	tstring m_desc;
	bool m_hasValue;
	tstring m_value;
	bool m_parsed;
	std::vector<tstring> m_fixedValues;
} Arg, *pArg;

class ArgsHelper
{
	std::vector<std::unique_ptr<Arg>> m_args;
	std::map<tstring, pArg> m_argNames;
	bool m_parsed = false;
	int m_flagCount = 0;
	tstring m_progName;
	tstring m_usage;
	bool m_hasError = false;
	tstring m_error;
	std::vector<tstring> m_textArguments;

public:

	void arg(tstring, tstring, tstring, bool, std::vector<tstring>);	
	bool parseArgs(int, LPTSTR*);
	bool flag(tstring, tstring&);
	bool exists(tstring);
	bool textArg(uint32_t, tstring&);
	tstring help();

	int flagCount()
	{
		return m_flagCount;
	}

	tstring progName()
	{
		return m_progName;
	}

	void usage(tstring use)
	{
		this->m_usage = use;
	}

	tstring error()
	{
		return m_error;
	}

	bool hasError()
	{
		return m_hasError;
	}
};
