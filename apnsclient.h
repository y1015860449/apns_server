/**
  *  
  * file apnsclient.h 
  * create by liulang 
  *  
  * client to send apns request and recive respone 
  *  
  * 2017-07-01 
  */
#ifndef __APNSCLIENT_H__	
#define __APNSCLIENT_H__	

#include "ssl_socket.h"

#include "postDataSender.h"
#include "clientbase.h"

class CPostDataSender;

//负责跟APNs的连接，并且发送推送请求

class CApnsClient:public CClientBase
{
public:

	CApnsClient(PUSH_CLIENT_TYPE type);
	
	~CApnsClient();

	bool init(int Nums);

	//添加推送任务（放到任务队列中）
	//bool AddTask(shared_ptr<CApnsPostData> data);


	//用在收到响应的回调
	bool CompeleteTask(shared_ptr<CApnsPostData> data);

private:
	bool Start();

	virtual shared_ptr<CApnsPostData> GeneratePostDataFromMsg(const ApnsData& msg)const override;

private:

	bool	m_bStart;
	CPostPduCacheMgr m_PostPduCacheMgr;			//管理已经发送的推送
};  



#endif

