#ifndef MSG_QUEUE_H
#define MSG_QUEUE_H

#include "Message.h"

#include <string>
#include <vector>

#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>
#include <boost/thread/condition.hpp>

using namespace std;

class MessageQueue;

typedef boost::shared_ptr<MessageQueue> MessageQueuePtr;

class MessageQueue {
private:
	std::vector<MessagePtr>					m_messageList;

	boost::mutex							m_mutex;
	boost::condition						m_cond;

public:
	MessageQueue();
	~MessageQueue();

	void WaitForFull();

	MessagePtr GetMessage();
	void PutMessage(MessagePtr msg);

	size_t GetQueueLength() { return m_messageList.size(); }
};

#endif