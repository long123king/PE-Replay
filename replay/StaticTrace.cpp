#include "StaticTrace.h"
#include "ImageInfo.h"

using namespace LIBDASM;

StaticTrace::StaticTrace(boost::uint32_t start, 
	boost::uint32_t code_section_start, 
	boost::uint32_t code_section_size)
:m_start(start)
,m_size(0)
,m_code_section_start(code_section_start)
,m_code_section_size(code_section_size)
{
}

StaticTrace::~StaticTrace(void)
{
}

void StaticTrace::process( std::vector<boost::uint32_t>& gen_targets,
						  std::set<boost::uint32_t>& lea_targets )
{
	INSTRUCTION instruction = {0,};

	// here what we can do it to walk directly towards the end of a continuous code block.
	boost::uint32_t inst_va = m_start;

	while (true)
	{
		memset(&instruction, 0, sizeof(instruction));

		if (IsAddrCode(inst_va))
		{
			if ( get_instruction(&instruction, 
				(boost::uint8_t*)(inst_va), 
				MODE_32) == 0)
				break;
		}
		else
		{
			LOQ(CONFIG_EXCETION,
							"[0x%08x] is not pointing to code sections!\n", inst_va);
			break;
		}

		m_size += instruction.length;

		if (instruction.type == INSTRUCTION_TYPE_JMP)
		{
			if (instruction.op1.type == OPERAND_TYPE_IMMEDIATE)
			{
				// direct control flow, CF
				boost::int32_t offset = 0;
				get_operand_immediate(&instruction.op1, (LIBDASM_DWORD*)&offset);
				boost::uint32_t target_va = 
					(boost::uint32_t)((boost::int32_t)inst_va + instruction.length + offset);

				gen_targets.push_back(target_va);
			}
			else
			{
				// indirect control flow, ICF
			}

			// jmp "is" end for trace
			break;
		}
		else if (instruction.type == INSTRUCTION_TYPE_JMPC)
		{
			if (instruction.op1.type == OPERAND_TYPE_IMMEDIATE)
			{
				// direct control flow, CF
				boost::int32_t offset = 0;
				get_operand_immediate(&instruction.op1, (LIBDASM_DWORD*)&offset);
				boost::uint32_t target_va = 
					(boost::uint32_t)((boost::int32_t)inst_va + instruction.length + offset);

				gen_targets.push_back(target_va);				
			}
			else
			{
				// indirect control flow, ICF
			}

			// jcc "is not" end for trace	
		}
		else if (instruction.type == INSTRUCTION_TYPE_CALL)
		{
			if (instruction.op1.type == OPERAND_TYPE_IMMEDIATE)
			{
				// direct control flow, CF
				boost::int32_t offset = 0;
				get_operand_immediate(&instruction.op1, (LIBDASM_DWORD*)&offset);
				boost::uint32_t target_va = 
					(boost::uint32_t)((boost::int32_t)inst_va + instruction.length + offset);

				gen_targets.push_back(target_va);
			}
			else
			{
				// indirect control flow, ICF
			}

			gen_targets.push_back(inst_va + instruction.length);
			// call "is" end for trace
			break;
		}
		else if (instruction.type == INSTRUCTION_TYPE_RET)
		{
			// ret "is" end for trace
			break;
		}
		else if (instruction.type == INSTRUCTION_TYPE_LEA)
		{
			if (instruction.op2.type == OPERAND_TYPE_MEMORY &&
				get_operand_basereg(&instruction.op2) == LIBDASM_REGISTER_NOP &&
				get_operand_indexreg(&instruction.op2) == LIBDASM_REGISTER_NOP)
			{
				// lea eax, [0xXXXXXXXX]
				boost::uint32_t disp = 0;
				if (get_operand_displacement(&instruction.op2, (LIBDASM_DWORD*)&disp))
				{
					//LOG_TRACE("structure at [0x%08x]\n", disp);
					lea_targets.insert(disp);
					// and these data structure areas are also padding with 0xCC to align with mask 0xFFFFFFF0
					// because this is data, that any traces intersected with these areas should be invalidate.
					// because trace should defend its own validity.
					// And what about the grey areas, I think it is the same, but
					// grey area is dealing with gaps, and here is dealing with traces.

					// "All things are full of labor, man cannot utter it."
					// "The eye is not satisfied with seeing,"
					// "nor the ear filled with hearing."
					// 
					// "The thing that has been, it is that which shall be;"
					// "The thing that has been done, it is that which shall be done;"
					// "There is nothing new under the sun."
					//								-- Ecclesiastes, Bible
				}
			}
		}

		inst_va += instruction.length;
	}
}
