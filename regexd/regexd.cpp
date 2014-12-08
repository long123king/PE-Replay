// regexd.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <string>
#include <iostream>
#include <boost/regex.hpp>

bool search(const std::string& pattern,const std::string& content)
{
	bool bMatch = false;
	boost::smatch what;
	boost::regex* exp = NULL;
	
	try 
	{
		exp = new boost::regex(pattern.c_str(), boost::regex_constants::icase);
	}
	catch (boost::regex_error& e)
	{
		//std::cout << "Invalid regular expression: " << pattern << " !" << std::endl;
		return false;
	}

	try
	{
		bMatch = boost::regex_search(content, what, *exp);
	}
	catch (boost::regex_error& e)
	{
		//printf("[DE_PATTERN]Bad regular expression: %s\n", e.what());
		return false;
	}
	catch (std::runtime_error& e)
	{
		//printf("[DE_PATTERN]Runtime error: %s\n", e.what());
		return false;
	}
	catch (std::bad_alloc& e)
	{
		//printf("[DE_PATTERN]Bad allocation: %s\n", e.what());
		return false;
	}

	if (bMatch)
	{
		unsigned i, j;
		printf("* %6s: %s    \n", "OK", pattern.c_str());
		std::cout << "----------------------------------------    \n\tGroups:    \n";
		for(i = 0; i < what.size(); ++i)
			std::cout << "      $" << i << " = \"" << what[i] << "\"    \n";
		std::cout << "----------------------------------------    \n";
	}
	else
	{
		printf("* %6s: %s    \n", "FAIL", pattern.c_str());
	}

	return bMatch;
}

bool match(const std::string& pattern,const std::string& content)
{
	bool bMatch = false;
	boost::smatch what;
	boost::regex* exp = NULL;

	try 
	{
		exp = new boost::regex(pattern.c_str(), boost::regex_constants::icase);
	}
	catch (boost::regex_error& e)
	{
		//std::cout << "Invalid regular expression: " << pattern << " !" << std::endl;
		return false;
	}

	try
	{
		bMatch = boost::regex_match(content, what, *exp);
	}
	catch (boost::regex_error& e)
	{
		//printf("[DE_PATTERN]Bad regular expression: %s\n", e.what());
		return false;
	}
	catch (std::runtime_error& e)
	{
		//printf("[DE_PATTERN]Runtime error: %s\n", e.what());
		return false;
	}
	catch (std::bad_alloc& e)
	{
		//printf("[DE_PATTERN]Bad allocation: %s\n", e.what());
		return false;
	}

	if (bMatch)
	{
		unsigned i, j;
		printf("* %6s: %s    \n", "OK", content.c_str());
		std::cout << "----------------------------------------    \n\tGroups:    \n";
		for(i = 0; i < what.size(); ++i)
			std::cout << "      $" << i << " = \"" << what[i] << "\"    \n";
		std::cout << "----------------------------------------    \n";
	}
	else
	{
		printf("* %6s: %s    \n", "FAIL", content.c_str());
	}

	return bMatch;
}

int main(int argc, const char* argv[])
{
	if (argc != 3)
	{
		printf("Usage: regexd pattern content\n");
		return 0;
	}

	std::string init_pattern = argv[1];
	std::string init_content = argv[2];

	std::string pattern = init_pattern;
	std::string content = init_content;

	printf("#RegExd Analysis#    \n\n");

	printf("\n##Phase 0: Initialize    \n");
	printf("----------------------------------------    \n");
	printf("***Pattern***: %s    \n", pattern.c_str());
	printf("***Content***: %s    \n", content.c_str());

	printf("\n##Phase 1: Search    \n");
	printf("----------------------------------------    \n");

	while (!search(pattern, content))
	{		
		if (pattern.size() > 1)
		{
			pattern.erase(pattern.size()-1);
		}
		else
		{
			break;
		}
	}

	if (pattern.size() == 0)
	{
		printf("* %6s: Definitely Wrong Pattern!    \n", "PANIC");
	}
	else
	{
		printf("\n##Phase 2: Match    \n");
		printf("----------------------------------------    \n");
		while (!match(pattern, content))
		{
			if (content.size() > 1)
			{
				content.erase(content.size()-1);
			}
			else
			{
				break;
			}
		}

		if (content.size() != 0)
		{
			printf("\n##Final Match    \n");
			printf("----------------------------------------    \n");
			printf("***Pattern***: **%s**[%s]   \n", pattern.c_str(), init_pattern.substr(pattern.size()).c_str());
			printf("***Content***: **%s**[%s]   \n", content.c_str(), init_content.substr(content.size()).c_str());
		}
	}

	return 0;
}

