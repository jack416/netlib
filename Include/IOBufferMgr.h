#ifndef __IOBUFFERMGR_H__
#define __IOBUFFERMGR_H__

#include "IOBuffer.h"
#include "../../ThreadLib/Include/Mutex.h"

#include <vector>

namespace netlib
{
	class IOBufferMgr
	{
	public:
		IOBufferMgr();
		~IOBufferMgr();

	public:
		unsigned char *AllocBuffer(size_t nSize);
		void WriteBuffer(size_t nLength);
		bool ReadBuffer(unsigned char *data, size_t nReadSize, bool bClear = true);
		void Clear();

		size_t GetLength();

	private:
		void CreateBuffer();

	private:
		typedef std::vector<IOBuffer*>	VecIOBuffer;
		VecIOBuffer	m_VecIOBuffer;
		IOBuffer	*m_pRecvBuffer;
		size_t		m_nBufferSize;
		mt::Mutex	m_Mutex;
	};
}

#endif