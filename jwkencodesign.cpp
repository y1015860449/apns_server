/**
  *  
  * file jwkencodesign.cpp 
  * create by liulang 
  *  
  * encode and sign the apns http2 head field
  *  
  * 2017-07-01 
  */

#include "jwkencodesign.h"
#include "base_tool.h"

#ifdef __cplusplus
extern "C" {
#endif
static void ECDSA_SIG_get0(const ECDSA_SIG *sig, const BIGNUM **pr, const BIGNUM **ps)
{
	if (pr != NULL)
		*pr = sig->r;
	if (ps != NULL)
		*ps = sig->s;
}


static const unsigned char to_b64_tab[] =
	"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static const unsigned char un_b64_tab[] = {
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 62,  255, 255, 255, 63,
	52,  53,  54,  55,  56,  57,  58,  59,  60,  61,  255, 255, 255, 255, 255, 255,
	255, 0,   1,   2,   3,   4,   5,   6,   7,   8,   9,   10,  11,  12,  13,  14,
	15,  16,  17,  18,  19,  20,  21,  22,  23,  24,  25,  255, 255, 255, 255, 255,
	255, 26,  27,  28,  29,  30,  31,  32,  33,  34,  35,  36,  37,  38,  39,  40,
	41,  42,  43,  44,  45,  46,  47,  48,  49,  50,  51,  255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255
};

unsigned char *jwk_base64_encode(const unsigned char *in, int len)
{
	const unsigned char *clear = (const unsigned char*) in;
	unsigned char *code = NULL;
	unsigned char *p;

	code = (unsigned char *)malloc(4 * ((len + 2) / 3) + 1);
	if (NULL == code)
	{
		return NULL;
	}
	p = code;

	while (len-- >0) {
		register int x, y;

		x = *clear++;
		*p++ = to_b64_tab[(x >> 2) & 63];

		if (len-- <= 0) {
			*p++ = to_b64_tab[(x << 4) & 63];
			*p++ = '=';
			*p++ = '=';
			break;
		}

		y = *clear++;
		*p++ = to_b64_tab[((x << 4) | ((y >> 4) & 15)) & 63];

		if (len-- <= 0) {
			*p++ = to_b64_tab[(y << 2) & 63];
			*p++ = '=';
			break;
		}

		x = *clear++;
		*p++ = to_b64_tab[((y << 2) | ((x >> 6) & 3)) & 63];

		*p++ = to_b64_tab[x & 63];
	}

	*p = 0;

	return (code);
}


string jwt_base64must(unsigned const char *src)
{

	if (!src)
	{
		return nullptr;
	}

	unsigned char *ctmp = jwk_base64_encode(src, (int)strlen((const char *) src));
	if (!ctmp)
	{
		return "";
	}

	string stmp = (char *)ctmp;
	delete ctmp;

	//return stmp;

	int len = stmp.size();
	int i, t;

	for (i = t = 0; i < len; i++) {
		switch (stmp[i]) {
		case '+':
			stmp[t++] = '-';
			break;
		case '/':
			stmp[t++] = '_';
			break;
		case '=':
			break;
		default:
			stmp[t++] = stmp[i];
		}
	}

	stmp[t] = '\0';

	return stmp;
}

string jwt_encode_str(const  char *keyPem, const  char *jsonBase64Str)
{
	EVP_MD_CTX *mdctx = NULL;
	ECDSA_SIG *ec_sig = NULL;
	const BIGNUM *ec_sig_r = NULL;
	const BIGNUM *ec_sig_s = NULL;
	BIO *bufkey = NULL;
	const EVP_MD *alg;
	int type;
	
	int pkey_type;
	unsigned char *sig;
	string ret;
	size_t slen;


	alg = EVP_sha256();
	type = EVP_PKEY_EC;


	static FILE *fp;
	static EVP_PKEY *pkey = NULL;
	if (!pkey)
	{
		if (!fp)
		{
			fp = fopen(keyPem, "r");

			if (!fp)
			{
				log("fopen %s", keyPem);
				return "";
			}
		}
		pkey = PEM_read_PrivateKey(fp, NULL, NULL, NULL);
		fclose(fp);
		fp = NULL;
	}

	if (pkey == NULL)
		SIGN_ERROR(EINVAL);

	pkey_type = EVP_PKEY_id(pkey);
	if (pkey_type != type)
		SIGN_ERROR(EINVAL);

	mdctx = EVP_MD_CTX_create();
	if (mdctx == NULL)
		SIGN_ERROR(ENOMEM);

	/* Initialize the DigestSign operation using alg */
	if (EVP_DigestSignInit(mdctx, NULL, alg, NULL, pkey) != 1)
		SIGN_ERROR(EINVAL);

	/* Call update with the message */
	if (EVP_DigestSignUpdate(mdctx, jsonBase64Str, strlen(jsonBase64Str)) != 1)
		SIGN_ERROR(EINVAL);

	/* First, call EVP_DigestSignFinal with a NULL sig parameter to get length
	 * of sig. Length is returned in slen */
	if (EVP_DigestSignFinal(mdctx, NULL, &slen) != 1)
		SIGN_ERROR(EINVAL);

	/* Allocate memory for signature based on returned size */
	sig = (unsigned char*)alloca(slen);
	if (sig == NULL)
		SIGN_ERROR(ENOMEM);

	/* Get the signature */
	if (EVP_DigestSignFinal(mdctx, sig, &slen) != 1)
		SIGN_ERROR(EINVAL);


		unsigned int degree, bn_len, r_len, s_len, buf_len;
		unsigned char *raw_buf;
		EC_KEY *ec_key;

		/* For EC we need to convert to a raw format of R/S. */

		/* Get the actual ec_key */
		ec_key = EVP_PKEY_get1_EC_KEY(pkey);
		if (ec_key == NULL)
			SIGN_ERROR(ENOMEM);

		degree = EC_GROUP_get_degree(EC_KEY_get0_group(ec_key));

		EC_KEY_free(ec_key);

		/* Get the sig from the DER encoded version. */
		ec_sig = d2i_ECDSA_SIG(NULL, (const unsigned char **)&sig, slen);
		if (ec_sig == NULL)
			SIGN_ERROR(ENOMEM);

		ECDSA_SIG_get0(ec_sig, &ec_sig_r, &ec_sig_s);
		r_len = BN_num_bytes(ec_sig_r);
		s_len = BN_num_bytes(ec_sig_s);
		bn_len = (degree + 7) / 8;
		if ((r_len > bn_len) || (s_len > bn_len))
			SIGN_ERROR(EINVAL);

		buf_len = 2 * bn_len;
		//stack malloc
		raw_buf = (unsigned char *)alloca(buf_len);
		if (raw_buf == NULL)
			SIGN_ERROR(ENOMEM);

		/* Pad the bignums with leading zeroes. */
		memset(raw_buf, 0, buf_len);
		BN_bn2bin(ec_sig_r, raw_buf + bn_len - r_len);
		BN_bn2bin(ec_sig_s, raw_buf + buf_len - s_len);

		ret = (char *)raw_buf;

jwt_sign_sha_pem_done:
	if (bufkey)
		BIO_free(bufkey);
	if (pkey)
	{
			//EVP_PKEY_free(pkey);
	}
	if (mdctx)
		EVP_MD_CTX_destroy(mdctx);
	if (ec_sig)
		ECDSA_SIG_free(ec_sig);

	//low probability to sign fail
	//Qux1dO7aFXbp16z1bW8M1Z0i187vtLXwXA
	if (ret.size() != 65)
	{
		log("ret.size():%d", ret.size());
		log("ret.size():%d", ret.size());
		log("ret.size():%d", ret.size());

		ret = jwt_encode_str(keyPem, jsonBase64Str);
	}
	return ret;

}
//int jwt_decode(const char *token, const unsigned char *key);

#ifdef __cplusplus
}
#endif

