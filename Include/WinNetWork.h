#ifndef __WIN_NETWORK_H__
#define __WIN_NETWORK_H__

#include "BaseNetWork.h"

namespace netlib
{
	class WinNetWork
		:public BaseNetWork
	{
	public:
		WinNetWork();
		~WinNetWork();

	protected:
		SOCKET ListenPort(int port);

		void* HandleIOEvent(void* param);
		DataResult RecvData(Connection *pConnect, size_t nSize);
		DataResult SendData(Connection *pConnect, size_t nSize);
	private:

	};
}
#endif