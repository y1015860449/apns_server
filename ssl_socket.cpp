#include "ssl_socket.h"

typedef unordered_map<net_handle_t, CSslSocket*> SocketMap;
SocketMap	g_ssl_socket_map;
//extern std::mutex gSockLock;



std::mutex lockSocket_map;
void AddSocket(CSslSocket* pSocket)
{
	std::lock_guard<std::mutex> lock(lockSocket_map);
	g_ssl_socket_map.insert(make_pair((net_handle_t)pSocket->GetSocket(), pSocket));
}

void RemoveSocket(CSslSocket* pSocket)
{
	std::lock_guard<std::mutex> lock(lockSocket_map);
	g_ssl_socket_map.erase((net_handle_t)pSocket->GetSocket());
}

CSslSocket* FindSocket(net_handle_t fd)
{
	std::lock_guard<std::mutex> lock(lockSocket_map);
	CSslSocket* pSocket = NULL;
	SocketMap::iterator iter = g_ssl_socket_map.find(fd);
	if (iter != g_ssl_socket_map.end())
	{
		pSocket = iter->second;
	}

	return pSocket;
}

CSslSocket::~CSslSocket()
{

	Close();

	if (m_ssl)
	{
		SSL_free(m_ssl);
		m_ssl = NULL;
	}

	if (m_sslCtx)
	{
		SSL_CTX_free(m_sslCtx);
		m_sslCtx = nullptr;
	}

}	

CSslSocket::CSslSocket()
{
	m_sslCtx = nullptr;
	m_ssl = nullptr;
	m_bInit = false;

	m_socket = INVALID_SOCKET;
	m_state = SOCKET_STATE_IDLE;

	m_pSslEventDispatch = nullptr;

	memset(&m_casIpv4, 0, sizeof(sockaddr_in));
}

bool CSslSocket::Init(CSslEventDispatch *pSslEventDispatch , const char *sslCeFileName , const char *sslRsaPkFileName)
{
	int nRet;
	m_sslCeFileName = sslCeFileName ? sslCeFileName :"";
	m_sslRsaPkFileName = sslRsaPkFileName ? sslRsaPkFileName : "";

	if (!pSslEventDispatch)
	{
		log("pSslEventDispatch is nullptr");
		return false;
	}
	m_pSslEventDispatch = pSslEventDispatch;

	const SSL_METHOD *sslMethod = SSLv23_client_method();

	if(nullptr == sslMethod)
	{
		log("SSLv23_client_method");
		return false;
	}

	m_sslCtx = SSL_CTX_new(sslMethod);
	if (NULL == m_sslCtx )
	{
		log("SSL_CTX_new");
		return false;
	}

	if (!m_sslCeFileName.empty())
	{
		if(!(nRet = SSL_CTX_use_certificate_file(m_sslCtx, m_sslCeFileName.c_str(), SSL_FILETYPE_PEM)))
		{
			int32_t nErrorCode = SSL_get_error(m_ssl, nRet);
			log("init ssl: use certificate file failed.errno: %d.", nErrorCode);
			SSL_CTX_free(m_sslCtx);
			m_sslCtx = NULL;
			return false;
		}	 
	}

	if (!m_sslRsaPkFileName.empty())
	{
		nRet = SSL_CTX_use_PrivateKey_file(m_sslCtx, m_sslRsaPkFileName.c_str(), SSL_FILETYPE_PEM);
		if(!nRet)
		{
			int32_t nErrorCode = SSL_get_error(m_ssl, nRet);
			printf("init ssl: use PrivateKey file failed.errno: %d.", nErrorCode);
			SSL_CTX_free(m_sslCtx);
			m_sslCtx = NULL;
			return false;
		}

		/* 检查用户私钥是否正确 */
		if (!SSL_CTX_check_private_key(m_sslCtx))
		{
			printf("init ssl: check PrivateKey failed");
			SSL_CTX_free(m_sslCtx);
			m_sslCtx = NULL;
			return false;
		}
	}


	m_ssl = SSL_new(m_sslCtx);
	if (NULL == m_ssl)
	{
		log("SSL_CTX_new error");
		return false;
	}

	m_bInit = true;
	return true;
}


void CSslSocket::SetSendBufSize(uint32_t send_size)
{
	int ret = setsockopt(m_socket, SOL_SOCKET, SO_SNDBUF, &send_size, 4);
	if (ret == SOCKET_ERROR) {
		log("set SO_SNDBUF failed for fd=%d", m_socket);
	}

	socklen_t len = 4;
	int size = 0;
	getsockopt(m_socket, SOL_SOCKET, SO_SNDBUF, &size, &len);
	log("socket=%d send_buf_size=%d", m_socket, size);
}

void CSslSocket::SetRecvBufSize(uint32_t recv_size)
{
	int ret = setsockopt(m_socket, SOL_SOCKET, SO_RCVBUF, &recv_size, 4);
	if (ret == SOCKET_ERROR) {
		log("set SO_RCVBUF failed for fd=%d", m_socket);
	}

	socklen_t len = 4;
	int size = 0;
	getsockopt(m_socket, SOL_SOCKET, SO_RCVBUF, &size, &len);
	log("socket=%d recv_buf_size=%d", m_socket, size);
}


int CSslSocket::_GetErrorCode()
{
	return errno;
}

bool CSslSocket::_IsBlock(int error_code)
{
	return ( (error_code == EINPROGRESS) || (error_code == EWOULDBLOCK) );
}

int CSslSocket::_SslShakeHands()
{

	if (m_socket <= 0)
	{
		log("_SslShakeHands m_socket <= 0!");
		return -1;
	}

	if (!m_ssl)
	{
		log("_SslShakeHands m_ssl == 0!");
		return -1;
	}
	SSL_set_fd(m_ssl, m_socket);
	//SSL_set_mode(m_ssl, SSL_MODE_ASYNC);

	int nRet = SSL_connect(m_ssl);
	if (1 != nRet)
	{
		int iRet = SSL_get_error(m_ssl, nRet);
		log("SSL_connect failed, socket:%d ssl err:%d, errno %d: %s", m_socket, iRet, errno, strerror(errno));
		return -1;
	}
	
	//struct cert_st *ce = m_ssl->cert;
	X509 * serverCertification = SSL_get_peer_certificate(m_ssl); //从SSL套接字中获取对方的证书信息
	if (NULL == serverCertification)
	{
		log("SSL_get_certificate error!");
		return -1;
	}

	char *cstrSslSubject = X509_NAME_oneline(X509_get_subject_name(serverCertification), 0, 0);
	char *cstrSslIssuer = X509_NAME_oneline(X509_get_issuer_name(serverCertification), 0, 0);	
	const char * cipher = SSL_get_cipher(m_ssl);

	log("cstrSslSubject:%s\n cstrSslIssuer:%s, cipher:%s\n", cstrSslSubject, cstrSslIssuer, cipher);

	X509_free(serverCertification);

	return m_socket;
}

int CSslSocket::Send(void* buf, int len)
{
	if (m_state != SOCKET_STATE_CONNECTED)
	{
		log("socekt state is: %d\n", m_state);
		return NETLIB_ERROR;
	}          

	//报文
	int n = SSL_write(m_ssl, (void*)buf, len);
	if (n <= 0)
	{
		int iRet = SSL_get_error(m_ssl, n);
		log("ssl err:%d, errno %d: %s", iRet, errno, strerror(errno));

		if (iRet == SSL_ERROR_WANT_READ || iRet == SSL_ERROR_WANT_WRITE)
		{
			n = 0;
			log("socket send block fd=%d", m_socket);
		}
		else
		{
			log("!!!send failed, error code: %d", errno);
			n =  -1;
		}

		return n;
	}
	return n;
}

int CSslSocket::Send(shared_ptr<CApnsPostData> data)
{
	return Send(data->body, data->bodyLen);
}

int CSslSocket::Recv(void* buf, int len)
{	
	if (m_state != SOCKET_STATE_CONNECTED)
	{
		log("socekt not SOCKET_STATE_CONNECTED");
		return NETLIB_ERROR;
	}

	 //报文
	int n = SSL_read(m_ssl, (void*)buf, len);
	if (n < 0)
	{
		int iRet = SSL_get_error(m_ssl, n);
		log("ssl err:%d, errno %d: %s", iRet, errno, strerror(errno));

		OnClose();
		RemoveSocket(this);
		return -1;
	}

	return n;
}

int CSslSocket::Close()
{

	m_state = SOCKET_STATE_CLOSING;

	log("socket:%d, close", m_socket);
	if (m_callback && m_callback_data)
		m_callback(m_callback_data, NETLIB_MSG_CLOSE, (net_handle_t)m_socket, NULL);

	if (m_ssl)
	{
		
		int32_t nRet = -1;
		SSL_set_shutdown(m_ssl, SSL_SENT_SHUTDOWN | SSL_RECEIVED_SHUTDOWN);

		nRet = SSL_shutdown(m_ssl);
		if (nRet == 0)
		{
			int32_t nErrorCode = SSL_get_error(m_ssl, nRet);
			log("ssl shutdown not finished, errno: %d.\n", nErrorCode);
		}
		else if (nRet == 1)
		{
			log("ssl shutdown successed.");
		}
		else if (nRet < 0)
		{
			int32_t nErrorCode = SSL_get_error(m_ssl, nRet);
			log("ssl shutdown failed, errno: %d.", nErrorCode);
		}

		nRet = SSL_clear(m_ssl);
		if (nRet != 1)
		{
			int32_t nErrorCode = SSL_get_error(m_ssl, nRet);
			log("ssl shutdown not finished, errno: %d.\n", nErrorCode);
		}
	}

	if (m_socket > 0 && m_pSslEventDispatch)
	{
		m_pSslEventDispatch->RemoveEvent(this, 0);

		RemoveSocket(this);
		close(m_socket);

		m_socket = -1;
	}
	else
	{
		log("m_socket <= 0");
	}
	return 0;
}

void CSslSocket::OnRead()
{

	if (m_state == SOCKET_STATE_LISTENING)
	{
		log("don`t to listen");
	}
	else if (m_state == SOCKET_STATE_CLOSING)
	{
		log("onread closed\n");
		return;
	}
	else
	{
		u_long avail = 0;
		if ( (ioctl(m_socket, FIONREAD, &avail) == SOCKET_ERROR) || (avail == 0) )
		{
			m_callback(m_callback_data, NETLIB_MSG_CLOSE, (net_handle_t)m_socket, NULL);
		}
		else
		{
			m_callback(m_callback_data, NETLIB_MSG_READ, (net_handle_t)m_socket, NULL);
		}
	}

}

//1. if the connect() is finished , the socket stat is being wirteable
//so the epoll will call this function
//2. some data send to sever and be ack,(io`s stat is changged) will call this function 
//getsockopt(m_socket, SOL_SOCKET, SO_SNDBUF
//ioctl(m_socket, SIOCOUTQ, &value);
//3. error occured???
void CSslSocket::OnWrite()
{
	if (m_state == SOCKET_STATE_CLOSING)
	{
		printf("onread closed\n");
		return;
	}

	m_callback(m_callback_data, NETLIB_MSG_WRITE, (net_handle_t)m_socket, NULL);
}

void CSslSocket::OnClose()
{
	Close();
}


int CSslSocket::ReConnect()
{

	log("ReConnect\n");

	int iRet = -1;
	if (!m_callback || !m_callback_data || m_strHost.empty() || m_remote_port <= 0)
	{
		return  -1;
	}
	
	if (m_state == SOCKET_STATE_CLOSING)
	{
		iRet =  SslConnectWebSite(m_callback, m_callback_data, m_strHost.c_str(), m_remote_port);

		if (iRet < 0)
		{
			Close();
		}
	}

	return iRet;
}

int SetBlockOrNot(int fd, int flag /*= 0*/)
{
	int opts;
	opts = fcntl(fd, F_GETFL);
	if (opts < 0)
	{
		return -1;
	}

	if (flag == 0)
	{
		opts &= ~O_NONBLOCK;
	}
	else
	{
		opts = opts | O_NONBLOCK;
	}

	if (fcntl(fd, F_SETFL, opts) < 0)
	{
		return -1;
	}
	return 0;
}

int CSslSocket::SslConnectWebSite(callback_t callback, void* callback_data, const char* sHost, const int16_t iPort)
{

	if(!m_bInit)
	{
		log("SslConnectWebSite not init");
		Init(nullptr, m_sslCeFileName.c_str(), m_sslRsaPkFileName.c_str());
	}

	m_callback = callback;
	m_callback_data = callback_data;

	if (!ResolveHostName(sHost, iPort))
	{
		log("get host %s Ip addr failed", sHost);

		return -1;
	}
	
	if ((m_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1)
	{
		log("create socket error!");
		Close();
		return -1;
	}	
	
	if(-1 == SetBlockOrNot(m_socket, 1))
	{
		log("setnonblocking socket :%d error!", m_socket);
		Close();
		return -1;
	}
	
	log("connect host %s, ip:%s, port:%u", m_strHost.c_str(), m_remote_ip.c_str(), m_remote_port);
	if (connect(m_socket,(struct sockaddr *)&m_casIpv4, sizeof(m_casIpv4)) == -1) 
	{
		if (errno == EHOSTUNREACH) 
		{
			log("EHOSTUNREACH socket :%d error!", m_socket);
			Close();
			m_remote_ip.clear();	
			return -1;
		} 
		else if (errno == EINPROGRESS) 
		{
			//ok
		} 
		else 
		{
			if (WaitReady(m_socket, 1000) != true)
			{
				log("WaitReady socket :%d error!", m_socket);
				Close();
				m_remote_ip.clear();
				return -1;
			}
		}				
	}

	//连接成功后获取本地ip,端口信息
	struct sockaddr_in locateSock;
	int nRet = sizeof(locateSock);
	if(getsockname(m_socket, (struct sockaddr *)&locateSock, (socklen_t *)&nRet) == -1)
	{
		log("%d setsockopt TCP_NODELAY errno:%s!", m_socket, strerror(errno));
		Close();
		return -1;
	}

	m_local_port = ntohs(locateSock.sin_port);
	m_local_addr = inet_ntoa(locateSock.sin_addr);

	log("tcp connect success, local port:%d, addr:%s",	m_local_port, m_local_addr.c_str());


	if (setsockopt(m_socket, IPPROTO_TCP, TCP_NODELAY, &nRet, sizeof(nRet)) == -1) {
		log("%d setsockopt TCP_NODELAY  error!", m_socket);
		Close();
		return -1;
	}

	if(-1 == SetBlockOrNot(m_socket, 0))
	{
		log("set blocking socket :%d error!", m_socket);
		Close();
		return -1;
	}

	//ssl 握手
	if(_SslShakeHands() < 0)
	{
		log("%d  _SslShakeHands  error!", m_socket);

		m_state = SOCKET_STATE_SSL_ERR;
		Close();

		return -1;
	}

	if(-1 == SetBlockOrNot(m_socket, 1))
	{
		log("set blocking socket :%d error!", m_socket);
		Close();
		return -1;
	}

	m_state = SOCKET_STATE_CONNECTED;

	AddSocket(this);


	if (m_pSslEventDispatch)
	{
		m_pSslEventDispatch->AddEvent(this, 5*60*1000);
	}

	return m_socket;
}

bool CSslSocket::ResolveHostName(const string s_hostname, const int16_t iPort)
{
	if (s_hostname.empty() || iPort <= 0)
	{
		log("ResolveHostName host or port not set");
		return false;
	}

	//域名和ip都没变
	if (m_strHost == s_hostname && iPort == m_remote_port)
	{
		//网络地址已经获取过
		if (!m_remote_ip.empty())
		{
			return true;
		}
	}

	m_strHost = s_hostname;
	m_remote_port = iPort;

	////////////////////////////////////////////////
	//重新通过域名和端口获取ip和网络地址
	m_remote_ip.clear();
	memset(&m_casIpv4, 0, sizeof(sockaddr_in));

	char service[NI_MAXSERV];
	snprintf(service, sizeof(service), "%u", iPort);

	struct addrinfo hints;
	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;

	struct addrinfo *res = NULL;

	int rv = getaddrinfo(s_hostname.c_str(), service, &hints, &res);
	if (rv != 0)
	{
		log("getaddrinfo %s\r\n", gai_strerror(rv));
		return false;
	}

	if (res)
	{
		memcpy(&m_casIpv4, (struct sockaddr_in *) res->ai_addr, sizeof(sockaddr_in));
	}
	m_remote_ip = inet_ntoa(m_casIpv4.sin_addr);
	
	return true;
}

bool CSslSocket::WaitReady(int sockFd, int32_t msec)
{
	struct pollfd   wfd[1];

	wfd[0].fd     = sockFd;
	wfd[0].events = POLLOUT;

	if (errno == EINPROGRESS)
	{
		int res;
		int err;
		socklen_t errlen = sizeof(err);

		if((res = poll(wfd, 1, msec)) == -1) 
		{
			log("poll error!");
			return false;
		} 
		else if(res == 0)
		{
			log("poll timeOut:%ld", msec);
			return false;
		}

		if(getsockopt(sockFd, SOL_SOCKET, SO_ERROR, &err, &errlen) == -1) 
		{
			log("getsockopt error!");
			return false;
		}

		if(err) 
		{
			log("getsockopt errno %d: %s !", errno, strerror(errno));
			return false;
		}
	}

	return true;
}


