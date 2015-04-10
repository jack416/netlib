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
		int	avrCount;/**< 平均连接数*/
		int	heartTime;/**< 心跳间隔时间*/
		int ioThreadCount;/**< I/O线程数*/
		int workThreadCount;/**< 工作线程数*/
		int nReconnectTime;/**< 重连*/
	}ServerConfig;

	class Connection;
	class INetFrame;
	class BaseServer;
	typedef std::map<SOCKET, Connection*>		MapConnection;
	typedef std::map<int,SOCKET>				MapListenPort;/**< 监听端口列表窗口*/
	typedef std::map<unsigned long long,SOCKET>	MapServer;/**< 目标服务器列表容器*/

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

		/**< 读取操作*/
		DataResult OnData(SOCKET sock, size_t nSize);

		/**< 发送操作*/
		DataResult OnSend(SOCKET sock, size_t nSize);

		void SendData(int nID, char *msg, int nSize);
		void CloseConnection(Connection *pConnect);

		/**< 线程函数*/
		void* NetWorkProc(void *param);
		void* ConnectProc(void *param);
		void* IOEventProc(void *param);
		void* CloseProc(void *param);
		
		bool MonitorConnect(Connection *pConnect);

		/**< 监听*/
		virtual SOCKET ListenPort(int port);

		/**< 处理所有被监听的描述符事件*/
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
		mt::MemPool*	m_pConnectionPool;/**< 连接内存池*/
		mt::Event		m_ExitEvent;/**< 退出事件*/
		int				m_nAvrCount;/**< 平均连接数*/
		bool			m_bIsStop;/**< 停止标志*/

		MapConnection	m_ConnecttionList;
		mt::Mutex		m_ConnectionMutex;

		int				m_nHeartTime;
		int				m_nReconnectTime;
		mt::Thread		m_MainThread;
		INetFrame*		m_pNetFrame;/**< 网络管理员*/

		mt::ThreadPool	m_IOThreadPool;/**< I/O线程池*/
		int				m_nIOThreadCount;/**< I/O线程池数量*/

		mt::ThreadPool	m_WorkThreadPool;/**< 工作线程池*/
		int				m_nWorkThreadCount;/**< 工作线程池数量*/

		BaseServer*		m_pNetServer;

		MapListenPort	m_ListenPortList;/**< 监听端口列表*/
		mt::Mutex		m_ListenPortMutex;

		MapServer		m_ServerList;/**< 目标服务器列表*/
		mt::Mutex		m_ServerMutex;
	};
}

#endif