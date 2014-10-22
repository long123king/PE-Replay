#pragma once

#include <vector>
#include <map>

#include "boost/cstdint.hpp"

#include "Logger.h"
#include "ImageInfo.h"

class StackFrame
{
public:
	boost::uint32_t _routine_addr;
	boost::uint32_t _return_addr;
	boost::uint32_t _call_addr;
	boost::uint32_t _stack_addr;
	std::string		_routine_name;

	StackFrame(boost::uint32_t rtn_addr, 
		boost::uint32_t ret_addr,
		boost::uint32_t call_addr,
		boost::uint32_t esp)
		:_routine_addr(rtn_addr)
		,_return_addr(ret_addr)
		,_call_addr(call_addr)
		,_stack_addr(esp)
	{
		_routine_name = ImageInfo::GetAddrLocationHint(_routine_addr);
	}
	StackFrame()
		:_routine_addr(0)
		,_return_addr(0)
		,_call_addr(0)
		,_stack_addr(0)
		,_routine_name("")
	{

	}
};

class ThreadStack
{
public:
	ThreadStack()
		:m_bNtCallbackReturn(false)
	{

	}

	~ThreadStack()
	{
		m_frames.clear();
	}

	void Push(boost::uint32_t rtn_addr, 
		boost::uint32_t ret_addr,
		boost::uint32_t call_addr,
		boost::uint32_t esp)
	{
		m_frames.push_back(
			StackFrame(rtn_addr, ret_addr, call_addr, esp));
	}

	StackFrame Peek()
	{
		if (m_bNtCallbackReturn)
		{
			if (m_frames.size() < 3)
			{
				//LOQ(CONFIG_ERROR, "NtCallbackReturn invoked with empty stack!\n");
				m_bNtCallbackReturn = false;
				throw std::exception("NtCallbackReturn invoked with empty stack!");
				//return;
			}
			else
			{
				m_frames.pop_back();
				m_frames.pop_back();
				m_frames.pop_back();
				m_bNtCallbackReturn = false;
			}
		}
		return *(m_frames.rbegin());
	}

	bool SkipCallback(boost::uint32_t return_addr, boost::uint32_t routine_addr, boost::uint32_t stack_addr)
	{
		// This logic still not satisfactory
		//for (std::vector<StackFrame>::reverse_iterator it=m_frames.rbegin();
		//	it!=m_frames.rend();
		//	it++)
		//{
		//	if (it->_return_addr == return_addr &&
		//		it->_stack_addr == (stack_addr + 4) &&
		//		boost::contains(it->_routine_name, "KiFastSystemCall"))
		//	{
		//		m_frames.resize((it - m_frames.rbegin()));
		//		return true;
		//	}
		//}
		return false;
	}

	bool ReturnToUpperFrame(boost::uint32_t return_addr, boost::uint32_t stack_addr)
	{
		for (std::vector<StackFrame>::reverse_iterator it=m_frames.rbegin();
			it!=m_frames.rend();
			it++)
		{
			if (it->_return_addr == return_addr &&
				it->_stack_addr == (stack_addr + 4))
			{
				//Display();
				//LOQ(CONFIG_CALL, "call to upper frame, return_addr: 0x%08x, stack_addr: 0x%08x\n",
				//	return_addr, stack_addr);
				m_frames.resize((m_frames.size() - (it - m_frames.rbegin() + 1)));
				//Display();
				//LOQ(CONFIG_CALL, "~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");
				return true;
			}
		}
		return false;
	}

	void Pop()
	{
		m_frames.pop_back();
	}

	void Display()
	{		
		for (boost::uint32_t i=0;
			i < m_frames.size();
			i++)
		{
			if (i == (m_frames.size() - 1))
			{
				LOQ(CONFIG_CALL, "--> ");
			}
			else
			{
				LOQ(CONFIG_CALL,"    ");
			}
			LOQ(CONFIG_CALL, "[#%8d] [0x%08x]:[0x%08x - 0x%08x] ==> [0x%08x] %s\n",
				i, 
				m_frames.at(i)._stack_addr,
				m_frames.at(i)._call_addr,
				m_frames.at(i)._return_addr,
				m_frames.at(i)._routine_addr,
				m_frames.at(i)._routine_name.c_str());
		}
	}

	void NotifyNtCallbackReturn()
	{
		m_bNtCallbackReturn = true;
	}

	bool Empty()
	{
		return m_frames.empty();
	}

private:
	std::vector<StackFrame> m_frames;
	bool m_bNtCallbackReturn;
};

class ShadowStack
{
public:
	static void filterCall(
		boost::uint32_t ins_addr, 
		boost::uint32_t target_addr,
		boost::uint32_t size,
		boost::uint32_t esp,
		boost::uint32_t tid);
	static void filterBranch(boost::uint32_t ins_addr, boost::uint32_t target_addr);
	static void filterReturn(
		boost::uint32_t ins_addr, 
		boost::uint32_t target_addr,
		boost::uint32_t ret_disp,
		boost::uint32_t esp,
		boost::uint32_t tid);
	static void notifyNtCallbackReturn(boost::uint32_t tid);
	
private:
	static bool ValidRoutineRange(boost::uint32_t rtn_start, boost::uint32_t rtn_end);
	static bool ValidReturnRange(boost::uint32_t ret_start, boost::uint32_t ret_end);

	static void displayStackFrames(boost::uint32_t tid);

private:
	static std::map<boost::uint32_t, ThreadStack*> s_mapThreadStack;	
};
