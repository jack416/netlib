#include "IOCP_NetFrame.h"
#include "Connection.h"
#include "WinNetWork.h"

namespace netlib
{
	WinNetWork::WinNetWork()
	{
#ifdef WIN32
		m_pNetFrame = new IOCP_NetFrame();
#endif
	}

	WinNetWork::~WinNetWork()
	{
#ifdef WIN32
		if (m_pNetFrame != NULL)
		{
			delete m_pNetFrame;
			m_pNetFrame = NULL;
		}
#endif
	}

	SOCKET WinNetWork::ListenPort(int port)
	{
#ifdef WIN32
		Socket slisten;
		if (!slisten.Init(SOCK_STREAM))
			return INVALID_SOCKET;

		if (!slisten.StartServer(port))
		{
			slisten.Close();
			return INVALID_SOCKET;
		}
		
		/**< °ÑÌ×½Ó×Ö°ó¶¨µ½ÍøÂç¿ò¼ÜÉÏ,·½±ã¼àÌý¸Ã²Ù×÷·ûµÄ×´Ì¬*/
		if (!m_pNetFrame->AssociateSocket(slisten.GetSocket()))
		{
			slisten.Close();
			return INVALID_SOCKET;
		}

		for (int i = 0; i < m_nIOThreadCount; i++)
		{
			if (!m_pNetFrame->PostAccept(slisten.GetSocket()))
			{
				slisten.Close();
				return INVALID_SOCKET;
			}
		}

		return slisten.Detach();
#endif
		return INVALID_SOCKET;
	}

	void* WinNetWork::HandleIOEvent(void* param)
	{
#ifdef WIN32
		IOCP_NetFrame::IOEvent events[1];
		int nCount = 1;

		while (!m_bIsStop)
		{
			if (!m_pNetFrame->WaitStatus(events, nCount, true))
				return NULL;

			if (nCount <= 0)
				continue;

			switch (events->type)
			{
			case IOCP_NetFrame::ET_Close:
				OnClose(events->sock);
				break;
			case IOCP_NetFrame::ET_Connected:
				OnConnect(events->client, false);
				m_pNetFrame->PostAccept(events->sock);
				break;
			case IOCP_NetFrame::ET_Recv:
				OnData(events->sock, events->nDataSize);
				break;
			case IOCP_NetFrame::ET_Send:
				OnSend(events->sock, events->nDataSize);
				break;
			default:
				break;
			}
		}
#endif
		return NULL;
	}

	DataResult WinNetWork::RecvData(Connection *pConnect, size_t nSize)
	{
#ifdef WIN32
		pConnect->WriteBuffer(nSize);
		if (!m_pNetFrame->PostRecv(pConnect->GetSocket()->GetSocket(), (char*)pConnect->AllocBuffer(MAX_BUFFER_SIZE), MAX_BUFFER_SIZE))
			return DVal_Invalid;

		return DVal_Valid;
#endif
		return DVal_Invalid;
	}

	DataResult WinNetWork::SendData(Connection *pConnect, size_t nSize)
	{
#ifdef WIN32
		try
		{
			unsigned char buffer[MAX_BUFFER_SIZE];
			if (nSize > 0)
				pConnect->m_SendBuffer.ReadBuffer(buffer, nSize);

			if (pConnect->m_SendBuffer.GetLength() <= 0)
			{
				pConnect->EndSend();
				if (pConnect->m_SendBuffer.GetLength() <= 0)
					return DVal_Valid;

				if (!pConnect->StartSend())
					return DVal_Valid;
			}

			int nLength = pConnect->m_SendBuffer.GetLength();
			if (nLength > MAX_BUFFER_SIZE)
			{
				pConnect->m_SendBuffer.ReadBuffer(buffer, MAX_BUFFER_SIZE, false);
				m_pNetFrame->PostSend(pConnect->GetSocket()->GetSocket(), (char*)buffer, MAX_BUFFER_SIZE);
			}
			else
			{
				pConnect->m_SendBuffer.ReadBuffer(buffer, nLength, false);
				m_pNetFrame->PostSend(pConnect->GetSocket()->GetSocket(), (char*)buffer, nLength);
			}
		}
		catch (...)
		{ }

		return DVal_Valid;
#endif
		return DVal_Invalid;
	}
}