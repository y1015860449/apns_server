#include "apns_push_post_mgr.h"
#include "apns_push_post.h"

CApnsPostMgr::CApnsPostMgr(CSslEventDispatch *pSslEventDispatch /*= nullptr*/)
	:CPostMgr(pSslEventDispatch)
{

}

CApnsPostMgr::~CApnsPostMgr()
{

}

CPost* CApnsPostMgr::createPost()
{
	return new CApnsPushPost;
}

