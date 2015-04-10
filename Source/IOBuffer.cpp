#include "IOBuffer.h"


#include <string.h>

namespace netlib
{
	mt::MemPool* IOBuffer::m_pBufferPool = new mt::MemPool(sizeof(IOBuffer), 100);

	IOBuffer::IOBuffer()
		:m_nSize(0)
		,m_nReadPos(0)
	{
	}

	IOBuffer::~IOBuffer()
	{
	}

	void *IOBuffer::operator new(size_t nObjSize)
	{
		void *pObj = m_pBufferPool->Alloc();
		return pObj;
	}

	void IOBuffer::operator delete(void *pObj)
	{
		m_pBufferPool->Free(pObj);
	}

	unsigned char *IOBuffer::AllocBuffer(size_t nSize)
	{
		if (nSize <=0 || nSize > MAX_BUFFER_SIZE)
			return NULL;

		return &m_rgData[m_nSize];
	}
	
	void IOBuffer::WriteBuffer(size_t nLength)
	{
		m_nSize += nLength;
	}
	
	size_t IOBuffer::ReadBuffer(unsigned char *data, size_t nReadSize, bool bClear/* = true*/)
	{
		size_t nRead;
		if (nReadSize > m_nSize - m_nReadPos)
			nRead = m_nSize - m_nReadPos;

		else
			nRead = nReadSize;

		::memcpy(data, &m_rgData[m_nReadPos], nRead);
		
		if (bClear)
			m_nReadPos += nRead;

		return nRead;
	}
}