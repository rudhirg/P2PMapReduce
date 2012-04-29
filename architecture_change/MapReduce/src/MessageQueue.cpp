#include "MessageQueue.h"

MessageQueue::MessageQueue()
{
}

MessageQueue::~MessageQueue()
{
	boost::mutex::scoped_lock L(m_mutex);
	m_messageList.clear();
}

void MessageQueue::WaitForFull()
{
	boost::mutex::scoped_lock L(m_mutex);
	m_cond.wait(L);
}

MessagePtr MessageQueue::GetMessage()
{
	boost::mutex::scoped_lock L(m_mutex);
	if(m_messageList.empty() == true)
		return MessagePtr();	// return NULL

	MessagePtr pmsg = m_messageList.front();
	m_messageList.erase( m_messageList.begin() );
	return pmsg;
}

void MessageQueue::PutMessage(MessagePtr msg)
{
	boost::mutex::scoped_lock L(m_mutex);
	m_messageList.push_back(msg);
	m_cond.notify_all();
}