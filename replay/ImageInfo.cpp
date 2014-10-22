#include "ImageInfo.h"

#include "boost/algorithm/string.hpp"

//using namespace LIBDASM;

ImageInfo::ImageInfo(IMG img)
:m_image(img)
,m_data_dirs(NULL)
{
	m_name = IMG_Name(img);
	m_address = IMG_StartAddress(img);
	m_size = IMG_SizeMapped(img);
	m_entry = IMG_Entry(img);
	m_main_executable = IMG_IsMainExecutable(img);
	m_static_executable = IMG_IsStaticExecutable(img);

	std::vector<std::pair<boost::uint32_t, boost::uint32_t>> grey_areas;

	if (m_main_executable)
	{
		s_main_module_start = m_address;
		s_main_module_size = m_size;


		//AnalyzeDataDirectory(grey_areas);
	}

	LOQ(CONFIG_IMG_NAME,
					"%s:\n", m_name.c_str());
	
	AnalyzeRelocationEntry();

	for (std::set<boost::uint32_t>::iterator 
		it = m_reloc_targets.begin();
		it != m_reloc_targets.end();
		it ++)
	{
		LOQ(CONFIG_IMG_STATIC,
			"reloc item [0x%08x] %s\n", *it, m_name.c_str());
	}

	return;
	for( SEC sec= IMG_SecHead(img); SEC_Valid(sec); sec = SEC_Next(sec) )
	{
		// read static section information,
		// 'cause it is more reliable to find out where code is,
		// and dynamic section may not contains .idata/.edata section.
		ImageSection* section = new ImageSection(sec);
		m_sections.push_back(section);

		/*if (m_main_executable && section->Executable())
		{
			CodeSectionSweeper* static_trace_provider = 
				new CodeSectionSweeper(img, sec, m_reloc_targets, section->Start(), section->Size());

			if (static_trace_provider == NULL)
			{
				throw exception("Fail to allocate StaticCoverage!");
			}

			for (std::vector<std::pair<boost::uint32_t, boost::uint32_t>>::iterator 
				it = grey_areas.begin();
				it != grey_areas.end();
				it ++)
			{
				static_trace_provider->markGreyArea(it->first, it->second);
			}

			static_trace_provider->process();

			static_trace_provider->spotGaps();

			static_trace_provider->postProcess();

			m_static_trace_providers.push_back(static_trace_provider);

			for (std::set<StaticTrace*, StaticTrace_Comparator>::iterator 
				it  = static_trace_provider->TracesBegin();
				it != static_trace_provider->TracesEnd();
				it ++)
			{
				LOQ(CONFIG_IMG_STATIC,
					"[0x%08x - 0x%08x] %s\n",
					(*it)->GetStart(),
					(*it)->GetEnd(),
					m_name.c_str());
			}
		}*/
	}
}

ImageInfo::~ImageInfo(void)
{
}

void ImageInfo::AppendImage( IMG img )
{	
	ImageInfo* image = new ImageInfo(img);
	s_images.push_back(image);
}

bool ImageInfo::IsAddrCode( boost::uint32_t addr )
{
	for (std::vector<ImageInfo*>::iterator it = s_images.begin();
		it != s_images.end();
		it++)
	{
		if ((*it)->IsAddrCode_Imp(addr))
		{
			return true;
		}
	}

	return false;
}

bool ImageInfo::IsAddrCode_Imp( boost::uint32_t addr )
{
	for (std::vector<ImageSection*>::iterator it = m_sections.begin();
		it != m_sections.end();
		it ++)
	{
		if ((*it)->Executable() &&
			(*it)->Contains(addr))
		{
			return true;
		}
	}

	return false;
}

void ImageInfo::AnalyzeRelocationEntry()
{
	// relocation analyze doesn't depend on ImageSection object

	boost::uint32_t reloc_section_start = 0;
	boost::uint32_t reloc_section_size = 0;

	for( SEC sec= IMG_SecHead(m_image); SEC_Valid(sec); sec = SEC_Next(sec) )
	{
		if (boost::iequals(SEC_Name(sec), ".reloc"))
		{
			reloc_section_start = SEC_Address(sec);
			reloc_section_size = SEC_Size(sec);
		}
	}

	if (reloc_section_start != 0 &&
		reloc_section_size != 0)
	{
		boost::uint8_t* reloc_block_addr = (boost::uint8_t*)reloc_section_start;

		while (reloc_block_addr < (boost::uint8_t*)(reloc_section_start + reloc_section_size))
		{
			boost::uint32_t block_base_addr = 
				SecureDereference<boost::uint32_t>((boost::uint32_t*)reloc_block_addr);
			boost::uint32_t block_len = 
				SecureDereference<boost::uint32_t>((boost::uint32_t*)(reloc_block_addr + 4));
			
			if (block_base_addr == 0 && block_len == 0)
			{
				break;
			}

			for (boost::uint32_t 
					i = 0;
					i < block_len;
					i += 2)
			{
				//boost::uint16_t reloc_item = *((boost::uint16_t*)(reloc_block_addr + i + 8));
				boost::uint16_t reloc_item =
					SecureDereference<boost::uint16_t>((boost::uint16_t*)(reloc_block_addr + i + 8));
				if ((reloc_item & 0xF000) == 0x3000)
				{
					reloc_item &= 0x0FFF;
					boost::uint32_t reloc_target =
						SecureDereference<boost::uint32_t>((boost::uint32_t*)(m_address + reloc_item + block_base_addr));
					if (IsAddrWithinModule(reloc_target))
					{
						m_reloc_targets.insert(reloc_target);
					}
					//LOG_IMG("reloc item [0x%08x]\n", reloc_target);
				}
			}

			reloc_block_addr += block_len;
		}
	}
}

void ImageInfo::AnalyzeDataDirectory(std::vector<std::pair<boost::uint32_t, boost::uint32_t>>& grey_areas)
{
	IMAGE_DOS_HEADER* dos_header =
		(IMAGE_DOS_HEADER*)(m_address);

	IMAGE_FILE_HEADER* file_header =
		(IMAGE_FILE_HEADER*)(m_address + dos_header->e_lfanew + 4);

	IMAGE_OPTIONAL_HEADER* opt_header = 
		(IMAGE_OPTIONAL_HEADER*)(m_address + dos_header->e_lfanew + 24);

	if (opt_header->Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC)
	{
		PIMAGE_OPTIONAL_HEADER32 optionalHeader;

		optionalHeader = (PIMAGE_OPTIONAL_HEADER32)opt_header;

		m_data_dirs = &optionalHeader->DataDirectory[0];
	}
	else if (opt_header->Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC)
	{
		PIMAGE_OPTIONAL_HEADER64 optionalHeader;

		optionalHeader = (PIMAGE_OPTIONAL_HEADER64)opt_header;

		m_data_dirs = &optionalHeader->DataDirectory[0];
	}

	if (m_data_dirs != NULL)
	{
		// First analyze "import directory table" --IDT
		// "import directory table" consists of an array of "import directory entry"s --IDE
		// one IDE entry for each DLL, and the last entry is empty(all zeroed).
		//
		// each IDE corresponds a IMAGE_IMPORT_DESCRIPTOR object,
		// and its member variables is as follows:
		// =================================================================================
		// +00:	--> ILT 
		//				ILT is short for "Import Lookup Table" --ILT
		//				entry in ILT is a name or an ordinal(index) describes an import symbol
		// ---------------------------------------------------------------------------------
		// +04: timestamp
		// ---------------------------------------------------------------------------------
		// +08: forwarder chain
		//				TBD
		// ---------------------------------------------------------------------------------
		// +0C: --> name
		//				ascii name of the Dll
		// ---------------------------------------------------------------------------------
		// +10: --> IAT
		//				IAT is short for "Import Address Table" --IAT
		//				before bound to the actual Dll, the table is identical to ILT
		// =================================================================================
		// ILT contains an array of 32/64 bit number
		// the role of ILT is only provide descriptive information for each import symbol
		// the role of IAT is to provide actual bound address for each import symbol
		// 
		// ILT entry:		
		// the highest bit is flag bit, 1 for ordinal, 0, for name
		// 1:	ordinal is just a number
		// 0:	-->Hint/Name table entry
		//
		// each Hint/Name table entry contains:
		// =================================================================================
		// +00:	Hint
		//			hint to the "Export Name Pointer Table" of the referenced Dll
		// ---------------------------------------------------------------------------------
		// +02: Name
		//			ascii name, zero terminated
		// ---------------------------------------------------------------------------------
		// +**: Padding
		//			2 bytes alignment
		// =================================================================================

		const char* data_dir_names[] = {
			"export table",
			"import table",
			"resource table",
			"exception table",
			"certificate table",
			"relocation table",
			"debug table",
			"architecture table",
			"global pointer",
			"tls table",
			"load config table",
			"bound import",
			"iat table",
			"delay import table",
			"clr runtime header",
			"",
			"",
			"",
			""
		};

		for (boost::uint32_t i = 0;
			i < opt_header->NumberOfRvaAndSizes;
			i++)
		{
			if (m_data_dirs[i].VirtualAddress == 0 &&
				m_data_dirs[i].Size == 0)
			{
				LOQ(CONFIG_IMG_STATIC,
					"data dir[%2d], [---------- - ----------] %s\n",
					i,					
					data_dir_names[i]);
			}
			else
			{
				LOQ(CONFIG_IMG_STATIC,
					"data dir[%2d], [0x%08x - 0x%08x] %s\n",
					i,
					m_address + m_data_dirs[i].VirtualAddress,
					m_address + m_data_dirs[i].VirtualAddress + m_data_dirs[i].Size,
					data_dir_names[i]);

				grey_areas.push_back(
					std::make_pair<boost::uint32_t, boost::uint32_t>(
						m_address + m_data_dirs[i].VirtualAddress,
						m_data_dirs[i].Size));
			}
		}

		IMAGE_DATA_DIRECTORY import_data_dir =
			m_data_dirs[IMAGE_DIRECTORY_ENTRY_IMPORT];

		if (IsAddrWithinModule(m_address + import_data_dir.VirtualAddress))
		{
			IMAGE_IMPORT_DESCRIPTOR* import_directory_entry = 
				(IMAGE_IMPORT_DESCRIPTOR*)(m_address + import_data_dir.VirtualAddress);

			while (import_directory_entry->Characteristics != 0)
			{
				boost::uint32_t ilt_addr =
					m_address + import_directory_entry->OriginalFirstThunk;

				boost::uint32_t iat_addr =
					m_address + import_directory_entry->FirstThunk;

				const char* dll_name =
					(const char*)(m_address + import_directory_entry->Name);

				LOQ(CONFIG_IMG_STATIC,
					"ILT at [0x%08x], IAT at [0x%08x], %s\n",
					ilt_addr,
					iat_addr,
					dll_name);

				boost::uint32_t ilt_size = 0;
				if (IsAddrWithinModule(ilt_addr))
				{
					if (opt_header->Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC)
					{
						boost::uint32_t* ilt_entry =
							((boost::uint32_t*)ilt_addr);

						while (*ilt_entry != 0)
						{
							if (((*ilt_entry) & 0x80000000) != 0)
							{
								LOQ(CONFIG_IMG_STATIC,
									"Ordinal 0x%08x\n", (*ilt_entry) & 0x7FFFFFFF);
							}
							else
							{
								std::string import_symbol_name;
								if (IsAddrWithinModule(m_address + ((*ilt_entry) & 0x7FFFFFFF)))
								{
									boost::uint32_t name_table_entry_addr =
										m_address + ((*ilt_entry) & 0x7FFFFFFF) + 2;
									if (IsAddrWithinModule(name_table_entry_addr))
									{
										import_symbol_name = 
											(const char*)(name_table_entry_addr);
									}
									
									LOQ(CONFIG_IMG_STATIC,
										"Name %s\n", import_symbol_name.c_str());
								}
							}
							ilt_size += 4;
							ilt_entry++;
						}
					}
					else if (opt_header->Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC)
					{
						boost::uint64_t* ilt_entry =
							((boost::uint64_t*)ilt_addr);

						while (*ilt_entry != 0)
						{
							if (((*ilt_entry) & 0x8000000000000000) != 0)
							{
								LOQ(CONFIG_IMG_STATIC,
									"Ordinal 0x%08x\n", (*ilt_entry) & 0x7FFFFFFFFFFFFFFF);
							}
							else
							{
								std::string import_symbol_name;
								if (IsAddrWithinModule(m_address + ((*ilt_entry) & 0x7FFFFFFFFFFFFFFF)))
								{
									boost::uint32_t name_table_entry_addr =
										m_address + ((*ilt_entry) & 0x7FFFFFFFFFFFFFFF) + 2;
									if (IsAddrWithinModule(name_table_entry_addr))
									{
										import_symbol_name = 
											(const char*)(name_table_entry_addr);
									}
									LOQ(CONFIG_IMG_STATIC,
										"Name %s\n", import_symbol_name.c_str());
								}
							}
							ilt_size += 8;
							ilt_entry++;
						}
					}
				}

				boost::uint32_t iat_size = 0;
				if (IsAddrWithinModule(iat_addr))
				{
					if (opt_header->Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC)
					{
						boost::uint32_t* iat_entry =
							((boost::uint32_t*)iat_addr);

						while (*iat_entry != 0)
						{
							LOQ(CONFIG_IMG_STATIC,
								"IAT [0x%08x]\n",
								*iat_entry);
							iat_size += 4;
							iat_entry++;
						}
					}
					else if (opt_header->Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC)
					{
						boost::uint64_t* iat_entry =
							((boost::uint64_t*)iat_addr);

						while (*iat_entry != 0)
						{
							LOQ(CONFIG_IMG_STATIC,
								"IAT [0x%016x]\n",
								*iat_entry);
							iat_size += 8;
							iat_entry++;
						}
					}
				}

				LOQ(CONFIG_IMG_STATIC,
					"Grey area [0x%08x - 0x%08x], [0x%08x - 0x%08x]\n",
					ilt_addr,
					ilt_addr + ilt_size,
					iat_addr,
					iat_addr + iat_size);

				grey_areas.push_back(
					std::make_pair<boost::uint32_t, boost::uint32_t>(
					ilt_addr,
					ilt_size));

				grey_areas.push_back(
					std::make_pair<boost::uint32_t, boost::uint32_t>(
					iat_addr,
					iat_size));
				
				import_directory_entry++;
			}
		}




		//LOG_RTN("Import data directory: [0x%08X - 0x%08X] %s\n",
		//	m_address + import_data_dir.VirtualAddress,
		//	(m_address + import_data_dir.VirtualAddress + import_data_dir.Size),
		//	m_name.c_str());

	}
}

ImageSection* ImageInfo::GetSectionByAddress( boost::uint32_t addr )
{
	for (std::vector<ImageSection*>::const_iterator 
		it = m_sections.begin();
		it != m_sections.end();
		it ++)
	{
		if ((*it)->Contains(addr))
		{
			return (*it);
		}
	}

	return NULL;
}

bool ImageInfo::IsAddrMapped( boost::uint32_t addr )
{
	for (std::vector<ImageInfo*>::iterator it = s_images.begin();
		it != s_images.end();
		it++)
	{
		if ((*it)->IsAddrMapped_Imp(addr))
		{
			return true;
		}
	}

	return true;
}

bool ImageInfo::IsAddrMapped_Imp( boost::uint32_t addr )
{
	return addr >= m_address &&
		addr < (m_address + m_size);
}

bool ImageInfo::IsAddrWithinModule( boost::uint32_t addr )
{
	return IsAddrMapped_Imp(addr);
}

std::string ImageInfo::GetRtnName( boost::uint8_t addr )
{
	for (std::vector<ImageInfo*>::iterator it = s_images.begin();
		it != s_images.end();
		it++)
	{
		std::string rtn_name = (*it)->GetRtnName_Imp(addr);
		if (!rtn_name.empty())
		{
			return rtn_name;
		}
	}

	return "";
}

std::string ImageInfo::GetRtnName_Imp(boost::uint32_t addr)
{
	for (std::vector<ImageSection*>::iterator it = m_sections.begin();
		it != m_sections.end();
		it++)
	{
		if ((*it)->HasRtn(addr))
		{
			return (*it)->GetRtnName(addr);
		}
	}

	return "";
}

bool ImageInfo::HasRtn( boost::uint32_t addr )
{
	for (std::vector<ImageSection*>::iterator it = m_sections.begin();
		it != m_sections.end();
		it++)
	{
		if ((*it)->HasRtn(addr))
		{
			return true;
		}
	}

	return false;
}

bool ImageInfo::verifyControlFlowTarget( boost::uint32_t addr )
{
	for (std::vector<ImageInfo*>::iterator it = s_images.begin();
		it != s_images.end();
		it++)
	{
		if ((*it)->verifyControlFlowTraget_Imp(addr))
		{
			return true;
		}
	}

	return false;
}

bool ImageInfo::verifyControlFlowTraget_Imp( boost::uint32_t addr )
{
	if (IsAddrCode_Imp(addr))
	{
		if (m_reloc_targets.find(addr) != m_reloc_targets.end())
		{
			return true;
		}

		// routine.
		if (HasRtn(addr))
		{
			return true;
		}

		if (m_main_executable)
		{
			//for (std::vector<CodeSectionSweeper*>::iterator
			//	it = m_static_trace_providers.begin();
			//	it != m_static_trace_providers.end();
			//it ++)
			//{
			//	if ((*it)->HasTraceStart(addr))
			//	{
			//		return true;
			//	}
			//}
		}
	}

	return false;
}

bool ImageInfo::IsAddrWithinMainModule( boost::uint32_t addr )
{
	if (s_main_module_start == 0 ||
		s_main_module_size == 0)
	{
		return true;
	}

	return addr >= s_main_module_start &&
		addr < (s_main_module_start + s_main_module_size);
}

void ImageInfo::DumpBinaryAround( boost::uint32_t addr, boost::uint32_t size )
{
	addr = addr & 0xFFFFFFF0;
	size = (size + 0xF) & 0xFFFFFFF0;

	if (IsAddrMapped(addr) &&
		IsAddrMapped(addr + size))
	{
		LOQ(CONFIG_DUMP,
			"Dump Binary Around [0x%08x - 0x%08x]\n", addr, size);
		unsigned long i = 0;
		for (i=0;i<16;i++)
		{
			LOQ(CONFIG_DUMP,
				"%4X", i);
		}
		LOQ(CONFIG_DUMP,
			"\n");
		for (i=0;i<16;i++)
		{
			LOQ(CONFIG_DUMP,
				"%4s", "__");
		}

		char lineSummary[17] = {0,};
		unsigned long pos = 0;
		for (i=0;i<size;i++)
		{
			if ((pos = i % 16) == 0)
			{
				if (i != 0)
				{
					LOQ(CONFIG_DUMP,
						" ---- %s\n", lineSummary);
					memset(lineSummary, 0, 17);
				}
				else
				{
					LOQ(CONFIG_DUMP,
						"\n");
				}		
			}

			LOQ(CONFIG_DUMP,
				"  %02X", *((boost::uint8_t*)addr + i));

			if (*((boost::uint8_t*)addr + i) >= 0x20 && *((boost::uint8_t*)addr + i) <= 0x7E)
			{
				lineSummary[pos] = *((boost::uint8_t*)addr + i);
			}
			else
			{
				lineSummary[pos] = ' ';
			}
		}

		if (size % 16 != 0)
		{
			for (i=0;i<16 - (size%16);i++)
			{
				LOQ(CONFIG_DUMP,
					"    ");
			}
		}

		LOQ(CONFIG_DUMP,
			" ---- %s\n", lineSummary);
		LOQ(CONFIG_DUMP,
			"\n");	
	}
}

void ImageInfo::DumpDisasmAround( boost::uint32_t addr )
{
	// First find trace that contains addr
	// Then proximate from the trace head to addr
	// Only Dump 0x20 bytes around addr

	//StaticTrace* trace = GetTrace(addr);
	//LOQ(CONFIG_DUMP,
	//	"trace is NULL for [0x%08x]\n", addr);
	//if (trace != NULL)
	//{
	//	LOQ(CONFIG_DUMP,
	//		"Dump Disassemble Around [0x%08x]\n", addr);

	//	boost::uint32_t trace_start = trace->GetStart();
	//	boost::uint32_t trace_end = trace->GetEnd();

	//	boost::uint32_t pos = trace_start;

	//	INSTRUCTION instruction = {0,};

	//	while (pos < trace_end &&
	//		pos < (addr + 0x20))
	//	{
	//		if (IsAddrCode(pos))
	//		{
	//			memset(&instruction, 0, sizeof(instruction));

	//			if ( get_instruction(&instruction, 
	//				(boost::uint8_t*)(pos), 
	//				MODE_32) == 0)
	//			{
	//				if (pos >= (addr + 0x20))
	//				{
	//					char disasm[1024] = {0,};
	//					get_instruction_string(&instruction, FORMAT_INTEL, 0, disasm, 1024);

	//					LOQ(CONFIG_DUMP,
	//						"[0x%08x] %s\n",
	//						pos,
	//						disasm);
	//				}

	//				pos += instruction.length;
	//			}
	//			else
	//			{
	//				// exception
	//				break;
	//			}
	//		}
	//	}
	//}
}

void ImageInfo::DumpDisasmAround( const char* tag, boost::uint32_t addr, boost::uint32_t size )
{
	//{
	//	LOQ(tag,
	//		"Dump Disassemble Around [0x%08x]\n", addr);

	//	boost::uint32_t trace_start = addr;
	//	boost::uint32_t trace_end = addr + size;

	//	boost::uint32_t pos = trace_start;

	//	INSTRUCTION instruction = {0,};

	//	while (pos < trace_end &&
	//		pos < (trace_start + 0x4000))
	//	{
	//		//if (IsAddrCode(pos))
	//		{
	//			memset(&instruction, 0, sizeof(instruction));

	//			if ( get_instruction(&instruction, 
	//				(boost::uint8_t*)(pos), 
	//				MODE_32) != 0)
	//			{
	//				//if (pos >= (addr + 0x20))
	//				{
	//					char disasm[1024] = {0,};
	//					get_instruction_string(&instruction, FORMAT_INTEL, 0, disasm, 1024);

	//					LOQ(tag,
	//						"[0x%08x] %s\n",
	//						pos,
	//						disasm);
	//				}

	//				pos += instruction.length;
	//			}
	//			else
	//			{
	//				// exception
	//				break;
	//			}
	//		}
	//	}
	//}
}

//StaticTrace* ImageInfo::GetTrace( boost::uint32_t addr )
//{
//	for (std::vector<ImageInfo*>::iterator it = s_images.begin();
//		it != s_images.end();
//		it++)
//	{
//		StaticTrace* trace = (*it)->GetTrace_Imp(addr);
//		if (trace != NULL)
//		{
//			return trace;
//		}
//	}
//
//	return NULL;
//}
//
//StaticTrace* ImageInfo::GetTrace_Imp( boost::uint32_t addr )
//{
//	for (std::vector<CodeSectionSweeper*>::iterator
//		it = m_static_trace_providers.begin();
//		it != m_static_trace_providers.end();
//	it ++)
//	{
//		StaticTrace* trace = (*it)->GetTraceAt(addr);
//		if (trace != NULL)
//		{
//			return trace;
//		}
//	}
//
//	return NULL;
//}

// Image->Section->Routine
std::string ImageInfo::GetAddrLocationHint( boost::uint32_t addr )
{
	for (std::vector<ImageInfo*>::iterator it = s_images.begin();
		it != s_images.end();
		it++)
	{
		if ((*it)->IsAddrMapped_Imp(addr))
		{
			return (*it)->GetAddrLocationHint_Imp(addr);
		}
	}

	return "";
}

std::string ImageInfo::GetAddrLocationHint_Imp( boost::uint32_t addr )
{
	std::string location_hint;	

	location_hint += m_name.substr(m_name.find_last_of("\\/") + 1);
	
	for (std::vector<ImageSection*>::const_iterator
		it = m_sections.begin();
		it != m_sections.end();
		it ++)
	{
		if ((*it)->Contains(addr))
		{
			location_hint += ".[";
			location_hint += (*it)->Name();
			location_hint += "]";
			//if ((*it)->HasRtn(addr))
			{
				location_hint += ".";
				location_hint += (*it)->WithinRtn(addr);
			}
		}
	}

	return location_hint;
}

boost::uint32_t ImageInfo::s_main_module_size = 0;

boost::uint32_t ImageInfo::s_main_module_start = 0;

std::vector<ImageInfo*> ImageInfo::s_images;
