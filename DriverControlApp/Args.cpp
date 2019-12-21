#include "Args.h"
#include <tchar.h>

void ArgsHelper::arg(
	tstring sArg,
	tstring lArg,
	tstring desc,
	bool hasVal)
{
	std::unique_ptr<Args> arg(new Args);
	arg->m_sArg = sArg;
	arg->m_lArg = lArg;
	arg->m_desc = desc;
	arg->m_hasValue = hasVal;
	m_args.push_back(std::move(arg));

	if (!sArg.empty())
	{
		m_argNames.insert(std::pair<tstring, pArgs>(sArg, m_args.back().get()));
	}

	if (!lArg.empty())
	{
		m_argNames.insert(std::pair<tstring, pArgs>(lArg, m_args.back().get()));
	}
}

bool ArgsHelper::parseArgs(int argc, LPTSTR* argv)
{
	m_progName = tstring(argv[0]);
	bool expectValue = false;

	std::map<tstring, pArgs>::const_iterator it;
	for (int i = 1; i < argc; i++)
	{
		tstring entry(argv[i]);

		if (expectValue)
		{
			it->second->m_value = entry;
			expectValue = false;
		}
		else if (entry.compare(0, 1, _T("-")) == 0)
		{
			if (m_textArguments.size() > 0)
			{
				tsstream ss;
				ss << _T("Flags need to be set before text arguments.") << std::endl;
				m_hasError = true;
				m_error = ss.str();
			}

			if (entry.compare(0, 2, _T("--")) == 0)
			{
				entry.erase(0, 2);

				it = m_argNames.find(entry);
				if (it == m_argNames.end())
				{
					tsstream ss;
					ss << _T("Long flag ") << entry << _T(" not found.") << std::endl;
					m_hasError = true;
					m_error = ss.str();

					return false;
				}

				it->second->m_parsed = true;
				m_flagCount++;

				if (it->second->m_hasValue)
				{
					expectValue = true;
				}
			}
			else
			{
				entry.erase(0, 1);			
				for (int i = 0; i < entry.length(); i++)
				{
					tstring k(&(entry[i]), 1);
					it = m_argNames.find(k);
					if (it == m_argNames.end())
					{
						tsstream ss;
						ss << _T("Short flag ") << k << _T(" not found.") << std::endl;
						m_hasError = true;
						m_error = ss.str();

						return false;
					}

					it->second->m_parsed = true;
					m_flagCount++;

					if (it->second->m_hasValue)
					{
						if (i != (entry.length() - 1))
						{
							tsstream ss;
							ss << _T("Flag ") << k << _T(" needs a value.") << std::endl;
							m_hasError = true;
							m_error = ss.str();

							return false;
						}
						else
						{
							expectValue = true;
						}
					}
				}
			}
		}
		else
		{
			m_textArguments.push_back(entry);
		}
	}

	m_parsed = true;

	return true;
}

bool ArgsHelper::flag(tstring flag, tstring& value)
{
	if (!m_parsed)
	{
		return false;
	}

	auto it = m_argNames.find(flag);
	if (it == m_argNames.end() || !it->second->m_parsed)
	{
		return false;
	}

	if (it->second->m_hasValue)
	{
		value = it->second->m_value;
	}

	return true;
}

bool ArgsHelper::exists(tstring flag)
{
	if (!m_parsed)
	{
		return false;
	}

	auto it = m_argNames.find(flag);
	if (it == m_argNames.end() || !it->second->m_parsed)
	{
		return false;
	}

	return true;
}

bool ArgsHelper::textArg(uint32_t index, tstring& value)
{
	if (index < m_textArguments.size())
	{
		value = m_textArguments.at(index);
		return true;
	}

	return false;
}

tstring ArgsHelper::help()
{
	tsstream ss;
	ss << std::endl << _T("Usage:") << std::endl;
	ss << _T("\t") << m_usage << std::endl;
	ss << std::endl;
	ss << _T("Options: ") << std::endl;

	for (auto it = m_args.cbegin(); it != m_args.cend(); it++)
	{
		ss << _T("-") << (*it)->m_sArg << _T("\t--") << (*it)->m_lArg << _T("\t\t") << (*it)->m_desc << std::endl;
	}

	return ss.str();
}
