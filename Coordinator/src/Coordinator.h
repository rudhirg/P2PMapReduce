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
#include "Database.h"
#include "Message.h"

class Coordinator;

typedef boost::shared_ptr<Coordinator> CoordinatorPtr;

// Singleton Class
class Coordinator : public boost::enable_shared_from_this<Coordinator>{
private:
	static CoordinatorPtr		m_ptrCoordinator;
	std::string					m_ipAddress;
	int							m_port;
	DatabasePtr					m_db;		// stores config infos
	boost::thread				m_coordThread;

	long						m_lastRandomPeerId;

	ConnectionManagerPtr		m_ptrConnMgr;

	MessageQueuePtr				m_pMsgQ;

	Coordinator();

	void run();
	void ProcessMessages(WMessagePtr pMsg);
	void HandleResponses(MessagePtr pMsg);
	void HandleRequests(MessagePtr pMsg);

	void HandlePeerOnline(MessagePtr pMsg);
	void HandlePeerOffline(MessagePtr pMsg);

	int ReadConfigFile(std::string configFile);

	std::vector<RemotePeers> GetPeerList(unsigned long& ts);

	// generates new peer id and see whether it does not already exists
	long CreateRandomPeerId();
	
public:
	static CoordinatorPtr GetCoordinator();

public:
	~Coordinator();
	
	void SetPort(int port) { m_port = port; }

	MessageQueuePtr GetMessageQueue() { return m_pMsgQ; }

	void Start(std::string configFile);
	void Kill();
	// handles the new connection request from remote peers
	void HandleConnectionRequest(ConnectionPtr pConn);
};

#endif