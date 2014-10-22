#include "ImageSectionRoutine.h"

ImageSectionRoutine::ImageSectionRoutine(RTN rtn)
:m_routine(rtn)
{
	m_name = RTN_Name(rtn);
	m_ins_nums = (boost::uint32_t)RTN_NumIns(rtn);
	m_address = (boost::uint32_t)RTN_Address(rtn);
	m_size = (boost::uint32_t)RTN_Size(rtn);
}

ImageSectionRoutine::~ImageSectionRoutine(void)
{
}
