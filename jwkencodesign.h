/**
  *  
  * file jwkencodesign.h 
  * create by liulang 
  *  
  * encode and sign the apns http2 head field
  *  
  * 2017-07-01 
  */

#ifndef	__JWKENCODESIGN_H__
#define	__JWKENCODESIGN_H__

#include <sys/time.h>
#include <stdlib.h>
#include <string>

#include <stdio.h>
#include <string.h>

#include <openssl/bio.h>
#include <openssl/evp.h>
#include <openssl/hmac.h>
#include <openssl/buffer.h>
#include <openssl/pem.h>

#include <openssl/ecdsa.h>

#include "utility.hpp"
#include "basetype.h"

#ifdef __cplusplus
extern "C" {
#endif

extern unsigned char *jwk_base64_encode(const unsigned char *in, int len);

extern std::string jwt_base64must(const unsigned char *src);

extern std::string jwt_encode_str(const  char *keyPem, const  char *jsonBase64Str);

extern int jwt_decode(const char *token, const unsigned char *key);

#ifdef __cplusplus
}
#endif

#endif	//jwkencodesign.h
