#ifndef __APNS_PUSH_POST_MGR_H__
#define __APNS_PUSH_POST_MGR_H__


#include "ssl_post_mgr.h"
class CSslEventDispatch;

class CApnsPostMgr : public CPostMgr
{
public:
	CApnsPostMgr(CSslEventDispatch *pSslEventDispatch = nullptr);
	virtual ~CApnsPostMgr();
private:
	virtual CPost* createPost() override;

};

#endif // __APNS_PUSH_POST_MGR_H__






