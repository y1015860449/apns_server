#include "apns_push_post.h"
#include "ssl_http2.h"
#include "CApnsPostData.h"
CSocketBase* CApnsPushPost::createSocket()
{
	return new CHttp2Socket();
}

int32_t CApnsPushPost::Request(shared_ptr<CApnsPostData> pData)
{
	log("Request\n");
	//m_Sendbuf = data;
	int iRet = GetSslSock()->Send(pData);
	if (iRet <= 0)
	{
		log("m_pSocket->Send");
		return -1;
	}

	//SetUsrable(false);

	//m_uPostIndex++;
	return iRet;
}
