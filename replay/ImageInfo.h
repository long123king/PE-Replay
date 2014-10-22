#pragma once

#include <vector>
#include <string>
#include <fstream>

#include "boost/cstdint.hpp"

#include "pin.H"
#include "ImageSection.h"

//#include "CodeSectionSweeper.h"


namespace PIN_Native_Windows{
#include <Windows.h>
};

using namespace PIN_Native_Windows;


class ImageInfo
{
public:
	ImageInfo(IMG img);
	~ImageInfo(void);

	// static member functions

	static void AppendImage(IMG img);

	static bool IsAddrCode(boost::uint32_t addr);
	static bool IsAddrMapped(boost::uint32_t addr);
	static bool IsAddrWithinMainModule(boost::uint32_t addr);	
	static bool verifyControlFlowTarget(boost::uint32_t addr);

	//static StaticTrace* GetTrace(boost::uint32_t addr);
	static std::string GetRtnName(boost::uint8_t addr);

	static void DumpBinaryAround(boost::uint32_t addr, boost::uint32_t size);
	static void DumpDisasmAround(boost::uint32_t addr);
	static void DumpDisasmAround(const char* tag, boost::uint32_t addr, boost::uint32_t size);

	static std::string GetAddrLocationHint(boost::uint32_t addr);

private:
	// non-static member functions
	std::string GetRtnName_Imp(boost::uint32_t addr);
	std::string GetAddrLocationHint_Imp(boost::uint32_t addr);
	//StaticTrace* GetTrace_Imp(boost::uint32_t addr);
	ImageSection* GetSectionByAddress(boost::uint32_t addr);

	bool verifyControlFlowTraget_Imp(boost::uint32_t addr);
	bool IsAddrMapped_Imp(boost::uint32_t addr);
	bool IsAddrCode_Imp(boost::uint32_t addr);
	bool IsAddrWithinModule(boost::uint32_t addr);
	bool HasRtn(boost::uint32_t addr);

	void AnalyzeRelocationEntry();
	void AnalyzeDataDirectory(std::vector<std::pair<boost::uint32_t, boost::uint32_t>>& grey_areas);

	template<typename T>
	T SecureDereference(T* addr);

private:
	IMG m_image;
	std::string m_name;
	boost::uint32_t m_address;
	boost::uint32_t m_size;
	boost::uint32_t m_entry;
	bool m_main_executable;
	bool m_static_executable;

	IMAGE_DATA_DIRECTORY* m_data_dirs;

	std::set<boost::uint32_t> m_reloc_targets;
	std::vector<ImageSection*> m_sections;
	//std::vector<CodeSectionSweeper*> m_static_trace_providers;

	static std::vector<ImageInfo*> s_images;
	static boost::uint32_t s_main_module_start;
	static boost::uint32_t s_main_module_size;
};

template<typename T>
T ImageInfo::SecureDereference( T* addr )
{
	{
		if (((boost::uint32_t)addr) >= m_address &&
			((boost::uint32_t)addr) < m_address + m_size)
		{
			return *addr;
		}

		throw std::exception("Insecure Dereference Within Image");
	}
}
