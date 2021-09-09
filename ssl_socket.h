#ifndef SSL_SOCKET_H
#define SSL_SOCKET_H


#include <openssl/ssl.h>
#include <openssl/bio.h>
 
#include <string> 
#include <memory> 
#include <sys/socket.h>

#include "ssl_event.h"
#include "utility.hpp"
#include "CApnsPostData.h"
class CApnsPostData;

void AddSocket(CSslSocket* pSocket);

void RemoveSocket(CSslSocket* pSocket);

CSslSocket* FindSocket(net_handle_t fd);


class CSocketBase
{
public:
	virtual ~CSocketBase(){}


	virtual bool Init(CSslEventDispatch *pSslEventDispatch = nullptr, const char *sslCeFileName = nullptr, const char *sslRsaPkFileName=nullptr ) = 0;

	virtual int SslConnectWebSite(callback_t callback, void* callback_data, const char* sHost, const int16_t iPort) = 0;

	virtual int ReConnect() = 0;


	virtual int Submit_ping() = 0;
	virtual int Send(void* buf, int len) = 0;

	virtual int Send(shared_ptr<CApnsPostData> data) = 0;

	virtual int Recv(void* buf, int len) = 0;

	virtual int Close() = 0;


	virtual void OnRead() = 0;

	virtual void OnWrite() = 0;

	virtual void OnClose() = 0;

	virtual int GetState() = 0;

	virtual SOCKET GetSocket() =0;


	virtual void SetWriteable(bool bFlag)
	{
		NOTUSED_ARG(bFlag);
	}

	virtual bool GetWriteable()
	{
		return false;
	}
};


class CSslSocket : public CSocketBase
{
public:
	virtual ~CSslSocket();
	CSslSocket();

	bool Init(CSslEventDispatch *pSslEventDispatch = nullptr, const char *sslCeFileName = nullptr, const char *sslRsaPkFileName = nullptr);

	callback_t GetCallBack(){return m_callback;}

	void* GetCallBackData(){return 	m_callback_data;}
	SOCKET GetSocket() { return m_socket; }


	void SetSocket(SOCKET fd) { m_socket = fd; }
	int GetState() { return m_state; }
	//void SetState(int stat) { m_state = stat; }

	SSL *GetSsl()
	{
		if (m_state == SOCKET_STATE_CONNECTED)
			return m_ssl;
		else
		{
			return nullptr;
		}
	}

	void SetState(uint8_t state) { m_state = state; }

	void SetCallback(callback_t callback) { m_callback = callback; }
	void SetCallbackData(void* data) { m_callback_data = data; }
	void SetRemoteIP(char* ip) { m_remote_ip = ip; }
	void SetRemotePort(uint16_t port) { m_remote_port = port; }
	void SetSendBufSize(uint32_t send_size);
	void SetRecvBufSize(uint32_t recv_size);
	const char*	GetRemoteIP() { return m_remote_ip.c_str(); }
	uint16_t	GetRemotePort() { return m_remote_port; }

	int SslConnectWebSite(callback_t callback, void* callback_data, const char* sHost, const int16_t iPort);

	int ReConnect();

	virtual int Send(void* buf, int len);

	virtual int Send(shared_ptr<CApnsPostData> data) override;

	virtual int Recv(void* buf, int len);

	virtual int Close();


	virtual void OnRead();

	virtual void OnWrite();

	virtual void OnClose();
public:

	virtual int Submit_ping() {return 0;}
private:

	int _SslShakeHands();

	bool ResolveHostName(const string s_hostname, const int16_t iPort);

	bool WaitReady(int sockFd, int32_t msec);

	int _GetErrorCode();
	bool _IsBlock(int error_code);

	string m_strHost;					
	struct sockaddr_in m_casIpv4;		
	string			m_remote_ip;		
	uint16_t		m_remote_port;

	callback_t		m_callback;
	void*			m_callback_data;

	uint8_t			m_state;
	SOCKET			m_socket;
	string 			m_local_addr;
	uint16_t		m_local_port;

	SSL_CTX* 		m_sslCtx;
	SSL* 			m_ssl;

	bool			m_bInit;
	CSslEventDispatch* m_pSslEventDispatch;


	//ssl ce and pk;
	string m_sslCeFileName;
	string m_sslRsaPkFileName;
};


#endif
