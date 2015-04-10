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
				if (iter->second & (1 << 0))/**< �ò������пɶ�����*/
				{
					/**< ETģʽ,���ÿ�ε����ݶ�ȡ���,������ɲ���Ԥ�������*/
					if (OnData(iter->first, 0) != DVal_Valid)
					{
						/**< ֤�������Ѿ���ȡ���,�������Ѿ��Ͽ�,�������´ζ��ڽ��ж�ȡ����,�Ƴ���־*/
						iter->second & ~(1 << 0);
					}
				}

				if (iter->second & (1 << 1))/**< ���Ͳ���*/
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
			
			/**< �޷������Ӳ������Ľ��ջ����ж�ȡ������*/
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

	/**< �����ط�Validʱ,������HandleList�Ĳ������ᱻ�Ƴ�,�������
	 *	1.DVal_Invalid,����ʧ��,�����Ѿ��Ͽ�
	 *	2.DVal_WaitForSend
	 *		1>.���������Ѿ����ͳɹ�,������HandleList�Ĳ������ᱻ�Ƴ�,�´δ�����ʱ���ٴ���Epoll���
	 *		2>.���д���������
	 *			��.�Ѿ�������һ�����Ͳ������̿�ʼ��,������HandleList�Ĳ������ᱻ�Ƴ�,������һ�����������ٴ����
	 *			��.û�з��Ͳ���,��ô������HandleList�Ĳ������ᱻ�Ƴ�,���ٴ���Epoll����ź�
	 *	�����ٴν���HandleIOEvent��ѭ��,����HandleList�еĲ�����,�ٴν���SendData
	 */
	DataResult LinuxNetWork::SendData(Connection *pConnect, size_t uSize)
	{
#ifndef WIN32
		try
		{
			DataResult ret = DVal_WaitForSend;
			unsigned char buffer[MAX_BUFFER_SIZE];
			int nSize = 0;/**< ÿ�η��͵����ݴ�С*/
			int nSendSize = 0;/**< ���������ݴ�С*/
			int nFinishedSize = 0;/**< ÿ��ʵ�ʷ��͵����ݴ�С*/
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
					pConnect->m_SendBuffer.ReadBuffer(buffer, nFinishedSize, true);/**< ���ͳ�ȥ�ĴӶ��������*/
					if (nFinishedSize < nSize)
						ret = DVal_WaitForSend;
				}
			}
			if (ret == DVal_Valid || ret == DVal_Invalid)
				return ret;
			
			/**< �������η��Ͳ���*/
			pConnect->EndSend();
			
			/**
			 *	���д����������Ѿ��������,���صȴ�����״̬,��ʱ�Ὣ�����ӵĲ�������HandleList�����е��Ƴ�,
			 *	�ȴ��ϴη�������������ʱ��Connection::SendData����PostSend,�Ӷ��ٴλص�Epoll�ȴ�״̬
			 **/
			if (pConnect->m_SendBuffer.GetLength() <= 0)
				return ret;
			
			/**
			 *	���д���������,������һ�η�������ʱ����һ�����������ڽ���,
			 *	ֱ�ӷ��صȴ�����״̬,��ʱ�Ὣ�����ӵĲ�������HandleList�����е��Ƴ�
			 *	��Ȼ������һ�����������л��ٴ���Epoll����Ӹ����ӵĲ�����,�������´�Epoll�ȴ���,
			 *	��Ȼ���Ի�ø����Ӳ������ķ����ź�
			 **/
			if (!pConnect->StartSend())
				return ret;

			/**
			 *	OK,û�з��Ͳ���,��ô���ڴ˿�ʼ��һ�η�������
			 **/
			m_pNetFrame->PostSend(pConnect->GetSocket()->GetSocket(), NULL, 0);
			return ret;
		}
		catch (...){}
#endif
		return DVal_Valid;
	}
}