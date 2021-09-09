/**
 * apnsclient.cpp 
 *  
 * the apns client ,connect to Apple`s APNs Server with http2 
 * send apns request to APNs and recv respone
 *  
 * ad by liulang 
 *  
 * 2017-07-01 
 */

#include "apnsclient.h"
#include "ssl_post_mgr.h"
#include "app_pushserver_manager.hpp"
#include "postDataSender.h"
#include "protohandle.h"
#include "apns_push_post_mgr.h"

#include "ConfigFileReader.h"

void ResponeCallBack(shared_ptr<CApnsPostData> data, void *userData)
{
	if (!userData)
	{
		log("ResponeCallBack userData nullptr");
		return;
	}

	CApnsClient *pClient = (CApnsClient *)userData;

	if (!pClient->CompeleteTask(data))
	{
		log("CompeleteTask false");
		return;
	}
	log("push msgId:%s, toId:%s recv respone %s", data->sMsgId.c_str(), data->sToId.c_str(), data->apnsRetStatus.c_str());

//	CAppPushserverManager::GetInstance()->GetServer()->OnApnsNotify(data);
}


bool CheckCallBack(shared_ptr<CApnsPostData> data, void *userData)
{
	if (!userData)
	{
		log("CheckCallBack userData nullptr");
		return false;
	}
	if (data->sendTimes >= 3)
	{
		log("post msgId %s, toId %s already sent 3 times with no respond,abort it", data->sMsgId.c_str(), data->sToId.c_str());
		return false;
	}

	CApnsClient *pClient = (CApnsClient *)userData;

	if (!pClient->AddTask(data))
	{
		log("CConnectionsThreads AddTask");

		return false;
	}

	return true;
}


CApnsClient::CApnsClient(PUSH_CLIENT_TYPE type)
	:m_bStart(false)
{
	m_clientType = type;
	if (m_clientType == PUSH_CLIENT_TYPE_APNS) {
		m_Host = "api.push.apple.com";
	} else {
		m_Host = "api.development.push.apple.com";
	}
	m_Port = 443;
	m_pPostMgr = new CApnsPostMgr(m_pSslEventDispatch.get());

	m_PostPduCacheMgr.Init(CheckCallBack, this);
}



CApnsClient::~CApnsClient()
{

}


bool CApnsClient::Start()
{	
	if (m_bStart) return m_bStart;
	if (!CClientBase::Start())
	{
		return false;
	}

	//创建线程监测已发送的推送是否超时无响应
	m_PostPduCacheMgr.CheckMap(10);

	m_bStart = true;
	return true;
}

bool CApnsClient::init(int Nums)
{
	int connNums = Nums;
	if (connNums <= 0 || connNums > 100)
	{
		connNums = 20;
	}

	const int connPerThread = 10;
	int nSendThreadNum = (connNums + connPerThread - 1) / connPerThread;
	nSendThreadNum = nSendThreadNum > 20 ? 20 : nSendThreadNum;
	log("sender count %d", nSendThreadNum);
	m_postSenders.clear();
	for (; nSendThreadNum > 0; --nSendThreadNum)
	{
		CPostDataSender * pSender = new CPostDataSender(m_pPostMgr, &m_PostPduCacheMgr);
		m_postSenders.push_back(pSender);
	}

	if (!m_pPostMgr->Init(connNums, ResponeCallBack, this, m_Host, m_Port))
	{
		log("m_pPostMgr init error");
		return false;
	}
	
	CAppPushserverManager *pManage = CAppPushserverManager::GetInstance();
	if (!pManage)
	{
		log("CAppPushserverManager GetInstance");
		return false;
	}
	if (!pManage->RegistClient(this))
	{
		log("CAppPushserverManager RegistClient");
		return false;
	}
	return Start();
}


bool CApnsClient::CompeleteTask(shared_ptr<CApnsPostData> data)
{
	return m_PostPduCacheMgr.Delete(data);
}


shared_ptr<CApnsPostData> CApnsClient::GeneratePostDataFromMsg(const ApnsData& msg)const
{
    return PostDataGenerator::GenerateAPNsPostData(msg);
}
