#include "IOBufferMgr.h"
#include "../../ThreadLib/Include/InterLock.h"

#include <new>
#include <string.h>

namespace netlib
{
	IOBufferMgr::IOBufferMgr()
		:m_pRecvBuffer(NULL)
		,m_nBufferSize(0)
	{
		m_VecIOBuffer.clear();
	}

	IOBufferMgr::~IOBufferMgr()
	{
		Clear();
	}

	void IOBufferMgr::CreateBuffer()
	{
		m_pRecvBuffer = new IOBuffer();
		if (m_pRecvBuffer == NULL)
			return;

		mt::AutoLock l(&m_Mutex);
		m_VecIOBuffer.push_back(m_pRecvBuffer);
	}

	unsigned char *IOBufferMgr::AllocBuffer(size_t nSize)
	{
		if (nSize > MAX_BUFFER_SIZE)
			return NULL;

		if (m_pRecvBuffer == NULL)
			CreateBuffer();

		unsigned char *pBuffer = m_pRecvBuffer->AllocBuffer(nSize);
		
		if (pBuffer == NULL)
		{
			CreateBuffer();
			pBuffer  =m_pRecvBuffer->AllocBuffer(nSize);
		}
		
		return pBuffer;
	}
	
	void IOBufferMgr::WriteBuffer(size_t nLength)
	{
		m_pRecvBuffer->WriteBuffer(nLength);
		mt::InterLockIncre((void*)&m_nBufferSize, nLength);
	}

	bool IOBufferMgr::ReadBuffer(unsigned char *data, size_t nReadSize, bool bClear/* = true*/)
	{
		if (nReadSize <= 0 || nReadSize > m_nBufferSize)
			return false;

		IOBuffer *pBuffer = NULL;
		mt::AutoLock l(&m_Mutex);
		VecIOBuffer::iterator iter = m_VecIOBuffer.begin();
		size_t nRecvSize = 0;
		size_t nStartIndex = 0;

		for (; iter != m_VecIOBuffer.end();)
		{
			pBuffer = *iter;
			nRecvSize = pBuffer->ReadBuffer(&data[nStartIndex], nReadSize, bClear);

			if (bClear)
				mt::InterLockDecre((void*)&m_nBufferSize, nRecvSize);

			if (nReadSize == nRecvSize)
				return true;

			nStartIndex += nRecvSize;
			nReadSize -= nRecvSize;

			if (bClear)
			{
				delete pBuffer;
				m_VecIOBuffer.erase(iter);
				iter = m_VecIOBuffer.begin();
			}
			else
			{
				iter++;
			}
		}

		return true;
	}

	void IOBufferMgr::Clear()
	{
		IOBuffer *pBuffer = NULL;

		for (VecIOBuffer::iterator iter = m_VecIOBuffer.begin(); iter != m_VecIOBuffer.end(); iter++)
		{
			pBuffer = *iter;
			delete pBuffer;
		}

		m_VecIOBuffer.clear();
		m_pRecvBuffer = NULL;
		m_nBufferSize = 0;
	}

	size_t IOBufferMgr::GetLength()
	{
		return m_nBufferSize;
	}
}