#pragma once

#include <string>

#include "boost/cstdint.hpp"
#include "boost/algorithm/string.hpp"

#include "pin.H"
#include "portability.H"

#include "Logger.h"
#include "Block.h"
#include "Frame.h"

#include "ShadowStack.h"

class REProgramInfo
{
public:
	REProgramInfo(const std::string& name);
	~REProgramInfo(void);

	// Encounter callback processing
	void Process_Encounter_Insn(INS ins);
	void Process_Encounter_Image(IMG img);
	void Process_Encounter_Rtn(RTN rtn);
	void Process_Encounter_Trace(TRACE trace);
	void Process_Encounter_Finalize();


	// Instrument callback processing
	void Process_Instrument_Call(
		boost::uint32_t ins_addr, 
		boost::uint32_t target, 
		boost::uint32_t size,
		boost::uint32_t esp,
		boost::uint32_t tid);
	void Process_Instrument_Syscall(
		boost::uint32_t ins_addr, 
		boost::uint32_t size,
		boost::uint32_t edx,
		boost::uint32_t ecx,
		boost::uint32_t tid);
	void Process_Instrument_Branch(boost::uint32_t ins_addr, boost::uint32_t target, bool taken);
	void Process_Instrument_Return(
		boost::uint32_t ins_addr, 
		boost::uint32_t target, 
		boost::uint32_t ret_disp,
		boost::uint32_t esp,
		boost::uint32_t tid);

	//void Process_Instrument_Trace_Tail(
	//	boost::uint32_t ins_addr, 
	//	boost::uint32_t esp,
	//	boost::uint32_t tid);

	void Process_Instrument_BBL(
		/*Block* block,*/
		boost::uint32_t bid,
		LEVEL_VM::CONTEXT* context,
		boost::uint32_t tid);

	void Process_Instrument_Normal_Instruction(
		boost::uint32_t ins_addr, 
		const char* disasm,
		boost::uint32_t len);

	void Process_Instrument_Read_Before(boost::uint32_t ins_addr, boost::uint32_t target, boost::uint32_t size, const char* disasm);
	void Process_Instrument_Write_Before(boost::uint32_t ins_addr, boost::uint32_t target, boost::uint32_t size);
	void Process_Instrument_Write_After();

	//void Process_Instrument_Lea(boost::uint32_t ins_addr, boost::uint32_t target, const char* disasm);

	//void Process_Instrument_Heap_Alloc_Before(boost::uint32_t size);
	//void Process_Instrument_Heap_Alloc_After(boost::uint32_t addr);
	//void Process_Instrument_Heap_Free_Before(boost::uint32_t addr);

	//void Process_Instrument_NtCallbackReturn(boost::uint32_t tid);

	void ExitProcess();

	//boost::uint32_t GetTid(boost::uint32_t original_tid);

private:
	std::string m_name;

	boost::uint32_t m_write_inst_addr;
	boost::uint32_t m_write_target;
	boost::uint32_t m_write_size;
};
