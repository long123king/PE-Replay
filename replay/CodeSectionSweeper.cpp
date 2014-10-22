#include "CodeSectionSweeper.h"

using namespace LIBDASM;

CodeSectionSweeper::CodeSectionSweeper(IMG img, 
	SEC sec, 
	const std::set<boost::uint32_t>& reloc_targets,
	boost::uint32_t section_start,
	boost::uint32_t section_size)
:m_img(img)
,m_sec(sec)
,m_reloc_targets(reloc_targets)
,m_code_section_start(section_start)
,m_code_section_size(section_size)
{
	boost::uint32_t entry_point = (boost::uint32_t)IMG_Entry(img);

	appendTarget(entry_point);	

	// add reloc and rtn entry as targets.
	for (std::set<boost::uint32_t>::iterator 
			it = m_reloc_targets.begin();
			it != m_reloc_targets.end();
			it ++)
	{
		appendTarget(*it);
	}

	for(RTN rtn= SEC_RtnHead(sec);
		RTN_Valid(rtn);
		rtn = RTN_Next(rtn) )
	{
		appendTarget(RTN_Address(rtn));
	}
}

CodeSectionSweeper::~CodeSectionSweeper(void)
{
}

void CodeSectionSweeper::process()
{
	std::vector<boost::uint32_t> tmp_target;
	while (!m_pending_targets.empty())
	{		
		boost::uint32_t target = *m_pending_targets.begin();
		m_pending_targets.erase(target);
		
		if (IsAddrCode(target) && !HasTraceAt(target))
		{
			StaticTrace* trace = new StaticTrace(target, m_code_section_start, m_code_section_size);
			if (trace == NULL)
			{
				printf("Fail to allocate StaticTrace for 0x%08x!\n", target);
				break;
			}
			trace->process(tmp_target, m_lea_targets);

			m_traces.insert(trace);
			m_map_traces[target] = trace;

			//LOG_SCOVERAGE("trace at [0x%08x]\n", target);

			for (std::vector<boost::uint32_t>::iterator it = tmp_target.begin();
				it != tmp_target.end();
				it++)
			{
				m_pending_targets.insert(*it);
				//LOG_SCOVERAGE("trace yield new target [0x%08x]\n", *it);
			}

			tmp_target.clear();
		}		
	}

	// now we need to process the split problem

	// first find out adjacent traces whose end is the same ( as many as possible )
	// then shrink the longer ones, in order to eliminate overlap among those traces

	// but the total number of traces will stay the same, 
	// .i.e the m_traces will stay the same

	for (std::set<StaticTrace*, StaticTrace_Comparator>::iterator it = ++m_traces.begin();
		it != m_traces.end();
		it++)
	{
		StaticTrace* former = *(--it);
		StaticTrace* latter = *(++it);

		if (former->GetEnd() == latter->GetEnd())
		{
			former->AdjustEnd(latter->GetStart());
		}
	}
}

void CodeSectionSweeper::postProcess()
{
#define MAX_LEA_PADDING_SEARCH_RANGE (4 * 1024)
	for (std::set<boost::uint32_t>::const_iterator
		it = m_lea_targets.begin();
		it != m_lea_targets.end();
		it ++)
	{
		boost::uint32_t lea_target_addr = *it;
		boost::uint32_t lea_target_end = lea_target_addr;
		LOQ(CONFIG_LEA,
			"lea target at [0x%08x]\n", lea_target_addr);
		// the invalid area should merge with its trailing 0xCC areas
		boost::uint32_t num_int3 = 0;
		while (lea_target_end++ < lea_target_addr + MAX_LEA_PADDING_SEARCH_RANGE)
		{
			if (*((boost::uint8_t*)lea_target_end) == 0xCC)
			{
				num_int3 ++;
			}
			else if (((lea_target_end) & 0xF) == 0 &&
				num_int3 > 4)
			{
				break;
			}
			else
			{
				num_int3 = 0;
			}
		}

		if (lea_target_end < lea_target_addr + MAX_LEA_PADDING_SEARCH_RANGE)
		{
			for (std::set<StaticTrace*, StaticTrace_Comparator>::iterator itt = m_traces.begin();
				itt != m_traces.end();
				itt++)
			{
				if ((*itt)->Intersect(lea_target_addr, lea_target_end - lea_target_addr))
				{
					LOQ(CONFIG_LEA,
						"Invalid trace [0x%08x - 0x%08x] by lea range[0x%08x - 0x%08x]\n",
						(*itt)->GetStart(), 
						(*itt)->GetEnd(),
						lea_target_addr,
						lea_target_end);
					m_traces.erase(itt);
					continue;
				}
				else if ((*itt)->BiggerThan(lea_target_end))
				{
					// optimize
					break;
				}
			}
		}
	}
}

void CodeSectionSweeper::appendTarget( boost::uint32_t target )
{
	if (IsAddrCode(target))
	{
		m_pending_targets.insert(target);
	}
}

bool CodeSectionSweeper::HasTraceAt( boost::uint32_t target )
{
	bool bHas = false;

	// it will be faster in most cases if we use backward iteration
	for (std::set<StaticTrace*, StaticTrace_Comparator>::const_reverse_iterator it = m_traces.rbegin();
		it != m_traces.rend();
		it++)
	{
		if ((*it)->Equals(target))
		{
			bHas = true;
			break;
		}
		else if ((*it)->SmallerThan(target))
		{
			// optimize
			break;
		}
	}

	return bHas;
}

void CodeSectionSweeper::spotGaps()
{
	std::vector<std::pair<boost::uint32_t, boost::uint32_t>> vec_ranges;

	std::vector<std::pair<boost::uint32_t, boost::uint32_t>> vec_gaps;

	std::pair<boost::uint32_t, boost::uint32_t> range_now(0, 0);

	for (std::set<StaticTrace*, StaticTrace_Comparator>::const_iterator it = m_traces.begin();
		it != m_traces.end();
		it++)
	{
		if (range_now.first == 0 && range_now.second == 0)
		{
			// a completely new range
			range_now.first = (*it)->GetStart();
			range_now.second = (*it)->GetEnd();
		}
		else if ((*it)->GetStart() == range_now.second)
		{
			// merge the current range with former one
			range_now.second = (*it)->GetEnd();
		}
		else
		{
			// we'v got a largest range, save it
			vec_ranges.push_back(range_now);

			range_now.first = (*it)->GetStart();
			range_now.second = (*it)->GetEnd();
		}
		
		//LOG_SCOVERAGE("[0x%08x - 0x%08x]\n",
		//	(*it)->GetStart(),
		//	(*it)->GetEnd());
	}


	//LOG_SCOVERAGE("***********************************\n"
	//	"Code Range:\n[0x%08x - 0x%08x]\n\n",
	//	m_code_start,
	//	m_code_start + m_code_size);

	boost::uint32_t gap_start = m_code_section_start;
	boost::uint32_t gap_end = m_code_section_start + m_code_section_size;

	for (std::vector<std::pair<boost::uint32_t, boost::uint32_t>>::iterator it = vec_ranges.begin();
		it != vec_ranges.end();
		it++)
	{
		gap_end = it->first;
		//LOG_SCOVERAGE("gap [0x%08x] starts at file:[%08x] ",
		//	gap_end - gap_start,
		//	m_pe->rva_to_file_offset(m_pe->va_to_rva(gap_start)));

		vec_gaps.push_back(
			std::make_pair<boost::uint32_t, boost::uint32_t>(gap_start, gap_end));

		gap_start = it->second;

		//LOG_SCOVERAGE("range [0x%08x - 0x%08x]\n",
		//	it->first,
		//	it->second);
	}

	gap_end = m_code_section_start + m_code_section_size;
	//LOG_SCOVERAGE("gap [0x%08x] starts at file:[%08x] ",
	//	gap_end - gap_start,
	//	m_pe->rva_to_file_offset(m_pe->va_to_rva(gap_start)));

	vec_gaps.push_back(
		std::make_pair<boost::uint32_t, boost::uint32_t>(gap_start, gap_end));


	// we need to identify these gaps, 


	for (std::vector<std::pair<boost::uint32_t, boost::uint32_t>>::iterator it = vec_gaps.begin();
		it != vec_gaps.end();
		it++)
	{
		// 1). if it is alignment related,
		// it should ends at somewhere aligned to 0xFFFFFFF0, 
		// and the gaps are all filled with 0xCC
		//if (IsRangeAlignPadding(it->first, it->second))
		//{
		//	//else
		//	//{
		//	//	LOG_SCOVERAGE("gap \"is\" align padding [0x%08x - 0x%08x], at file offset : [%08x]\n",
		//	//		gap_start,
		//	//		gap_end,
		//	//		m_pe->rva_to_file_offset(m_pe->va_to_rva(gap_start)));
		//	//}
		//}	
		//else if (IsRangeComboTrampoline(it->first, it->second))
		//{

		//}

		gap_start = it->first;
		gap_end = it->second;

		if (TrimGap(gap_start, gap_end))
		{
			
		}
		else if (IsRangeIntersectedWithGreyArea(gap_start, gap_end, vec_gaps))
		{

		}
		else
		{
			linearSweepGap(gap_start, gap_end);
		}

	}

	process();
}

void CodeSectionSweeper::linearSweepGap( boost::uint32_t gap_start, boost::uint32_t gap_end )
{
	// 2). if it is skipped, we should scan it manually,
	// .i.e we need to resolve possible target address manually.

	// 3). there are also jmp/switch indirect table in code section

	// 4). E9 XX XX XX XX should also be skipped.

	// 5). Epilogue spotting around gap_end

	/************************************************************************/
	// How to linear sweep gap?
	// process instructions one after another
	// the purpose of the sweep is to find out more valid traces
	// 1. if an invalid instruction found, the containing trace is also invalid
	// 2. if 0xCC found, just skip it
	// 3. [X]if trampoline found, skip it, and log the potential trace it points at
	// 4. due to linear sweep, we don't recurse to its control transferring targets
	// This is the last step of processing, no further process is required.
	// [Update] Seems another round of process needs to be invoked after linearSweep
	/************************************************************************/

	

	boost::uint32_t trace_start = gap_start;

	boost::uint32_t inst_pre = gap_start;
	boost::uint32_t inst_post = gap_start;

	// inst_post == inst_pre + instruction.length

	while (inst_pre < gap_end)
	{
		INSTRUCTION instruction = {0,};
		//memset(&instruction, 0, sizeof(instruction));

		if (IsRangeAlignPaddingStart(inst_pre, gap_end))
		{
			// skip align padding
			StaticTrace* trace = 
				StaticTrace::AssembleTrace(trace_start, inst_post - trace_start, m_code_section_start, m_code_section_size);

			if (trace != NULL)
			{
				//LOG_SCOVERAGE("New Trace [0x%08x - 0x%08x]\n",
				//	trace->GetStart(),
				//	trace->GetEnd());
				m_traces.insert(trace);
				m_map_traces[trace_start] = trace;
			}

			trace_start = inst_pre;
			inst_post = inst_pre;
		}

		if (IsAddrCode(inst_pre))
		{
			if ( get_instruction(&instruction, 
				(boost::uint8_t*)(inst_pre), 
				MODE_32) == 0)
			{
				inst_post++;
				trace_start = inst_post;

				inst_pre = inst_post;
				continue;
			}
		}
		else
		{
			inst_post++;
			trace_start = inst_post;

			inst_pre = inst_post;
			continue;
		}
		
		inst_post = inst_pre + instruction.length;

		if (instruction.type == INSTRUCTION_TYPE_JMP)
		{
			if (instruction.op1.type == OPERAND_TYPE_IMMEDIATE)
			{
				// direct control flow, CF
				boost::int32_t offset = 0;
				get_operand_immediate(&instruction.op1, (LIBDASM_DWORD*)&offset);
				boost::uint32_t target_va = 
					(boost::uint32_t)((boost::int32_t)inst_pre + instruction.length + offset);

				m_pending_targets.insert(target_va);				
			}

			StaticTrace* trace = 
				StaticTrace::AssembleTrace(trace_start, inst_post - trace_start, m_code_section_start, m_code_section_size);
			if (trace != NULL)
			{
				//LOG_SCOVERAGE("New Trace [0x%08x - 0x%08x]\n",
				//	trace->GetStart(),
				//	trace->GetEnd());
				m_traces.insert(trace);
				m_map_traces[trace_start] = trace;
			}

			trace_start = inst_post;

			// jmp "is" end for trace
			//break;
		}
		else if (instruction.type == INSTRUCTION_TYPE_JMPC)
		{
			if (instruction.op1.type == OPERAND_TYPE_IMMEDIATE)
			{
				// direct control flow, CF
				boost::int32_t offset = 0;
				get_operand_immediate(&instruction.op1, (LIBDASM_DWORD*)&offset);
				boost::uint32_t target_va = 
					(boost::uint32_t)((boost::int32_t)inst_pre + instruction.length + offset);

				m_pending_targets.insert(target_va);				
			}
		}
		else if (instruction.type == INSTRUCTION_TYPE_CALL)
		{
			StaticTrace* trace = 
				StaticTrace::AssembleTrace(trace_start, inst_post - trace_start, m_code_section_start, m_code_section_size);
			if (trace != NULL)
			{
				//LOG_SCOVERAGE("New Trace [0x%08x - 0x%08x]\n",
				//	trace->GetStart(),
				//	trace->GetEnd());
				m_traces.insert(trace);
				m_map_traces[trace_start] = trace;
			}

			trace_start = inst_post;
			// call "is" end for trace
			//break;
		}
		else if (instruction.type == INSTRUCTION_TYPE_RET)
		{
			StaticTrace* trace = 
				StaticTrace::AssembleTrace(trace_start, inst_post - trace_start, m_code_section_start, m_code_section_size);
			if (trace != NULL)
			{
				//LOG_SCOVERAGE("New Trace [0x%08x - 0x%08x]\n",
				//	trace->GetStart(),
				//	trace->GetEnd());
				m_traces.insert(trace);
				m_map_traces[trace_start] = trace;
			}

			trace_start = inst_post;
			// ret "is" end for trace
			//break;
		}

		inst_pre = inst_post;
	}
}

void CodeSectionSweeper::calculateCoverage()
{
	boost::uint32_t static_trace_size = 0;
	for (std::set<StaticTrace*, StaticTrace_Comparator>::const_iterator it = m_traces.begin();
		it != m_traces.end();
		it++)
	{
		static_trace_size += (*it)->GetSize();
		LOQ(CONFIG_SUMMARY,
				"[0x%08x - 0x%08x]\n", (*it)->GetStart(), (*it)->GetEnd());
	}

	LOQ(CONFIG_SUMMARY,
		"Static Trace Coverage: %.2f%% (%d/%d)\n", 
		(double)static_trace_size * 100. / (double)m_code_section_size,
		static_trace_size, 
		m_code_section_size);
}


bool CodeSectionSweeper::justifyTrace( boost::uint32_t trace_start, boost::uint32_t trace_size )
{
	// if the trace is consecutive and continuous, we believe it is a valid trace.

	INSTRUCTION instruction = {0,};

	boost::uint32_t trace_end = trace_start + trace_size;

	boost::uint32_t inst_pre = trace_start;
	boost::uint32_t inst_post = trace_start;

	// inst_post == inst_pre + instruction.length

	while (inst_pre < trace_size)
	{
		if (IsRangeAlignPaddingStart(inst_pre, trace_end))
		{
			// No, it is definitely an invalid trace.
			return false;
		}

		if (IsAddrCode(inst_pre))
		{
			if ( get_instruction(&instruction, 
				(boost::uint8_t*)(inst_pre), 
				MODE_32) == 0)
			{
				// No, it is definitely an invalid trace.
				return false;
			}
		}
		else
		{
			// No, it is definitely an invalid trace.
			return false;
		}

		inst_post = inst_pre + instruction.length;

		if (instruction.type == INSTRUCTION_TYPE_JMP ||
			instruction.type == INSTRUCTION_TYPE_CALL ||
			instruction.type == INSTRUCTION_TYPE_RET)
		{
			// No, it is definitely an invalid trace.
			return false;
		}

		inst_pre = inst_post;
	}

	return true;
}

void CodeSectionSweeper::markGreyArea( boost::uint32_t start, boost::uint32_t size )
{
	StaticTrace* grey_area = 
		StaticTrace::AssembleTrace(start, size, m_code_section_start, m_code_section_size);

	m_grey_areas.insert(grey_area);

	// 1. all traces that intersected with grey area must be excluded.
	// 2. and a more gentle way to do this is just subtract the grey area part.
}
