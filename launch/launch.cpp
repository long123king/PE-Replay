// launch.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <Windows.h>
#include <Shellapi.h>

#pragma comment(lib, "shell32.lib")


int _tmain(int argc, _TCHAR* argv[])
{
	if (argc < 2)
	{
		printf("Usage: launch filename [parameters]\n");
		return 0;
	}

	ShellExecuteW(NULL, L"open", argv[1], NULL, NULL, SW_HIDE);

	//STARTUPINFO si = {0,};
	//si.cb = sizeof(&si);

	//PROCESS_INFORMATION pi = {0,};

	//_TCHAR* commandline = NULL;

	//commandline = argv[1];

	//bool success = CreateProcessW(
	//	NULL, 
	//	argv[1], 
	//	NULL, 
	//	NULL, 
	//	false, 
	//	CREATE_SUSPENDED, 
	//	NULL, 
	//	NULL, 
	//	&si, 
	//	&pi);

	//if (success)
	//{
	//	wprintf(L"Succeed to launch %s\n", argv[1]);
	//}

	return 0;
}

