#include <string>
#include <vector>
#include "boost/cstdint.hpp"

class Block;

struct RE_CONTEXT
{
	boost::uint32_t m_eip;
	boost::uint32_t m_esp;
	boost::uint32_t m_ebp;

	boost::uint32_t m_eax;
	boost::uint32_t m_ebx;
	boost::uint32_t m_ecx;
	boost::uint32_t m_edx;

	boost::uint32_t m_esi;
	boost::uint32_t m_edi;
};


#define RE_STACK_SLOTS_NUM	8

struct RE_STACK
{
	boost::uint32_t m_slots[RE_STACK_SLOTS_NUM];
};

/*
*	For the case of replay, log has the following requirements:
*	1. No need to split file.
*	2. File format must be compact, no need to support readability.
*	3. For very large frame-producing applications, we must not log all frames.
*	4. The logical relationships is sometimes much more important than detailed frames.
*	5. Trace Esp and Ebp, use them to identify the opportunity to capture frames.
*/

class Frame
{
public:
	Frame(/*Block* block*/boost::uint32_t bid);
	~Frame();

	boost::uint32_t GetId(){ return m_id; }
	void Serialize();

public:
	RE_CONTEXT m_context;
	RE_STACK m_stack;

private: 
	boost::uint32_t m_id;
	boost::uint32_t m_bid;
	static boost::uint32_t s_last_id;
	static boost::uint32_t s_last_serialized_id;
	//Block*	m_block;
public:
	static std::vector<Frame*> s_frames;
};

