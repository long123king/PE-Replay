#include "boost/algorithm/string.hpp"
#include "ShadowStack.h"


void ShadowStack::filterCall(
	boost::uint32_t ins_addr, 
	boost::uint32_t target_addr, 
	boost::uint32_t size,
	boost::uint32_t esp,
	boost::uint32_t tid)
{
	if (s_mapThreadStack.find(tid) == s_mapThreadStack.end())
	{
		s_mapThreadStack[tid] = new ThreadStack();
	}

	//if (target_addr == 0x77c35de5)
	//{
	//	int kkk = 0;
	//}
	ThreadStack* stack = s_mapThreadStack[tid];

	stack->Push(target_addr, ins_addr + size, ins_addr, esp);
	//LOQ(CONFIG_CALL, "call %s\n", ImageInfo::GetAddrLocationHint(target_addr).c_str());

	//displayStackFrames();

	//LOQ(CONFIG_CALL,
	//	"\n************************************\n");
	//LOQ(CONFIG_CALL,
	//	"[0x%08x] --> [0x%08x]\n", ins_addr, target_addr);
	//LOQ(CONFIG_CALL,
	//	"\n************************************\n");

	//if (ImageInfo::IsAddrWithinMainModule(ins_addr) &&
	//	!ImageInfo::IsAddrWithinMainModule(target_addr))
	{
		// main module call other image routines
		LOQ(CONFIG_XIMG_BOUNDARY,
			"[0x%08x] %10s [0x%08x]%s\n",
			ins_addr,
			"call",
			target_addr,
			ImageInfo::GetAddrLocationHint(target_addr).c_str());
	}
}

void ShadowStack::filterBranch( boost::uint32_t ins_addr, boost::uint32_t target_addr )
{
	
}

void ShadowStack::filterReturn(
	boost::uint32_t ins_addr, 
	boost::uint32_t target_addr, 
	boost::uint32_t ret_disp,
	boost::uint32_t esp,
	boost::uint32_t tid)
{
	//if (ImageInfo::IsAddrWithinMainModule(target_addr) &&
	//	!ImageInfo::IsAddrWithinMainModule(ins_addr))
	{
		LOQ(CONFIG_XIMG_BOUNDARY,
			"[0x%08x] %10s [0x%08x]%s\n",
			target_addr,
			"ret",
			ins_addr,
			ImageInfo::GetAddrLocationHint(ins_addr).c_str());
	}

	if (s_mapThreadStack.find(tid) == s_mapThreadStack.end())
	{
		//LOQ(CONFIG_CALL, "Error: stack doesn't exists for thread %d\n", tid);
		return;
	}

	ThreadStack* stack = s_mapThreadStack[tid];

	//LOQ(CONFIG_RETURN,
	//	"\n************************************\n");
	//LOQ(CONFIG_RETURN,
	//	"        [0x%08x] <-- [0x%08x]\n", target_addr, ins_addr);
	//LOQ(CONFIG_RETURN,
	//	"\n************************************\n");
	if (!stack->Empty())
	{
		
		StackFrame item = stack->Peek();
		
		//LOQ(CONFIG_RETURN, "\n#########################################\nFrames:\n");
		//LOQ(CONFIG_RETURN,"0x%08x --> 0x%08x\n", item._return_addr, item._routine_addr);
		//LOQ(CONFIG_RETURN,"0x%08x <-- 0x%08x\n", target_addr, ins_addr);
		//LOQ(CONFIG_RETURN, "ret 0x%x\n", ret_disp);
		//displayStackFrames();		

		if (item._return_addr == target_addr/* &&
			item._stack_addr == esp + 4 */
			//&& 
			//ValidRoutineRange(item._routine_addr, ins_addr)
			)
		{
			//LOG_INS("Valid call from [0x%08x] to [0x%08x], and return from [0x%08x] to [0x%08x]\n",
			//	item._return_addr,
			//	item._routine_addr,
			//	ins_addr,
			//	target_addr);

			//LOQ(CONFIG_RETURN, "%16s\t[0x%08x] : 0x%08x --> 0x%08x\n", 
			//	"call",
			//	item._stack_addr, 
			//	item._call_addr, 
			//	item._routine_addr);
			//LOQ(CONFIG_RETURN, "%16s\t[0x%08x] : 0x%08x <-- 0x%08x\n", 
			//	"return",
			//	esp, 
			//	target_addr,
			//	ins_addr);
			//stack->Display();
			//LOQ(CONFIG_RETURN, "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");
		}
		else if (stack->ReturnToUpperFrame(target_addr, esp))
		{
			//LOQ(CONFIG_RETURN, "%16s\t[0x%08x] : 0x%08x --> 0x%08x\n", 
			//	"call",
			//	item._stack_addr, 
			//	item._call_addr, 
			//	item._routine_addr);
			//LOQ(CONFIG_RETURN, "%16s\t[0x%08x] : 0x%08x <-- 0x%08x\n", 
			//	"return",
			//	esp, 
			//	target_addr,
			//	ins_addr);
			//stack->Display();
			//LOQ(CONFIG_RETURN, "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");
			return;
		}
		else
		{
			// process KiFastSystemCallRet
			//if (boost::contains(
			//	ImageInfo::GetAddrLocationHint(ins_addr), "KiFastSystemCallRet") &&
			//	stack->SkipCallback(target_addr, ins_addr, esp))
			//{
			//	return;
			//}

			

			std::string ret_hint = ImageInfo::GetAddrLocationHint(ins_addr);
			std::string target_hint = ImageInfo::GetAddrLocationHint(target_addr);
			//boost::uint32_t ret_addr_on_stack = 0;//*((boost::uint32_t*)esp);
			//boost::uint32_t call_addr_on_stack = 0;//*((boost::uint32_t*)(item._stack_addr-4));
			//ImageInfo::DumpDisasmAround(CONFIG_RETURN, item._return_addr, 40);
			//ImageInfo::DumpDisasmAround(CONFIG_RETURN, target_addr, 40);
			LOQ(CONFIG_RETURN, "\n#########################################\n");
			LOQ(CONFIG_RETURN, "Error: CFI(Control Flow Integrity) violation ! \n\n");
			LOQ(CONFIG_RETURN, "%16s\t[0x%08x] : 0x%08x --> 0x%08x\n", 
				"call",
				item._stack_addr, 
				item._call_addr, 
				item._routine_addr);
			LOQ(CONFIG_RETURN, "%16s\t[0x%08x] : 0x%08x <-- 0x%08x\n", 
				"return",
				esp, 
				target_addr,
				ins_addr);
			//LOQ(CONFIG_RETURN, "*(0x%08x) : 0x%08x\n", item._stack_addr-4, call_addr_on_stack);
			//LOQ(CONFIG_RETURN, "*(0x%08x) : 0x%08x\n", esp, ret_addr_on_stack);
			LOQ(CONFIG_RETURN, "\n");
			////LOQ(CONFIG_RETURN, "ret 0x%x\n", ret_disp);
			//ImageInfo::DumpDisasmAround(CONFIG_RETURN, item._routine_addr,
			//	ins_addr - item._routine_addr > 40? 40: ins_addr - item._routine_addr);

			LOQ(CONFIG_CALL, "\tThread %d:\n", tid);
			stack->Display();
		}

		// no matter what happens, we just do as told to detach the last frame.
		stack->Pop();
	}

	//LOG_INS("Invalid return from [0x%08x] to [0x%08x]\n",
	//	ins_addr,
	//	target_addr);
}

bool ShadowStack::ValidRoutineRange( boost::uint32_t rtn_start, boost::uint32_t rtn_end )
{
	return (rtn_start ^ rtn_end) < 0x00010000;
	// 4KB = 4 * 1024 = 2 ^ (2 + 10) = 0x00001000
	//// The routine may really cross big range

}

bool ShadowStack::ValidReturnRange( boost::uint32_t ret_start, boost::uint32_t ret_end )
{
	return (ret_start ^ ret_end) < 0x00000010;
}

void ShadowStack::notifyNtCallbackReturn(boost::uint32_t tid)
{
	if (s_mapThreadStack.find(tid) == s_mapThreadStack.end())
	{
		//LOQ(CONFIG_CALL, "Error: stack doesn't exists for thread %d\n", tid);
		return;
	}

	ThreadStack* stack = s_mapThreadStack[tid];	
	stack->NotifyNtCallbackReturn();
}


std::map<boost::uint32_t, ThreadStack*> ShadowStack::s_mapThreadStack;


