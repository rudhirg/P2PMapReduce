#ifndef TIMER_H
#define TIMER_H

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include "Message.h"
#include <boost/asio.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>
#include <boost/enable_shared_from_this.hpp>

class TimerI;
class TimerMessage;

typedef boost::shared_ptr<TimerI> TimerIPtr;
typedef boost::shared_ptr<TimerMessage> TimerMessagePtr;

class TimerI {
public:
	virtual void MessageEvent(SYS_MSG msg) = 0;
};

class TimerMessage : public boost::enable_shared_from_this<TimerMessage> {
private:
	static TimerMessagePtr								m_timerMessage;

	boost::asio::io_service								m_io;
	std::vector<boost::asio::deadline_timer>			m_timerList;
	boost::thread										m_thread;

	TimerIPtr											m_pObj;


public:
	TimerMessage() {}

	~TimerMessage() {}

	static TimerMessagePtr GetTimerMessage();
	/*void Start();
	void run();

	void Post(TimerIPtr pObj, SYS_MSG msg, int duration);*/

	void Init(TimerIPtr pObj);
	void CreateTimer(/*TimerIPtr pObj,*/ SYS_MSG msg, int duration);
};

#endif