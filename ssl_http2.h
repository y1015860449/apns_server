/** 
 *  file ssl_http2.h
 *  nghttp2 to connect apns whit http2 date: 2017072
 * create by liulang 
 */

#ifndef __SSL_HTTP2_H__
#define __SSL_HTTP2_H__



#include "ssl_event.h"
#include "ssl_socket.h"
#include "ssl_post_mgr.h"
#include "protohandle.h"

class CApnsPostData;
#include <list>

#include <nghttp2/nghttp2.h>

/*
enum
{
	NETLIB_MSG_CONNECT = 1,
	NETLIB_MSG_CONFIRM,
	NETLIB_MSG_READ,
	NETLIB_MSG_WRITE,
	NETLIB_MSG_CLOSE,
	NETLIB_MSG_TIMER,
    NETLIB_MSG_LOOP
};*/


enum
{
    HTTP_PING_RESPONE = 6,
	HTTP_MSG_RESPONE = 7,
	HTTP_USEABLE = 8,
	HTTP_NOT_USEABLE = 9,
};

enum
{
	POST_STATUS_INIT =	 0,				//init
	POST_STATUS_REQUEST_SEND = 1,		//request send
	POSt_STATUS_RESPONE_ECODE = 2,		//respone. 200 Success
	POST_STATUS_RESPONE_APNS_ID  = 3,	//apns-id
	POST_STATUS_RESPONE_DATA_CHUNK=4 	//if err,not 200, the reason return with data chunk callback
};


typedef struct _StreamData
{
	_StreamData()
	{
		pPostData = nullptr;
		streamId = -1;
		iPostStatus = POST_STATUS_INIT;

		//ipostTimeTick = time(NULL);
	}

	~_StreamData()
	{
		if (!pPostData)
		{
			//pPostData.reset();
		}
		//ipostTimeTick = time(NULL);
	}

	int iPostStatus;
	shared_ptr<CApnsPostData> pPostData;
	int streamId;
}StreamData;

class CStreamMap
{
public:

	~CStreamMap();
	StreamData 	*GetStream(int streamId);
	StreamData 	*GetAndDel(int streamId);
	int		Insert(StreamData*);

	void 		Clear();

private:
	std::mutex m_mapMutex;
	map<int, StreamData *> m_streamMap;
};


class CHttp2Socket;

typedef struct _bodyCallBackData
{
	char *body;
	int len;
}BodyCallBackData;

class CHttp2Socket : public CSslSocket
{

	static void httpCallback(void* callback_data, uint8_t msg, uint32_t handle, void* pParam);

public:

	CHttp2Socket()
	{
		m_callbacks = nullptr;
		m_session = nullptr;
		m_Writeable = false;
		//m_streamDataCur = nullptr;
	}

	~CHttp2Socket()
	{
		nghttp2_session_del(m_session);
		m_session = nullptr;
	}

	int SslConnectWebSite(callback_t callback, void* callback_data, const char* host, const int16_t port);

	int Submit_request(nghttparr *nva, int navArrayLen, char *body, int len);

	int Submit_ping();

	int ReConnect();

	int Send(void *data, int length);
	int Recv(void *data, int length);


	int Send(shared_ptr<CApnsPostData> data);

	void OnRead();

	void OnWrite();

	void OnClose();

	int Close();

	void SetWriteable(bool bFlag)
	{
		std::lock_guard<std::mutex> lock(m_mutexWrite);
		m_Writeable = bFlag;
	}

	bool GetWriteable()
	{
		std::lock_guard<std::mutex> lock(m_mutexWrite);
		return m_Writeable;
	}

	//StreamData		*m_streamDataCur;
	CStreamMap		m_streamDataMap;

private:

	std::mutex	m_SendRecvLineRun;

	void SetNghttp2Callbacks();

	//static int 		_Decode_status_code(const uint8_t *value, size_t len);

	static int 		_On_header_callback(nghttp2_session *session, const nghttp2_frame *frame, const uint8_t *name, size_t namelen, const uint8_t *value, size_t valuelen, uint8_t flags, void *userp);

	static int 		_On_data_chunk_recv_callback(nghttp2_session *session, uint8_t flags, int32_t stream_id, const uint8_t *data, size_t len, void *user_data);

	static size_t 	_On_send_callback(nghttp2_session *session, const uint8_t *data, size_t length, int flags, void *user_data);

	static size_t 	_On_recv_callback(nghttp2_session *session, uint8_t *buf, size_t length, int flags, void *user_data);

	static size_t 	_On_Body_read_callback(nghttp2_session *session, int32_t stream_id, uint8_t *buf, 
										   size_t length, uint32_t *data_flags, nghttp2_data_source *source, void *user_data);

	std::mutex	m_mutexWrite;
	bool m_Writeable;
	int		m_countWrite;

	nghttp2_session_callbacks *m_callbacks;

	nghttp2_session *m_session;
	void *m_userData;

	string 			m_hostStr;
	int16_t			m_port;
};

#endif // __SSL_HTTP2_H__
