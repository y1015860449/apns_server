#include "base_tool.h"
#include <uuid/uuid.h>
#include <sys/time.h>

CSLog g_imlog = CSLog(LOG_MODULE_IM);

uint64_t get_tick_count()
{
	struct timeval tval;
	uint64_t ret_tick;
	gettimeofday(&tval, NULL);
	ret_tick = tval.tv_sec * 1000L + tval.tv_usec / 1000L;
	return ret_tick;
}

std::string formatString(const char *lpcszFormat,...)
{
    char *pszStr = NULL;
    if (NULL != lpcszFormat)
    {
        va_list marker;
        va_start(marker, lpcszFormat); //初始化变量参数
        pszStr = new char[1024];
        memset(pszStr, '\0', 1024);
        vsnprintf(pszStr, 1024, lpcszFormat, marker);
        va_end(marker); //重置变量参数
    }
    std::string strResult(pszStr);
    delete[]pszStr;
    return strResult;
}

std::string randomStrGen(int length) 
{
    srand(time(NULL));
    static std::string charset = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890";
    std::string result;
    result.resize(length);
    for (int i = 0; i < length; i++)
    {
        result[i] = charset[rand() % charset.length()];
    }
    return result;
}

std::string getUuId()
{
    uuid_t uuid;
    uuid_generate(uuid);
    char buf[64] = {0};
    uuid_unparse(uuid, buf);
    return buf;
}
