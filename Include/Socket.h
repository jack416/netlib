#ifndef __SOCKET_H__
#define __SOCKET_H__

#ifdef WIN32
#include <Windows.h>
#define socklen_t int
#else
#include <errno.h>
#include <error.h>
#include <string.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <sys/types.h>
#include <unistd.h>

#define INVALID_SOCKET (unsigned int)-1
#define SOCKET_ERROR (unsigned int)-1
#define closesocket close
typedef unsigned int SOCKET;
#endif

#include <assert.h>
#include <time.h>
#include <string>

namespace netlib
{
	class Socket
	{
	public:
		static bool SocketStartup();
		static void SocketCleanup();

		static bool InitForIOCP(SOCKET);

	public:
		Socket();
		Socket(SOCKET, int);
		~Socket();

		SOCKET GetSocket();
		bool Init(int);

		void Close();

		bool IsClosed();

		int Send(const void*, int, int nFlags = 0);
		int Receive(void*, int, bool bCheckData = false, long lSecond = 0, long lMinSecond = 0);
		bool Connect(const char*, unsigned short);
		bool StartServer(int);
		bool Accept(Socket&);
		bool InitPeerAddress();
		void GetPeerAddress(std::string&, int&);
		bool InitLocalAddress();
		void GetLocalAddress(std::string&, int&);
		bool SetSockMode(bool bBlock = false);
		bool SetSockOpt(int nOptName, const void* lpOptValue, int nOptLen, int nLevel = SOL_SOCKET);
		void Attach(SOCKET hSocket);
		SOCKET Detach();

	protected:
		bool TimeOut(long, long);
		void GetAddress(const sockaddr_in &, std::string &, int &);
		bool Bind(unsigned short , char *lpszAddress = NULL);
		bool Listen(int nConnection = SOMAXCONN);

	private:
		SOCKET			m_hSocket;
		bool			m_bIsBlock;
		bool			m_bIsOpen;
		sockaddr_in		m_sockAddr;

		std::string		m_strPeerAddr;
		std::string		m_strLocalAddr;
		int				m_nPeerPort;
		int				m_nLocalPort;
	};
}

#endif
