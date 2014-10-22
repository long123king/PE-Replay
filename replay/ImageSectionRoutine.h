#pragma once

#include <string>

#include "boost/cstdint.hpp"

#include "pin.H"


class ImageSectionRoutine
{
public:
	ImageSectionRoutine(RTN rtn);
	~ImageSectionRoutine(void);

	boost::uint32_t Start()
	{
		return m_address;
	}

private:
	RTN m_routine;
	std::string m_name;
	boost::uint32_t m_ins_nums;
	boost::uint32_t m_address;
	boost::uint32_t m_size;
};
