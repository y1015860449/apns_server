#include "clientbase.h"
#include "ssl_event.h"
#include "postDataSender.h"
#include "CApnsPostData.h"

CClientBase::CClientBase()
	:m_clientType(PUSH_CLIENT_TYPE_UNKNOWN), m_pPostMgr(nullptr)
{
	m_pSslEventDispatch = GetSslEventDispatch();	//Ĭ��ֵ
}

CClientBase::~CClientBase()
{
	if (m_pPostMgr)
	{
		delete m_pPostMgr;
	}
}

std::shared_ptr<CSslEventDispatch> CClientBase::GetSslEventDispatch()
{
	static std::shared_ptr<CSslEventDispatch> pSslEventDispatch = std::shared_ptr<CSslEventDispatch>(new CSslEventDispatch);
	return pSslEventDispatch;
}

bool CClientBase::AddTask(const ApnsData& msg)
{
	bool bRet = false;
	if (m_postSenders.empty())
	{
		log("sender is empty");
		return bRet;	
	}

	shared_ptr<CApnsPostData> pPostData = GeneratePostDataFromMsg(msg);
	if (!pPostData)
	{
		log("###GenerateAPNsPostData false");
		return false;
	}

	return AddTask(pPostData);
}

bool CClientBase::AddTask(shared_ptr<CApnsPostData> data)
{
	bool bRet = false;
	if (m_postSenders.empty()) 
	{
		log("sender is empty");
		return bRet;	
	}

	int index = atoi(data->sToId.c_str()) % m_postSenders.size();
	log("add task index is %d", index);
	return m_postSenders[index]->PostData(data);

}

bool CClientBase::startEpoll()
{
	if (!m_pSslEventDispatch)
	{
		log("m_pSslEventDispatch is nullptr");

		return false;
	}
	m_pSslEventDispatch->StartDispatch();

	return true;
}

bool CClientBase::Start()
{
	if (!m_pPostMgr)
	{
		log("m_pPostMgr is nullptr");
		return false;
	}

	if (!m_pSslEventDispatch)
	{
		log("m_pSslEventDispatch is nullptr");
		return false;
	}

	startEpoll();


	for (unsigned int i = 0; i < m_postSenders.size(); ++i)
	{
		m_postSenders[i]->StartThread();
	}

	m_pPostMgr->Start();
	return true;
}
