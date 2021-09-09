//
//protohandle.h
//PushServer
//
//Createdbylangon12/06/2017.
//Copyright©2017lang.Allrightsreserved.
//推送推送头及负载信息
//

#ifndef __PROTOHANDLE_H__
#define __PROTOHANDLE_H__

#include <string.h>
#include <string>
#include <memory>

#include <nghttp2/nghttp2.h>
#include "json/json.h"

#include "utility.hpp"
#include "basetype.h"
#include "base_tool.h"
#include "singleton.h"
class CApnsPostData;

#define HTTP2VERSION

static const uint8_t u8HTTP2VERSION = 8;   //http2 version
static const uint16_t u16AUTHJWTLEN = 512; //classCHeadProvidertokenofjwt长度
static const uint8_t u8PATHLEN = 128;	   //classCHeaddivece-token路径
static const uint16_t u16HEADLEN = 512;	   //classCHeadCHead序列化长度
static const uint8_t u8TOPICLEN = 32;	   //classCHeadm_sTopiclen
static const uint8_t u8TOKENLEN = 64;	   //classCHeaddevicetokenlen=64base64

/*
":method"
":path"
"apns-id:"
"apns-topic"
"authorization"
*/
class nghttparr
{
public:
	string method;
	string path;
	string apns_id;
	string apns_topic;
	string authorization;
};

class CHead;
class CPayLoad;

/*
   {
   "aps":{
			"alert":{
					"title":"Game Request",
					"body":"Bob wants to play poker",
					"action-loc-key":"PLAY"
					},
			"badge":5
		},
   "acme1":"bar",
   "acme2":["bang","whiz"]
   }
   */
/*//Payload消息格式
//一级节点			二级节点							三级节点
---------------------------------------------------------------------------------
|														|title			String
|														|body			String
|														|title-loc-key	Stringornull
|														|title-loc-args	Arrayofstringsornull
| aps	dictionary	|alert		DictORString			|action-loc-key	Stringornull
|														|loc-key		String
|														|loc-args		Arrayofstrings
|														|launch-image	String
														-------------------------------
|					|badge				Number			|
|					|sound				String			|
|					|content-available	Number			|
|					|category			String			|
|					|thread-id			String			|
---------------------------------------------------------------------------------
自定义类型1	自定义	|
自定义类型2	自定义	|
---------------------------------------------------------------------------------
*/

using namespace std;

/*CAppOpenJson:自定义APP打开后的页面跳转,payLoad中的用户自定义数据*/
/*前提：点击推送进入APP时，APP能读取到推送的payLoad*/
/*自定义一些打开APP后的跳转信息，APP根据这些信息决定跳转到哪个页面*/

struct CAppOpenJson
{
	Json::Value GetJsonValue();

	void Clear();

	bool SetOpenVal(int type, const string &id, const string &str);

	const static char type[12]; //flag标签"notify_type"
	int utype;					//推送消息的类型，根据类型确定打开APP后跳转到哪个页面,参考im.pub.proto中的MsgType

	const static char id[3]; //flag标签"id"
	string sid;

	const static char name[5]; //flag标签"name"
	string sname;

	const static char url[4]; //flag标签"url"
	string surl;

	const static char extra[6]; //flag标签"extra"
	string sextra;
};

//APNs消息payload大小
//1. 基于token的为的消息可以为4KB (4096 bytes，普通消息)，或是5KB (5120 bytes音视频消息)；
//2. 基于老的二进制消息，为2KB（2048 bytes）
class CPayLoad
{
public:
	CPayLoad()
	{
		m_nBadge = 0;
		//m_vTitle_loc_args.clear();
	};

	CPayLoad(const char *sTitle, const char *sBody);

	const CPayLoad &operator=(const CPayLoad &c) = delete;
	CPayLoad(const CPayLoad &c) = delete;

	~CPayLoad(){
		//if (!m_vTitle_loc_args.empty())	//会自动调用m_vTitle_loc_args析构函数的
		//{
		//	m_vTitle_loc_args.clear();
		//}
	};

	void Clear();

	bool SetData(const ApnsData &apnsData);

	bool SetTitle(const string &sTitle);

	bool SetBody(const string &sBody);

	//bool	SetTitle_loc_key(char *sTitle_loc_key);
	//bool	SetTitle_loc_args(vector<string> &vec);

	//bool	SetLaunch_image(char *sLaunch_image);

	bool SetToId(const string &strToid);
	string GetToId();

	bool SetAppOpenJson(int utype, const string &sid, const string &str);

	/*
	   获取Json数据
	   */
	//bool GetJson(char *sGetValue) const;

	/**
	 *获取payload的json数据
	 @return true or false
	 */
	string GetJson_s();

	bool SetBadge(int badge);

	int GetBadge() const;

private:
	//alert三级节点数据
	string m_sTitle; //标题
	string m_sBody;	 //消息内容

	string m_sLaunch_image; //图片路径，和客户端协定
	string m_custom;

	//二级节点数据
	uint32_t m_nBadge; //显示数量 default don`t set

	string m_strToId;
	CAppOpenJson m_appOpenJson;
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/*
	:method = POST
	:scheme = https
	:path = /3/device/00fc13adff785122b4ad28809a3420982341241421348097878e577c991de8f0
	host = api.development.push.apple.com
	authorization = bearer eyAia2lkIjogIjhZTDNHM1JSWDciIH0.eyAiaXNzIjogIkM4Nk5WOUpYM0QiLCAiaWF0IjogIjE0NTkxNDM1ODA2NTAiIH0.MEYCIQDzqyahmH1rz1s-LFNkylXEa2lZ_aOCX4daxxTZkVEGzwIhALvkClnx5m5eAT6Lxw7LZtEQcH6JENhJTMArwLf3sXwi
	apns-id = eabeae54-14a8-11e5-b60b-1697f925ec7b
	apns-expiration = 0			//可以丢弃
	apns-priority = 10
	apns-topic = <MyAppTopic>	//通常是你App的bundle id
*/

/*
  const nghttp2_nv nva[] = {MAKE_NV(":method", "POST"),
  MAKE_NV(":path", "/3/device/6d48f191c76a88d2e817831916199adfccdb278c2ae46379a64650b11305118f"),
  MAKE_NV("apns-id:", "401734E6-08FF-927C-FA84-7F10298B3BCC"),
  MAKE_NV("apns-topic", "com.Mosh.NumberEngine"),
  MAKE_NV("authorization",
  "bearer eyAiYWxnIjogIkVTMjU2IiwgImtpZCI6ICI1UzdXODI5WFlFIiB9.eyAiaXNzIjogIlBGOUhRSldMMjQiLCAiaWF0IjogMTUwMTQ4OTg4NCB9.MEQCIFKVEiHt6CquI4qYx7ILDO-95BbaV2UEPstUjY-OAWBAAiBk-u1E3oodWoPeU_LLvkZ7ieZuJ1Ge2GPjf35lzRdDHg")};
  */

static const string str_post = "POST";
class CHead
{
public:
	CHead()
	{

		Clear();
	};

	~CHead()
	{
		Clear();
	};

	CHead(const CHead &) = delete;
	CHead &operator=(const CHead &) = delete;

	void Clear();

	bool Init(std::string keyId, std::string teamId, const char *sTopic, const char *sAuthorization);

	bool SetDeviceToken(const char *sDeviceToken);

	//设置jwt的值，调用第三方库libjwthttps://jwt.io
	bool SetAuthorization(const char *key);

	//设置bundleID,ce方式,如果有多个,必须指定一个,否则可为默认,token方式必须且只有一个
	bool SetTopic(const char *sTopic);

	//设置apns主机地址：测试，沙盒，https，feedback
	//bool SetHost(const char *sHost);

	bool SetAPNsId(const char *APNsId)
	{
		string msg = getUuId();
		transform(msg.begin(), msg.end(), msg.begin(), ::toupper);

		if (nullptr == APNsId)
		{
			return false;
		}
		//m_sAPNsId = APNsId;
		m_sAPNsId = msg;

		return true;
	}

	const char *GetDeviceToken() const;
	const char *GetAuthorization() const;
	const char *GetTopic() const;
	//const char *GetHost() const;

	nghttparr *GetNghttp2Nv(int &len);

	//bool	GetHttpHeadStr(char *str);

	//desc:

	/**
	 获取HTTP头
	 @param str 待拷贝的内存首地址
	 @param uMemTotal 待拷贝的内存块的大小
	 @return true or  false
	 */
	//bool	GetHttpHeadStr_s(char* const str, uint32_t &uMemTotal);

	//forDebug
	void PrintAll();

	static time_t m_timestampSecAuth;

private:
	//uint8_t		m_nDeviceVer;		//设备版本号

	static const char *m_sMethod;	//--POST所有的推送方法,固定
	static const char *m_sChemme;	//--https所有的推送连接,固定
	char m_sDeviceToken[u8PATHLEN]; // /3/device/"divece-token"后面tokeid根据不同的设备变化，前面不变

	//string		m_sHost;			//指向全局变量，不需要释放资源，apns主机地址：测试，沙盒，https，feedback

	static std::mutex m_AuthorLock;
	static char m_sAuthorization[u16AUTHJWTLEN]; //Providertokenofjwt,https必须设置,ce如果设置,请求将被忽略!!

	std::string m_sAPNsId; //可由服务器返回，这里可以不配置
	//uint32_t				m_nExpiration;		//延迟发送，默认为0，立即发送
	//uint8_t				m_nPriority;		//10--APNs将立即发送，5--APNs可能会延迟或丢失，默认为10
	char m_sTopic[u8TOPICLEN]; //bundleID.ce方式,如果有多个,必须指定一个,否则可为默认,token方式必须且只有一个
	std::string m_keyId;
	std::string m_teamId;
};

/*
 *处理来之业务服务器的消息
 */
//
class CApnsPostData;
class PostDataGenerator
{
public:
	static std::shared_ptr<CApnsPostData> GenerateAPNsPostData(const ApnsData &msg);
	static std::shared_ptr<CApnsPostData> GenerateVoipHttp2PostData(const ApnsData &msg);
};

class CHttp2Config : public Singleton<CHttp2Config>
{
public:
	CHttp2Config(){};
	~CHttp2Config(){};

public:
	void InitHttp2Config(const std::string topic, const std::string keyId, const std::string teamId, const std::string authKey, PUSH_CLIENT_TYPE type)
	{
		m_topicId = topic;
		m_keyId = keyId;
		m_teamId = teamId;
		m_authKey = authKey;
		m_pushType = type;
	};
	std::string GetTopicId() { return m_topicId; };
	std::string GetKeyId() { return m_keyId; };
	std::string GetTeamId() { return m_teamId; };
	std::string GetAuthKey() { return m_authKey; };
	PUSH_CLIENT_TYPE GetPushClientType() { return m_pushType; };

private:
	std::string m_topicId;
	std::string m_keyId;
	std::string m_teamId;
	std::string m_authKey;
	PUSH_CLIENT_TYPE m_pushType;
};

#endif // __PROTOHANDLE_H__
