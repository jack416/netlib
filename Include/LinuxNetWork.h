#ifndef __LINUX_NETWORK_H__
#define __LINUX_NETWORK_H__

#include "BaseNetWork.h"

namespace netlib
{
	class LinuxNetWork
		:public BaseNetWork
	{
	public:
		LinuxNetWork();
		~LinuxNetWork();

	protected:
		SOCKET ListenPort(int port);

		void* HandleIOEvent(void* param);
		DataResult RecvData(Connection *pConnect, size_t nSize);
		DataResult SendData(Connection *pConnect, size_t uSize);

	private:

	};
}
#endif