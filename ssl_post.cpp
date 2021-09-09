#include "ssl_post.h"
#include "CApnsPostData.h"

#define READ_BUF_SIZE	2048
#define NETLIB_MAX_SOCKET_BUF_SIZE		(128 * 1024)

CSocketBase* CPost::createSocket()
{
	return new CSslSocket();
}


bool CPost::InitConnection(string host, uint16_t port, ResponeCallBack_t fun, void* callback_data, CSslEventDispatch *pevent, const char *sslCeFileName , const char *sslRsaPkFileName )
{

	if (!fun || !callback_data)
	{
		log("fun || !callback_data");
		return false;
	}

	m_appCallBack = fun;
	m_appCallBackData = callback_data;
	
	m_pSocket = createSocket();
	if (!m_pSocket)
	{
		log("new CHttp2Socket");
		return false;
	}
	
	if(!m_pSocket->Init(pevent,sslCeFileName,sslRsaPkFileName))
	{
		log("sslsocket->Init error");
		return false;
	}

	//callback_t callBackFun = (callback_t)&CPost::ResponeCallBack;

	//printf("call back func address:%p, data:%p\n", callBackFun, this);
	if (m_pSocket->SslConnectWebSite(ResponeCallBack, this, host.c_str(), port) <= 0)
	{
		log("SslConnectWebSite");
		m_pSocket->Close();
		return false;
	}

	SetUsrable(true);


	//m_respHeartBeatTime = time(NULL);
	SetRespHeartBeatTime(time(NULL));

	return true;
}

int32_t CPost::Request(shared_ptr<CApnsPostData> pData)
{
	return Send(pData->body, pData->bodyLen);
}


bool CPost::TimeOut()
{
	if (!GetUsrable())
	{
		return true;
	}

    if (time(NULL) - GetRespHeartBeatTime() > 20) {
        if (m_pSocket) {
            m_pSocket->Submit_ping(); 
        } 

    }
	//test
	//log("time(NULL) - GetRespHeartBeatTime:%d", time(NULL)-GetRespHeartBeatTime());

	//return false first, 这里close/respone/request 存在竞争关系, 暂且这么处理, 不完美.
	if (time(NULL) - GetRespHeartBeatTime() > 60*5)
	{
		SetUsrable(false);
		return false;
	}
	
	return false;
}


void CPost::AppCallBackDond(void *data)
{

	shared_ptr<CApnsPostData> *pdata = (shared_ptr<CApnsPostData> *)data;
	m_appCallBack(*pdata, m_appCallBackData);
}


void CPost::SetRespHeartBeatTime(time_t param)
{
	std::lock_guard<std::mutex> lock(m_resMutex);

	m_respHeartBeatTime = param;

}

time_t CPost::GetRespHeartBeatTime()
{
	std::lock_guard<std::mutex> lock(m_resMutex);

	return m_respHeartBeatTime;
}

//static int responeDebug = 0;

//std::mutex CPost::m_callbackmutex;
void CPost::ResponeCallBack(void* callback_data, uint8_t msg, uint32_t handle, void* pParam)
{
	//std::lock_guard<std::mutex> lock(&CPost::m_callbackmutex);
	//log("%d, msg type:%d\n", handle, msg);
	NOTUSED_ARG(handle);

	CPost *post = (CPost *) callback_data;
	//post->SetHeartbeat();
    if(HTTP_PING_RESPONE == msg)
    {
		post->SetRespHeartBeatTime(time(NULL));
    }
	else if (NETLIB_MSG_CLOSE == msg)
	{
		log("close by peer");
		post->SetUsrable(false);
	}
	else if (HTTP_MSG_RESPONE == msg)
	{
		//respone to callback app
		post->AppCallBackDond(pParam);
		post->SetRespHeartBeatTime(time(NULL));
	}
	else if (NETLIB_MSG_READ == msg)
	{
		post->OnRead();
	}
	else if (NETLIB_MSG_WRITE == msg)
	{
		post->OnWrite();
	}
	else
	{
		log("unknow msg:%d\n", msg);
	}
}

void CPost::OnRead()
{
	for (;;)
	{
		uint32_t free_buf_len = m_in_buf.GetAllocSize() - m_in_buf.GetWriteOffset();
		if (free_buf_len < READ_BUF_SIZE)
			m_in_buf.Extend(READ_BUF_SIZE);

		int ret = m_pSocket->Recv(m_in_buf.GetBuffer() + m_in_buf.GetWriteOffset(), READ_BUF_SIZE);

		if (ret <= 0)
			break;

		m_recv_bytes += ret;
		m_in_buf.IncWriteOffset(ret);

		SetRespHeartBeatTime(time(NULL));
	}

	//默认情况下不对收到的数据做任何处理
	while (m_in_buf.GetWriteOffset())
	{
		m_in_buf.Read(NULL, m_in_buf.GetWriteOffset());
	}

}

void CPost::OnWrite()
{
	if (!m_busy)
		return;

	while (m_out_buf.GetWriteOffset() > 0) 
	{
		int send_size = m_out_buf.GetWriteOffset();
		if (send_size > NETLIB_MAX_SOCKET_BUF_SIZE) 
		{
			send_size = NETLIB_MAX_SOCKET_BUF_SIZE;
		}

		int ret = m_pSocket->Send(m_out_buf.GetBuffer(), send_size);
		if (ret <= 0) 
		{
			ret = 0;
			break;
		}

		m_out_buf.Read(NULL, ret);
	}

	if (m_out_buf.GetWriteOffset() == 0) 
	{
		m_busy = false;
	}

	log("onWrite, remain=%d ", m_out_buf.GetWriteOffset());
}

int CPost::Send(void* data, int len)
{
	m_last_send_tick = get_tick_count();
	//	++g_send_pkt_cnt;

	if (m_busy)
	{
		m_out_buf.Write(data, len);
		return len;
	}

	int offset = 0;
	int remain = len;
	while (remain > 0) 
	{
		int send_size = remain;
		if (send_size > NETLIB_MAX_SOCKET_BUF_SIZE) 
		{
			send_size = NETLIB_MAX_SOCKET_BUF_SIZE;
		}

		int ret = m_pSocket->Send(data, len);
		if (ret <= 0) 
		{
			ret = 0;
			break;
		}

		offset += ret;
		remain -= ret;
	}

	if (remain > 0)
	{
		m_out_buf.Write((char*)data + offset, remain);
		m_busy = true;
		log("send busy, remain=%d ", m_out_buf.GetWriteOffset());
	}
	else
	{
		OnWriteCompelete();
	}

	return len;
}


// TODO: your implementation here

