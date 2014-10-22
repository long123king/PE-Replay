#pragma once

#include <string>
#include <fstream>
#include <map>

#include "boost/cstdint.hpp"
#include "boost/format.hpp"

#include "pin.H"
#include "portability.H"

#include "Config.h"

#ifndef LOQ
#define LOQ Logger::LogAsConfig
#endif

#ifndef LOF
#define LOF Logger::LogToF
#endif


class Logger
{
public:
	Logger(const char* filename);
	~Logger(void);

	static void LogAsConfig(const char* config, const char* format, ...);

	static void LogTo(const char* filename, const char* log_line);

	static void LogToF( const char* filename, const char** format);

	static void LogToF( const char* filename, const char* format, ... );

	static Logger* GetInstance(const char* filename)
	{
		if (s_loggers.find(filename) == s_loggers.end())
		{
			s_loggers[filename] = new Logger(filename);
		}

		return s_loggers[filename];
	}

	static void Dummy_Log(const char* format, ...)
	{

	}


	static void Finalize();

	void Logline(const char* line);

	void LoglineF( const char** format);

	void Flush();

	void SplitFile();

	void Disable()
	{
		m_bEnabled = false;
	}

private:
	std::ofstream* m_out;

	boost::uint32_t m_size;
	boost::uint32_t m_file_size; // used for split log file to smaller ones.

	bool m_bEnabled;

	std::string m_original_filename;
	boost::uint32_t m_file_nums;

	std::string m_buffer;
	static std::map<std::string, Logger*> s_loggers;

	PIN_MUTEX m_log_mutex;	
};
