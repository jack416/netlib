// TestServer.h: interface for the TestServer class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_TESTSERVER_H__CD091694_1C7D_42D0_A893_DB0B6482ADC8__INCLUDED_)
#define AFX_TESTSERVER_H__CD091694_1C7D_42D0_A893_DB0B6482ADC8__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "../Include/BaseServer.h"

class TestServer : public netlib::BaseServer  
{
public:
	TestServer();
	virtual ~TestServer();

public:
	void OnConnect(int nID);
	void OnClose(int nID);
	void OnEvent(int nID);
	void *MainProc(void *param);
	
};

#endif // !defined(AFX_TESTSERVER_H__CD091694_1C7D_42D0_A893_DB0B6482ADC8__INCLUDED_)
