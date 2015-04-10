#include "Socket.h"
#include <stdio.h>

#ifdef WIN32
#pragma comment(lib, "ws2_32.lib")
#endif

namespace netlib
{
	bool Socket::SocketStartup()
	{
#ifdef WIN32
		WSADATA wsaData;

		WORD wVersion = MAKEWORD(1, 1);
		int nRet = ::WSAStartup(wVersion, &wsaData);
		if (nRet != 0)
			return false;

		if (LOBYTE (wsaData.wVersion) != 1 || HIBYTE(wsaData.wVersion) != 1)
		{
			::WSACleanup();
			return false;
		}
#endif
		return true;
	}

	void Socket::SocketCleanup()
	{
#ifdef WIN32
		::WSACleanup();
#endif
	}

	bool Socket::InitForIOCP(SOCKET hSocket)
	{
		bool bRet = true;
#ifdef WIN32
		bRet = (::setsockopt(hSocket, SOL_SOCKET, SO_UPDATE_ACCEPT_CONTEXT, (char*)&hSocket, sizeof(hSocket)) == 0);
#endif
		return bRet;
	}

	Socket::Socket()
	{
		m_hSocket = INVALID_SOCKET;
		m_bIsBlock = true;
		m_bIsOpen = false;
	}

	Socket::Socket(SOCKET s, int proType)
	{
		m_hSocket = s;
		m_bIsBlock = true;
		m_bIsOpen = false;
		Init(proType);
		InitPeerAddress();
		InitLocalAddress();
	}

	Socket::~Socket()
	{
	}

	SOCKET Socket::GetSocket()
	{
		return m_hSocket;
	}

	bool Socket::Init(int proType)
	{
		if (m_bIsOpen)
			return true;

		if (m_hSocket == INVALID_SOCKET)
		{
			m_hSocket = ::socket(AF_INET, proType, 0);
			if (m_hSocket == INVALID_SOCKET)
				return false;
		}

		m_bIsOpen = true;

		return true;
	}

	void Socket::Close()
	{
		if (m_hSocket == INVALID_SOCKET)
			return;

		if (m_bIsOpen)
		{
			closesocket(m_hSocket);
		}
		m_bIsOpen = false;
		m_hSocket = INVALID_SOCKET;
	}

	bool Socket::IsClosed()
	{
		return !m_bIsOpen;
	}

	int Socket::Send(const void* pBuffer, int nBufLen, int nFlags)
	{
		int nSendLen = ::send(m_hSocket, (char*)pBuffer, nBufLen, nFlags);
		if (nSendLen < 0)
		{
#ifdef WIN32
			int nErrCode = ::GetLastError();
			if (nErrCode == WSAEWOULDBLOCK)
				return 0;
			return SOCKET_ERROR;
#else
			if (errno == EAGAIN)
				return 0;
			return SOCKET_ERROR;
#endif
		}

		if (nSendLen <= nBufLen)
			return nSendLen;

		return SOCKET_ERROR;
	}

	int Socket::Receive(void *pBuffer, int nBufLen, bool bCheckData, long lSecond, long lMinSecond)
	{
		if (TimeOut(lSecond, lMinSecond))
			return SOCKET_ERROR;

		int nRet = 0, nFlags = 0;
		if (bCheckData)
			nFlags = MSG_PEEK;

		nRet = ::recv(m_hSocket, (char*)pBuffer, nBufLen, nFlags);
		if (nRet == 0)
			return SOCKET_ERROR;

		if (nRet != SOCKET_ERROR)
			return nRet;

#ifdef WIN32
		int nErrCode = ::GetLastError();
		if (nErrCode == WSAEWOULDBLOCK)
			return 0;
#else
		if (errno == EAGAIN)
			return 0;
#endif
		return SOCKET_ERROR;
	}

	bool Socket::Connect(const char* lpszAddress, unsigned short nPort)
	{
		assert(lpszAddress != NULL);

		sockaddr_in sockAddr;
		::memset(&sockAddr, 0, sizeof(sockAddr));
		sockAddr.sin_family = AF_INET;
		sockAddr.sin_port = htons(nPort);
		sockAddr.sin_addr.s_addr = inet_addr(lpszAddress);

		if (::connect(m_hSocket, (sockaddr*)&sockAddr, sizeof(sockAddr)) != SOCKET_ERROR)
		{
			InitPeerAddress();
			InitLocalAddress();
			return true;
		}

		return false;
	}

	bool Socket::StartServer(int nPort)
	{
		if (!Bind(nPort, NULL))
			return false;

		return Listen();
	}

	bool Socket::Accept(Socket &client)
	{
		assert(client.m_hSocket == INVALID_SOCKET);
		client.m_hSocket = ::accept(m_hSocket, NULL, NULL);
		if (client.m_hSocket == INVALID_SOCKET)
		{
#ifdef WIN32
			if (GetLastError() == WSAEWOULDBLOCK)
				return 0;
#else
			if (errno == EAGAIN)
				return 0;
#endif
			return false;
		}

		client.m_bIsOpen = true;
		client.InitPeerAddress();
		client.InitLocalAddress();
		
		return true;
	}

	bool Socket::InitPeerAddress()
	{
		assert(m_hSocket != INVALID_SOCKET);

		sockaddr_in sockAddr;
		::memset(&sockAddr, 0, sizeof(sockAddr));
		socklen_t nSockLen = sizeof(sockAddr);

		if (::getpeername(m_hSocket, (sockaddr*)&sockAddr, &nSockLen) == SOCKET_ERROR)
		{
			return false;
		}

		m_nPeerPort = ntohs(sockAddr.sin_port);
		m_strPeerAddr = inet_ntoa(sockAddr.sin_addr);

		return true;
	}

	void Socket::GetPeerAddress(std::string &strPeerAddr, int &nPort)
	{
		strPeerAddr = m_strPeerAddr;
		nPort = m_nPeerPort;
	}

	bool Socket::InitLocalAddress()
	{
		sockaddr_in sockAddr;
		::memset(&sockAddr, 0, sizeof(sockAddr));
		socklen_t nSockLen = sizeof(sockAddr);

		if (::getsockname(m_hSocket, (sockaddr*)&sockAddr, &nSockLen) == SOCKET_ERROR)
		{
			return false;
		}

		m_nLocalPort = ntohs(sockAddr.sin_port);
		m_strLocalAddr = inet_ntoa(sockAddr.sin_addr);
		
		return true;
	}

	void Socket::GetLocalAddress(std::string &strLocalAddr, int &nPort)
	{
		strLocalAddr = m_strLocalAddr;
		nPort = m_nLocalPort;
	}

	bool Socket::SetSockMode(bool bBlock)
	{
		m_bIsBlock = bBlock;
#ifdef WIN32
		unsigned long arg = 1;
		if (m_bIsBlock)	arg = 0;
		int nRet = ::ioctlsocket(m_hSocket, FIONBIO, &arg);
		if (nRet == SOCKET_ERROR)
			return false;
#else
		int nFlags = fcntl(m_hSocket, F_GETFL, 0);
		if (m_bIsBlock)
		{
			fcntl(m_hSocket, F_SETFL, nFlags & (~O_NONBLOCK & 0xffffffff));
		}
		else
		{
			fcntl(m_hSocket, F_SETFL, nFlags|O_NONBLOCK);
		}
#endif
		return true;
	}

	bool Socket::SetSockOpt(int nOptName, const void* lpOptValue, int nOptLen, int nLevel/* = SOL_SOCKET*/)
	{
		return (::setsockopt(m_hSocket, nLevel, nOptName, (char*)lpOptValue, nOptLen) != SOCKET_ERROR);
	}

	void Socket::Attach(SOCKET hSocket)
	{
		m_hSocket = hSocket;
		m_bIsBlock = true;
		m_bIsOpen = true;
		InitPeerAddress();
		InitLocalAddress();
	}
	
	SOCKET Socket::Detach()
	{
		SOCKET sock = m_hSocket;
		m_hSocket = INVALID_SOCKET;
		m_bIsBlock = true;
		m_bIsOpen = false;
		return sock;
	}

	bool Socket::TimeOut(long lSecond, long lMinSecond)
	{
		if (lSecond <= 0 || lMinSecond <= 0)
			return false;

		timeval	outtime;
		outtime.tv_sec = lSecond;
		outtime.tv_usec = lMinSecond;
		
		int nRet;

#ifdef WIN32
		FD_SET readfds = {1, m_hSocket};
		nRet = ::select(0, &readfds, NULL, NULL, &outtime);
#else
		fd_set readfds;
		FD_ZERO(&readfds);
		FD_SET(m_hSocket, &readfds);
		nRet = ::select(m_hSocket + 1, &readfds, NULL, NULL, &outtime);
#endif
		if (nRet == SOCKET_ERROR)
			return true;

		if (nRet == 0)
			return true;

		return false;
	}
	
	void Socket::GetAddress(const sockaddr_in &sockAddr, std::string &strAddr, int &nPort)
	{
		strAddr = inet_ntoa(sockAddr.sin_addr);
		nPort = ntohs(sockAddr.sin_port);
	}
	
	bool Socket::Bind(unsigned short nPort, char *lpszAddress/* = NULL*/)
	{
		::memset(&m_sockAddr, 0, sizeof(m_sockAddr));
		m_sockAddr.sin_family = AF_INET;
		if (lpszAddress == NULL)
		{
			m_sockAddr.sin_addr.s_addr = INADDR_ANY;
		}
		else
		{
			unsigned long lRet = ::inet_addr(lpszAddress);
			if (lRet == INADDR_ANY)
				return false;

			m_sockAddr.sin_addr.s_addr = lRet;
		}

		m_sockAddr.sin_port = htons(nPort);
		
		return (::bind(m_hSocket, (sockaddr*)&m_sockAddr, sizeof(m_sockAddr)) != SOCKET_ERROR);
	}

	bool Socket::Listen(int nConnection/* = SOMAXCONN*/)
	{
		return (::listen(m_hSocket, nConnection) != SOCKET_ERROR);
	}
}
