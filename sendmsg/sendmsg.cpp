// sendmsg.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#include <Windows.h>

int main(int argc, const char* argv[])
{
	if (argc != 5)
	{
		printf("Usage: sendmsg hwnd msg wparam lparam\n");
		return 0;
	}

	int hwnd = (int)strtol(argv[1], NULL, 16);
	int msg = (int)strtol(argv[2], NULL, 16);
	int wparam = (int)strtol(argv[3], NULL, 16);
	int lparam = (int)strtol(argv[4], NULL, 16);

	printf("SendMessage(0x%08x, 0x%02x, %d, %d);\n", hwnd, msg, wparam, lparam);

	SendMessage((HWND)hwnd, msg, wparam, lparam);
	return 0;
}

