/**
 *  
 * ssl_event.cpp
 */

#include "ssl_event.h"
#include "ssl_socket.h"
#include "base_tool.h"

#define MIN_TIMER_DURATION 100 // 100 miliseconds
#define MAX_EVENT_NUMS 1024 * 10 * 10 * 5

CSslEventDispatch *CSslEventDispatch::m_pSslEventDispatch = NULL;

CSslEventDispatch::CSslEventDispatch()
	: m_waitTime(100), running(false)
{
	m_epfd = epoll_create(MAX_EVENT_NUMS);
	if (m_epfd == -1)
	{
		printf("epoll_create failed\n");
	}

	m_event_state = EVENT_NONE;
}

CSslEventDispatch::~CSslEventDispatch()
{
	m_event_state = EVENT_NONE;
}

void CSslEventDispatch::AddEvent(CSslSocket *sslSocket, uint64_t interval)
{
	NOTUSED_ARG(interval);
	std::lock_guard<std::mutex> lock(m_lockAddEvent);

	if (sslSocket->GetSocket() > 0)
	{
		_AddEvent(sslSocket->GetSocket());
	}
}

void CSslEventDispatch::_AddEvent(SOCKET fd)
{

	log("ep add event socket:%d\n", fd);
	struct epoll_event ev;
	ev.events = EPOLLIN | EPOLLOUT | EPOLLET | EPOLLPRI | EPOLLERR | EPOLLHUP | EPOLLRDHUP;

	ev.data.fd = fd;
	if (epoll_ctl(m_epfd, EPOLL_CTL_ADD, fd, &ev) != 0)
	{
		log("epoll_ctl() failed, errno=%d", errno);
	}

	m_event_state = EVENT_READABLE | EVENT_WRITEABLE;
}

void CSslEventDispatch::RemoveEvent(CSslSocket *sslSocket, uint8_t socket_event)
{
	std::lock_guard<std::mutex> lock(m_lockRemoveEvent);
	if (sslSocket->GetSocket() > 0)
	{
		_RemoveEvent(sslSocket->GetSocket(), socket_event);
	}
}

void CSslEventDispatch::_RemoveEvent(SOCKET fd, uint8_t socket_event)
{
	NOTUSED_ARG(socket_event);
	if (epoll_ctl(m_epfd, EPOLL_CTL_DEL, fd, NULL) != 0)
	{
		printf("epoll_ctl failed, errno=%d", errno);
	}
}

void CSslEventDispatch::StartDispatch(uint32_t wait_timeout)
{
	m_waitTime = wait_timeout;
	if (running)
	{
		log("sslEventDispatch %p already running, set waitTime to %d", this, m_waitTime);
		return;
	}

	OnThreadRun();
}

void CSslEventDispatch::StopDispatch()
{
	running = false;
}

void CSslEventDispatch::OnThreadRun(void)
{
	struct epoll_event *events = NULL;

	events = new struct epoll_event[MAX_EVENT_NUMS];
	if (!events)
		return;
	running = true;
	log("sslEventDispatch %p start success", this);
	std::thread epollThread([events, this]()
	{
		int nfds = 0;
		while (running)
		{
			//static uint64_t curr_tick = get_tick_count();Ò
			nfds = epoll_wait(this->m_epfd, events, MAX_EVENT_NUMS, m_waitTime);
			for (int i = 0; i < nfds; i++)
			{
				int ev_fd = events[i].data.fd;
				CSslSocket *pSocket = FindSocket(ev_fd);

				if (!pSocket)
					continue;

#ifdef EPOLLRDHUP
				if (events[i].events & EPOLLRDHUP)
				{
					log("On Peer Close, socket=%d\n", ev_fd);
					pSocket->OnClose();
				}
#endif

				if (events[i].events & EPOLLIN)
				{
					//log("OnRead, socket=%d\n", ev_fd);
					pSocket->OnRead();
				}

				if (events[i].events & EPOLLOUT)
				{
					//log("OnWrite, socket=%d\n", ev_fd);
					pSocket->OnWrite();
				}

				if (events[i].events & (EPOLLPRI | EPOLLERR | EPOLLHUP))
				{
					log("OnClose, socket=%d\n", ev_fd);
					pSocket->OnClose();
				}
			}
		}
	});
	epollThread.detach(); 
	delete events;
}
