#include <string>
#include <set>
#include <vector>
#include "boost/cstdint.hpp"

class Block
{
public:
	Block(boost::uint32_t eip);
	~Block();

	void AddFrame(boost::uint32_t frame_id)
	{ 
		m_frames.insert(frame_id); 
	}

	void AddDisasmLine(const char* disasm_line)
	{
		m_disasm += disasm_line;
		m_disasm += " _|_ ";
	}

	void AddRawLine(const char* raw_line)
	{
		m_raw += raw_line;
		m_raw += " _|_ ";
	}

	void SetEip(boost::uint32_t eip)
	{
		m_eip = eip;
	}

	void SetTid(boost::uint32_t tid)
	{
		m_tid = tid;
	}

	boost::uint32_t GetBid() 
	{
		return m_id;
	}

	void Serialize();

private:
	std::set<boost::uint32_t> m_frames;
	std::string m_disasm;
	std::string m_raw;
	boost::uint32_t m_id;
	boost::uint32_t m_tid;
	boost::uint32_t m_eip;
	static boost::uint32_t s_last_id;
public:
	static std::vector<Block*> s_blocks;
};

