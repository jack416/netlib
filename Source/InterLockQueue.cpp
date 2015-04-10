#include "InterLockQueue.h"
#include "../../ThreadLib/Include/InterLock.h"

#include <stdio.h>


namespace netlib
{
	InterLockQueue::InterLockQueue(int nSize)
		:m_nSize(nSize)
	{
		m_nWriteCount = nSize;/**< 初始化时,可写入个数与最大个数相同*/
		m_nReadCount = 0;
		m_queue = new Queue[m_nSize];
		m_push = 0;
		m_pop = 0;

		Clear();
	}

	InterLockQueue::~InterLockQueue()
	{
		if (m_queue != NULL)
		{
			delete [] m_queue;
		}
	}

	bool InterLockQueue::Push(void *pObj)
	{
		if (pObj == NULL)
			return false;

		if (m_nWriteCount <= 0)/**< 列表已满*/
			return false;

		if (mt::InterLockDecre((void*)&m_nWriteCount, 1) <= 0)
		{
			mt::InterLockIncre((void*)&m_nWriteCount, 1);
			return false;
		}

		unsigned int nPos = mt::InterLockIncre((void*)m_push, 1);
		nPos = nPos % m_nSize;

		if (!m_queue[nPos].IsEmpty)
		{
			m_push--;
			mt::InterLockIncre((void*)&m_nWriteCount, 1);
			return false;
		}

		m_queue[nPos].pObj = pObj;
		m_queue[nPos].IsEmpty = false;
		mt::InterLockIncre((void*)&m_nReadCount, 1);/**< 可读数据加1*/

		return true;
	}

	void *InterLockQueue::Pop()
	{
		if (m_nReadCount <= 0)
			return NULL;

		if (mt::InterLockDecre((void*)&m_nReadCount, 1) <= 0)
		{
			mt::InterLockIncre((void*)&m_nReadCount, 1);
			return NULL;
		}

		unsigned int nPos = mt::InterLockIncre((void*)&m_pop, 1);
		nPos = nPos % m_nSize;

		if (m_queue[nPos].IsEmpty)
		{
			m_pop--;
			mt::InterLockIncre((void*)&m_nReadCount, 1);
			return NULL;
		}

		void *pObj = m_queue[nPos].pObj;
		m_queue[nPos].pObj = NULL;
		m_queue[nPos].IsEmpty = true;

		mt::InterLockIncre((void*)&m_nWriteCount, 1);/**< 可写数加1*/

		return pObj;
	}

	void InterLockQueue::Clear()
	{
		if (m_queue == NULL)
			return;

		unsigned int i = 0;

		m_nWriteCount = m_nSize;
		m_nReadCount = 0;

		for (i = 0; i < m_nSize; i++)
		{
			m_queue[i].IsEmpty = true;
			m_queue[i].pObj = NULL;
		}
	}
}