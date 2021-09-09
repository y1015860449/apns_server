//
//  utility.hpp
//  APNS_PushServer
//
//  Created by lang on 28/06/2017.
//  Copyright © 2017 lang. All rights reserved.
//

#ifndef utility_h
#define utility_h

#include <sys/time.h>
#include <stdlib.h>
#include <string>
#include <memory>
#include <stdio.h>
#include <string.h>
#include <string>
#include <memory>

#define SIGN_ERROR(V)	do{log("%d", (int)V); ret=""; goto jwt_sign_sha_pem_done; }while(0);

#define TEST

static const short THREADPOOL_MAX_SIZE 			= 500;			//thread pool max size
static const short THREADPOOL_MIN_SIZE 			= 1;			//thread pool min size
static const short THREADPOOL_DEFAULT_SIZE 		= 100;			//thread pool default

static const short POST_BUF_SIZE 				= 5120;



class CApnsPostData;
typedef  void (*ResponeCallBack_t)(std::shared_ptr<CApnsPostData>, void *userData);

#include <openssl/crypto.h>

enum  PUSH_CLIENT_TYPE
{
	PUSH_CLIENT_TYPE_UNKNOWN,
	PUSH_CLIENT_TYPE_APNS_DEV,
	PUSH_CLIENT_TYPE_APNS,
	PUSH_CLIENT_TYPE_VOIP_DEV,
    PUSH_CLIENT_TYPE_VOIP

};

struct ApnsData
{
    std::string strBody;
    std::string strCustom;
    std::string strToken;
    int nSound;
    uint32_t nBadge;
    uint32_t nToId;
};

const std::string JWT_HEAD = "{ \"alg\": \"ES256\", \"kid\": \"%s\" }";
const std::string JWT_CLAIMS = "{ \"iss\": \"%s\", \"iat\": %d }";
const std::string HEAD_BEARER_S = "bearer %s.%s";

const std::string apns_url = "api.push.apple.com";
const std::string apns_dev_url = "api.development.push.apple.com";

#endif /* utility_hpp */
