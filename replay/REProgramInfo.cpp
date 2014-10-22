#include "REProgramInfo.h"
#include "ImageInfo.h"
#include <time.h>
#include "Config.h"


#define FILTER_MAIN_MODULE(entry, addr) ((!CONFIG_ENTRY_MAIN_MODULE(entry) || ImageInfo::IsAddrWithinMainModule(addr)))
#define FILTER_CODE_SECTION(entry, addr) ((!CONFIG_ENTRY_CODE_SECTION(entry) || ImageInfo::IsAddrCode(addr)))
#define FILTER_MAIN_MODULE_AND_CODE_SECTION(entry, addr) (FILTER_MAIN_MODULE(entry, addr) && FILTER_CODE_SECTION(entry, addr))
#define FILTER_CONFIG(entry, addr) (CONFIG_ENTRY_ENABLED(entry) && FILTER_MAIN_MODULE_AND_CODE_SECTION(entry, addr))

#define MALLOC "_malloc"
#define FREE "_free"

/*
 * section: instrument callbacks
 */


// instructions instrument

void Instrument_Syscall(
	void* program,
	boost::uint32_t ins_addr,
	boost::uint32_t size,
	LEVEL_VM::CONTEXT* context,
	boost::uint32_t tid)
{
	((REProgramInfo*)program)->Process_Instrument_Syscall(
		ins_addr, 
		size, 
		PIN_GetContextReg(context, REG_EDX), 
		PIN_GetContextReg(context, REG_ECX), 
		tid);
}

//void Instrument_Trace_Tail(
//					 void* program, 
//					 boost::uint32_t ins_addr, 
//					 LEVEL_VM::CONTEXT* context,
//					 boost::uint32_t tid)
//{
//	((ProgramInfo*)program)->Process_Instrument_Trace_Tail(
//		ins_addr, 
//		PIN_GetContextReg(context, REG_ESP), 
//		tid);
//}

void Instrument_Call(
	void* program, 
	boost::uint32_t ins_addr, 
	boost::uint32_t target, 
	boost::uint32_t size,
	LEVEL_VM::CONTEXT* context,
	boost::uint32_t tid)
{
	((REProgramInfo*)program)->Process_Instrument_Call(
		ins_addr, 
		target, 
		size, 
		PIN_GetContextReg(context, REG_ESP), 
		tid);
}

void Instrument_Branch(
	 void* program, 
	 boost::uint32_t ins_addr, 
	 boost::uint32_t target, 
	 bool taken)
{
	((REProgramInfo*)program)->Process_Instrument_Branch(ins_addr, target, taken);
}

void Instrument_Return(
	void* program, 
	boost::uint32_t ins_addr, 
	boost::uint32_t target, 
	boost::uint32_t ret_disp,
	LEVEL_VM::CONTEXT* context,
	boost::uint32_t tid)
{
	((REProgramInfo*)program)->Process_Instrument_Return(
		ins_addr, 
		target,
		ret_disp, 
		PIN_GetContextReg(context, REG_ESP),
		tid);
}

void Instrument_Normal_Instruction(
	void* program, 
	boost::uint32_t ins_addr, 
	const char* disasm,
	boost::uint32_t len)
{
	((REProgramInfo*)program)->Process_Instrument_Normal_Instruction(ins_addr, disasm, len);
}

void Instrument_BBL(
	void* program,
	/*Block* block,*/
	boost::uint32_t bid,
	LEVEL_VM::CONTEXT* context,
	boost::uint32_t tid
	)
{
	((REProgramInfo*)program)->Process_Instrument_BBL(bid, context, tid);
}

//void Instrument_Lea(void* program, boost::uint32_t ins_addr, boost::uint32_t disp, const char* disasm)
//{
//	((ProgramInfo*)program)->Process_Instrument_Lea(ins_addr, disp, disasm);
//}
//
//void Instrument_Lea_Base(
//	void* program, boost::uint32_t ins_addr, boost::uint32_t base_addr, boost::uint32_t disp, const char* disasm)
//{
//	((ProgramInfo*)program)->Process_Instrument_Lea(ins_addr, base_addr + disp, disasm);
//}
//
//void Instrument_Lea_Base_Index(
//	void* program, 
//	boost::uint32_t ins_addr, 
//	boost::uint32_t base_addr,
//	boost::uint32_t index,
//	boost::uint32_t scale,
//	boost::uint32_t disp,
//	const char* disasm)
//{
//	((ProgramInfo*)program)->Process_Instrument_Lea(ins_addr, base_addr + (scale * index) + disp, disasm);
//}

void Instrument_Read_Before(
	void* program, 
	boost::uint32_t ins_addr,
	boost::uint32_t target,
	boost::uint32_t size,
	bool is_predict,
	const char* disasm)
{
	if (!is_predict)
	{
		// read operation actually happened
		((REProgramInfo*)program)->Process_Instrument_Read_Before(
			ins_addr,
			target,
			size,
			disasm);
	}
}

void Instrument_Write_Before(
	void* program,
	boost::uint32_t ins_addr,
	boost::uint32_t target,
	boost::uint32_t size,
	bool is_predict)
{
	if (!is_predict)
	{
		// write operation actually happened
		((REProgramInfo*)program)->Process_Instrument_Write_Before(
			ins_addr,
			target,
			size);
	}
}

void Instrument_Write_After(
	void* program)
{
	((REProgramInfo*)program)->Process_Instrument_Write_After();
}

//void Instrument_NtCallbackReturn(
//	void* program,
//	boost::uint32_t tid)
//{
//	((ProgramInfo*)program)->Process_Instrument_NtCallbackReturn(tid);
//}
//
//void Instrument_Heap_Alloc_Before(void* program, ADDRINT size)
//{
//	((ProgramInfo*)program)->Process_Instrument_Heap_Alloc_Before(size);
//}
//
//void Instrument_Heap_Alloc_After(void* program, ADDRINT addr)
//{
//	((ProgramInfo*)program)->Process_Instrument_Heap_Alloc_After(addr);
//}
//
//void Instrument_Heap_Free_Before(void* program, ADDRINT addr)
//{
//	((ProgramInfo*)program)->Process_Instrument_Heap_Free_Before(addr);
//}
/* ===================================================================== */



REProgramInfo::REProgramInfo(const std::string& name)
:m_name(name)
,m_write_inst_addr(0)
,m_write_target(0)
,m_write_size(0)
{

}

REProgramInfo::~REProgramInfo(void)
{

}












/*
 * section: encounter callback processing 
 */

void REProgramInfo::Process_Encounter_Insn( INS ins )
{	
	// Read & Write
	if (FILTER_CONFIG(CONFIG_MEM_RD_BEFORE, INS_Address(ins)))
	{
		if (INS_IsMemoryRead(ins))
		{
			std::string* disasm = new std::string(INS_Disassemble(ins));
			// INS_IsPrefetch(ins) means the ins has not be executed at last.
			if (INS_HasMemoryRead2(ins))
			{
				// read 2 operands at the same time
				INS_InsertPredicatedCall(
					ins, IPOINT_BEFORE, (AFUNPTR)Instrument_Read_Before,
					IARG_PTR, this, 
					IARG_INST_PTR,
					IARG_MEMORYREAD2_EA,
					IARG_MEMORYREAD_SIZE,
					IARG_BOOL, INS_IsPrefetch(ins),
					IARG_PTR, disasm->c_str(),
					IARG_END);
			}

			INS_InsertPredicatedCall(
				ins, IPOINT_BEFORE, (AFUNPTR)Instrument_Read_Before,
				IARG_PTR, this, 
				IARG_INST_PTR,
				IARG_MEMORYREAD_EA,
				IARG_MEMORYREAD_SIZE,
				IARG_BOOL, INS_IsPrefetch(ins),
				IARG_PTR, disasm->c_str(),
				IARG_END);

		}	
	}		

	if (INS_IsMemoryWrite(ins))
	{
		if (FILTER_CONFIG(CONFIG_MEM_WR_BEFORE, INS_Address(ins)))
		{
			INS_InsertPredicatedCall(
				ins, IPOINT_BEFORE, (AFUNPTR)Instrument_Write_Before,
				IARG_PTR, this, 
				IARG_INST_PTR,
				IARG_MEMORYWRITE_EA,
				IARG_MEMORYWRITE_SIZE,
				IARG_BOOL, INS_IsPrefetch(ins),
				IARG_END);
		}

		if (FILTER_CONFIG(CONFIG_MEM_WR_AFTER, INS_Address(ins)))
		{
			// if it is write, we should log what it has written
			if (INS_HasFallThrough(ins))
			{
				INS_InsertCall(
					ins, IPOINT_AFTER, (AFUNPTR)Instrument_Write_After,
					IARG_PTR, this, 
					IARG_END);
			}
			if (INS_IsBranchOrCall(ins))
			{
				INS_InsertCall(
					ins, IPOINT_TAKEN_BRANCH, (AFUNPTR)Instrument_Write_After,
					IARG_PTR, this, 
					IARG_END);
			}		
		}
	}	

	/*if (FILTER_CONFIG(CONFIG_LEA, INS_Address(ins)))
	{
		if (INS_IsLea(ins) && INS_OperandIsAddressGenerator(ins, 1))
		{
			std::string* disasm = new std::string(INS_Disassemble(ins));

			REG base_reg = INS_OperandMemoryBaseReg(ins, 1);
			REG index_reg = INS_OperandMemoryIndexReg(ins, 1);
			boost::int32_t displacement = INS_OperandMemoryDisplacement(ins, 1);
			boost::uint32_t scale = INS_OperandMemoryScale(ins, 1);

			if (REG_valid(base_reg))
			{
				if (REG_valid(index_reg))
				{
					INS_InsertCall(
						ins,
						IPOINT_BEFORE,
						AFUNPTR(Instrument_Lea_Base_Index),
						IARG_PTR, this,
						IARG_INST_PTR, 
						IARG_REG_VALUE, base_reg,
						IARG_REG_VALUE, index_reg,
						IARG_ADDRINT, scale,
						IARG_ADDRINT, displacement,
						IARG_PTR, disasm->c_str(),
						IARG_END);
				}
				else
				{
					INS_InsertCall(
						ins,
						IPOINT_BEFORE,
						AFUNPTR(Instrument_Lea_Base),
						IARG_PTR, this,
						IARG_INST_PTR, 
						IARG_REG_VALUE, base_reg,
						IARG_ADDRINT, displacement,
						IARG_PTR, disasm->c_str(),
						IARG_END);
				}
			}
			else
			{
				INS_InsertCall(
					ins,
					IPOINT_BEFORE,
					AFUNPTR(Instrument_Lea),
					IARG_PTR, this,
					IARG_INST_PTR, 
					IARG_ADDRINT, displacement,
					IARG_PTR, disasm->c_str(),
					IARG_END);
			}
		}
	}*/

	// Control Flow
	if (INS_IsBranch(ins))
	{
		if (FILTER_CONFIG(CONFIG_BRANCH, INS_Address(ins)))
		{
			INS_InsertCall(
				ins, 
				IPOINT_BEFORE, 
				AFUNPTR(Instrument_Branch),
				IARG_PTR, this, 
				IARG_INST_PTR , 
				IARG_BRANCH_TARGET_ADDR, 
				IARG_BRANCH_TAKEN, 
				IARG_END);
		}
	}
	else if (INS_IsCall(ins))
	{
		if (FILTER_CONFIG(CONFIG_CALL, INS_Address(ins)) ||
			FILTER_CONFIG(CONFIG_XIMG_BOUNDARY, INS_Address(ins)))
		{
			INS_InsertCall(
				ins, 
				IPOINT_BEFORE, 
				AFUNPTR(Instrument_Call),
				IARG_PTR, this, 
				IARG_INST_PTR , 
				IARG_BRANCH_TARGET_ADDR, 
				IARG_UINT32, INS_Size(ins),
				IARG_CONST_CONTEXT,
				IARG_UINT32, IARG_THREAD_ID, 
				IARG_END);
		}
	}
	else if (INS_IsRet(ins))
	{
		if (FILTER_CONFIG(CONFIG_RETURN, INS_Address(ins)) ||
			FILTER_CONFIG(CONFIG_XIMG_BOUNDARY, INS_Address(ins)))
		{
			boost::uint32_t ret_disp = 0;

			if (INS_OperandCount(ins) > 0 && INS_OperandIsImmediate(ins, 0))
			{
				ret_disp = INS_OperandImmediate(ins, 0);
			}

			INS_InsertCall(
				ins, 
				IPOINT_BEFORE, 
				AFUNPTR(Instrument_Return),
				IARG_PTR, this, 
				IARG_INST_PTR , 
				IARG_BRANCH_TARGET_ADDR, 
				IARG_UINT32, ret_disp,
				IARG_CONST_CONTEXT,
				IARG_UINT32, IARG_THREAD_ID,  
				IARG_END);
		}
	}
	else if (INS_IsSyscall(ins))
	{
		// if(FILTER_CONFIG())
		{
			INS_InsertCall(
				ins, 
				IPOINT_BEFORE, 
				AFUNPTR(Instrument_Syscall),
				IARG_PTR, this, 
				IARG_INST_PTR, 
				IARG_UINT32, INS_Size(ins),
				IARG_CONST_CONTEXT,
				IARG_UINT32, IARG_THREAD_ID, 
				IARG_END);
		}
	}

	if (FILTER_CONFIG(CONFIG_INSN_NOMRAL, INS_Address(ins)))
	{
		std::string* disasm = new std::string(INS_Disassemble(ins));
		INS_InsertCall(
			ins,
			IPOINT_BEFORE,
			AFUNPTR(Instrument_Normal_Instruction),
			IARG_PTR, this,
			IARG_INST_PTR, 
			IARG_PTR, disasm->c_str(),
			IARG_UINT32, INS_Size(ins),
			IARG_END);
	}
}

void REProgramInfo::Process_Encounter_Image( IMG img )
{
	ImageInfo::AppendImage(img);

	/*if (FILTER_CONFIG(CONFIG_HEAP_TRACE, IMG_StartAddress(img)))
	{
		RTN mallocRtn = RTN_FindByName(img, MALLOC);
		if (RTN_Valid(mallocRtn))
		{
			RTN_Open(mallocRtn);
			RTN_InsertCall(mallocRtn, 
				IPOINT_BEFORE, 
				(AFUNPTR)Instrument_Heap_Alloc_Before, 
				IARG_PTR, this,
				IARG_G_ARG0_CALLEE,
				IARG_END);

			RTN_InsertCall(mallocRtn, 
				IPOINT_AFTER, 
				(AFUNPTR)Instrument_Heap_Alloc_After, 
				IARG_PTR, this,
				IARG_G_RESULT0, 
				IARG_END);
			RTN_Close(mallocRtn);
		}

		RTN freeRtn = RTN_FindByName(img, FREE);
		if (RTN_Valid(freeRtn))
		{
			RTN_Open(freeRtn);
			RTN_InsertCall(freeRtn, 
				IPOINT_BEFORE, 
				(AFUNPTR)Instrument_Heap_Free_Before,
				IARG_PTR, this,
				IARG_G_ARG0_CALLEE, 
				IARG_END);
			RTN_Close(freeRtn);
		}
	}*/
}

void REProgramInfo::Process_Encounter_Rtn( RTN rtn )
{
	//if (RTN_Valid(rtn) && 
	//	RTN_Name(rtn).compare("NtCallbackReturn") == 0)
	//{
	//	RTN_Open(rtn);
	//	// we just notify ShadowStack that the next filterReturn 
	//	// should first pop the latest 3 stack frames off the vector.
	//	RTN_InsertCall(rtn, 
	//		IPOINT_BEFORE, 
	//		(AFUNPTR)Instrument_NtCallbackReturn,
	//		IARG_PTR, this,
	//		IARG_UINT32, IARG_THREAD_ID,  
	//		IARG_END);
	//	RTN_Close(rtn);
	//}

	//if (RTN_Valid(rtn) && 
	//	RTN_Name(rtn).compare("VirtualAllocEx") == 0)
	//{
	//	RTN_Open(rtn);
	//	// we just notify ShadowStack that the next filterReturn 
	//	// should first pop the latest 3 stack frames off the vector.
	//	RTN_InsertCall(rtn, 
	//		IPOINT_BEFORE, 
	//		(AFUNPTR)Instrument_NtCallbackReturn,
	//		IARG_PTR, this,
	//		IARG_UINT32, IARG_THREAD_ID,  
	//		IARG_END);
	//	RTN_Close(rtn);
	//}
}

void REProgramInfo::Process_Encounter_Trace( TRACE trace )
{
	if (TRACE_NumBbl(trace) != 0 &&
		ImageInfo::IsAddrWithinMainModule(TRACE_Address(trace)))
	{
		// Use the first Block id(Bid) to represent Trace id.
		boost::uint32_t tid = 0;
		for( BBL bbl = TRACE_BblHead(trace); BBL_Valid(bbl); bbl = BBL_Next(bbl) )
		{
			// Construct a BBL static representation, i.e. disasm
			// Add instrument to capture dynamic BBL representation, i.e. Context at entry, and stack snapshot.
			if (BBL_Valid(bbl))
			{
				//Block* block = new Block();
				Block block(BBL_Address(bbl));

				if (tid == 0)
				{
					BBL_InsertCall(bbl, 
						IPOINT_BEFORE, 
						(AFUNPTR)Instrument_BBL,
						IARG_PTR, this,
						IARG_UINT32, block.GetBid(),
						IARG_CONST_CONTEXT,
						IARG_UINT32, IARG_THREAD_ID,  
						IARG_END);

					tid = block.GetBid();
				}

				for (INS ins = BBL_InsHead(bbl); INS_Valid(ins) && ins != BBL_InsTail(bbl); ins = INS_Next(ins))
				{
					block.AddDisasmLine(INS_Disassemble(ins).c_str());

					char raw_bytes[64] = {0,};
					for (boost::uint32_t i =0;i<INS_Size(ins);i++)
					{
						sprintf(raw_bytes+(3*i), "%02X ", *((boost::uint8_t*)(INS_Address(ins) + i)));
					}
					block.AddRawLine(raw_bytes);
				}

				block.AddDisasmLine(INS_Disassemble(BBL_InsTail(bbl)).c_str());	
				char raw_bytes[64] = {0,};
				for (boost::uint32_t i =0;i<INS_Size(BBL_InsTail(bbl));i++)
				{
					sprintf(raw_bytes+(3*i), "%02X ", *((boost::uint8_t*)(INS_Address(BBL_InsTail(bbl)) + i)));
				}
				block.AddRawLine(raw_bytes);
				block.SetTid(tid);
				block.Serialize();
				//Block::s_blocks.push_back(block);
			}
		}
	}
}

#define CIRCULAR_LENGTH 10
#define ESP_JUMP_RANGE	128
boost::uint32_t circuluar[CIRCULAR_LENGTH] = {0,};
boost::uint32_t square_circular[CIRCULAR_LENGTH/2] = {0,};
boost::uint32_t last_ebp = 0;
boost::uint32_t last_esp = 0;
void REProgramInfo::Process_Instrument_BBL(boost::uint32_t bid,/*Block* block,*/ LEVEL_VM::CONTEXT* context, boost::uint32_t tid )
{
	//Frame* frame = new Frame(bid);
	Frame frame(bid);

	//block->AddFrame(frame->GetId());

	frame.m_context.m_eip = PIN_GetContextReg(context, REG_EIP);
	frame.m_context.m_esp = PIN_GetContextReg(context, REG_ESP);
	frame.m_context.m_ebp = PIN_GetContextReg(context, REG_EBP);

	boost::uint32_t eip = frame.m_context.m_eip;
	boost::uint32_t ebp = frame.m_context.m_ebp;
	boost::uint32_t esp = frame.m_context.m_esp;

	bool bSkip = false;
	/***********************************************
	*	Skip Strategy 1. skip circular
	************************************************/
	// Calculate circular length according to Eip
	// If circular segment length is less than N, skip it.

	for (int i=0;i<CIRCULAR_LENGTH ;i++)
	{
		if (circuluar[i] == eip)
		{
			bSkip = true;
		}

		circuluar[i] = circuluar[(i+1) % CIRCULAR_LENGTH];
	}

	circuluar[CIRCULAR_LENGTH-1] = eip;

	if (!bSkip)
	{
		for (int i=0;i<CIRCULAR_LENGTH/2 ;i++)
		{
			if (square_circular[i] == eip)
			{
				bSkip = true;
			}
		}
	}


	/***********************************************
	*	Skip Strategy 2. skip replicate stack frame
	************************************************/
	if (last_ebp == ebp && (last_ebp - ebp) < ESP_JUMP_RANGE)
	{
		bSkip = true;
	}

	//if ((last_esp < esp) ||
	//	(last_esp - esp < ESP_JUMP_RANGE))
	//{
	//	bSkip = false;
	//}

	last_ebp = ebp;

	if (!bSkip)
	{
		for (int i=0;i<CIRCULAR_LENGTH/2 && eip != 0;i++)
		{
			square_circular[i] = square_circular[(i+1) % (CIRCULAR_LENGTH/2)];
		}
		square_circular[(CIRCULAR_LENGTH/2)-1] = eip;
	}
	else //(bSkip)
	{
		return;
	}

	frame.m_context.m_eax = PIN_GetContextReg(context, REG_EAX);
	frame.m_context.m_ebx = PIN_GetContextReg(context, REG_EBX);
	frame.m_context.m_ecx = PIN_GetContextReg(context, REG_ECX);
	frame.m_context.m_edx = PIN_GetContextReg(context, REG_EDX);
	
	

	frame.m_context.m_esi = PIN_GetContextReg(context, REG_ESI);
	frame.m_context.m_edi = PIN_GetContextReg(context, REG_EDI);

	boost::uint32_t* stack = (boost::uint32_t*)frame.m_context.m_esp;

	for (int i=0 ; i< RE_STACK_SLOTS_NUM; i++)
	{
		frame.m_stack.m_slots[i] = *(stack + i);
	}

	frame.Serialize();

	//Frame::s_frames.push_back(frame);

	// First obtain a timeline id.
	// Then extract context registers.
	// Then make a snapshot of stack.
}

//void ProgramInfo::Process_Instrument_Lea( boost::uint32_t ins_addr, boost::uint32_t target, const char* disasm )
//{
//	if (ImageInfo::IsAddrCode(target) &&
//		ImageInfo::IsAddrWithinMainModule(target))
//	{
//		LOQ(CONFIG_LEA,
//			"[0x%08x]: %16s [0x%08x] %s\n",
//			ins_addr,
//			"lea",
//			target,
//			disasm);
//	}
//}
//
//// This is not a perfect solution to find out all heap allocation.
//// Actually there are 2 granularity heap allocation,
//// 1. HeapAlloc, alloc a heap once
//// 2. malloc is dependent on run-time library, such as msvcrt.dll
//
//
//void ProgramInfo::Process_Instrument_Heap_Alloc_Before( boost::uint32_t size )
//{
//	LOQ(CONFIG_HEAP_TRACE,
//		"%10s size [0x%08x]\n",
//		"alloc",
//		size);
//}
//
//void ProgramInfo::Process_Instrument_Heap_Alloc_After( boost::uint32_t addr )
//{
//	LOQ(CONFIG_HEAP_TRACE,
//		"--> %10s addr [0x%08x]\n",
//		"buffer",
//		addr);
//}
//
//void ProgramInfo::Process_Instrument_Heap_Free_Before( boost::uint32_t addr )
//{
//	LOQ(CONFIG_HEAP_TRACE,
//		"--> %10s addr [0x%08x]\n",
//		"free",
//		addr);
//}
//
//void ProgramInfo::Process_Instrument_NtCallbackReturn(boost::uint32_t tid)
//{
//	tid = GetTid(tid);
//	ShadowStack::notifyNtCallbackReturn(tid);
//}
//
//std::set<boost::uint32_t> tids;
//boost::uint32_t ProgramInfo::GetTid( boost::uint32_t original_tid )
//{
//	boost::uint32_t tid = GetCurrentThreadId();
//	tids.insert(tid);
//	if (tids.size() > 1)
//	{
//		for (std::set<boost::uint32_t>::iterator it = tids.begin();
//			it != tids.end();
//			it++)
//		{
//			LOQ(CONFIG_INSN_NOMRAL, "[TID] : %d\n", *it);
//		}
//	}
//	//LOQ(CONFIG_INSN_NOMRAL, "[TID]: 0x%08x, from [0x%08x]\n", GetCurrentThreadId(), original_tid);
//	return tid;
//}
//
//void ProgramInfo::Process_Instrument_Trace_Tail(
//	boost::uint32_t ins_addr, boost::uint32_t esp, boost::uint32_t tid )
//{
//	LOQ(CONFIG_TRACE_TAIL, "[Tid: 0x%08x] [Stack Pointer: 0x%08x] : 0x%08x\n", tid, esp, ins_addr);
//}

void REProgramInfo::Process_Encounter_Finalize()
{	
	Logger::Finalize();
}











/*
 * section: instrument callback processing
 */


void REProgramInfo::Process_Instrument_Call(
	boost::uint32_t ins_addr, 
	boost::uint32_t target, 
	boost::uint32_t size,
	boost::uint32_t esp,
	boost::uint32_t tid)
{
	//tid = GetTid(tid);
	//if (CONFIG_ENTRY_ENABLED(CONFIG_XIMG_BOUNDARY) ||
	//	CONFIG_ENTRY_ENABLED(CONFIG_CALL))
	//{
	//	ShadowStack::filterCall(ins_addr, target, size, esp, tid);
	//}

	//if (CONFIG_ENTRY_ENABLED(CONFIG_CALL))
	//{
	//	LOQ(CONFIG_CALL,
	//		"[0x%08x]: %10s [0x%08x] %s\n",			
	//		ins_addr,
	//		"call",
	//		target,
	//		ImageInfo::GetRtnName(target).c_str());
	//}

	if (ImageInfo::IsAddrWithinMainModule(ins_addr) &&
		!ImageInfo::verifyControlFlowTarget(target))
	{
		// we can dump the memory and disasm around ins_addr and target.
		LOQ(CONFIG_ABNORMAL_CF,
			"Invalid Call from [0x%08x] to [0x%08x]\n",
			ins_addr, 
			target);

		ImageInfo::DumpBinaryAround(ins_addr, 0x80);
		ImageInfo::DumpBinaryAround(target, 0x80);

		ImageInfo::DumpDisasmAround(ins_addr);

		ExitProcess();
	}
}

void REProgramInfo::Process_Instrument_Syscall( 
	boost::uint32_t ins_addr, 
	boost::uint32_t size,
	boost::uint32_t edx, 
	boost::uint32_t ecx, 
	boost::uint32_t tid )
{
	//LOQ(CONFIG_INSN_NOMRAL, "[0x%08x] sysenter eip in edx : [0x%08x], esp in ecx : [0x%08x]\n",
	//	ins_addr,
	//	edx,
	//	ecx);
}

void REProgramInfo::Process_Instrument_Branch( boost::uint32_t ins_addr, boost::uint32_t target, bool taken)
{
	if (taken &&
		CONFIG_ENTRY_ENABLED(CONFIG_BRANCH))
	{
		LOQ(CONFIG_BRANCH,
			"[0x%08x]: %10s loc_0x%08x\n",			
			ins_addr,
			"branch",
			target);
	}

	if (taken &&
		ImageInfo::IsAddrWithinMainModule(ins_addr) &&
		!ImageInfo::verifyControlFlowTarget(target))
	{
		LOQ(CONFIG_ABNORMAL_CF,
			"Invalid Branch from [0x%08x] to [0x%08x]\n",
			ins_addr, 
			target);

		ImageInfo::DumpBinaryAround(ins_addr, 0x80);
		ImageInfo::DumpBinaryAround(target, 0x80);

		ImageInfo::DumpDisasmAround(ins_addr);

		ExitProcess();
	}
}

void REProgramInfo::Process_Instrument_Return( 
	boost::uint32_t ins_addr, 
	boost::uint32_t target, 
	boost::uint32_t ret_disp,
	boost::uint32_t esp,
	boost::uint32_t tid)
{
	//if (CONFIG_ENTRY_ENABLED(CONFIG_RETURN))
	//{
	//	LOQ(CONFIG_RETURN,
	//		"[0x%08x]: %10s loc_0x%08x\n",			
	//		ins_addr,
	//		"ret",
	//		target);
	//}
	//tid = GetTid(tid);
	//ShadowStack::filterReturn(ins_addr, target, ret_disp, esp, tid);
}


void REProgramInfo::Process_Instrument_Normal_Instruction( 
	boost::uint32_t ins_addr, 
	const char* disasm,
	boost::uint32_t len)
{
	char raw_bytes[64] = {0,};
	for (boost::uint32_t i =0;i<len;i++)
	{
		sprintf(raw_bytes+(3*i), "%02X ", *((boost::uint8_t*)(ins_addr + i)));
	}

	LOQ(CONFIG_INSN_NOMRAL,
		"0x%08x : %30s\t\t%s\n",
		ins_addr,
		raw_bytes,
		disasm);
}

void REProgramInfo::ExitProcess()
{
	if (!PIN_IsProcessExiting())
	{
		PIN_ExitApplication(0);
	}
}

void REProgramInfo::Process_Instrument_Read_Before( 
	boost::uint32_t ins_addr, boost::uint32_t target, boost::uint32_t size , const char* disasm)
{
	if (ImageInfo::IsAddrCode(target))
	{
		if (size <= 4)
		{
			boost::uint32_t value = 0;
			memcpy(&value, ((boost::uint8_t*)target), size);
			LOQ(CONFIG_MEM_RD_BEFORE,
				"[0x%08x]: %16s [0x%08x] %d bytes [0x%08x], %s\n", 
				ins_addr,
				"Read from",
				target,
				size,
				value, 
				disasm);
		}
		else
		{
			LOQ(CONFIG_MEM_RD_BEFORE,
				"[0x%08x]: %16s [0x%08x] %d bytes -- %s\n", 
				ins_addr,
				"Read from",
				target,
				size,
				disasm);

			ImageInfo::DumpBinaryAround(target, size);
		}	
	}	
}

void REProgramInfo::Process_Instrument_Write_Before( 
	boost::uint32_t ins_addr, boost::uint32_t target, boost::uint32_t size )
{
	m_write_inst_addr = ins_addr;
	m_write_target = target;
	m_write_size = size;

	if (ImageInfo::IsAddrCode(target))
	{
		if (size <= 4)
		{
			boost::uint32_t value = 0;
			memcpy(&value, ((boost::uint8_t*)target), size);
			LOQ(CONFIG_MEM_WR_BEFORE,
				"[0x%08x]: %16s [0x%08x] %d bytes [0x%08x]\n", 
				ins_addr,
				"Before Write to",
				target,
				size,
				value);
		}
		else
		{
			LOQ(CONFIG_MEM_WR_BEFORE,
				"[0x%08x]: %16s [0x%08x] %d bytes --\n", 
				ins_addr,
				"Before Write to",
				target,
				size);

			ImageInfo::DumpBinaryAround(target, size);
		}	
	}	
}

void REProgramInfo::Process_Instrument_Write_After()
{
	if (ImageInfo::IsAddrCode(m_write_target))
	{
		if (m_write_size <= 4)
		{
			boost::uint32_t value = 0;
			memcpy(&value, ((boost::uint8_t*)m_write_target), m_write_size);
			LOQ(CONFIG_MEM_WR_AFTER,
				"[0x%08x]: %16s [0x%08x] %d bytes [0x%08x]\n", 
				m_write_inst_addr,
				"After Write to",
				m_write_target,
				m_write_size,
				value);
		}
		else
		{
			LOQ(CONFIG_MEM_WR_AFTER,
				"[0x%08x]: %16s [0x%08x] %d bytes --\n", 
				m_write_inst_addr,
				"After Write to",
				m_write_target,
				m_write_size);

			ImageInfo::DumpBinaryAround(m_write_target, m_write_size);
		}	
	}	
}
