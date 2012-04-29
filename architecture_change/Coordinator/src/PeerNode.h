#ifndef PEER_NODE_H
#define PEER_NODE_H

#include "ConnectionManager.h"
#include "Connection.h"
#include "Log.h"

#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>
#include <string>

class PeerNode;

typedef boost::shared_ptr<PeerNode> PeerNodePtr;

class PeerNode {
private:
	std::string					m_ipAddress;
	int							m_port;
	bool						m_bConnected;
	ConnectionPtr				m_pConn;

	boost::mutex				m_mutex;

public:
	PeerNode();
	~PeerNode() {}

	void SetIpAddr(std::string ip) { m_ipAddress = ip; }
	void SetPort(int port) { m_port = port; }

	int Connect();

	int SendMessage(std::string mesg); // sends the message to the peer

	static PeerNodePtr CreatePeerNode(std::string ipAddress = "127.0.0.1", int port = 0);
};

#endif