#ifndef PEER_H
#define PEER_H

#include "PeerNode.h"

#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>
#include <boost/enable_shared_from_this.hpp>

#include <pugixml.hpp>

#include <string>
#include "MessageQueue.h"
#include "ConnectionManager.h"
#include "Database.h"

class Peer;

typedef boost::shared_ptr<Peer> PeerPtr;

class Peer : public boost::enable_shared_from_this<Peer> {
private:
	static PeerPtr				m_ptrPeer;
	std::string					m_ipAddress;
	int							m_port;
	DatabasePtr					m_db;		// stores config infos
	boost::thread				m_peerThread;

	ConnectionManagerPtr		m_ptrConnMgr;

	MessageQueuePtr				m_pMsgQ;

	PeerNodePtr					m_pCoordinatorNode;

	Peer();
	void run();
	void ProcessMessages(MessagePtr pMsg);

	int ReadConfigFile(std::string configFile, PeerNodePtr pCoordNode);
	
public:
	static PeerPtr GetPeer();

public:
	~Peer();

	void SetPort(int port) { m_port = port; }

	MessageQueuePtr GetMessageQueue() { return m_pMsgQ; }

	void Start(std::string configFile);
	void Kill();

	// handles the new connection request from remote peers
	void HandleConnectionRequest(ConnectionPtr pConn);

	// test methods
	void TestSendMesgs(std::string msg);
};

#endif