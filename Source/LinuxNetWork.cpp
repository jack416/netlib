#include "Epoll_NetFrame.h"
#include "LinuxNetWork.h"
#include "Connection.h"

#include <map>

namespace netlib
{
	LinuxNetWork::LinuxNetWork()
	{
#ifndef WIN32
		m_pNetFrame = new Epoll_NetFrame();
#endif
	}

	LinuxNetWork::~LinuxNetWork()
	{
#ifndef WIN32
		if (m_pNetFrame != NULL)
		{
			delete m_pNetFrame;
			m_pNetFrame = NULL;
		}
#endif
	}

	SOCKET LinuxNetWork::ListenPort(int port)
	{
#ifndef WIN32
		Socket slisten;
		if (!slisten.Init(SOCK_STREAM))
			return INVALID_SOCKET;

		slisten.SetSockMode(false);
		if (!slisten.StartServer(port))
		{
			slisten.Close();
			return INVALID_SOCKET;
		}

		m_pNetFrame->AssociateSocket(slisten.GetSocket());
		if (!m_pNetFrame->PostAccept(slisten.GetSocket()))
		{
			slisten.Close();
			return INVALID_SOCKET;
		}

		return slisten.Detach();
#endif
		return INVALID_SOCKET;
	}

	void* LinuxNetWork::HandleIOEvent(void* param)
	{
#ifndef WIN32
		int nCount = MAXPOLLSIZE;
		Epoll_NetFrame::IOEvent events[MAXPOLLSIZE];
		int i = 0;
		Socket sock;
		Socket client;
		std::map<SOCKET, int> HandleList;
		std::map<SOCKET, int>::iterator iter;

		int type = 0;

		while (!m_bIsStop)
		{
			if (HandleList.size() <= 0)
				m_pNetFrame->WaitStatus(events, nCount, true);
			else
				m_pNetFrame->WaitStatus(events, nCount, false);

			for (i = 0; i < nCount; i++)
			{
				if (events[i].type == Epoll_NetFrame::ET_Accept)
				{
					OnConnect(events[i].sock, false);
					continue;
				}

				if (events[i].type == Epoll_NetFrame::ET_Read)
					type = (1 << 0);
				else
					type = (1 << 1);

				iter = HandleList.find(events[i].sock);
				if (iter == HandleList.end())
					HandleList.insert(std::map<SOCKET, int>::value_type(events[i].sock, type));
				else
					iter->second |= type;
			}

			for (iter = HandleList.begin(); iter != HandleList.end(); iter++)
			{
				if (iter->second & (1 << 0))/**< 该操作符有可读数据*/
				{
					/**< ET模式,需把每次的数据读取完成,以免造成不可预测的问题*/
					if (OnData(iter->first, 0) != DVal_Valid)
					{
						/**< 证明数据已经读取完成,或连接已经断开,无须在下次对期进行读取操作,移除标志*/
						iter->second & ~(1 << 0);
					}
				}

				if (iter->second & (1 << 1))/**< 发送操作*/
				{
					if (OnSend(iter->first, 0) != DVal_Valid)
					{
						iter->second & ~(1 << 1);
					}
				}
			}

			iter = HandleList.begin();
			while (iter != HandleList.end())
			{
				if (!(iter->second &(1<<0)|(1<<1)))
				{
					HandleList.erase(iter);
					iter = HandleList.begin();
				}
				else
					iter++;
			}
		}
#endif
		return NULL;
	}

	DataResult LinuxNetWork::RecvData(Connection *pConnect, size_t nSize)
	{
#ifndef WIN32
		unsigned char *pBuffer = NULL;
		int nRead = 0;
		unsigned int nMaxRecv = 0;

		while (nMaxRecv < 1048576)
		{
			pBuffer = pConnect->AllocBuffer(MAX_BUFFER_SIZE);
			nRead = pConnect->GetSocket()->Receive(pBuffer, MAX_BUFFER_SIZE);
			
			/**< 无法正常从操作符的接收缓冲中读取到数据*/
			if (nRead < 0)
				return DVal_Invalid;

			if (nRead == 0)/**< EAGAIN*/
			{
				if (!m_pNetFrame->PostRecv(pConnect->GetSocket()->GetSocket(), NULL, 0))
					return DVal_Invalid;
				return DVal_WaitForRecv;
			}

			nMaxRecv += nRead;
			pConnect->WriteBuffer(nRead);
		}
#endif
		return DVal_Valid;
	}

	/**< 当返回非Valid时,存在于HandleList的操作符会被移除,情况分析
	 *	1.DVal_Invalid,发送失败,连接已经断开
	 *	2.DVal_WaitForSend
	 *		1>.所有数据已经发送成功,存在于HandleList的操作符会被移除,下次触发送时会再次向Epoll添加
	 *		2>.还有待发送数据
	 *			①.已经有另外一个发送操作流程开始了,存在于HandleList的操作符会被移除,由另外一个发送流程再次添加
	 *			②.没有发送操作,那么存在于HandleList的操作符会被移除,并再次向Epoll添加信号
	 *	否则再次进入HandleIOEvent的循环,处理HandleList中的操作符,再次进入SendData
	 */
	DataResult LinuxNetWork::SendData(Connection *pConnect, size_t uSize)
	{
#ifndef WIN32
		try
		{
			DataResult ret = DVal_WaitForSend;
			unsigned char buffer[MAX_BUFFER_SIZE];
			int nSize = 0;/**< 每次发送的数据大小*/
			int nSendSize = 0;/**< 待发送数据大小*/
			int nFinishedSize = 0;/**< 每次实际发送的数据大小*/
			nSendSize = pConnect->m_SendBuffer.GetLength();
			if (nSendSize > 0)
			{
				nSize = 0;
				if (nSendSize > MAX_BUFFER_SIZE)
				{
					pConnect->m_SendBuffer.ReadBuffer(buffer, MAX_BUFFER_SIZE, false);
					nSize += MAX_BUFFER_SIZE;
					nSendSize -= MAX_BUFFER_SIZE;
					ret = DVal_Valid;
				}
				else
				{
					pConnect->m_SendBuffer.ReadBuffer(buffer, nSendSize, false);
					nSize += nSendSize;
					nSendSize = 0;
					ret = DVal_WaitForSend;
				}

				nFinishedSize = pConnect->GetSocket()->Send((void*)buffer, nSize);
				if (nFinishedSize == -1)
					ret = DVal_Invalid;
				else
				{
					pConnect->m_SendBuffer.ReadBuffer(buffer, nFinishedSize, true);/**< 发送出去的从队列中清除*/
					if (nFinishedSize < nSize)
						ret = DVal_WaitForSend;
				}
			}
			if (ret == DVal_Valid || ret == DVal_Invalid)
				return ret;
			
			/**< 结束本次发送操作*/
			pConnect->EndSend();
			
			/**
			 *	所有待发送数据已经发送完毕,返回等待发送状态,此时会将该连接的操作符从HandleList队列中的移除,
			 *	等待上次服务器发送数据时在Connection::SendData触发PostSend,从而再次回到Epoll等待状态
			 **/
			if (pConnect->m_SendBuffer.GetLength() <= 0)
				return ret;
			
			/**
			 *	还有待发送数据,尝试下一次发送流程时已有一个发送流程在进行,
			 *	直接返回等待发送状态,此时会将该连接的操作符从HandleList队列中的移除
			 *	当然在另外一个发送流程中会再次向Epoll中添加该连接的操作符,所以在下次Epoll等待中,
			 *	依然可以获得该连接操作符的发送信号
			 **/
			if (!pConnect->StartSend())
				return ret;

			/**
			 *	OK,没有发送操作,那么就在此开始下一次发送流程
			 **/
			m_pNetFrame->PostSend(pConnect->GetSocket()->GetSocket(), NULL, 0);
			return ret;
		}
		catch (...){}
#endif
		return DVal_Valid;
	}
}