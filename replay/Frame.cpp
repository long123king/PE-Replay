#include "Frame.h"
#include "Logger.h"

#define FRAME_SERIAL_FILE	"frames.csv"

boost::uint32_t Frame::s_last_id = 0;

Frame::Frame(boost::uint32_t bid/*Block* block*/)
:m_bid ( bid )/*m_block(block)*/
{
	m_id = ++s_last_id;
	memset(&m_context, 0, sizeof(m_context));
	memset(&m_stack.m_slots, 0, sizeof(boost::uint32_t) * RE_STACK_SLOTS_NUM);
}

Frame::~Frame()
{

}

void Frame::Serialize()
{
	// Id, Bid, Eip, Esp, Ebp, Eax, Ebx, Ecx, Edx, Esi, Edi, Stack_0 ..... Stack_31
	LOF(FRAME_SERIAL_FILE, 
		"%d,"
		"%d,"
		"%d,"
		"0x%08x,"	// Eip
		"0x%08x,"
		"0x%08x,"
		"0x%08x,"	// Eax
		"0x%08x,"
		"0x%08x,"
		"0x%08x,"
		"0x%08x,"	// Esi
		"0x%08x,"
		"0x%08x,"	// Stack_0
		"0x%08x,"
		"0x%08x,"
		"0x%08x,"
		"0x%08x,"
		"0x%08x,"	// Stack_5
		"0x%08x,"
		//"0x%08x,"
		//"0x%08x,"
		//"0x%08x,"
		//"0x%08x,"	// Stack_10
		//"0x%08x,"
		//"0x%08x,"
		//"0x%08x,"
		//"0x%08x,"
		//"0x%08x,"	// Stack_15
		//"0x%08x,"
		//"0x%08x,"
		//"0x%08x,"
		//"0x%08x,"
		//"0x%08x,"	// Stack_20
		//"0x%08x,"
		//"0x%08x,"
		//"0x%08x,"
		//"0x%08x,"
		//"0x%08x,"	// Stack_25
		//"0x%08x,"
		//"0x%08x,"
		//"0x%08x,"
		//"0x%08x,"
		//"0x%08x,"	// Stack_30
		"0x%08x\n",
		++s_last_serialized_id,
		m_id,
		m_bid,
		m_context.m_eip,
		m_context.m_esp,
		m_context.m_ebp,
		m_context.m_eax,
		m_context.m_ebx,
		m_context.m_ecx,
		m_context.m_edx,
		m_context.m_esi,
		m_context.m_edi,
		m_stack.m_slots[0],
		m_stack.m_slots[1],
		m_stack.m_slots[2],
		m_stack.m_slots[3],
		m_stack.m_slots[4],
		m_stack.m_slots[5],
		m_stack.m_slots[6],
		m_stack.m_slots[7]/*,
		m_stack.m_slots[8],
		m_stack.m_slots[9],
		m_stack.m_slots[10],
		m_stack.m_slots[11],
		m_stack.m_slots[12],
		m_stack.m_slots[13],
		m_stack.m_slots[14],
		m_stack.m_slots[15],
		m_stack.m_slots[16],
		m_stack.m_slots[17],
		m_stack.m_slots[18],
		m_stack.m_slots[19],
		m_stack.m_slots[20],
		m_stack.m_slots[21],
		m_stack.m_slots[22],
		m_stack.m_slots[23],
		m_stack.m_slots[24],
		m_stack.m_slots[25],
		m_stack.m_slots[26],
		m_stack.m_slots[27],
		m_stack.m_slots[28],
		m_stack.m_slots[29],
		m_stack.m_slots[30],
		m_stack.m_slots[31]*/
		);
}

boost::uint32_t Frame::s_last_serialized_id = 0;

std::vector<Frame*> Frame::s_frames;
