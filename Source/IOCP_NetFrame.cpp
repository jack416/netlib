#include "IOCP_NetFrame.h"

#ifdef WIN32
#pragma comment(lib, "mswsock.lib")
#pragma comment(lib, "ws2_32.lib")
#endif

namespace netlib
{
	IOCP_NetFrame::IOCP_NetFrame()
		:m_IOPool(sizeof(IOOverlapped), 50)
		,m_nProcCount(0)
		//,m_lpfnAcceptEx(NULL)
	{
#ifdef WIN32
		SYSTEM_INFO sysinfo;
		::GetSystemInfo(&sysinfo);

		m_nProcCount = sysinfo.dwNumberOfProcessors;
#endif
	}

	IOCP_NetFrame::~IOCP_NetFrame()
	{
	}

	bool IOCP_NetFrame::Start(int nMaxSize)
	{
#ifdef WIN32
		
		int nNumberOfConcurrentThreads = 0;
		if (m_nProcCount > 0)
			nNumberOfConcurrentThreads = m_nProcCount *2 + 2;

		m_hCompletionPort = ::CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, nNumberOfConcurrentThreads);

		if (m_hCompletionPort == NULL)
			return false;
#endif
		return true;
	}
	
	bool IOCP_NetFrame::Stop()
	{
#ifdef WIN32
		::memset(&m_stopOverlapped.overLapped, 0, sizeof(OVERLAPPED));
		m_stopOverlapped.wsaBuffer.buf = NULL;
		m_stopOverlapped.wsaBuffer.len = 0;
		m_stopOverlapped.overLapped.Internal = 0;
		m_stopOverlapped.sock = 0;
		m_stopOverlapped.IOType = IOCP_NetFrame::ET_Close;

		::PostQueuedCompletionStatus(m_hCompletionPort, 0, NULL, (OVERLAPPED*)&m_stopOverlapped);
#endif
		return true;
	}
	
	bool IOCP_NetFrame::AssociateSocket(SOCKET s)
	{
#ifdef WIN32
		/*DWORD dwBytes = 0;
		GUID GuidAcceptEx = WSAID_ACCEPTEX;
		if(SOCKET_ERROR == ::WSAIoctl(s, SIO_GET_EXTENSION_FUNCTION_POINTER, &GuidAcceptEx, sizeof(GuidAcceptEx), &m_lpfnAcceptEx, sizeof(m_lpfnAcceptEx), &dwBytes, NULL, 	NULL))
			return false;  */

		if (::CreateIoCompletionPort((HANDLE)s, m_hCompletionPort, (DWORD)s, 0) == NULL)
			return false;
#endif
		return true;
	}
	
	bool IOCP_NetFrame::WaitStatus(void *event, int &nCount, bool bBlock)
	{
#ifdef WIN32
		IOEvent *pEvent = (IOEvent*)event;
		nCount = 0;

		DWORD dwIOSize = 0;
		IOOverlapped *pOverlapped = NULL;
		if (!::GetQueuedCompletionStatus(m_hCompletionPort, &dwIOSize, (PDWORD)&pEvent[nCount].sock, (OVERLAPPED**)&pOverlapped, INFINITE))
		{
			DWORD dwErrorCode = ::GetLastError();
			if (dwErrorCode == ERROR_INVALID_HANDLE)
			{
				try
				{
					if (pOverlapped != NULL)
					{
						m_IOPool.Free(pOverlapped);
						pOverlapped = NULL;
						return true;
					}
				}
				catch (...)
				{
				}
				return true;
			}

			if (dwErrorCode == ERROR_OPERATION_ABORTED)
			{
				if (pOverlapped->IOType == IOCP_NetFrame::ET_Connected)
				{
					m_IOPool.Free(pOverlapped);
					pOverlapped = NULL;
					return false;
				}
			}
			if (pOverlapped->IOType == IOCP_NetFrame::ET_Connected)
			{
				PostAccept(pEvent[nCount].sock);
			}
			else
			{
				pEvent[nCount].type = IOCP_NetFrame::ET_Close;
				nCount++;
			}
		}

		else if (pOverlapped->IOType == IOCP_NetFrame::ET_Close)
		{
			Stop();
			return false;
		}
		else if (dwIOSize == 0 && pOverlapped->IOType == IOCP_NetFrame::ET_Recv)
		{
			pEvent[nCount].type = IOCP_NetFrame::ET_Close;
			nCount++;
		}
		else
		{
			pEvent[nCount].type = pOverlapped->IOType;
			pEvent[nCount].client = pOverlapped->sock;
			pEvent[nCount].data = pOverlapped->wsaBuffer.buf;
			pEvent[nCount].nDataSize = dwIOSize;
			nCount++;
		}
		m_IOPool.Free(pOverlapped);
		pOverlapped = NULL;
#endif
		return true;
	}
	
	bool IOCP_NetFrame::PostAccept(SOCKET s)
	{
#ifdef WIN32
		if (s == INVALID_SOCKET)
			return false;

		IOOverlapped *pOverlapped = new (m_IOPool.Alloc())IOOverlapped;
		if (pOverlapped == NULL)
			return false;

		DWORD dwLocalAddressLength, dwRemoteAddressLength;
		char outPutBuf[sizeof(SOCKADDR_IN) * 2 + 32];

		::memset(&pOverlapped->overLapped, 0, sizeof(OVERLAPPED));
		dwLocalAddressLength = sizeof(SOCKADDR_IN) + 16;
		dwRemoteAddressLength = sizeof(SOCKADDR_IN) + 16;
		::memset(outPutBuf, 0, sizeof(SOCKADDR_IN) * 2 + 32);

		Socket client;
		client.Init(SOCK_STREAM);
		pOverlapped->sock = client.GetSocket();
		pOverlapped->IOType = IOCP_NetFrame::ET_Connected;
		if (!::AcceptEx(s, client.GetSocket(), outPutBuf, 0, dwLocalAddressLength, dwRemoteAddressLength, NULL, &pOverlapped->overLapped))
		{
			int nErrCode = WSAGetLastError();
			if (nErrCode != ERROR_IO_PENDING)
			{
				m_IOPool.Free(pOverlapped);
				pOverlapped = NULL;
				return false;
			}
		}
#endif
		return true;
	}
	
	bool IOCP_NetFrame::PostRecv(SOCKET s, char *pBuffer, unsigned int nBufLen)
	{
#ifdef WIN32
		IOOverlapped *pOverlapped = new (m_IOPool.Alloc())IOOverlapped;
		if (pOverlapped == NULL)
			return false;

		::memset(&pOverlapped->overLapped, 0, sizeof(OVERLAPPED));
		pOverlapped->wsaBuffer.buf = pBuffer;
		pOverlapped->wsaBuffer.len = nBufLen;
		pOverlapped->overLapped.Internal = 0;
		pOverlapped->sock = s;
		pOverlapped->IOType = IOCP_NetFrame::ET_Recv;

		DWORD dwRecv = 0;
		DWORD dwFlags = 0;
		if (::WSARecv(s, &pOverlapped->wsaBuffer, 1, &dwRecv, &dwFlags, &pOverlapped->overLapped, NULL))
		{
			int nErrCode = ::WSAGetLastError();
			if (nErrCode != ERROR_IO_PENDING)
			{
				m_IOPool.Free(pOverlapped);
				pOverlapped = NULL;
				return false;
			}
		}
#endif
		return true;
	}
	
	bool IOCP_NetFrame::PostSend(SOCKET s, char *pBuffer, unsigned int nBufLen)
	{
#ifdef WIN32
		IOOverlapped *pOverlapped = new (m_IOPool.Alloc())IOOverlapped;
		if (pOverlapped == NULL)
			return false;

		::memset(&pOverlapped->overLapped, 0, sizeof(OVERLAPPED));
		pOverlapped->wsaBuffer.buf = pBuffer;
		pOverlapped->wsaBuffer.len = nBufLen;
		pOverlapped->overLapped.Internal = 0;
		pOverlapped->sock = s;
		pOverlapped->IOType = IOCP_NetFrame::ET_Send;

		DWORD dwSend = 0;
		DWORD dwFlags = 0;

		if (::WSASend(s, &pOverlapped->wsaBuffer, 1, &dwSend, dwFlags, &pOverlapped->overLapped, NULL))
		{
			int nErrCode = ::WSAGetLastError();
			if (nErrCode != ERROR_IO_PENDING)
			{
				m_IOPool.Free(pOverlapped);
				pOverlapped = NULL;
				return false;
			}
		}
#endif
		return true;
	}
}