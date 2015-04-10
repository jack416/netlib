#include "Epoll_NetFrame.h"
#include "../../ThreadLib/Include/CodeAddress.h"

#ifndef WIN32
#include <sys/epoll.h>
#endif

namespace netlib
{
	Epoll_NetFrame::Epoll_NetFrame()
	{
#ifndef WIN32
		m_bIsStop = true;
		m_QueueAccept = new InterLockQueue(MAXPOLLSIZE);
		m_QueueRead = new InterLockQueue(MAXPOLLSIZE);
		m_QueueSend = new InterLockQueue(MAXPOLLSIZE);
#endif
	}

	Epoll_NetFrame::~Epoll_NetFrame()
	{
#ifndef WIN32
		if (m_QueueAccept != NULL)
		{
			delete m_QueueAccept;
			m_QueueAccept = NULL;
		}

		if (m_QueueRead != NULL)
		{
			delete m_QueueRead;
			m_QueueRead = NULL;
		}

		if (m_QueueSend != NULL)
		{
			delete m_QueueSend;
			m_QueueSend = NULL;
		}
#endif
	}

	bool Epoll_NetFrame::Start(int nMaxSize)
	{
#ifndef WIN32
		/**< ignore sigpipe*/
		struct sigaction sa;
		sa.sa_handler = SIG_IGN;
		sigaction(SIGPIPE, &sa, 0);

		m_nMaxSize = nMaxSize;
		m_sExit = socket(PF_INET, SOCK_STREAM, 0);

		m_hEpollAccept = ::epoll_create(m_nMaxSize);
		m_hEpollRead = ::epoll_create(m_nMaxSize);
		m_hEpollSend = ::epoll_create(m_nMaxSize);

		if (m_hEpollAccept == -1 || m_hEpollRead == -1 || m_hEpollSend == -1)
			return false;

		m_bIsStop = false;

		m_ThreadAccept.Run((DWORD)GET_MEMBER_CALLBACK(Epoll_NetFrame, WaitAcceptProc), (DWORD)this, (DWORD)NULL);
		m_ThreadRead.Run((DWORD)GET_MEMBER_CALLBACK(Epoll_NetFrame, WaitReadProc), (DWORD)this, (DWORD)NULL);
		m_ThreadSend.Run((DWORD)GET_MEMBER_CALLBACK(Epoll_NetFrame, WaitSendProc), (DWORD)this, (DWORD)NULL);
#endif
		return true;
	}
	
	bool Epoll_NetFrame::Stop()
	{
#ifndef WIN32
		if (m_bIsStop)
			return true;

		m_bIsStop = true;

		PostAccept(m_sExit);
		PostSend(m_sExit, NULL, 0);
		PostRecv(m_sExit, NULL, 0);

		m_ThreadAccept.Stop();
		m_ThreadRead.Stop();
		m_ThreadSend.Stop();

#endif
		return true;
	}
	
	bool Epoll_NetFrame::AssociateSocket(SOCKET s)
	{
#ifndef WIN32
		epoll_event ev;
		ev.events = EPOLLONESHOT;
		ev.data.fd = s;

		if (::epoll_ctl(m_hEpollRead, EPOLL_CTL_ADD, s, &ev) < 0)
			return false;

		if (::epoll_ctl(m_hEpollSend, EPOLL_CTL_ADD, s, &ev) < 0)
			return false;
#endif
		return true;
	}
	
	bool Epoll_NetFrame::WaitStatus(void *event, int &nCount, bool bBlock)
	{
#ifndef WIN32
		nCount = 0;
		IOEvent *pEvent = (IOEvent*)event;
		void *sock;

		while (!m_bIsStop)
		{
			while (!m_bIsStop)
			{
				sock = m_QueueAccept->Pop();
				if (sock == NULL)
				{
					m_WaitEventAccept.Set();
					break;
				}
				pEvent[nCount].sock = (SOCKET)sock;
				pEvent[nCount++].type = Epoll_NetFrame::ET_Accept;
			}

			while (!m_bIsStop)
			{
				sock = m_QueueSend->Pop();
				if (sock == NULL)
				{
					m_WaitEventSend.Set();
					break;
				}
				pEvent[nCount].sock = (SOCKET)sock;
				pEvent[nCount++].type = Epoll_NetFrame::ET_Send;
			}

			while (!m_bIsStop)
			{
				sock = m_QueueRead->Pop();
				if (sock == NULL)
				{
					m_WaitEventRead.Set();
					break;
				}
				pEvent[nCount].sock = (SOCKET)sock;
				pEvent[nCount++].type = Epoll_NetFrame::ET_Read;
			}

			if (nCount > 0)
				break;

			if (!bBlock)
				return false;

			m_EpollEvent.WaitEvent();
		}
#endif
		return true;
	}
	
	bool Epoll_NetFrame::PostAccept(SOCKET s)
	{
#ifndef WIN32
		epoll_event ev;
		ev.events = EPOLLIN|EPOLLET;/**< ʹ��ET��ʽ*/
		ev.data.fd = s;

		if (epoll_ctl(m_hEpollAccept, EPOLL_CTL_ADD, s, &ev) < 0)
			return false;
#endif
		return true;
	}
	
	bool Epoll_NetFrame::PostRecv(SOCKET s, char *pBuffer, unsigned int nBufLen)
	{
#ifndef WIN32
		epoll_event ev;
		ev.events = EPOLLIN|EPOLLONESHOT;
		ev.data.fd = s;

		if (epoll_ctl(m_hEpollRead, EPOLL_CTL_MOD, s, &ev) < 0)
			return false;
#endif
		return true;
	}
	
	bool Epoll_NetFrame::PostSend(SOCKET s, char *pBuffer, unsigned int nBufLen)
	{
#ifndef WIN32
		epoll_event ev;
		ev.events = EPOLLOUT|EPOLLONESHOT;
		ev.data.fd = s;

		if (::epoll_ctl(m_hEpollSend, EPOLL_CTL_MOD, s, &ev) < 0)
			return false;
#endif
		return true;
	}

	bool Epoll_NetFrame::RemoveSocket(SOCKET s)
	{
#ifndef WIN32
		if (::epoll_ctl(m_hEpollRead, EPOLL_CTL_DEL, s, NULL) < 0)
			return false;

		if (::epoll_ctl(m_hEpollSend, EPOLL_CTL_DEL, s, NULL) < 0)
			return false;
#endif
		return true;
	}

	void *Epoll_NetFrame::WaitAcceptProc(void *param)
	{
#ifndef WIN32
		epoll_event *ev = new epoll_event[m_nMaxSize];
		Socket sock;
		Socket client;
		SOCKET hSock;

		while (!m_bIsStop)
		{
			int nPollCount = ::epoll_wait(m_hEpollAccept, ev, m_nMaxSize, -1);
			if (nPollCount == -1)
				break;

			for (int i = 0; i < nPollCount; i++)
			{
				if (ev[i].data.fd == m_sExit)
				{
					::closesocket(m_sExit);
					break;
				}

				sock.Detach();
				sock.Attach(ev[i].data.fd);

				while (true)
				{
					sock.Accept(client);
					if (client.GetSocket() == INVALID_SOCKET)
						break;

					client.SetSockMode();/**< ������*/
					hSock = client.Detach();
					AssociateSocket(hSock);

					while (!m_QueueAccept->Push((void*)hSock))
						m_WaitEventAccept.WaitEvent();

					m_EpollEvent.Set();
				}
			}
		}

		delete [] ev;
#endif
		return NULL;
	}
	
	void *Epoll_NetFrame::WaitReadProc(void *param)
	{
#ifndef WIN32
		epoll_event *ev = new epoll_event[m_nMaxSize];

		while (!m_bIsStop)
		{
			int nPollCount = ::epoll_wait(m_hEpollRead, ev, m_nMaxSize, -1);
			if (nPollCount == -1)
				break;

			for (int i = 0; i < nPollCount; i++)
			{
				if (ev[i].data.fd == m_sExit)
				{
					::closesocket(m_sExit);
					break;
				}

				while (!m_QueueRead->Push((void*)ev[i].data.fd))
					m_WaitEventRead.WaitEvent();

				m_EpollEvent.Set();
			}
		}

		delete [] ev;
#endif
		return NULL;
	}
	
	void *Epoll_NetFrame::WaitSendProc(void *param)
	{
#ifndef WIN32
		epoll_event *ev = new epoll_event[m_nMaxSize];

		while (!m_bIsStop)
		{
			int nPollCount = ::epoll_wait(m_hEpollSend, ev, m_nMaxSize, -1);
			if (nPollCount == -1)
				break;

			for (int i = 0; i < nPollCount; i++)
			{
				if (ev[i].data.fd == m_sExit)
				{
					::closesocket(m_sExit);
					break;
				}

				while (!m_QueueSend->Push((void*)ev[i].data.fd))
					m_WaitEventSend.WaitEvent();

				m_EpollEvent.Set();
			}
		}

		delete [] ev;
#endif
		return NULL;
	}
}
