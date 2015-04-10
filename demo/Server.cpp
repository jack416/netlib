// TestServer.cpp: implementation of the TestServer class.
//
//////////////////////////////////////////////////////////////////////

#include "Server.h"
#include "../Include/Connection.h"
#include <stdio.h>
#include <string.h>
#ifndef WIN32
#include <unistd.h>
#endif

TestServer::TestServer()
{
	Listen(8888);
	Listen(6666);
	Listen(9999);
}

TestServer::~TestServer()
{

}

void* TestServer::MainProc(void* pParam)
{
	while (IsRunning())
	{
#ifndef WIN32
		usleep( 1000 * 1000 );
#else
		Sleep( 1000 );
#endif
	}
	
	return 0;
}

void TestServer::OnConnect(int nID)
{
	printf("新连接(%d)已经成功连入服务\n", nID);
}

void TestServer::OnClose(int nID)
{
	/**< 此时已经不能再根据nID获取到连接对象*/
	printf("连接(%d)已经断开\n", nID);
}

void TestServer::OnEvent(int nID)
{
	netlib::Connection *pCon = GetConnection(nID);
	if (pCon != NULL)
	{
		unsigned char c[256];

		if (!pCon->ReadData(c, 2, false))
			return;
		
		unsigned short len = 0;
		::memcpy(&len, c, 2);
		len += 2;
		if (len > 256)
		{
			printf("What the hell was going on !!!!!!!!\n");
			pCon->Close();
			return;
		}
		if (!pCon->ReadData(c, len))
			return;

		pCon->SendData(c, len);
	}
}
