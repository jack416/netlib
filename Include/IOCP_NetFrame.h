#ifndef __IOCP_NETFRAME_H__
#define __IOCP_NETFRAME_H__

#ifdef WIN32
#include <winsock2.h>  
#include <mswsock.h>
#include <windows.h>
#else
typedef int SOCKADDR_IN;
typedef int WSAOVERLAPPED;
typedef int WSABUF;
typedef int HANDLE;
typedef int OVERLAPPED;
#endif

#include "INetFrame.h"
#include "../../ThreadLib/Include/MemPool.h"
#include "../../ThreadLib/Include/Mutex.h"

namespace netlib
{
	
	class IOCP_NetFrame
		:public INetFrame
	{
	public:
		enum EventType
		{
			ET_Unknow = 0,
			ET_Connected,
			ET_Close,
			ET_Recv,
			ET_Send,
		};

		typedef struct
		{
			SOCKET sock;
			SOCKET client;
			EventType type;
			char *data;
			unsigned int nDataSize;
		}IOEvent;

		typedef struct IOOverlapped
		{
			OVERLAPPED overLapped;
			WSABUF wsaBuffer;
			SOCKET sock;
			EventType IOType;
		}IOOverlapped;

	public:
		IOCP_NetFrame();
		virtual ~IOCP_NetFrame();

		/**< 实例INetFrame的所有抽象函数*/
	public:
		bool Start(int nMaxSize);
		bool Stop();
		bool AssociateSocket(SOCKET s);
		bool WaitStatus(void *IOEvent, int &nCount, bool bBlock);
		bool PostAccept(SOCKET s);
		bool PostRecv(SOCKET s, char *pBuffer, unsigned int nBufLen);
		bool PostSend(SOCKET s, char *pBuffer, unsigned int nBufLen);

	private:
		mt::MemPool	m_IOPool;/**< IO操作内存池*/
		SOCKET	m_sockListen;
		HANDLE	m_hCompletionPort;
		int		m_nProcCount;
		IOOverlapped	m_stopOverlapped;
		//LPFN_ACCEPTEX	m_lpfnAcceptEx;
	};
}

#endif