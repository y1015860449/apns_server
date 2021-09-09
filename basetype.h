#ifndef __BASE_TYPE_H__
#define __BASE_TYPE_H__

#include <string.h>
#include <string>
#include <stdarg.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <memory>
#include <vector>
#include <list>
#include <map>
#include <mutex>
#include <thread>
#include <ctype.h>
#include <algorithm>
#include <ext/hash_map>
#include <unordered_map>
#include <thread>
#include <condition_variable>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/epoll.h>
#include <sys/stat.h>
#include <netdb.h>
#include <fcntl.h>
#include <poll.h>

using namespace std;

typedef unsigned char	uchar_t;
typedef int				net_handle_t;
typedef int				conn_handle_t;

typedef int	SOCKET;
typedef int BOOL;

const int SOCKET_ERROR	= -1;
const int INVALID_SOCKET = -1;

typedef void (*callback_t)(void* callback_data, uint8_t msg, uint32_t handle, void* pParam);


enum {
	NETLIB_OK		= 0,
	NETLIB_ERROR	= -1
};

enum
{
    NETLIB_MSG_CONNECT = 1,
    NETLIB_MSG_CONFIRM,
    NETLIB_MSG_READ,
    NETLIB_MSG_WRITE,
    NETLIB_MSG_CLOSE,
    NETLIB_MSG_TIMER,
    NETLIB_MSG_LOOP
};


#endif
