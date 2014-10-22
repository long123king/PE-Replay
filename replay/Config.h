#pragma once

#include <string>
#include <vector>
#include <map>



#define CONFIG_IMG_NAME				"image_name"
#define CONFIG_IMG_ADDR_RANGE		"image_addr_range"
#define CONFIG_IMG_SEC				"image_section"
#define CONFIG_IMG_RTN				"image_routine"
#define CONFIG_IMG_RELOC			"image_reloc"
#define CONFIG_IMG_STATIC			"image_static"
#define CONFIG_CALL					"call"
#define CONFIG_BRANCH				"branch"
#define CONFIG_RETURN				"return"
#define CONFIG_LEA					"lea"
#define CONFIG_INSN_NOMRAL			"insn_normal"
#define CONFIG_MEM_RD_BEFORE		"mem_rd_before"
#define CONFIG_MEM_WR_BEFORE		"mem_wr_before"
#define CONFIG_MEM_WR_AFTER			"mem_wr_after"
#define CONFIG_EXCETION				"exception"
#define CONFIG_DUMP					"dump"
#define CONFIG_SUMMARY				"summary"
#define CONFIG_ABNORMAL_CF			"abnormal_control_flow"
#define CONFIG_XSEC_BOUNDARY		"cross_sec_boundary"
#define CONFIG_XIMG_BOUNDARY		"cross_img_boundary"
#define CONFIG_HEAP_TRACE			"heap_trace"
#define CONFIG_VEX_IR				"vex_ir"

#define CONFIG_TRACE_TAIL			"trace_tail"

#ifndef CONFIG_ENTRY_ENABLED
#define CONFIG_ENTRY_ENABLED Config::IsEntryEnabled
#endif

#ifndef CONFIG_ENTRY_MAIN_MODULE
#define CONFIG_ENTRY_MAIN_MODULE Config::WithinMainModule
#endif

#ifndef CONFIG_ENTRY_CODE_SECTION
#define CONFIG_ENTRY_CODE_SECTION Config::WithinCodeSection
#endif

// 1. we should also be able to limit the range for dump,
// e.g. only dump disasm within main module
// 
// 2. we should be able to dump normal control flow event
// and abnormal event.

class Config_Entry
{
public:
	bool _bEnable;
	bool _bWithinMainModule;
	bool _bWithinCodeSection;
	std::string _filename;
	Config_Entry()
		:_bEnable(false)
		,_bWithinMainModule(false)
		,_bWithinCodeSection(false)
		,_filename("")
	{

	}
};

class Config
{
public:
	static bool Initialize(const char* filename, const char* samplename);

	static bool IsEntryEnabled(const char* name);
	static bool WithinMainModule(const char* name);
	static bool WithinCodeSection(const char* name);
	static std::string GetEntryFilename(const char* name);

	static std::string GetFullFilename(const char* name);
private:
	static bool s_overwrite;
	static std::string s_report_folder;
	static std::map<std::string, Config_Entry> s_map_entries;
};
