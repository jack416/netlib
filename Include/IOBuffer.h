#ifndef __IOBUFFER_H__
#define __IOBUFFER_H__

#include <stddef.h>
#include "../../ThreadLib/Include/MemPool.h"

#define MAX_BUFFER_SIZE	8192

class mt::MemPool;

namespace netlib
{
	class IOBuffer
	{
		friend class IOBufferMgr;
	public:
		unsigned char *AllocBuffer(size_t nSize);
		void WriteBuffer(size_t nLength);
		size_t ReadBuffer(unsigned char *data, size_t nReadSize, bool bClear = true);

		void *operator new(size_t uObjSize);
		void operator delete(void *pObj);

	private:
		IOBuffer();
		~IOBuffer();

	private:
		unsigned char m_rgData[MAX_BUFFER_SIZE];/**< 缓冲区*/
		size_t m_nSize;/**< 数据大小*/
		size_t m_nReadPos;/**< 读取数据的开始位置*/

		static mt::MemPool *m_pBufferPool;
	};
}

#endif