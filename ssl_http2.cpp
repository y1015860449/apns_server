/** 
 *  file ssl_http2.cpp
 *  nghttp2 to connect apns whit http2 date: 2017072
 * create by liulang 
 */


#include "ssl_http2.h"
#include "CApnsPostData.h"

#define MAKE_NV(NAME, VALUE)\
  {\
    (uint8_t *)NAME, (uint8_t *)(VALUE).c_str(), strlen(NAME), (VALUE).size(),\
        NGHTTP2_NV_FLAG_NO_INDEX \
  }
  
/* 																				                                                               			 																				                                                               			 
#define MAKE_NV(NAME, VALUE)\
  {\
    (uint8_t *)NAME, (uint8_t *)(VALUE).c_str(), strlen(NAME), (VALUE).size(),\
        NGHTTP2_NV_FLAG_NONE\
  }	
*/

CStreamMap::~CStreamMap()
{
	Clear();
}

StreamData* CStreamMap::GetAndDel(int streamId)
{
	std::lock_guard<std::mutex> lock(m_mapMutex);
	if (m_streamMap.empty())
	{
		log("m_streamMap empty");
		return false;
	}

	map<int, StreamData* >::iterator it = m_streamMap.find(streamId);

	if (it != m_streamMap.end())
	{
		StreamData *data = it->second;
		if (data)
		{
			if (data->streamId!=streamId)
			{
				log("data->streamId:%d != streamId:%d", data->streamId, streamId);
				return nullptr;
			}
			m_streamMap.erase(it);

			return data;			
		}
		else
		{
			log("data is nullptr");
		}
	}

	log("m_streamMap not found streamId %d", streamId);
	return nullptr;
}


StreamData 	* CStreamMap::GetStream(int streamId)
{
	std::lock_guard<std::mutex> lock(m_mapMutex);
	if (m_streamMap.empty())
	{
		log("m_streamMap empty");
		return false;
	}

	map<int, StreamData* >::iterator it = m_streamMap.find(streamId);

	if (it != m_streamMap.end())
	{
		StreamData *data = it->second;
		if (data)
		{
			if (data->streamId != streamId)
			{
				log("data->streamId:%d != streamId:%d", data->streamId, streamId);
				return nullptr;
			}
			return data;
		}
		else
		{
			log("data is nullptr");
		}
	}

	log("m_streamMap not found streamId %d", streamId);
	return nullptr;
}


int	CStreamMap::Insert(StreamData *data)
{
	std::lock_guard<std::mutex> lock(m_mapMutex);
	if (!data)
	{
		log("data is nullptr");
		return -1;
	}

	if (data->streamId < 0)
	{
		log("streamId is <0");
		return -1;
	}

	if (!m_streamMap.insert(make_pair(data->streamId, data)).second)
	{
		log("m_streamMap insert :%d", data->streamId);
		return -1;
	}
	
	return data->streamId;
}

void CStreamMap::Clear()
{
	std::lock_guard<std::mutex> lock(m_mapMutex);
	map<int, StreamData *>::iterator it = m_streamMap.begin();
	for (; it != m_streamMap.end(); )
	{
		delete it->second;
		m_streamMap.erase(it++);
	}
}

void CHttp2Socket::httpCallback(void* callback_data, uint8_t msg, uint32_t handle, void* pParam)
{
	NOTUSED_ARG(callback_data);
	NOTUSED_ARG(msg);
	NOTUSED_ARG(handle);
	NOTUSED_ARG(pParam);
}

//regist the callback function just the apns needed
void CHttp2Socket::SetNghttp2Callbacks()
{
	nghttp2_session_callbacks_set_send_callback(m_callbacks, 
					(nghttp2_send_callback)_On_send_callback);

	nghttp2_session_callbacks_set_recv_callback(m_callbacks, 
					(nghttp2_recv_callback)_On_recv_callback);
	/*
	//the program did not need to know the err reasion of the status(not 200), so not regist this callBack
	//if you want to know the status of(200-success, 429 )err reasion, you can visit this website as below:
	https://developer.apple.com/library/content/documentation/NetworkingInternet/Conceptual/RemoteNotificationsPG/
	CommunicatingwithAPNs.html#//apple_ref/doc/uid/TP40008194-CH11-SW1
	*/
	nghttp2_session_callbacks_set_on_data_chunk_recv_callback(m_callbacks, 
					(nghttp2_on_data_chunk_recv_callback)_On_data_chunk_recv_callback);

	nghttp2_session_callbacks_set_on_header_callback(m_callbacks, 
					(nghttp2_on_header_callback)_On_header_callback);
}


int CHttp2Socket::ReConnect()
{
	if (m_session)
	{
		nghttp2_session_del(m_session);
		m_session = nullptr;
	}
	
	return SslConnectWebSite(GetCallBack(), GetCallBackData(), m_hostStr.c_str(), m_port);
}

int CHttp2Socket::SslConnectWebSite(callback_t callback, void* callback_data, const char* host, const int16_t port)
{

	if (!callback || !callback_data)
	{
		log("callback or callback_data is null!");
		return -1;
	}

	if (!host || port <= 0)
	{
		log("callback or callback_data is null!");
		return -1;
	}
	
	m_hostStr = host;
	m_port = port;

	if (CSslSocket::SslConnectWebSite(callback, callback_data, host, port) <= 0)
	{
		log("SslConnectWebSite!");
		return -1;
	}

	int iRet = nghttp2_session_callbacks_new(&m_callbacks);

	if (iRet != 0) 
	{
		log("nghttp2_session_callbacks_new", iRet);
		return -1;
	}

	SetNghttp2Callbacks();

	iRet = nghttp2_session_client_new(&m_session, m_callbacks, this);
	if (iRet != 0)
	{
		log("nghttp2_session_client_new", iRet);
		return -1;
	}

	nghttp2_session_callbacks_del(m_callbacks);

	//iRet = nghttp2_submit_settings(m_session, NGHTTP2_FLAG_NONE, NULL, 0);
	iRet = nghttp2_submit_settings(m_session, NGHTTP2_FLAG_ACK, NULL, 0);
	if (iRet != 0)
	{
		log("nghttp2_submit_settings", iRet);
		return -1;
	}

	SetWriteable(true);
	return GetSocket();
}

size_t CHttp2Socket::_On_send_callback(nghttp2_session *session, const uint8_t *data,
									 size_t length, int flags, void *user_data) 
{
	NOTUSED_ARG(session);
	NOTUSED_ARG(flags);
	CHttp2Socket *http = (CHttp2Socket*)user_data;

	
    log("send_callback, socket=%d", http->GetSocket());
	if (http->GetSocket() <= 0)
	{
		log("socket:%d, SSL_write", http->GetSocket());
		return -1;
	}
	
	//printf("send: %s, len:%d\n", (char *)data, length);
	return http->Send((void*)data, length);
}

size_t CHttp2Socket::_On_recv_callback(nghttp2_session *session, uint8_t *buf, size_t length, int flags, void *user_data)
{
	NOTUSED_ARG(session);
	NOTUSED_ARG(flags);
	CHttp2Socket *http = (CHttp2Socket*) user_data;
    
    log("recv_callback, socket=%d", http->GetSocket());

	if (http->GetSocket() <= 0)
	{
		log("_On_recv_callback socket <= 0!");
		return -1;
	}
//ping cause this callback???
    http->GetCallBack()(http->GetCallBackData(), HTTP_PING_RESPONE, http->GetSocket(), NULL);
	return http->Recv(buf, length);
}


static const char *errval = "403";
//static const char *succVal = "200";
int CHttp2Socket::_On_header_callback(nghttp2_session *session, const nghttp2_frame *frame,
                     const uint8_t *name, size_t namelen,
                     const uint8_t *value, size_t valuelen,
                     uint8_t flags,
                     void *userp)
{
	NOTUSED_ARG(session);
	NOTUSED_ARG(namelen);
	NOTUSED_ARG(valuelen);
	NOTUSED_ARG(flags);

	int32_t stream_id = frame->hd.stream_id;
	CHttp2Socket *http = (CHttp2Socket*) userp;

	log("stream %d %s: %s\n", stream_id, name, value);
	if(frame->hd.type == NGHTTP2_HEADERS)
	{
		if (0 == strcmp(":status", (char *)name))
		{			
			//printf("%s: %s\n", name, value);
			StreamData *streamData = http->m_streamDataMap.GetStream(stream_id);
			if (!streamData)
			{
				log("streamData is nullptr");
				return -1;
			}

			if (strcmp(errval, (const char *)value) == 0)
			{
				CHead::m_timestampSecAuth = 0;
				//int i  = ::decode_status_code(value, valuelen);
			}

			//log("pop stream:%d", streamData->streamId);

			if (streamData && streamData->pPostData)
			{
				streamData->pPostData.get()->apnsRetStatus = (const char *)value;
			}
			else
			{
				log("http->m_streamDataMap.PopFront(stream_id);");
				return 0;
			}

			streamData->iPostStatus = POSt_STATUS_RESPONE_ECODE;
		}
		else if (0 == strcmp((char *)name, "apns-id"))
		{
			StreamData *streamData = http->m_streamDataMap.GetAndDel(stream_id);
			if (!streamData)
			{
				log("streamData is nullptr");
				return -1;
			}
			//shared_ptr<CApnsPostData> pData;

			if (streamData->iPostStatus != POSt_STATUS_RESPONE_ECODE)
			{
				log("POSt_STATUS_RESPONE_ECODE");
			}

			if (streamData && streamData->pPostData)
			{
				//pData = streamData->pPostData;

				streamData->iPostStatus = POST_STATUS_RESPONE_APNS_ID;
				if (streamData->pPostData.get()->nva->apns_id != string((char *)value))
				{
					log("pPostData->sMsgId:%s, apns ret Id:%s\n\n\n\n", streamData->pPostData.get()->nva->apns_id.c_str(), value);
				}
				//streamData-> = (char *)value;
			}
			else
			{
				log("status, apns-id is not respone as a couple");
			}
			
			http->GetCallBack()(http->GetCallBackData(), HTTP_MSG_RESPONE, http->GetSocket(), &streamData->pPostData);

			
			delete streamData;
			streamData = nullptr;
			
		}
		//for Debug
	}
	else
	{
		log("on_header type:%d, %s: %s\n", name, value);
	}

	return 0;
}

//need not to use 
int CHttp2Socket::_On_data_chunk_recv_callback(nghttp2_session *session, uint8_t flags,
                                       int32_t stream_id, const uint8_t *data,
                                       size_t len, void *user_data)
{
	NOTUSED_ARG(session);
	NOTUSED_ARG(flags);
	NOTUSED_ARG(stream_id);
//	NOTUSED_ARG(user_data);

	CHttp2Socket *http = (CHttp2Socket*)user_data;
	printf("on_data_chunk_recv_callback\n");

	printf("[INFO] C <---------------------------- S (DATA chunk) %lu bytes\n",(unsigned long int)len);

	log("data_chunk:%s, socket=%d", data, http->GetSocket());
	return 0;
}

int CHttp2Socket::Send(shared_ptr<CApnsPostData> data)
{

	std::lock_guard<std::mutex> lock(m_SendRecvLineRun);

	if (!GetWriteable())
	{
		log("GetWriteable is false;");
		return -1;
	}
	
	if (!data)
	{
		log("CApnsPostData nullptr");
		return -1;
	}

	log("CHttp2Socket Submit_request, msgId = %s, toId = %s",data->sMsgId.c_str(), data->sToId.c_str());

	int iRet = Submit_request(data->nva, data->navArrayLen, data->body, data->bodyLen);
	if (iRet > 0)
	{
		StreamData *streamData = new StreamData();
		if (!streamData)
		{
			log("new StreamData()");
		}
		else
		{
			streamData->iPostStatus = POST_STATUS_REQUEST_SEND;
			streamData->pPostData = data;
			//streamData->ipostTimeTick = time(NULL);
			streamData->streamId = iRet;

			m_streamDataMap.Insert(streamData);
		}
	}

	return iRet;
}

int CHttp2Socket::Send(void* data, int length)
{

	//char *tmpTest = (char *)data;
	int rv = SSL_write(GetSsl(), data, (int)length);
	if (rv <= 0) 
	{
		log("SSL_write %s, %d", data, length);

		int err = SSL_get_error(GetSsl(), rv);
		if (err == SSL_ERROR_WANT_WRITE || err == SSL_ERROR_WANT_READ) 
		{
			rv = NGHTTP2_ERR_WOULDBLOCK;

			log("socket:%d SSL_write NGHTTP2_ERR_WOULDBLOCK", GetSocket());
			//http->Close();
			SetWriteable(false);
		} 
		else 
		{
			log("socket:%d NGHTTP2_ERR_CALLBACK_FAILURE Close", GetSocket());
			rv = NGHTTP2_ERR_CALLBACK_FAILURE;
			Close();
		}
	}

	return rv;
}


int CHttp2Socket::Recv(void *data, int length)
{

	std::lock_guard<std::mutex> lock(m_SendRecvLineRun);

	int rv = 0;
	rv = SSL_read(GetSsl(), data, (int)length);
	if (rv < 0)
	{
		int err = SSL_get_error(GetSsl(), rv);
		if (err == SSL_ERROR_WANT_WRITE || err == SSL_ERROR_WANT_READ) 
		{
			rv = NGHTTP2_ERR_WOULDBLOCK;
			//log("socket:%d, NGHTTP2_ERR_WOULDBLOCK", GetSocket());
		} 
		else 
		{
			rv = NGHTTP2_ERR_CALLBACK_FAILURE;
			log("socket:%d, NGHTTP2_ERR_CALLBACK_FAILURE close", GetSocket());
			Close();
		}
	}
	else if (rv == 0) 
	{
		log("ssl read %s, %d", data, length);
		rv = NGHTTP2_ERR_EOF;
		Close();
	}
	return rv;
}

void CHttp2Socket::OnRead()
{
	if (!m_session)
	{
		log("m_session is null may be first call back.");
		return;
	}

	if (nghttp2_session_want_read(m_session))
	{
		int rv;
		rv = nghttp2_session_recv(m_session);
		if (rv != 0) 
		{
			log("nghttp2_session_recv", rv);
		}
		else
		{
			return;
		}
	}
	else
	{
		log("nghttp2_session_want_read");
		return;
	}
}



void CHttp2Socket::OnWrite()
{
	//log("OnWrite OnWrite");
	SetWriteable(true);
	return;
}

int CHttp2Socket::Close()
{
	log("socket:%d Close", GetSocket());
	m_streamDataMap.Clear();

	CSslSocket::Close();
	//SetWriteable(false);

	if (m_session)
	{
		nghttp2_session_del(m_session);
		m_session = nullptr;
	}
	return 0;
}

void CHttp2Socket::OnClose()
{
	log("socket:%d OnClose", GetSocket());
	Close();
}

size_t 	CHttp2Socket::_On_Body_read_callback(nghttp2_session *session, int32_t stream_id, uint8_t *buf, 
										   size_t length, uint32_t *data_flags, nghttp2_data_source *source, void *user_data)
{
//	NOTUSED_ARG(user_data);
	NOTUSED_ARG(stream_id);
	NOTUSED_ARG(length);
	NOTUSED_ARG(session);
	if (!source->ptr)
	{
		return -1;
	}
	
	CHttp2Socket *http = (CHttp2Socket*)user_data;
	BodyCallBackData *data = (BodyCallBackData *)source->ptr;
	log("post body):%s,socket=", data->body, http->GetSocket());

	memcpy(buf, (char *)data->body, data->len);

	*data_flags = *data_flags | NGHTTP2_DATA_FLAG_EOF;

	size_t iRet = data->len;

	delete data;

	return iRet;
}

int CHttp2Socket::Submit_request(nghttparr *nva, int navArrayLen, char *body, int len)
{
	if (!nva || !body)
	{
		log("!nva || !body");
		return -1;
	}
	
	int stream_id = 0;

	BodyCallBackData *bodyCallBackData = new BodyCallBackData;
	bodyCallBackData->body = body;
	bodyCallBackData->len = len;

	nghttp2_data_provider provider;
	provider.source.ptr = (bodyCallBackData);

	provider.read_callback = (nghttp2_data_source_read_callback)_On_Body_read_callback;

	if (navArrayLen < 5)
	{
		log("navArrayLen is little than 5");
	}

	if (!m_session)
	{
		log("m_session is null");
		return -1;
	}

	char bodyLen[10];
	sprintf(bodyLen, "%d", len);
	const nghttp2_nv nv[] = 
	{
		MAKE_NV(":method", string("POST")),
		MAKE_NV(":path", nva->path),
		MAKE_NV("apns-id", nva->apns_id),
		MAKE_NV("apns-topic", nva->apns_topic),
		MAKE_NV("authorization", nva->authorization),
		//MAKE_NV("Content-Length", string(bodyLen)),
	};

	//printf("method:%s\n", nva->method.c_str());
	//printf("path:%s\n", nva->path.c_str());
	//printf("apns-id:%s\n", nva->apns_id.c_str());
	//printf("apns-topic:%s\n", nva->apns_topic.c_str());
	//printf("authorization:%s\n", nva->authorization.c_str());

	stream_id = nghttp2_submit_request(m_session, NULL, nv,
						 sizeof(nv)/sizeof(nv[0]), &provider, this);
	if (stream_id < 0) 
	{
		log("nghttp2_submit_request", stream_id);
	}

	//log("[INFO] Stream ID = %d, socket:%d\n", stream_id, GetSocket());

	if (stream_id  > 0)
	{
		int rv = nghttp2_session_send(m_session);
		if (rv != 0) 
		{
			log("nghttp2_session_send");
			return -1;
		}

	}
	
	//log("[INFO] Stream ID = %d, socket:%d\n", stream_id, GetSocket());

	return stream_id;
}

int CHttp2Socket::Submit_ping()
{
	log("Submit_ping");

	if (!m_session)
	{
		log("m_session is null");
		return -1;
	}
    char buf[8] = {0};	
    //int iRet = nghttp2_submit_ping(m_session, 0x1, nullptr);
    int iRet = nghttp2_submit_ping(m_session, 0x0, (uint8_t*)buf);

    if (iRet < 0)
    {
    	log("nghttp2_submit_ping, errcode = %d", iRet);
        return iRet;
    }
    
    iRet = nghttp2_session_send(m_session);
    if (iRet < 0) {
    	log("nghttp2_session_send , errcode = %d", iRet);

    }
    return iRet;
}
