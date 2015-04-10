// stdafx.cpp : source file that includes just the standard includes
//	dmk_static.pch will be the pre-compiled header
//	stdafx.obj will contain the pre-compiled type information

#include "Server.h"

#include <stdio.h>
#include <string>
#include <iostream>
#include <vector>
using namespace std;

#ifdef WIN32
#ifdef _DEBUG
#pragma comment(lib, "../lib/netlibd.lib")
#else
#pragma comment(lib, "../lib/netlib.lib" )
#endif
#endif

int main()
{
	TestServer ser;
	
	netlib::ServerConfig config = { 5000, 0, 4, 4, 0 };

	ser.SetConfig(config);
	ser.StartServer();
	ser.WaitStop();
	printf( "exit\n" );
	
	return 0;
}
