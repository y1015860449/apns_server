/**
 *  ssl_post_mgr.h
 *  
 *  
 */

#ifndef SSL_POST_MGR_H
#define SSL_POST_MGR_H

#include "ssl_socket.h"
#include "ssl_http2.h"
#include "protohandle.h"
#include "ssl_post.h"
#include "basetype.h"

static const int HearBeatThreadCheckNum = 100;
typedef struct _HeartRange
{
	int begin;
	int end;
	//int total;
	//int maxConns;
	void *usrData;
}HeartRange;

class CPost;
/*
 * Post管理类，数量监管， 心跳检测 
 * 上层回路管理 
 * threads info: 
 * 1. main thread listenning to recv logic msg , //init to connect huawei push Server 
 * 2. send thread to send pushmsg to huawei push Server 
 * 3. event thread loop to check huawei recv data and the err or close event,do recv and close 
 * 4. heartbeat threads to check the connections(200), and (close)reconnct to huawei push Server 
 * @author root (7/21/17)
 */
class CPostMgr
{
public:
	CPostMgr(CSslEventDispatch *pSslEventDispatch = nullptr)
	{
		m_uPostIndex = 0;
		m_uPostMaxConns = 0;
		m_uHeartBeatTimeCheck = time(NULL);

		m_uPerThreadConns = 10;

		m_pSslEventDispatch = pSslEventDispatch;
	}

	virtual ~CPostMgr()
	{

	}

	bool Init(int uMaxConnections, ResponeCallBack_t fun, void* data, string host, uint16_t port, const char *sslCeFileName = nullptr, const char *sslRsaPkFileName = nullptr);

	bool Start();

	int32_t Post(shared_ptr<CApnsPostData> pData);
private:

	virtual CPost* createPost();

private:
	void CheckHeartBeat(HeartRange *range);

	int GetPerThreadConns(){ return m_uPerThreadConns; }

	static void* HeartBeatCheckThread(void* data);

	int GetPostSize();

	CPost* GetPostIndex(int index);

	void InsertPost(CPost * data);

	static void* Increase(void* data);

	int AddThreadEventConns(int MaxNum);

private:

	string 		m_sHost;
	uint16_t 	m_uPort;
	ResponeCallBack_t 	m_callBackFun;
	void* 		m_callback_data;

	std::mutex			m_vecMutex;
	vector<CPost *>	m_postVec;

	int 	m_uPostIndex;		//最后一次发送推送的Post的索引
	int 	m_uPerThreadConns;	//每一个线程连接数
	int 	m_uPostMaxConns;

	uint32_t m_uHeartBeatTimeCheck;// = time(NULL);

	CSslEventDispatch *m_pSslEventDispatch;

	string m_sslCeFileName;				//证书文件名称
	string m_sslRsaPkFileName;			//私钥文件名称
};


#endif

