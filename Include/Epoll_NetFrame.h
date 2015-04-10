#ifndef __EPOLL_NETFRAME_H__
#define __EPOLL_NETFRAME_H__

#include "INetFrame.h"
#include "InterLockQueue.h"
#include "../../ThreadLib/Include/Thread.h"
#include "../../ThreadLib/Include/Event.h"

namespace netlib
{
	class Epoll_NetFrame
		:public INetFrame
	{
	public:
		enum EventType
		{
			ET_Accept = 0,
			ET_Read,
			ET_Send,
		};

		typedef struct
		{
			SOCKET sock;
			EventType type;
		}IOEvent;

	public:
		Epoll_NetFrame();
		virtual ~Epoll_NetFrame();

	public:
		bool Start(int nMaxSize);
		bool Stop();
		bool AssociateSocket(SOCKET s);
		bool WaitStatus(void *IOEvent, int &nCount, bool bBlock);
		bool PostAccept(SOCKET s);
		bool PostRecv(SOCKET s, char *pBuffer, unsigned int nBufLen);
		bool PostSend(SOCKET s, char *pBuffer, unsigned int nBufLen);

		bool RemoveSocket(SOCKET s);

	private:
		void *WaitAcceptProc(void *param);
		void *WaitReadProc(void *param);
		void *WaitSendProc(void *param);

	private:
		bool				m_bIsStop;
		int					m_nMaxSize;
		SOCKET				m_sExit;
		mt::Event			m_EpollEvent;

		int					m_hEpollAccept;
		mt::Thread			m_ThreadAccept;
		mt::Event			m_WaitEventAccept;
		InterLockQueue*		m_QueueAccept;
		
		int					m_hEpollRead;
		mt::Thread			m_ThreadRead;
		mt::Event			m_WaitEventRead;
		InterLockQueue*		m_QueueRead;

		int					m_hEpollSend;
		mt::Thread			m_ThreadSend;
		mt::Event			m_WaitEventSend;
		InterLockQueue*		m_QueueSend;
	};
}

#endif