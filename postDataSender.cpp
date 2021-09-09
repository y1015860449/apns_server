#include "postDataSender.h"
#include "app_pushserver_manager.hpp"

CPostDataSender::CPostDataSender(CPostMgr *pPostMgr, CPostPduCacheMgr *pPostPduCacheMgr)
	:m_pPostMgr(pPostMgr), m_pPostPduCacheMgr(pPostPduCacheMgr)
{
}

CPostDataSender::~CPostDataSender()
{
	StopThread();
}

void CPostDataSender::StartThread()
{
	if (m_bRunning)
	{
		return;
	}
	OnThreadRun();
}

void CPostDataSender::StopThread()
{
	if (m_bRunning)
	{
		m_bRunning = false;
	}
	m_condition.notify_all();
}

void CPostDataSender::OnThreadRun(void)
{
	m_bRunning = true;
	std::thread tmpThread([this]()
	{
		while (m_bRunning)
		{
			std::unique_lock<std::mutex> lock(this->m_mtx);
			while (this->m_PackStreamQueue.GetSize() == 0 && m_bRunning)
			{
				this->m_condition.wait(lock);
			}
			log("CPostDataSender thread ready for SendData");
			if (this->m_PackStreamQueue.GetSize() > 0)
			{
				shared_ptr<CApnsPostData> pData = this->m_PackStreamQueue.PopFront();

				if (m_pPostPduCacheMgr && this->m_pPostPduCacheMgr->Insert(pData) < 0)
				{
					log("m_pPostPduCacheMgr Insert");
				}
				this->m_pPostMgr->Post(pData); //
			}
		}
	});

	tmpThread.join();
}
bool CPostDataSender::PostData(shared_ptr<CApnsPostData> pPostData)
{
	bool bRet = false;

	std::unique_lock<std::mutex> lock(m_mtx);
	if (m_bRunning)
	{
		if (m_PackStreamQueue.PushBack(pPostData) > 0)
		{
			bRet = true;
		}
		m_condition.notify_all();
	}
	else
	{
		log("###CPostDataSender not running");
	}

	return bRet;
}


