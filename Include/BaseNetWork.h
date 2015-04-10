#ifndef __NETWORK_H__
#define __NETWORK_H__

#include "Socket.h"

#include "../../ThreadLib/Include/ThreadPool.h"
#include "../../ThreadLib/Include/Mutex.h"
#include "../../ThreadLib/Include/Event.h"
#include "../../ThreadLib/Include/MemPool.h"
#include "../../ThreadLib/Include/InterLock.h"
#include "../../ThreadLib/Include/CodeAddress.h"

#include <map>
#include <string>

namespace netlib
{

	enum DataResult
	{
		DVal_Valid = 0,
		DVal_Invalid,
		DVal_WaitForRecv,
		DVal_WaitForSend,
	};

	typedef struct
	{
		int	avrCount;/**< ƽ��������*/
		int	heartTime;/**< �������ʱ��*/
		int ioThreadCount;/**< I/O�߳���*/
		int workThreadCount;/**< �����߳���*/
		int nReconnectTime;/**< ����*/
	}ServerConfig;

	class Connection;
	class INetFrame;
	class BaseServer;
	typedef std::map<SOCKET, Connection*>		MapConnection;
	typedef std::map<int,SOCKET>				MapListenPort;/**< �����˿��б���*/
	typedef std::map<unsigned long long,SOCKET>	MapServer;/**< Ŀ��������б�����*/

	class BaseNetWork
	{
		friend class BaseServer;
	public:
		BaseNetWork();
		virtual ~BaseNetWork();

	public:
		void SetConfig(ServerConfig config);

		bool StartServer();
		void StopServer();
		void WaitStop();

		void CloseConnection(SOCKET sock);
		bool Listen(int port);
		bool Connect(const char *ip, int port);

		Connection *GetConnection(int nID);
	protected:
		
		bool OnConnect(SOCKET sock, bool bIsServer);
		void OnClose(SOCKET sock);

		/**< ��ȡ����*/
		DataResult OnData(SOCKET sock, size_t nSize);

		/**< ���Ͳ���*/
		DataResult OnSend(SOCKET sock, size_t nSize);

		void SendData(int nID, char *msg, int nSize);
		void CloseConnection(Connection *pConnect);

		/**< �̺߳���*/
		void* NetWorkProc(void *param);
		void* ConnectProc(void *param);
		void* IOEventProc(void *param);
		void* CloseProc(void *param);
		
		bool MonitorConnect(Connection *pConnect);

		/**< ����*/
		virtual SOCKET ListenPort(int port);

		/**< �������б��������������¼�*/
		virtual void* HandleIOEvent(void* param);
		virtual DataResult RecvData(Connection *pConnect, size_t nSize);
		virtual DataResult SendData(Connection *pConnect, size_t nSize);

	private:
		void* Main(void* param);

		void HeartMonitor();

		void ReConnectAll();
		void CloseConnection(MapConnection::iterator iter);

		bool ListenAll();

		SOCKET ConnectOtherServer(const char* ip, int port);
		bool ConnectAll();

	protected:
		mt::MemPool*	m_pConnectionPool;/**< �����ڴ��*/
		mt::Event		m_ExitEvent;/**< �˳��¼�*/
		int				m_nAvrCount;/**< ƽ��������*/
		bool			m_bIsStop;/**< ֹͣ��־*/

		MapConnection	m_ConnecttionList;
		mt::Mutex		m_ConnectionMutex;

		int				m_nHeartTime;
		int				m_nReconnectTime;
		mt::Thread		m_MainThread;
		INetFrame*		m_pNetFrame;/**< �������Ա*/

		mt::ThreadPool	m_IOThreadPool;/**< I/O�̳߳�*/
		int				m_nIOThreadCount;/**< I/O�̳߳�����*/

		mt::ThreadPool	m_WorkThreadPool;/**< �����̳߳�*/
		int				m_nWorkThreadCount;/**< �����̳߳�����*/

		BaseServer*		m_pNetServer;

		MapListenPort	m_ListenPortList;/**< �����˿��б�*/
		mt::Mutex		m_ListenPortMutex;

		MapServer		m_ServerList;/**< Ŀ��������б�*/
		mt::Mutex		m_ServerMutex;
	};
}

#endif