#include "ImageSection.h"

#include "boost/algorithm/string.hpp"

ImageSection::ImageSection(SEC sec)
:m_section(sec)
,m_code(false)
,m_initialized_data(false)
,m_uninitialized_data(false)
{
	m_name = SEC_Name(sec);
	m_address = SEC_Address(sec);
	m_size = SEC_Size(sec);
	m_readable = SEC_IsReadable(sec);
	m_writeable = SEC_IsWriteable(sec);
	m_executable = SEC_IsExecutable(sec);

	/************************************************************************/
	//	SEC_TYPE_REGREL 	 relocations
	//	SEC_TYPE_DYNREL 	 dynamic relocations
	//	SEC_TYPE_EXEC 		 contains code
	//	SEC_TYPE_DATA 	     contains initialized data
	//	SEC_TYPE_LSDA 	     old exception_info (obsolete)
	//	SEC_TYPE_BSS 	     contains uninitialized data
	//	SEC_TYPE_LOOS 	     OS specific.
	//	SEC_TYPE_USER 	     Application specific.
	/************************************************************************/

	switch (SEC_Type(sec))
	{
	case SEC_TYPE_REGREL:
		{
			LOQ(CONFIG_IMG_SEC,
				"section %10s [relocations]\n", m_name.c_str());
		}
		break;
	case SEC_TYPE_DYNREL:
		{
			LOQ(CONFIG_IMG_SEC,
				"section %10s [dynamic relocations]\n", m_name.c_str());
		}
		break;
	case SEC_TYPE_EXEC:
		{
			LOQ(CONFIG_IMG_SEC,
				"section %10s [contains code]\n", m_name.c_str());
			m_code = true;
		}
		break;
	case SEC_TYPE_DATA:
		{
			LOQ(CONFIG_IMG_SEC,
				"section %10s [contains initialized data]\n", m_name.c_str());
			m_initialized_data = true;
		}
		break;
	case SEC_TYPE_LSDA:
		{
			LOQ(CONFIG_IMG_SEC,
				"section %10s [old exception_info (obsolete)]\n", m_name.c_str());
		}
		break;
	case SEC_TYPE_BSS:
		{
			LOQ(CONFIG_IMG_SEC,
				"section %10s [contains uninitialized data]\n", m_name.c_str());
			m_uninitialized_data = true;
		}
		break;
	case SEC_TYPE_LOOS:
		{
			LOQ(CONFIG_IMG_SEC,
				"section %10s [OS specific.]\n", m_name.c_str());
		}
		break;
	case SEC_TYPE_USER:
		{
			LOQ(CONFIG_IMG_SEC,
				"section %10s [Application specific.]\n", m_name.c_str());
		}
		break;
	default:
		{
		}
		break;
	}


	m_text = boost::iequals(m_name, ".text") ;

	if (m_text)
	{
		for( RTN rtn = SEC_RtnHead(sec); RTN_Valid(rtn); rtn = RTN_Next(rtn) )
		{
			//ImageSectionRoutine* routine = new ImageSectionRoutine(rtn);
			//m_routines.push_back(routine);
			LOQ(CONFIG_IMG_RTN,
				"routine [0x%08x] %s\n",
				RTN_Address(rtn),
				RTN_Name(rtn).c_str());
			m_addr_rtns[RTN_Address(rtn)] = 
				std::make_pair(RTN_Name(rtn),
				RTN_Address(rtn) + RTN_Size(rtn));
		}
	}
}

ImageSection::~ImageSection(void)
{
}
