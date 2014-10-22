#include "Logger.h"

#include <stdarg.h>

#define BUF_SIZE (4096 * 4)
// 100 MB file
#define BIGGEST_FILE_SIZE (100 * 1024 * 1024)
// Max 2 GB log, if exceeds, terminate
#define BIGGEST_FILE_NUMS 20

Logger::Logger(const char* filename)
:m_bEnabled(true)
,m_size(0)
,m_file_size(0)
,m_file_nums(1)
,m_original_filename(filename)
{
	m_buffer.resize(BUF_SIZE);
	PIN_MutexInit(&m_log_mutex);

	m_original_filename.resize(m_original_filename.length() - 4); // remove ".clg" extension

	char partial_file_name[1024] = {0,};
	sprintf_s(partial_file_name, 1024, "%s_%03d.clg", m_original_filename.c_str(), m_file_nums);
	m_out = new std::ofstream(partial_file_name, std::ofstream::out | std::ofstream::trunc);
}

Logger::~Logger(void)
{
	PIN_MutexLock(&m_log_mutex);
	Flush();

	m_out->close();
	PIN_MutexUnlock(&m_log_mutex);

	PIN_MutexFini(&m_log_mutex);
}	

void Logger::Flush()
{
	m_out->write(m_buffer.c_str(), m_size);
	m_out->flush();
	m_buffer.clear();
	m_file_size += m_size;
	m_size = 0;	

	if (m_file_size >= BIGGEST_FILE_SIZE)
	{
		SplitFile();
	}
}

void Logger::SplitFile()
{
	if (m_file_nums > BIGGEST_FILE_NUMS)
	{
		// too many logs, terminate process
		throw exception("Too many logs");
	}

	char partial_file_name[1024] = {0,};
	sprintf_s(partial_file_name, 1024, "%s_%03d.clg", m_original_filename.c_str(), ++m_file_nums);
	
	try
	{
		m_out->close();
		
		m_out = new std::ofstream(
			partial_file_name, std::ofstream::out | std::ofstream::trunc);
	}
	catch (exception* e)
	{
		throw exception("Fail to swap file stream");
	}

	m_file_size = 0;
	//m_file_nums++;
}

void Logger::LogTo( const char* filename, const char* log_line )
{
	if (s_loggers.find(filename) == s_loggers.end())
	{
		s_loggers[filename] = new Logger(filename);
	}

	s_loggers[filename]->Logline(log_line);
}

void Logger::LogToF( const char* filename, const char** format)
{	
	if (s_loggers.find(filename) == s_loggers.end())
	{
		s_loggers[filename] = new Logger(filename);
	}

	s_loggers[filename]->LoglineF(format);
}

void Logger::LoglineF( const char** format)
{
	// reference http://stackoverflow.com/a/3742999/941650 for string formating
	std::string dest;
	int n, size=100;
	bool b=false;
	va_list marker;

	while (!b)
	{
		dest.resize(size);
		va_start(marker, *format);
		n = vsnprintf((char*)dest.c_str(), size, *format, marker);
		va_end(marker);
		if ((n>0) && ((b=(n<size))==true)) 
			dest.resize(n); 
		else 
			size*=2;
	}

	Logline(dest.c_str());
}

void Logger::Logline( const char* line )
{
	boost::uint32_t len = strlen(line);
	PIN_MutexLock(&m_log_mutex);
	
	if (len + m_size >= BUF_SIZE)
	{
		Flush();
	}

	
	m_buffer.replace(m_size, len, line);

	m_size += len;
	PIN_MutexUnlock(&m_log_mutex);
}

void Logger::Finalize()
{
	for (std::map<std::string, Logger*>::iterator it = s_loggers.begin();
		it != s_loggers.end();
		it++)
	{
		delete it->second;
	}

	s_loggers.clear();
}

void Logger::LogAsConfig( const char* config, const char* format, ... )
{
	if (Config::IsEntryEnabled(config))
	{
		std::string filename = Config::GetEntryFilename(config);
		if (!filename.empty())
		{
			LogToF(filename.c_str(), &format);
		}
	}
}

void Logger::LogToF( const char* config, const char* format, ... )
{
	{
		std::string filename = Config::GetFullFilename(config);
		if (!filename.empty())
		{
			LogToF(filename.c_str(), &format);
		}
	}
}

std::map<std::string, Logger*> Logger::s_loggers;
