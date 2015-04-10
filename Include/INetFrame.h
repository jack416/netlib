#ifndef __NET_FRAME_H__
#define __NET_FRAME_H__

#include "Socket.h"

#define MAXPOLLSIZE	20000

namespace netlib
{
	class INetFrame
	{
	public:
		virtual bool Start(int nMaxSize)										= 0;
		virtual bool Stop()														= 0;
		virtual bool AssociateSocket(SOCKET s)									= 0;
		virtual bool WaitStatus(void *IOEvent, int &nCount, bool bBlock)		= 0;
		virtual bool PostAccept(SOCKET s)										= 0;
		virtual bool PostRecv(SOCKET s, char *pBuffer, unsigned int nBufLen)	= 0;
		virtual bool PostSend(SOCKET s, char *pBuffer, unsigned int nBufLen)	= 0;
	};
}

#endif