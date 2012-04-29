#include "TimerMessage.h"

TimerMessagePtr	TimerMessage::m_timerMessage;

TimerMessagePtr TimerMessage::GetTimerMessage()
{
	if(m_timerMessage == NULL) {
		m_timerMessage = TimerMessagePtr(new TimerMessage());
	}
	return m_timerMessage;
}

void TimerMessage::Init(TimerIPtr pObj)
{
	m_pObj = pObj;
}

//void TimerMessage::Start()
//{
//	m_thread = boost::thread(&TimerMessage::run, this);
//}
//
//void TimerMessage::run()
//{
//	while (true) {
//		WMessagePtr pMesg = m_pMsgQ->GetMessage();
//		// if empty wait for queue to get full
//		if( pMesg == NULL ) {
//			m_pMsgQ->WaitForFull();
//			continue;
//		}
//		ProcessMessages(pMesg);
//	}
//}
//
//void TimerMessage::Post(TimerIPtr pObj, SYS_MSG msg, int duration)
//{
//	
//}

void TimerMessage::CreateTimer(/*TimerIPtr pObj, */SYS_MSG msg, int duration)
{
	boost::asio::deadline_timer t(m_io, boost::posix_time::seconds(duration));

	WMessagePtr pMsg(new WMessage());
	t.async_wait(boost::bind(&TimerI::MessageEvent, m_pObj, msg));

	m_io.poll();

}