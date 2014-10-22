#include "Block.h"
#include "Logger.h"

#define BLOCK_SERIAL_FILE	"blocks.csv"

Block::Block(boost::uint32_t eip)
{
	m_id = ++s_last_id;
	m_tid = m_id;
	m_eip = eip;
}

Block::~Block()
{

}

void Block::Serialize()
{
	// id, tid, raw, disasm
	LOF(BLOCK_SERIAL_FILE,
		"%d,%d,0x%08x,\"%s\",\"%s\"\n",
		m_id,
		m_tid,
		m_eip,
		m_raw.c_str(),
		m_disasm.c_str());
}

boost::uint32_t Block::s_last_id = 0;

std::vector<Block*> Block::s_blocks;
