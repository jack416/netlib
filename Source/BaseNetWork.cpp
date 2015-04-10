#include <stdio.h>
#include "BaseNetWork.h"
#include "BaseServer.h"
#include "Connection.h"

namespace netlib
{
	BaseNetWork::BaseNetWork()
	{
		Socket::SocketStartup();

		m_pConnectionPool = NULL;
		m_bIsStop = true;
		m_nHeartTime = 0;
		m_nReconnectTime = 0;
		m_pNetFrame = NULL;
		m_pNetServer = NULL;
		m_nIOThreadCount = 8;
		m_nWorkThreadCount = 8;
		m_nAvrCount = 10000;
	}

	BaseNetWork::~BaseNetWork()
	{
		StopServer();
		if (m_pConnectionPool != NULL)
		{
			delete m_pConnectionPool;
			m_pConnectionPool = NULL;
		}
		Socket::SocketCleanup();
	}

	void BaseNetWork::SetConfig(ServerConfig config)
	{
		m_nAvrCount = config.avrCount;
		m_nHeartTime = config.heartTime;
		m_nIOThreadCount = config.ioThreadCount;
		m_nWorkThreadCount = config.workThreadCount;
		m_nReconnectTime = config.nReconnectTime;
	}

	bool BaseNetWork::StartServer()
	{
		if (!m_bIsStop)
			return true;

		m_bIsStop = false;

		int nCount = 2;
		for (nCount = 2; nCount * nCount < m_nAvrCount * 2; nCount++);

		if (nCount < 200)
			nCount = 200;

		if (NULL != m_pConnectionPool)
		{
			delete m_pConnectionPool;
			m_pConnectionPool = NULL;
		}

		m_pConnectionPool = new mt::MemPool(sizeof(Connection), nCount);
		if (m_pConnectionPool == NULL)
		{
			StopServer();
			return false;
		}

		if (!m_pNetFrame->Start(MAXPOLLSIZE))
		{
			StopServer();
			return false;
		}

		m_WorkThreadPool.Start(m_nWorkThreadCount);

		for (int i = 0; i < m_nIOThreadCount; i++)
		{
			m_IOThreadPool.PushTask((DWORD)GET_MEMBER_CALLBACK(BaseNetWork, NetWorkProc), (DWORD)this, (DWORD)NULL);
		}
		m_IOThreadPool.Start(m_nIOThreadCount);

		if (!ListenAll())
		{
			StopServer();
			return false;
		}
		ConnectAll();

		m_MainThread.Run((DWORD)GET_MEMBER_CALLBACK(BaseNetWork, Main), (DWORD)this, (DWORD)NULL);

		return true;
	}

	void BaseNetWork::StopServer()
	{
		if (m_bIsStop)
			return;

		m_bIsStop = true;

		m_pNetFrame->Stop();
		m_ExitEvent.Set();
		m_MainThread.Stop(3000);
		m_IOThreadPool.Stop();
		m_WorkThreadPool.Stop();
	}

	void BaseNetWork::WaitStop()
	{
		m_MainThread.Join();
	}

	Connection *BaseNetWork::GetConnection(int nID)
	{
		Connection *pCon = NULL;
		mt::AutoLock l(&m_ConnectionMutex);
		MapConnection::iterator iter = m_ConnecttionList.find(nID);
		if (iter == m_ConnecttionList.end())
			return NULL;

		pCon = iter->second;
		return pCon;
	}

	void BaseNetWork::CloseConnection(SOCKET sock)
	{
		mt::AutoLock l(&m_ConnectionMutex);
		MapConnection::iterator iter = m_ConnecttionList.find(sock);
		if (iter == m_ConnecttionList.end())
			return;

		CloseConnection(iter);
	}

	void BaseNetWork::CloseConnection(MapConnection::iterator iter)
	{
		Connection *pCon = iter->second;
		m_ConnecttionList.erase(iter);

		pCon->Disconnect();

		if (pCon->ReleaseRead() == 0)
			CloseConnection(pCon);

		pCon->Release();
	}

	void BaseNetWork::CloseConnection(Connection *pConnect)
	{
		if (pConnect->TryClose())
		{
			pConnect->AddRef();
			m_WorkThreadPool.PushTask((DWORD)GET_MEMBER_CALLBACK(BaseNetWork, CloseProc), (DWORD)this, (DWORD)pConnect);
		}
	}

	bool BaseNetWork::Listen(int port)
	{
		mt::AutoLock l(&m_ListenPortMutex);
		std::pair<MapListenPort::iterator, bool> result = m_ListenPortList.insert(MapListenPort::value_type(port, INVALID_SOCKET));
		MapListenPort::iterator iter = result.first;

		if (!result.second && iter->second != INVALID_SOCKET)
			return true;

		if (m_bIsStop)
			return true;

		iter->second = ListenPort(port);
		if (iter->second != INVALID_SOCKET)
			return true;

		return false;
	}

	bool BaseNetWork::Connect(const char *ip, int port)
	{
		unsigned long long uAddr;

		unsigned char addr[8];
		int b[4] = { 0 };
		char tmp[20] = { 0 };

		::sscanf(ip, "%d.%d.%d.%d", &b[0], &b[1], &b[2], &b[3]);
		::sprintf(tmp, "%d.%d.%d.%d", b[0], b[1], b[2], b[3]);

		if (!strcmp(tmp, ip))
			return false;

		for (int i = 0; i < 4; i++)
			addr[i] = b[i];

		::memcpy(&addr[4], &port, 4);
		::memcpy(&uAddr, addr, 8);

		mt::AutoLock l(&m_ServerMutex);

		std::pair<MapServer::iterator, bool> result = m_ServerList.insert(MapServer::value_type(uAddr, INVALID_SOCKET));
		MapServer::iterator iter = result.first;

		if (!result.second && iter->second != INVALID_SOCKET)
			return true;

		if (m_bIsStop)
			return true;

		iter->second = ConnectOtherServer(ip, port);

		if (iter->second == INVALID_SOCKET)
			return false;

		l.Unlock();

		if (!OnConnect(iter->second, true))
			iter->second = INVALID_SOCKET;

		return true;
	}

	bool BaseNetWork::OnConnect(SOCKET sock, bool bIsServer)
	{
		Connection *pCon = new (m_pConnectionPool->Alloc())Connection(sock, bIsServer, m_pNetFrame, this, m_pConnectionPool);
		if (pCon == NULL)
		{
			::closesocket(sock);
			return false;
		}

		pCon->GetSocket()->SetSockMode(false);/**< ������*/

		mt::AutoLock l(&m_ConnectionMutex);
		pCon->RefreshHeart();

		std::pair<MapConnection::iterator, bool> result = m_ConnecttionList.insert(MapConnection::value_type(pCon->GetSocket()->GetSocket(), pCon));
		pCon->AddRef();
		l.Unlock();

		m_WorkThreadPool.PushTask((DWORD)GET_MEMBER_CALLBACK(BaseNetWork, ConnectProc), (DWORD)this, (DWORD)pCon);

		return true;
	}

	void BaseNetWork::OnClose(SOCKET sock)
	{
		mt::AutoLock l(&m_ConnectionMutex);
		MapConnection::iterator iter = m_ConnecttionList.find(sock);
		if (iter == m_ConnecttionList.end())
			return;

		CloseConnection(iter);
	}

	DataResult BaseNetWork::OnData(SOCKET sock, size_t nSize)
	{
		DataResult ret = DVal_Invalid;

		mt::AutoLock l(&m_ConnectionMutex);
		MapConnection::iterator iter = m_ConnecttionList.find(sock);

		if (iter == m_ConnecttionList.end())
			return ret;

		Connection *pCon = iter->second;
		pCon->AddRef();
		pCon->RefreshHeart();
		l.Unlock();

		try
		{
			ret = RecvData(pCon, nSize);
			if (ret == DVal_Invalid)
			{
				pCon->Release();
				OnClose(sock);
				return ret;
			}

			if (pCon->AddRead() > 0)
			{
				pCon->Release();
				return ret;
			}

			m_WorkThreadPool.PushTask((DWORD)GET_MEMBER_CALLBACK(BaseNetWork, IOEventProc), (DWORD)this, (DWORD)pCon);
		}
		catch (...){}

		return ret;
	}

	DataResult BaseNetWork::OnSend(SOCKET sock, size_t nSize)
	{
		DataResult ret = DVal_Invalid;

		mt::AutoLock l(&m_ConnectionMutex);
		MapConnection::iterator iter = m_ConnecttionList.find(sock);
		if (iter == m_ConnecttionList.end())
			return ret;

		Connection *pCon = iter->second;

		pCon->AddRef();
		l.Unlock();

		try
		{
			if (pCon->IsConnected())
				ret = SendData(pCon, nSize);
		}
		catch (...)
		{
		}

		pCon->Release();

		return ret;
	}

	void BaseNetWork::SendData(int nID, char *msg, int nSize)
	{
		mt::AutoLock l(&m_ConnectionMutex);

		MapConnection::iterator iter = m_ConnecttionList.find(nID);
		if (iter == m_ConnecttionList.end())
			return;

		Connection *pCon = iter->second;

		pCon->AddRef();
		l.Unlock();

		if (pCon->IsConnected())
			pCon->SendData((unsigned char*)msg, nSize);

		pCon->Release();
	}

	void BaseNetWork::HeartMonitor()
	{
		if (m_nHeartTime <= 0)
			return;

		MapConnection::iterator iter;
		Connection *pCon = NULL;
		time_t curTime = time(NULL);
		time_t lastTime;

		mt::AutoLock l(&m_ConnectionMutex);

		for (iter = m_ConnecttionList.begin(); iter != m_ConnecttionList.end(); iter)
		{
			pCon = iter->second;
			if (pCon->IsServer())
			{
				iter++;
				continue;
			}

			lastTime = pCon->GetLastHeartBeep();
			if (curTime < lastTime || curTime - lastTime < m_nHeartTime)
			{
				iter++;
				continue;
			}

			CloseConnection(iter);
			iter = m_ConnecttionList.begin();
		}

		l.Unlock();
	}

	void BaseNetWork::ReConnectAll()
	{
		if (m_nReconnectTime <= 0)
			return;
		static time_t lastTime = time(NULL);
		time_t curTime = time(NULL);

		if (m_nReconnectTime > curTime - lastTime)
			return;

		ConnectAll();
	}

	bool BaseNetWork::ListenAll()
	{
		bool bRet = true;

		mt::AutoLock l(&m_ListenPortMutex);

		MapListenPort::iterator iter = m_ListenPortList.begin();

		char szPort[256];

		for (; iter != m_ListenPortList.end(); iter++)
		{
			if (iter->second != INVALID_SOCKET)
				continue;

			iter->second = ListenPort(iter->first);

			if (iter->second == INVALID_SOCKET)
			{
				sprintf(szPort, "%s", iter->first);
				bRet = false;
			}
		}

		return bRet;
	}

	SOCKET BaseNetWork::ConnectOtherServer(const char* ip, int port)
	{
		Socket sock;
		if (!sock.Init(SOCK_STREAM))
			return INVALID_SOCKET;

		if (!sock.Connect(ip, port))
		{
			sock.Close();
			return INVALID_SOCKET;
		}
#ifdef WIN32
		sock.InitForIOCP(sock.GetSocket());
#endif

		return sock.Detach();
	}

	bool BaseNetWork::ConnectAll()
	{
		if (m_bIsStop)
			return false;

		bool bRet = true;
		mt::AutoLock l(&m_ServerMutex);
		MapServer::iterator iter = m_ServerList.begin();

		char ip[25];
		int port;

		for (; iter != m_ServerList.end(); iter++)
		{
			if (iter->second != INVALID_SOCKET)
				continue;

			unsigned char addr[8];
			::memcpy(addr, &(iter->first), 8);
			::sprintf(ip, "%d.%d.%d.%d", addr[0], addr[1], addr[2], addr[3]);
			::memcpy(&port, &addr[4], 4);

			iter->second = Connect(ip, port);

			if (iter->second == INVALID_SOCKET)
			{
				bRet = false;
				continue;
			}

			if (!OnConnect(iter->second, true))
				iter->second = INVALID_SOCKET;
		}

		return bRet;
	}


	void *BaseNetWork::Main(void *param)
	{
		while (!m_bIsStop)
		{
			if (m_ExitEvent.WaitEvent(1000))
				break;

			HeartMonitor();
			ReConnectAll();
		}

		return NULL;
	}

	void *BaseNetWork::NetWorkProc(void *param)
	{
		return HandleIOEvent(param);
	}

	void *BaseNetWork::ConnectProc(void *param)
	{
		Connection *pCon = (Connection*)param;
		m_pNetServer->OnConnect(pCon->GetID());

		if (pCon->IsConnected())
		{
			if (!MonitorConnect(pCon))
			{
				CloseConnection(pCon->GetSocket()->GetSocket());
			}
		}
		pCon->Release();

		return NULL;
	}

	void *BaseNetWork::IOEventProc(void *param)
	{
		Connection *pCon = (Connection*)param;

		while (!m_bIsStop)
		{
			pCon->SetRead();
			m_pNetServer->OnEvent(pCon->GetID());
			if (!pCon->IsConnected())
				break;

			if (pCon->HasData())
				continue;

			if (pCon->ReleaseRead() == 1)
				break;
		}

		if (!pCon->IsConnected())
			CloseConnection(pCon);

		pCon->Release();

		return NULL;
	}

	void *BaseNetWork::CloseProc(void *param)
	{
		Connection *pCon = (Connection*)param;

		if (pCon->IsServer())
		{

			SOCKET sock = pCon->GetID();

			mt::AutoLock l(&m_ServerMutex);
			MapServer::iterator iter = m_ServerList.begin();
			for (; iter != m_ServerList.end(); iter++)
			{
				if (iter->second == sock)
				{
					iter->second = INVALID_SOCKET;
					break;
				}
			}
		}

		m_pNetServer->OnClose(pCon->GetID());

		pCon->GetSocket()->Close();
		pCon->Release();

		return NULL;
	}


	bool BaseNetWork::MonitorConnect(Connection *pConnect)
	{
		if (!m_pNetFrame->AssociateSocket(pConnect->GetSocket()->GetSocket()))
			return false;

#ifdef WIN32
		return m_pNetFrame->PostRecv(pConnect->GetSocket()->GetSocket(), (char*)pConnect->AllocBuffer(MAX_BUFFER_SIZE), MAX_BUFFER_SIZE);
#else
		return m_pNetFrame->PostRecv(pConnect->GetSocket()->GetSocket(), NULL, 0);
#endif
	}

	SOCKET BaseNetWork::ListenPort(int port)
	{
		return INVALID_SOCKET;
	}

	void *BaseNetWork::HandleIOEvent(void* param)
	{
		return NULL;
	}

	DataResult BaseNetWork::RecvData(Connection *pConnect, size_t nSize)
	{
		return DVal_Invalid;
	}

	DataResult BaseNetWork::SendData(Connection *pConnect, size_t uSize)
	{
		return DVal_Invalid;
	}
}
