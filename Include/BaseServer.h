#ifndef __BASESERVER_H__
#define __BASESERVER_H__

#include "BaseNetWork.h"
#include "../../ThreadLib/Include/Thread.h"

namespace netlib
{
	class BaseServer
	{
	public:
		BaseServer();
		virtual ~BaseServer();

	public:
		virtual void OnConnect(int nID);
		virtual void OnClose(int nID);
		virtual void OnEvent(int nID);
		virtual void *MainProc(void *param);
		

		void SetConfig(ServerConfig config);

		bool StartServer();
		void StopServer();
		void WaitStop();
		bool IsRunning();

		bool Listen(int port);
		bool Connect(const char *ip, int port);
		void CloseConnection(int nID);

		void SendData(int hostID, char *data, int nSize);

		Connection *GetConnection(int nID);
	private:
		void *_MainProc(void *param);
	private:
		BaseNetWork*	m_pNetWork;
		bool			m_bIsStop;
		mt::Thread		m_MainThread;
	};
}

#endif