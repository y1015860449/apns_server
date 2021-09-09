#ifndef __CLIENTBASE_H__
#define __CLIENTBASE_H__

#include <string>
#include <memory>
#include <vector>
#include "utility.hpp"
#include "postDataSender.h"
#include "ssl_event.h"
#include "CApnsPostData.h"

class CClientBase
{
public:

	CClientBase();
	
	virtual ~CClientBase();

	//初始化并启动
	virtual bool init(int nNums) = 0;

	//添加推送任务（放到任务队列中)
	bool AddTask(const ApnsData& msg);

	//用在超时无响应的回调中用到
	bool AddTask(std::shared_ptr<CApnsPostData> data);

	PUSH_CLIENT_TYPE getClientType()const{ return m_clientType; }

	static	std::shared_ptr<CSslEventDispatch> GetSslEventDispatch();
protected:
	bool startEpoll();
	virtual bool Start();
private:
	virtual std::shared_ptr<CApnsPostData> GeneratePostDataFromMsg(const ApnsData& msg)const = 0;
protected:
	PUSH_CLIENT_TYPE		m_clientType;

	std::vector<CPostDataSender*>  m_postSenders;	//发送线程池
	std::shared_ptr<CSslEventDispatch> m_pSslEventDispatch;
	CPostMgr	*m_pPostMgr;					//发送器管理器

	std::string		m_Host;			//要连接的服务器的地址
	unsigned int	m_Port;				//要连接的服务器的端口
};  
#endif // __CLIENTBASE_H__





