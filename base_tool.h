#ifndef __BASE_TOOL_H__
#define __BASE_TOOL_H__

#include "basetype.h"

#include "slog_api.h"

#define NOTUSED_ARG(v) ((void)v)

#define LOG_MODULE_IM  "apns-push"
extern CSLog g_imlog;
#define __FILENAME__ (strrchr(__FILE__, '/') ? (strrchr(__FILE__, '/') + 1):__FILE__)
#define log(fmt, args...)  g_imlog.Info("<%s>|<%d>|<%s>," fmt, __FILENAME__, __LINE__, __FUNCTION__, ##args)

uint64_t get_tick_count();
std::string formatString(const char *lpcszFormat,...);
std::string randomStrGen(int length);
std::string getUuId();

#endif
