#pragma once

#include <string>
#include <map>
#include <set>

#include "boost/cstdint.hpp"

#include "pin.H"
#include "ImageSectionRoutine.h"

#include "Logger.h"

class ImageSection
{
public:
	ImageSection(SEC sec);
	~ImageSection(void);

	bool Contains(boost::uint32_t addr)
	{
		return m_address <= addr &&
			(m_address + m_size) > addr;
	}

	bool Executable()
	{
		/*return m_text &&
			m_executable;*/
		return m_executable;
	}

	std::string Name()
	{
		return m_name;
	}

	bool HasRtn(boost::uint32_t addr)
	{
		return m_addr_rtns.find(addr) != m_addr_rtns.end();
	}

	std::string WithinRtn(boost::uint32_t addr)
	{
		for (std::map<boost::uint32_t, 
			std::pair<std::string, boost::uint32_t>>::const_iterator
			it = m_addr_rtns.begin();
			it != m_addr_rtns.end();
			it ++)
		{
			if (it->first <= addr &&
				it->second.second > addr)
			{				
				if ((addr - it->first) == 0)
				{
					return it->second.first;
				}
				else
				{
					char offset[32] = {0,};
					sprintf(offset, "_0x%08x", (addr - it->first));
					return it->second.first + offset;
				}
				
			}
		}

		return "";
	}

	std::string GetRtnName(boost::uint32_t addr)
	{
		return m_addr_rtns[addr].first;
	}

	boost::uint32_t Start()
	{
		return m_address;
	}

	boost::uint32_t Size()
	{
		return m_size;
	}

	boost::uint32_t End()
	{
		return m_address + m_size;
	}

	void GetRtnAddrs(std::set<boost::uint32_t>& rtn_addrs)
	{
		for (std::vector<ImageSectionRoutine*>::iterator
			it = m_routines.begin();
			it != m_routines.end();
			it ++)
		{
			rtn_addrs.insert((*it)->Start());
		}
	}

private:
	SEC m_section;
	std::string m_name;
	boost::uint32_t m_address;
	boost::uint32_t m_size;
	bool m_readable;
	bool m_writeable;
	bool m_executable;
	bool m_text;

	bool m_code;
	bool m_initialized_data;
	bool m_uninitialized_data;

	std::vector<ImageSectionRoutine*> m_routines;

	std::map<boost::uint32_t, 
		std::pair<std::string, boost::uint32_t>> m_addr_rtns;
};
