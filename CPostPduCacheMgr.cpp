#include "CPostPduCacheMgr.h"
#include "CApnsPostData.h"
// TODO: your implementation here


CPostPduCacheMgr::CPostPduCacheMgr()
	:m_timeOut(10), m_bRunning(false)
{
	m_index = 0;

	m_pCacheMap = new map<int, shared_ptr<CApnsPostData>>;
}

CPostPduCacheMgr::~CPostPduCacheMgr()
{

}

void CPostPduCacheMgr::Init(CheckCallBack_t fun, void *userData)
{
	m_checkCallBack = fun;
	m_userData = userData;

	m_index = 0;
}

int CPostPduCacheMgr::Insert(shared_ptr<CApnsPostData> pdata)
{

	if (!pdata)
	{
		log("postData is nullptr!");
		return -1;
	}

	CApnsPostData *postData = pdata.get();
	if (!postData)
	{
		log("pdata.get() is nullptr!");
		return -1;
	}

	std::lock_guard<std::mutex> lock(m_mapMutex);
	m_index++;

	postData->mapIndex = m_index;
	postData->sendTime = time(NULL);
	++postData->sendTimes;
	
	//log("post msgId:%s, toId:%s send %d times", postData->sMsgId.c_str(), postData->sToId.c_str(), postData->sendTimes);
	//return m_index;

	if(!(m_pCacheMap->insert(pair<int, shared_ptr<CApnsPostData> >(m_index, pdata))).second )
	{
		map<int, shared_ptr<CApnsPostData>>::iterator it = m_pCacheMap->find(m_index);
		if (it != m_pCacheMap->end())
		{
			log("the index is exist!");
			//big Error!
			/*
			delete pCache;
			pCache = nullptr;
			*/
			return -1;
		}
	}

	if (m_index > 0x7ffffffd)
	{
		m_index = 0;
	}

	return m_index;
}


bool CPostPduCacheMgr::Delete(shared_ptr<CApnsPostData> pdata)
{
	if (!pdata)
	{
		log("pdata is nullptr");
		return false;
	}
	
	CApnsPostData *data;
	data = pdata.get();
	if (!data)
	{
		log("data is nullptr");
		return false;
	}

	return Delete(data->mapIndex);
}


bool CPostPduCacheMgr::Delete(int index)
{

	//return true;
	std::lock_guard<std::mutex> lock(m_mapMutex);

	if (m_pCacheMap->empty())
	{
		log("m_cacheMap empty");
		return  false;
	}

	map<int, shared_ptr<CApnsPostData> >::iterator it = m_pCacheMap->find(index);

	if (it != m_pCacheMap->end())
	{
		shared_ptr<CApnsPostData> pdata = it->second;
		
		if (pdata)
		{
			CApnsPostData *pCache = pdata.get();

			pdata.reset();
			m_pCacheMap->erase(it);
			return true;

			if (pCache->mapIndex!=index)
			{
				log("pCache->mapIndex:%d!=index:%d", pCache->mapIndex, index);

				//m_pCacheMap->erase(it);
				//return false;
			}
			
			log("delete apnsid:%s mapIndex:%d", pCache->sMsgId.c_str(), pCache->mapIndex);
			m_pCacheMap->erase(it);

			return true;			
		}
		else
		{
			log("pCache");
		}
	}

	log("m_cacheMap not found index %d", index);
	return false;
}

int CPostPduCacheMgr::CheckCache(time_t timeRate)
{
	std::lock_guard<std::mutex> lock(m_mapMutex);

	if (!m_checkCallBack || !m_userData)
	{
		log("m_checkCallBack, m_userData null");
		return -1;
	}

	if (m_pCacheMap->size() > 0)
		log("pduCache map size:%d\n", m_pCacheMap->size());
	
	map<int, shared_ptr<CApnsPostData>>::iterator it = m_pCacheMap->begin();

	CApnsPostData *pCache;
	for (; it != m_pCacheMap->end();)
	{
		pCache = it->second.get();

		if (pCache->Timeout(timeRate))
		{
			m_checkCallBack(it->second, m_userData);
			m_pCacheMap->erase(it++);
		}
		else
		{
			++it;
		}
	}//for

	return 0;
}

void CPostPduCacheMgr::StopThread()
{
	if (m_bRunning)
	{
		m_bRunning = false;
	}
}

void CPostPduCacheMgr::StartThread()
{
	if (m_bRunning)
	{
		return;
	}
	OnThreadRun();
}

void CPostPduCacheMgr::OnThreadRun(void)
{
	m_bRunning = true;
	log("CPostPduCacheMgr %p checkMap thread start, timeOut = %d", this, m_timeOut);
	std::thread tmpThread([this]()
	{
		while (m_timeOut && m_bRunning)
		{
			this->CheckCache(m_timeOut);
			sleep(15);
		}
	});
	tmpThread.detach();
	m_bRunning = false;
	log("CPostPduCacheMgr %p checkMap thread stop", this);
}

int CPostPduCacheMgr::CheckMap(time_t timeRate)
{
	m_timeOut = timeRate > 100000 ? 100000 : timeRate;
	if (!m_bRunning)		//����߳�û�������������߳�
		StartThread();
	else
		log("CPostPduCacheMgr %p checkMap thread already running,timeout set to %d", this, m_timeOut);
	return true;
}

