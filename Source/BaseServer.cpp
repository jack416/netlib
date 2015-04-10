#include "BaseServer.h"
#include "BaseNetWork.h"
#include "WinNetWork.h"
#include "LinuxNetWork.h"

namespace netlib
{
	BaseServer::BaseServer()
	{
#ifdef WIN32
		m_pNetWork = new WinNetWork();
#else
		m_pNetWork = new LinuxNetWork();
#endif
		m_pNetWork->m_pNetServer = this;
	}

	BaseServer::~BaseServer()
	{
		delete m_pNetWork;
	}

	void *BaseServer::_MainProc(void *param)
	{
		return MainProc(param);
	}

	void BaseServer::SetConfig(ServerConfig config)
	{
		m_pNetWork->SetConfig(config);
	}

	bool BaseServer::StartServer()
	{
		if (m_pNetWork == NULL)
			return false;
		if (!m_pNetWork->StartServer())
			return false;

		m_bIsStop = false;

		m_MainThread.Run((DWORD)GET_MEMBER_CALLBACK(BaseServer, _MainProc), (DWORD)this, (DWORD)NULL);
		return true;
	}

	void BaseServer::StopServer()
	{
		if (m_pNetWork == NULL)
			return;

		m_bIsStop = true;
		m_MainThread.Stop(5000);

		m_pNetWork->StopServer();
	}

	void BaseServer::WaitStop()
	{
		if (m_pNetWork == NULL)
			return;

		m_pNetWork->WaitStop();
		m_MainThread.Join();
	}

	bool BaseServer::IsRunning()
	{
		return !m_bIsStop;
	}

	bool BaseServer::Listen(int port)
	{
		return m_pNetWork->Listen(port);
	}

	bool BaseServer::Connect(const char *ip, int port)
	{
		m_pNetWork->Connect(ip, port);
		return true;
	}

	void BaseServer::CloseConnection(int nID)
	{
		m_pNetWork->CloseConnection(nID);
	}

	void BaseServer::SendData(int nID, char *data, int nSize)
	{
		m_pNetWork->SendData(nID, data, nSize);
	}

	Connection *BaseServer::GetConnection(int nID)
	{
		return m_pNetWork->GetConnection(nID);
	}

	/**< ³éÏóº¯Êý*/
	void BaseServer::OnConnect(int nID)
	{

	}

	void BaseServer::OnClose(int nID)
	{

	}

	void BaseServer::OnEvent(int nID)
	{

	}

	void *BaseServer::MainProc(void *param)
	{
		return NULL;
	}
}