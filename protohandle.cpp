//
//  protohandle.cpp
//  PushServer
//
//  Created by lang on 14/06/2017.
//  Copyright © 2017 lang. All rights reserved.
//
#include <stdio.h>
#include <string.h>

#include <json/json.h>

#include <iostream>

#include "protohandle.h"
#include "jwkencodesign.h"
#include "CApnsPostData.h"
#include "ssl_post_mgr.h"
#include "ConfigFileReader.h"


static_assert(sizeof(CHead) < POST_BUF_SIZE, "sizeof(CHead) > POST_BUF_SIZE");
static_assert(sizeof(CPayLoad) < POST_BUF_SIZE, "sizeof(CPayLoad) > POST_BUF_SIZE");

const char *CHead::m_sMethod = "POST";
const char *CHead::m_sChemme = "https";

using namespace std;


time_t	CHead::m_timestampSecAuth = 0;
std::mutex	CHead::m_AuthorLock;
char	CHead::m_sAuthorization[u16AUTHJWTLEN] = { 0 };//Providertokenofjwt,https必须设置,ce如果设置,请求将被忽略!!

//pushType 消息类型 0:普通消息   1:p2p音频 2:好友邀请 3:群邀请 4:会议呼叫 5:频道聊天
enum IOS_PUSH_TYPE
{
	PUSH_TYPE_CHAT = 0,
	PUSH_TYPE_P2P_CALL = 1,
	PUSH_TYPE_CONTACTS = 2,
	PUSH_TYPE_GRP_CONTACTS = 3,
	PUSH_TYPE_REFERENCE_CALL = 4,
	PUSH_TYPE_CHNN_CHAT = 5
};




//////////////////////////////////////////////////////////////////////////////////////////////////////////
const char CAppOpenJson::type[12] = "notify_type";
const char CAppOpenJson::id[3] = "id";
const char CAppOpenJson::name[5] = "name";
const char CAppOpenJson::url[4] = "url";
const char CAppOpenJson::extra[6] = "extra";
Json::Value CAppOpenJson::GetJsonValue()
{
	Json::Value jroot;

	jroot[type] = utype;

	if (!sid.empty())
	{
		jroot[id] = sid;
	}

	if (!sname.empty())
	{
		jroot[name] = sname;
	}

	if (!surl.empty())
	{
		jroot[url] = surl;
	}

	if (!sextra.empty())
	{
		jroot[extra] = sextra;
	}

	return jroot;
}


void CAppOpenJson::Clear()
{
	utype = -1;
	sid.clear();
	sname.clear();
	surl.clear();
	sextra.clear();
}

bool CAppOpenJson::SetOpenVal(int type, const string& id, const string& str)
{
	Clear();
	utype = type;
	sid = id;

	switch (utype)
	{
	case 0:
		break;
	case 1:
		sname = str;
		break;
	case 2:
		surl = str;
		break;
	case 3:
		sextra = str;
		break;
	default:
		break;
	}
	return true;
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////

bool CPayLoad::SetAppOpenJson(int type, const string& sid, const string& str)
{
	return m_appOpenJson.SetOpenVal(type, sid, str);
}


CPayLoad::CPayLoad(const char *sTitle, const char *sBody)
{
	//m_vTitle_loc_args.clear();

	if (sTitle != nullptr)
	{
		m_sTitle = sTitle;
	}

	if (sBody != nullptr)
	{
		m_sBody = sBody;
	}
}

void CPayLoad::Clear()
{
	m_nBadge = 0;
	m_sTitle.clear();					//标题
	m_sBody.clear();					//消息内容
	m_appOpenJson.Clear();
}

bool CPayLoad::SetTitle(const string& sTitle)
{
	m_sTitle = sTitle;
	return true;
}

bool	CPayLoad::SetBody(const string& sBody)
{
	m_sBody = sBody;
	return true;

}

bool CPayLoad::SetToId(const string&strToid)
{
	m_strToId = strToid;
	return true;
}



std::string CPayLoad::GetToId()
{
	return m_strToId;
}

string CPayLoad::GetJson_s()
{
	Json::Value 		root;
	Json::Value			alertValue;

	//标题
	if (!m_sTitle.empty())
	{
		alertValue["title"] = m_sTitle;
	}
	//内容
	if (!m_sBody.empty()) {
		alertValue["body"] = m_sBody;
	}
	else
	{
		log("the m_sBody is empty!\n");
		return string();
	}

	if (!m_sLaunch_image.empty())
	{
		alertValue["launch-image"] = m_sLaunch_image;
	}


	Json::Value apsValue;
	apsValue["action"] = m_appOpenJson.GetJsonValue();

	apsValue["badge"] = GetBadge();	//1;
	apsValue["sound"] = "chime.aiff";
	//apsValue["category"] =  "mycategory";
	apsValue["alert"] = alertValue;

	//用户自定义数据类型
	//Json::Value customInValue;
	//customInValue["mykey"] =  "myvalue";
	//Json::Value customValue;
	//customValue["custom"] = customInValue;

	root["aps"] = apsValue;
	root["custom"] = m_custom;

	return root.toStyledString();
}

bool CPayLoad::SetBadge(int badge)
{
	m_nBadge = badge;
	return true;
}

int CPayLoad::GetBadge() const
{
	return m_nBadge;
}

bool CPayLoad::SetData(const ApnsData& apnsData)
{
	Clear();
	SetBody(apnsData.strBody);
	std::string sToId = formatString("%d", apnsData.nToId);
	SetToId(sToId);
	SetBadge(apnsData.nBadge);
	m_custom = apnsData.strCustom;
	SetAppOpenJson(1, sToId, sToId);
	return true;
}

void CHead::Clear()
{
	memset(m_sDeviceToken, 0, u8PATHLEN);
	memset(m_sTopic, 0, u8TOPICLEN);
	//根据不同的设备版本，确定不同的链路，以及连接方式，
	//指向全局变量
	//m_sHost.clear();

	m_sAPNsId.clear();
}

bool CHead::Init(std::string keyId, std::string teamId, const char* sTopic, const char* sAuthorization)
{
	m_keyId = keyId;
	m_teamId = teamId;
	return SetTopic(sTopic) && SetAuthorization(sAuthorization);
}

bool CHead::SetDeviceToken(const char *sDeviceToken)
{
	//printf("%s, %s\n", m_sDeviceToken, sDeviceToken);
	if (sDeviceToken == nullptr)
	{
		log("Error Device token: nullptr!\n");
		return false;
	}

	if (strlen(sDeviceToken) != u8TOKENLEN/*base 64*/)
	{
		log("Error Length Device token:%s!\n", sDeviceToken);
		return false;
	}

	if (!memset((void *)m_sDeviceToken, 0, (size_t)u8PATHLEN))
	{
		log("memset m_sPath false");
		return false;
	}
	sprintf(m_sDeviceToken, "/3/device/%s", sDeviceToken);

	//printf("%s, %s\n", m_sDeviceToken, sDeviceToken);

	return true;
}

/*{
	"alg": "ES256",
	"kid": "ABC123DEFG"
	}
	{
	"iss": "DEF123GHIJ",
	"iat": 1437179036
	}
	*/
// head->base64, body->base64,  sig = sign(head->base64+bodybase64) , sig->base64
bool CHead::SetAuthorization(const char *key)
{

	std::lock_guard<std::mutex> lock(m_AuthorLock);

	//log("SetAuthorization");
	//If the timestamp for token issue is not within the last hour, APNs rejects 
	//subsequent push messages, returning an ExpiredProviderToken (403) error.
	//here set the time is half of a hour: 1800 = 1*60*30
	time_t tm = time(NULL);
	if (tm - CHead::m_timestampSecAuth < 1800 && CHead::m_timestampSecAuth != 0)
	{
		//log("m_timestampSecAuth  1800 true m_sAuthorization:%s", m_sAuthorization);
		return true;
	}

	CHead::m_timestampSecAuth = tm;

	if (key == nullptr)
	{
		log("Error provider key!\n");
		return false;
	}

	char head[JWT_HEAD.size() + m_keyId.size()];
	sprintf(head, JWT_HEAD.c_str(), m_keyId.c_str());

	char claims[JWT_CLAIMS.size() + m_teamId.size() + 10]; //time`s must bit is 10 for dec
	sprintf(claims, JWT_CLAIMS.c_str(), m_teamId.c_str(), (int)tm);

	string sBase = jwt_base64must((const unsigned char *)head);
	if (sBase.empty())
	{
		log("sBase jwt_base64must");
		return false;
	}

	string strClaims = jwt_base64must((const unsigned char *)claims);
	if (strClaims.empty())
	{
		log("sBase jwt_base64must");
		return false;
	}

	sBase = sBase + "." + strClaims;

	string sSigned = jwt_encode_str(key, sBase.c_str());
	if (sSigned.empty())
	{
		log("sBase jwt_encode_str");
		return false;
	}

	string sSignedBase = jwt_base64must((const unsigned char *)sSigned.c_str());
	if (sSignedBase.empty())
	{
		log("sSignedBase jwt_base64must");
		return false;
	}

	if (u16AUTHJWTLEN < sBase.size() + sSignedBase.size() + HEAD_BEARER_S.size())
	{
		log("size is too big, more than u16AUTHJWTLEN!");
		return  false;
	}


	if (!memset((void *)m_sAuthorization, 0, (size_t)u16AUTHJWTLEN))
	{
		log("memset m_sAuthorization false");
		return false;
	}

	sprintf(m_sAuthorization, HEAD_BEARER_S.c_str(), sBase.c_str(), sSignedBase.c_str());
	//sprintf(m_sAuthorization,"bearer .MEYCIQCAMVPvNN_");

	return true;

}

//bundle ID.ce方式,如果有多个,必须指定一个,否则可为默认,token方式必须且只有一个
bool CHead::SetTopic(const char* sTopic)
{
	if (sTopic == nullptr || strlen(sTopic) == 0)
	{
		log("SetTopic error! strlen(sTopic) == 0\n");
		return false;
	}

	if (nullptr != memcpy(m_sTopic, sTopic, strlen(sTopic)))
	{
		return true;
	}
	else
	{
		log("SetTopic memcpy error!\n");
		return false;
	}
}

const char* CHead::GetDeviceToken() const
{
	return m_sDeviceToken;
}

const char* CHead::GetAuthorization() const
{
	return m_sAuthorization;
}

const char* CHead::GetTopic() const
{
	return m_sTopic;
}

nghttparr *CHead::GetNghttp2Nv(int &len)
{
	//need free
	nghttparr *pNva = new nghttparr;
	if (!pNva)
	{
		log("new nghttparr[sizeof(nva)/ sizeof(nghttparr)] is null");
		return nullptr;
	}

	len = 5;
	pNva->method = str_post;
	pNva->path = m_sDeviceToken;
	pNva->apns_id = m_sAPNsId;
	pNva->apns_topic = m_sTopic;
	pNva->authorization = m_sAuthorization;

	return pNva;
}

/////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////
std::shared_ptr<CApnsPostData> PostDataGenerator::GenerateAPNsPostData(const ApnsData& msg)
{
	std::shared_ptr<CApnsPostData> pPostData(NULL);

	//设置CHead
	static CHead	head;				//CHead vec for apns
	static bool bInit = false;
	if (!bInit)
	{
		CHttp2Config* pHttpConf = CHttp2Config::GetInstance();
		if (!head.Init(pHttpConf->GetKeyId(), pHttpConf->GetTeamId(), pHttpConf->GetTopicId().c_str(), pHttpConf->GetAuthKey().c_str()))
		{
			return pPostData;
		}
	}
	head.SetAPNsId("5e4e54654");
	head.SetDeviceToken(msg.strToken.c_str());

	//设置payLoad
	CPayLoad	payLoad;			//CPayLoad vec for apns
	if (!payLoad.SetData(msg))
	{
		log("Init payLoad failed");
		return pPostData;
	}

	//创建pPostData并设置值
	pPostData = std::shared_ptr<CApnsPostData>(new CApnsPostData);
	if (!pPostData)
	{
		return pPostData;
	}


	pPostData->nva = head.GetNghttp2Nv(pPostData->navArrayLen);
	if (!pPostData->nva)
	{
		log("GetNghttp2Nv is null!\n");
		return  shared_ptr<CApnsPostData>(NULL);
	}


	pPostData->sToId = payLoad.GetToId();
//	pPostData->sMsgId = msg.smsgid();

	string jsonStr = payLoad.GetJson_s();
	if (jsonStr.empty())
	{
		log("payload GetJson false!\n");
		return  shared_ptr<CApnsPostData>(NULL);
	}

	char *body = (char *)malloc(jsonStr.size() + 1);
	if (body == nullptr)
	{

		log("payload GetJson false!\n");
		return  shared_ptr<CApnsPostData>(NULL);
	}

	strcpy(body, jsonStr.c_str());

	pPostData->body = body;
	pPostData->bodyLen = jsonStr.size();

	log("%s\n", jsonStr.c_str());


	return pPostData;

}

//new fun
/*
string getiOSPayLoadJsonStr(const IM::Server::IMPushToUserReq& msg)
{
	Json::Value extendValue;
	Json::Value categoryValue;
	categoryValue["pushType"] = PUSH_TYPE_CHAT;
	categoryValue["action"] = 0;
	if(msg.emsgtype() == im::P2P_CALL)
	{
		categoryValue["pushType"] = PUSH_TYPE_P2P_CALL;
		Json::Value p2pValue;
		p2pValue["fromId"] = msg.sfromid();
		p2pValue["callId"] = msg.scallid();
		p2pValue["callType"] = msg.calltype();
		p2pValue["msgTime"] = (long long)getCurrentTime();
		extendValue["p2p_call"] = p2pValue;
	}
	else if(msg.emsgtype() == im::CONTACTS)
	{
		categoryValue["pushType"] = PUSH_TYPE_CONTACTS;
	}
	else if(msg.emsgtype() == im::GRP_CONTACTS)
	{
		categoryValue["pushType"] = PUSH_TYPE_GRP_CONTACTS;
	}
	else if(msg.emsgtype() == im::REFERENCE_CALL)
	{
		categoryValue["pushType"] = PUSH_TYPE_REFERENCE_CALL;
	}
	else if(msg.emsgtype() == im::CHNN_TALK)
	{
		categoryValue["pushType"] = PUSH_TYPE_CHNN_CHAT;
	}
	categoryValue["extend"] = extendValue;
	Json::Value apsValue;
	apsValue["alert"] = msg.sbody();
	apsValue["sound"] = "default";
	apsValue["category"] = categoryValue;
	Json::Value 	root;
	root["aps"] = apsValue;
	
	return root.toStyledString();
}
*/
/*
std::shared_ptr<CApnsPostData> PostDataGenerator::GenerateVoipHttp2PostData(const IM::Server::IMPushToUserReq& msg)
{

	std::shared_ptr<CApnsPostData> pPostData(NULL);

	//设置CHead
	static CHead	head;				//CHead vec for apns
	static bool bInit = false;
	if (!bInit)
	{
		if (!head.Init(pHttpConf->GetTopicId().c_str(), pHttpConf->GetAuthKey().c_str()))
		{
			return pPostData;
		}
	}
	head.SetAPNsId(msg.smsgid().c_str());
	head.SetDeviceToken(msg.stotoken().c_str());

   // //设置payLoad
   // CPayLoad	payLoad;			//CPayLoad vec for apns
   // if (!payLoad.SetData(msg))
   // {
   // 	log("Init payLoad failed");
   // 	return pPostData;
   // }


	//创建pPostData并设置值
	pPostData = std::shared_ptr<CApnsPostData>(new CApnsPostData);
	if (!pPostData)
	{
		return pPostData;
	}


	pPostData->nva = head.GetNghttp2Nv(pPostData->navArrayLen);
	if (!pPostData->nva)
	{
		log("GetNghttp2Nv is null!\n");
		return  shared_ptr<CApnsPostData>(NULL);
	}
	
    pPostData->sToId = msg.stoid();
	pPostData->sMsgId = msg.smsgid();

	string jsonStr = getiOSPayLoadJsonStr(msg);
	if (jsonStr.empty())
	{
		log("payload GetJson false!\n");
		return  shared_ptr<CApnsPostData>(NULL);
	}

	char *body = (char *)malloc(jsonStr.size() + 1);
	if (body == nullptr)
	{

		log("payload GetJson false!\n");
		return  shared_ptr<CApnsPostData>(NULL);
	}

	strcpy(body, jsonStr.c_str());

	pPostData->body = body;
	pPostData->bodyLen = jsonStr.size();

	log("%s\n", jsonStr.c_str());


	return pPostData;
}
*/
