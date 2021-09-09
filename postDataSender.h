#ifndef __POSTDATASENDER_H__
#define __POSTDATASENDER_H__

//class CApnsPostData;
#include "CApnsPostData.h"
#include "CPostPduCacheMgr.h"
#include "basetype.h"

/*
 * TODO: Add class description
 * 
 * @author   root
 */

class CPostDataSender
{
public:
	CPostDataSender(CPostMgr *pPostMgr, CPostPduCacheMgr *pPostPduCacheMgr);
	~CPostDataSender();

	void StopThread(); //Stop packet processing thread.
	void StartThread();
	void OnThreadRun(void);						   // Thread of Process queue message
	bool PostData(shared_ptr<CApnsPostData> data); //Post message to queue ,waitting for process.
private:
	std::mutex m_mtx;
	std::condition_variable m_condition; 
	bool m_bRunning;
	CPackStreamQueue m_PackStreamQueue;
	CPostMgr *m_pPostMgr;
	CPostPduCacheMgr *m_pPostPduCacheMgr;
};

#endif // __POSTDATASENDER_H__
