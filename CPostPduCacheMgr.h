#ifndef __CPOSTPDUCACHEMGR_H_INCL__
#define __CPOSTPDUCACHEMGR_H_INCL__

#include "ssl_post_mgr.h"
#include "CApnsPostData.h"
/**
 * TODO: Add class description
 * 
 * @author   root
 */

typedef  bool(*CheckCallBack_t)(shared_ptr<CApnsPostData>, void *userData);


//推送任务缓存器， 缓存已经发送的推送消息

class CPostPduCacheMgr
{
public:
	// Constructor
	CPostPduCacheMgr();

	void Init(CheckCallBack_t fun, void *userData);

	// Destructor
	virtual ~CPostPduCacheMgr();


	int Insert(shared_ptr<CApnsPostData>);

	bool Delete(shared_ptr<CApnsPostData> pdate);

	int CheckMap(time_t timeRate);

	void StopThread(); 
	void StartThread();
	void OnThreadRun(void) ;

private:
	bool Delete(int index);
	int CheckCache(time_t timeRate);
private:
	int				m_timeOut;	//超时时间的长度
	bool			m_bRunning;	

	CheckCallBack_t m_checkCallBack;
	void *m_userData;

	int m_index;
	std::mutex	m_mapMutex;
	map<int, shared_ptr<CApnsPostData>> *m_pCacheMap;
};

#endif // __CPOSTPDUCACHEMGR_H_INCL__
