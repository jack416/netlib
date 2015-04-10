#ifndef __CONNECTION_H__
#define __CONNECTION_H__

#include "INetFrame.h"
#include "IOBufferMgr.h"
#include "Socket.h"

#include "../../ThreadLib/Include/MemPool.h"
#include "../../ThreadLib/Include/Mutex.h"

#include <string>
#include <time.h>

namespace netlib
{
	class BaseNetWork;

	class Connection
	{
		friend class WinNetWork;
		friend class LinuxNetWork;
	public:
		Connection(SOCKET sock, bool bIsServer, INetFrame *pNetFrame, BaseNetWork *pNetWork, mt::MemPool *pConnectionPool);
		virtual ~Connection();

	public:
		unsigned char *AllocBuffer(size_t nSize);
		void WriteBuffer(size_t nLength);

		int GetID();
		Socket *GetSocket();

		bool HasData();
		unsigned int GetLength();

		bool ReadData(unsigned char *data, size_t nLength, bool bClear = true);
		bool SendData(const unsigned char *data, size_t nSize);
		bool StartSend();
		void EndSend();
		bool TryClose();
		void Close();

		void RefreshHeart();
		time_t GetLastHeartBeep();

		bool IsServer();
		bool IsConnected();
		void Disconnect();

		long AddRead();
		long ReleaseRead();
		void SetRead();

		void AddRef();
		void Release();

		void GetAddress(std::string &strAddress, int &nPort);
		void GetServerAddress(std::string &strAddress, int &nPort);

	private:
		int m_nRefCount;
		bool m_bIsConnected;
		int m_nCloseCount;

		IOBufferMgr m_RecvBuffer;
		int m_nReadCount;
		bool m_bReadable;

		IOBufferMgr m_SendBuffer;
		int m_nSendCount;
		bool m_bSendable;
		mt::Mutex m_SendMutex;

		Socket m_Socket;

		INetFrame* m_pNetFrame;
		BaseNetWork* m_pNetWork;

		int m_id;
		time_t m_LastHeart;
		bool m_bIsServer;
		mt::MemPool* m_pConnectionPool;
	};
}

#endif
