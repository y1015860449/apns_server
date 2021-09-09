#ifndef __CAPNSPOSTDATA_H_INCL__
#define __CAPNSPOSTDATA_H_INCL__

#include <queue>
#include "protohandle.h"

/**
 * TODO: 需要执行的单个任务的数据
 * 
 * @author   root
 */
class nghttparr;
class CApnsPostData
{
public:
	CApnsPostData();
	~CApnsPostData();
	bool Timeout(int iTimeRate);

	nghttparr *nva;
	int navArrayLen;
	char *body;
	int bodyLen;

	string sMsgId;
	string sToId;
	string apnsRetStatus;

	time_t sendTime;   //发送的时间
	int mapIndex;	   //发送时的索引
	int sendTimes = 0; //发送的次数
};

//需要执行的任务队列
class CPackStreamQueue
{
public:
	CPackStreamQueue()
	{
		//default size
		m_uMaxQueueSize = 0x7ffffffe;
	}

	CPackStreamQueue(int MaxQueueSize) : m_uMaxQueueSize(MaxQueueSize)
	{
	}

	~CPackStreamQueue()
	{
	}

	shared_ptr<CApnsPostData> PopFront()
	{
		std::lock_guard<std::mutex> autoLock(m_MutexLock);

		if (m_packQueue.empty())
		{
			//printf("queue is empty!\n");
			return nullptr;
		}

		shared_ptr<CApnsPostData> pack = m_packQueue.front();

		m_packQueue.pop();

		return pack;
	}

	int PushBack(shared_ptr<CApnsPostData> pPack, bool bForce = false)
	{
		std::lock_guard<std::mutex> autoLock(m_MutexLock);

		if (!pPack)
		{
			printf("pPack is null!\n");
			return -1;
		}
		if (m_packQueue.size() >= m_uMaxQueueSize && !bForce)
		{
			printf("queue size is >= %d!\n", m_uMaxQueueSize);
			return -1;
		}

		m_packQueue.push(pPack);

		return m_packQueue.size();
		;
	}

	uint32_t GetSize()
	{
		std::lock_guard<std::mutex> autoLock(m_MutexLock);
		return m_packQueue.size();
	}

private:
	queue<shared_ptr<CApnsPostData> > m_packQueue;
	unsigned int m_uMaxQueueSize;
	std::mutex m_MutexLock;
};

#endif // __CAPNSPOSTDATA_H_INCL__
