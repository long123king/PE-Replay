#pragma once

#include <string>
#include <fstream>
#include <set>
#include <map>

#include "boost/cstdint.hpp"
#include "StaticTrace.h"
#include "libdasm.h"
#include "ImageSection.h"

#include "pin.H"
#include "portability.H"

class CodeSectionSweeper
{
public:
	CodeSectionSweeper(IMG img, 
		SEC sec, 
		const std::set<boost::uint32_t>& reloc_targets,
		boost::uint32_t section_start,
		boost::uint32_t section_size);

	~CodeSectionSweeper(void);

	void markGreyArea(boost::uint32_t start, boost::uint32_t size);

	void appendTarget(boost::uint32_t target);

	void process();

	void postProcess();
	
	void spotGaps();

	void linearSweepGap(boost::uint32_t gap_start, boost::uint32_t gap_end);

	void calculateCoverage();


	std::set<StaticTrace*, StaticTrace_Comparator>::iterator TracesBegin()
	{
		return m_traces.begin();
	}

	std::set<StaticTrace*, StaticTrace_Comparator>::iterator TracesEnd()
	{
		return m_traces.end();
	}

	bool HasTraceStart(boost::uint32_t addr)
	{
		return m_map_traces.find(addr) != m_map_traces.end();
	}

	StaticTrace* GetTraceAt(boost::uint32_t addr)
	{		
		for (std::set<StaticTrace*, StaticTrace_Comparator>::const_iterator it = m_traces.begin();
			it != m_traces.end();
			it++)
		{
			if ((*it)->Contains(addr, 1))
			{
				return *it;
				break;
			}
			else if ((*it)->BiggerThan(addr))
			{
				// optimize
				break;
			}
		}

		return NULL;
	}

private:

	bool IsAddrCode(boost::uint32_t addr)
	{
		//for (std::vector<ImageSection*>::iterator
		//	it = m_sections.begin();
		//	it != m_sections.end();
		//it ++)
		//{
		//	if ((*it)->Executable() &&
		//		(*it)->Contains(addr))
		//	{
		//		return true;
		//	}
		//}

		if (addr >= m_code_section_start &&
			addr < m_code_section_start + m_code_section_size)
		{
			return true;
		}

		return false;
	}

	bool justifyTrace(boost::uint32_t trace_start, boost::uint32_t trace_size);

	bool HasTraceAt(boost::uint32_t target);	

	bool IsRangeIntersectedWithGreyArea(boost::uint32_t& start, boost::uint32_t& end, 
		std::vector<std::pair<boost::uint32_t, boost::uint32_t>>& vec_gaps)
	{
		bool bResult = false;

		boost::int32_t start_offset = start;
		boost::int32_t end_offset = end;

		if (start_offset == -1 || 
			end_offset == -1 ||
			start_offset >= end_offset)
		{
			return false;
		}

		// iterate all over grey areas, if intersected with a grey area,
		// then trim it.

		for (std::set<StaticTrace*, StaticTrace_Comparator>::iterator 
			it = m_grey_areas.begin();
			it != m_grey_areas.end();
			it ++)
		{
			if ((*it)->Contains(start_offset, end_offset - start_offset))
			{
				// Trim to zero
				start = end;
				bResult = true;
			}
			else if ((*it)->Pertains(start_offset, end_offset - start_offset))
			{
				// Trim one trace to two	
				vec_gaps.push_back(std::make_pair<boost::uint32_t, boost::uint32_t>(
					(*it)->GetEnd(),
					end));

				end = (*it)->GetStart();
			}
			else if ((*it)->Intersect(start_offset, end_offset - start_offset))
			{
				// Trim partially
				if ((*it)->GetStart() < start_offset)
				{
					start = (*it)->GetEnd();
				}
				else
				{
					end = (*it)->GetStart();
				}
			}
		}
		return bResult;
	}

	bool IsRangeAlignPaddingStart(boost::uint32_t& start, boost::uint32_t end)
	{
		bool bResult = false;

		boost::int32_t start_offset = start;
		boost::int32_t end_offset = end;

		if (start_offset == -1 || 
			end_offset == -1 ||
			start_offset >= end_offset)
		{
			return false;
		}

		for (boost::int32_t i = start_offset;
			i != end_offset;
			i++)
		{
			if (IsAddrCode(i))
			{
				boost::uint8_t ch = *((boost::uint8_t*)i);
				if ( ch == 0xCC)
				{
					start++;
					bResult = true;
				}
				else
				{
					break;
				}
			}			
			else
			{
				break;
			}
		}

		return bResult;
	}

	bool IsRangeAlignPaddingEnd(boost::uint32_t start, boost::uint32_t& end)
	{
		bool bResult = false;

		boost::int32_t start_offset = start;
		boost::int32_t end_offset = end;

		if (start_offset == -1 || 
			end_offset == -1 ||
			start_offset >= end_offset)
		{
			return false;
		}

		for (boost::int32_t i = end_offset-1;
			i >= start_offset;
			i--)
		{
			if (IsAddrCode(i))
			{
				boost::uint8_t ch = *((boost::uint8_t*)i);
				if ( ch == 0xCC)
				{
					end--;
					bResult = true;
				}
				else
				{
					break;
				}
			}
			else
			{
				break;
			}
			
		}

		return bResult;
	}

	bool IsRangeSingleTrampoline(boost::uint32_t& start, boost::uint32_t end)
	{
		if ((end - start) >= 5)
		{
			boost::uint32_t addr = 0;
			boost::int32_t section_offset = start;			

			if (section_offset != -1 &&
				IsAddrCode(section_offset))
			{
				boost::uint8_t ch = *((boost::uint8_t*)section_offset);

				if (ch == 0xE9)
				{
					addr += *((boost::uint8_t*) section_offset + 1) ;
					addr += *((boost::uint8_t*) section_offset + 2) << 8;
					addr += *((boost::uint8_t*) section_offset + 3) << 16;
					addr += *((boost::uint8_t*) section_offset + 4) << 24;

					m_pending_targets.insert(addr);

					StaticTrace* trace = 
						StaticTrace::AssembleTrace(start, 5, m_code_section_start, m_code_section_size);
					if (trace != NULL)
					{
						//LOG_SCOVERAGE("New Trace [0x%08x - 0x%08x]\n",
						//	trace->GetStart(),
						//	trace->GetEnd());
						m_traces.insert(trace);
						m_map_traces[start] = trace;
					}

					start += 5;

					return true;
				}
	
			}
		}
		return false;
	}

	bool IsRangeComboTrampoline(boost::uint32_t& start, boost::uint32_t end)
	{
		// N trampolines
		// N trampolines and align padding

		bool bResult = false;

		while (IsRangeSingleTrampoline(start, end))
		{
			bResult = true;
		}

		return bResult;
	}

	// return:
	//   true, all gap are trimmed to zero length
	//   false, only header part of gap are trimmed, the left part are passed back via parameters reference
	bool TrimGap(boost::uint32_t& start, boost::uint32_t& end)
	{
		while ((IsRangeComboTrampoline(start, end) ||
			IsRangeAlignPaddingStart(start, end) || 
			IsRangeAlignPaddingEnd(start, end)) &&
			start < end)
		{

		}

		if (start == end)
		{
			return true;
		}
		else 
		{
			return false;
		}
	}

private:
	std::set<StaticTrace*, StaticTrace_Comparator> m_traces;
	std::map<boost::uint32_t, StaticTrace*> m_map_traces;
	std::set<boost::uint32_t> m_pending_targets;

	IMG m_img;
	SEC m_sec;
	std::set<boost::uint32_t> m_reloc_targets;
	boost::uint32_t m_code_section_start;
	boost::uint32_t m_code_section_size;

	std::set<boost::uint32_t> m_lea_targets;

	std::set<StaticTrace*, StaticTrace_Comparator> m_grey_areas;
};
