#ifndef __INTERLOCK_QUEUE_H__
#define __INTERLOCK_QUEUE_H__

namespace netlib
{
	class InterLockQueue
	{
		typedef struct
		{
			void *pObj;
			bool IsEmpty;
		}Queue;

	public:
		InterLockQueue(int nSize);
		~InterLockQueue();

	public:
		bool Push(void *pObj);
		void *Pop();

		void Clear();

	private:
		Queue	*m_queue;
		unsigned int m_push;
		unsigned int m_pop;
		unsigned int m_nSize;
		int	m_nWriteCount;
		int	m_nReadCount;
	};
}

#endif