#ifndef COORDINATOR_H
#define COORDINATOR_H

#include "PeerNode.h"

#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>
#include <boost/enable_shared_from_this.hpp>

#include <pugixml.hpp>

#include <string>
#include "MessageQueue.h"
#include "ConnectionManager.h"

class Coordinator;

typedef boost::shared_ptr<Coordinator> CoordinatorPtr;

// Singleton Class
class Coordinator : public boost::enable_shared_from_this<Coordinator>{
private:
	static CoordinatorPtr		m_ptrCoordinator;
	std::string					m_ipAddress;
	int							m_port;
	boost::thread				m_coordThread;
	ConnectionManagerPtr		m_ptrConnMgr;

	MessageQueuePtr				m_pMsgQ;
	Coordinator();

	void run();
	void ProcessMessages(MessagePtr pMsg);

	int ReadConfigFile(std::string configFile);
	
public:
	static CoordinatorPtr GetCoordinator();

public:
	~Coordinator();
	
	void SetPort(int port) { m_port = port; }

	MessageQueuePtr GetMessageQueue() { return m_pMsgQ; }

	void Start(std::string configFile);
	void Kill();
};

#endif