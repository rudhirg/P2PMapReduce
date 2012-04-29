#include "MessageQueue.h"

MessageQueue::MessageQueue()
{
}

MessageQueue::~MessageQueue()
{
	boost::recursive_mutex::scoped_lock L(m_mutex);
	m_messageList.clear();
}

void MessageQueue::WaitForFull()
{
	boost::recursive_mutex::scoped_lock L(m_mutex);
	m_cond.wait(L);
}

WMessagePtr MessageQueue::GetMessage()
{
	boost::recursive_mutex::scoped_lock L(m_mutex);
	if(m_messageList.empty() == true)
		return WMessagePtr();	// return NULL

	WMessagePtr pmsg = m_messageList.front();
	m_messageList.erase( m_messageList.begin() );
	return pmsg;
}

void MessageQueue::PutMessage(WMessagePtr msg)
{
	boost::recursive_mutex::scoped_lock L(m_mutex);
	m_messageList.push_back(msg);
	m_cond.notify_all();
}