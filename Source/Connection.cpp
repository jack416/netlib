#include "Connection.h"
#include "BaseNetWork.h"

namespace netlib
{
	Connection::Connection(SOCKET sock, bool bIsServer, INetFrame *pNetFrame, BaseNetWork *pNetWork, mt::MemPool *pConnectionPool)
		: m_Socket(sock, SOCK_STREAM)
		, m_pConnectionPool(pConnectionPool)
		, m_pNetWork(pNetWork)
		, m_pNetFrame(pNetFrame)
		, m_nRefCount(1)
		, m_nReadCount(0)
		, m_nSendCount(0)
		, m_nCloseCount(0)
		, m_bReadable(false)
		, m_bSendable(false)
		, m_bIsConnected(true)
		, m_bIsServer(bIsServer)
	{
		m_id = m_Socket.GetSocket();
#ifdef WIN32
		Socket::InitForIOCP(sock);
#endif
		m_Socket.InitPeerAddress();
		m_Socket.InitLocalAddress();
	}

	Connection::~Connection()
	{
	}

	unsigned char *Connection::AllocBuffer(size_t nSize)
	{
		return m_RecvBuffer.AllocBuffer(nSize);
	}

	void Connection::WriteBuffer(size_t nLength)
	{
		m_RecvBuffer.WriteBuffer(nLength);
	}

	int Connection::GetID()
	{
		return m_id;
	}

	Socket *Connection::GetSocket()
	{
		return &m_Socket;
	}

	bool Connection::HasData()
	{
		return m_bReadable && m_RecvBuffer.GetLength() > 0;
	}

	unsigned int Connection::GetLength()
	{
		return m_RecvBuffer.GetLength();
	}

	bool Connection::ReadData(unsigned char *data, size_t nLength, bool bClear)
	{
		m_bReadable = m_RecvBuffer.ReadBuffer(data, nLength, bClear);
		return m_bReadable;
	}

	bool Connection::SendData(const unsigned char *data, size_t nSize)
	{
		try
		{
			unsigned char *outBuffer = NULL;
			int nSend = 0;/**< 实际发送大小*/

			mt::AutoLock l(&m_SendMutex);

			if (m_SendBuffer.GetLength())
			{
				nSend = m_Socket.Send(data, nSize);
			}
			if (nSend == -1)
				return false;

			if (nSize == nSend)
				return true;

			nSize -= nSend;

			while (1)
			{
				if (nSize < MAX_BUFFER_SIZE)
				{
					outBuffer = m_SendBuffer.AllocBuffer(nSize);
					::memcpy(outBuffer, &data[nSend], nSize);
					m_SendBuffer.WriteBuffer(nSize);
					break;
				}
				else
				{
					outBuffer = m_SendBuffer.AllocBuffer(MAX_BUFFER_SIZE);
					::memcpy(outBuffer, &data[nSend], MAX_BUFFER_SIZE);
					m_SendBuffer.WriteBuffer(MAX_BUFFER_SIZE);
					nSend += MAX_BUFFER_SIZE;
					nSize -= MAX_BUFFER_SIZE;
				}
			}
			if (!StartSend())
				return true;

			m_pNetFrame->PostSend(m_Socket.GetSocket(), NULL, 0);
		}
		catch (...)
		{ }
		return true;
	}

	bool Connection::StartSend()
	{
		if (mt::InterLockIncre((void*)&m_nSendCount, 1) != 0)
			return false;

		return true;
	}

	void Connection::EndSend()
	{
		m_nSendCount = 0;
	}

	bool Connection::TryClose()
	{
		if (mt::InterLockIncre((void*)&m_nCloseCount, 1) == 0)
		{
			return true;
		}
		return false;
	}

	void Connection::Close()
	{
		m_pNetWork->CloseConnection(m_id);
	}

	void Connection::RefreshHeart()
	{
		m_LastHeart = time(NULL);
	}

	time_t Connection::GetLastHeartBeep()
	{
		return m_LastHeart;
	}

	bool Connection::IsServer()
	{
		return m_bIsServer;
	}

	bool Connection::IsConnected()
	{
		return m_bIsConnected;
	}

	void Connection::Disconnect()
	{
		m_bIsConnected = false;
	}

	long Connection::AddRead()
	{
		return mt::InterLockIncre((void*)&m_nReadCount, 1);
	}

	long Connection::ReleaseRead()
	{
		return mt::InterLockDecre((void*)&m_nReadCount, 1);
	}

	void Connection::SetRead()
	{
		m_nReadCount = 1;
	}

	void Connection::AddRef()
	{
		mt::InterLockIncre((void*)&m_nRefCount, 1);
	}

	void Connection::Release()
	{
		if (mt::InterLockDecre((void*)&m_nRefCount, 1) == 1)
		{
			if (m_pConnectionPool == NULL)
			{
				delete this;
				return;
			}
			//this->~Connection();
			m_pConnectionPool->Free(this);
		}
	}

	void Connection::GetAddress(std::string &strAddress, int &nPort)
	{
		if (!m_bIsServer)
			m_Socket.GetPeerAddress(strAddress, nPort);
		else
			m_Socket.GetLocalAddress(strAddress, nPort);
	}

	void Connection::GetServerAddress(std::string &strAddress, int &nPort)
	{
		if (m_bIsServer)
			m_Socket.GetPeerAddress(strAddress, nPort);
		else
			m_Socket.GetLocalAddress(strAddress, nPort);
	}
}