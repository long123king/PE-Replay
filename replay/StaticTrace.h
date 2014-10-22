#pragma once

#include <set>

#include "boost/cstdint.hpp"

// weird, switch the following two lines will cause compile error
#include "Logger.h"
#include "libdasm.h"

class StaticTrace
{
	friend class StaticTrace_Comparator;
public:
	StaticTrace(boost::uint32_t start,
		boost::uint32_t code_section_start, 
		boost::uint32_t code_section_size);
	~StaticTrace(void);

	void process(std::vector<boost::uint32_t>& gen_targets,
		std::set<boost::uint32_t>& lea_targets);

	bool Intersect(const boost::uint32_t start, const boost::uint32_t size)
	{
		return !(m_start + m_size <= start ||
			m_start >= (start + size));
	}

	bool Equals(const boost::uint32_t start)
	{
		return (m_start == start);
	}

	bool Same(const boost::uint32_t start, const boost::uint32_t size)
	{
		return m_start == start && m_size == size;
	}

	bool Contains(const boost::uint32_t start, const boost::uint32_t size)
	{
		return m_start <= start && m_start + m_size >= start + size;
	}

	bool Pertains(const boost::uint32_t start, const boost::uint32_t size)
	{
		return start <= m_start && start + size >= m_start + m_size;
	}

	bool BiggerThan(const boost::uint32_t& start)
	{
		return m_start > start;
	}

	bool SmallerThan(const boost::uint32_t& start)
	{
		return m_start < start;
	}

	boost::uint32_t GetStart()
	{
		return m_start;
	}

	boost::uint32_t GetEnd()
	{
		return m_start + m_size;
	}

	boost::uint32_t GetSize()
	{
		return m_size;
	}

	bool IsAddrCode(boost::uint32_t addr)
	{
		return addr >= m_code_section_start &&
			addr < m_code_section_start + m_code_section_size;
	}

	void AdjustEnd(boost::uint32_t end)
	{
		m_size = end - m_start;
	}

	static StaticTrace* AssembleTrace(boost::uint32_t start, 
		boost::uint32_t size,
		boost::uint32_t code_section_start, 
		boost::uint32_t code_section_size)
	{
		StaticTrace* trace =
			new StaticTrace(start, code_section_start, code_section_size);
		if (trace != NULL)
		{
			trace->m_size = size;
		}
		return trace;
	}

private: 
	boost::uint32_t m_start;
	boost::uint32_t m_size;

	boost::uint32_t m_code_section_start;
	boost::uint32_t m_code_section_size;
};

class StaticTrace_Comparator
{
public:
	bool operator()(const StaticTrace* op1, const StaticTrace* op2)
	{
		return op1->m_start < op2->m_start;
	}
};
