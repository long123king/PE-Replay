#include "Config.h"
#define _CPPUNWIND

#include "boost/property_tree/ptree.hpp"
#include "boost/property_tree/xml_parser.hpp"
#include "boost/foreach.hpp"
#include "boost/algorithm/string.hpp"
#include "boost/filesystem.hpp"

bool Config::Initialize( const char* filename , const char* samplename)
{
	//boost::property_tree::ptree config_tree;

	//try 
	//{
	//	read_xml(filename, config_tree);
	//	s_report_folder = config_tree.get<std::string>("config.report.folder");
	//	s_overwrite = config_tree.get<bool>("config.report.overwrite");
	//	BOOST_FOREACH(boost::property_tree::ptree::value_type &entry,
	//		config_tree.get_child("config.processing"))
	//	{
	//		Config_Entry config_entry;
	//		//config_entry._name = entry.first;
	//		boost::optional<bool> instrument = entry.second.get_optional<bool>("instrument");
	//		if (instrument)
	//		{
	//			config_entry._bEnable = instrument.get();
	//		}
	//		boost::optional<bool> main_module = entry.second.get_optional<bool>("main_module");
	//		if (main_module)
	//		{
	//			config_entry._bWithinMainModule = main_module.get();
	//		}
	//		boost::optional<bool> code_section = entry.second.get_optional<bool>("code_section");
	//		if (code_section)
	//		{
	//			config_entry._bWithinCodeSection = code_section.get();
	//		}
	//		config_entry._filename = s_report_folder;
	//		config_entry._filename += samplename;
	//		config_entry._filename += "\\";
	//		config_entry._filename += entry.second.get<std::string>("filename");

	//		//s_entries.push_back(config_entry);
	//		s_map_entries[entry.first] = config_entry;
	//	}
	//}
	//catch (std::exception& e)
	//{
	//	printf("Exception occurs when parsing config file, details:\n%s",
	//		e.what());

	//	return false;
	//}

	s_report_folder = filename;
	//if (s_overwrite)
	{
		try 
		{
			boost::filesystem::path report_folder_path(s_report_folder);
			// report folder, if not exists, create
			if (!boost::filesystem::exists(report_folder_path))
			{
				boost::filesystem::create_directory(report_folder_path);
				//boost::filesystem::permissions(report_folder_path, 
				//	boost::filesystem::perms::add_perms | boost::filesystem::perms::all_all);
			}	

			s_report_folder += samplename;
			s_report_folder += "\\";

			if (!boost::filesystem::exists(s_report_folder))
			{
				boost::filesystem::create_directory(s_report_folder);
				//boost::filesystem::permissions(report_folder_path, 
				//	boost::filesystem::perms::add_perms | boost::filesystem::perms::all_all);
			}

			boost::filesystem::path sample_folder_path(s_report_folder);

			// clear this directory
			boost::filesystem::directory_iterator it_end;
			for( boost::filesystem::directory_iterator iter(sample_folder_path) ; 
				iter != it_end ; 
				++iter ) {
				if ( is_regular_file( *iter ) )
				{
					// this is a file 
					//cout << iter->path; 
					
					if (!boost::filesystem::remove(iter->path()))
						wprintf(L"Fail to delete %s\n", iter->path().c_str());

					// rest of the code
				}
			}

			// sample folder, if exists, remove it, and then create it.
			//if (boost::filesystem::exists(sample_folder_path))
			//{
			//	boost::filesystem::remove_all(sample_folder_path);
			//}
			//boost::filesystem::create_directory(sample_folder_path);
			//boost::filesystem::permissions(sample_folder_path, 
			//	boost::filesystem::perms::add_perms | boost::filesystem::perms::all_all);	

		}
		catch (std::exception& e)
		{
			printf("Exception occurs when parsing config file, details:\n%s",
				e.what());

			return false;
		}

	}

	return true;
}

bool Config::IsEntryEnabled( const char* name )
{
	if (s_map_entries.find(name) != s_map_entries.end())
	{
		return s_map_entries[name]._bEnable;
	}

	return false;
}

std::string Config::GetEntryFilename( const char* name )
{
	if (s_map_entries.find(name) != s_map_entries.end())
	{
		return s_map_entries[name]._filename;
	}

	return "";
}

bool Config::WithinMainModule( const char* name )
{
	if (s_map_entries.find(name) != s_map_entries.end())
	{
		return s_map_entries[name]._bWithinMainModule;
	}

	return false;
}

bool Config::WithinCodeSection( const char* name )
{
	if (s_map_entries.find(name) != s_map_entries.end())
	{
		return s_map_entries[name]._bWithinCodeSection;
	}

	return false;
}

std::string Config::GetFullFilename( const char* name )
{
	return s_report_folder + name;
}

std::map<std::string, Config_Entry> Config::s_map_entries;

std::string Config::s_report_folder;

bool Config::s_overwrite;
