#ifndef __SSL_POST_H_INCL__
#define __SSL_POST_H_INCL__

#include "ssl_post_mgr.h"
#include "buffer.h"

/**
 * 异步，发送post请求
 * 
 * @author lang (7/21/17)
 */
class CPost
{
public:
	
	CPost()
	{
		m_pSocket = nullptr;
		//m_uPostIndex = 0;
		//m_respHeartBeatTime = time(NULL);
		SetRespHeartBeatTime(time(NULL));
		SetUsrable(false);
	}
	virtual ~CPost()
	{
		if (!m_pSocket)
		{
			delete m_pSocket;
			m_pSocket = nullptr;
		}
	}

	bool InitConnection(string host, uint16_t port, ResponeCallBack_t fun, void* callback_data, CSslEventDispatch *pevent, const char *sslCeFileName = nullptr, const char *sslRsaPkFileName = nullptr);
	
	virtual int32_t Request(shared_ptr<CApnsPostData> pData);

	//int Ping();

	//static std::mutex m_callbackmutex;
	static void ResponeCallBack(void* callback_data, uint8_t msg, uint32_t handle, void* pParam);

	bool TimeOut();

	bool GetUsrable()
	{
		std::lock_guard<std::mutex> lock(m_lockUsr);
		return m_usrable;
	}

	void SetUsrable(bool bFlag)
	{
		std::lock_guard<std::mutex> lock(m_lockUsr);
		m_usrable = bFlag;
	}

	CSocketBase *GetSslSock() 
	{
		return m_pSocket;
	}
	
	void AppCallBackDond(void *data);

	void SetRespHeartBeatTime(time_t param);
	time_t GetRespHeartBeatTime();
protected:
	int Send(void* data, int len);
private:
	virtual CSocketBase* createSocket();
	virtual void OnRead();
	virtual void OnWrite();
	virtual void OnClose() {}
	virtual void OnWriteCompelete() {};
private:

	//int m_uPostIndex;
	CSocketBase	*m_pSocket;		//为了保证socket可用率，可定时断开socket后重连（暂时还未实现）

	std::mutex 		m_resMutex;
	time_t		m_respHeartBeatTime;

	std::mutex 		m_lockUsr;
	bool		m_usrable;

	CSimpleBuffer	m_in_buf;
	uint32_t		m_recv_bytes = 0;
	uint64_t		m_last_recv_tick;

	CSimpleBuffer	m_out_buf;
	bool			m_busy = false;
	uint64_t		m_last_send_tick;

	ResponeCallBack_t m_appCallBack;
	void *m_appCallBackData;
};




#endif // __SSL_POST_H_INCL__
