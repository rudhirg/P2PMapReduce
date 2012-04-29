#ifndef PEER_NODE_H
#define PEER_NODE_H

#include "ConnectionManager.h"
#include "Connection.h"
#include "Message.h"
#include "Peer.h"
#include "Log.h"

#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>
#include <boost/enable_shared_from_this.hpp>

#include <string>

class PeerNode;

typedef boost::shared_ptr<PeerNode> PeerNodePtr;

class PeerNode : public boost::enable_shared_from_this<PeerNode> {
private:
	std::string					m_ipAddress;
	int							m_port;
	bool						m_bConnected;
	bool						m_bIsCoordinatorNode;
	ConnectionPtr				m_pConn;

	boost::mutex				m_mutex;

public:
	PeerNode();
	~PeerNode() {}

	boost::shared_ptr<PeerNode> f()
    {
        return shared_from_this();
    }

	bool IsCoordinatorNode() { return m_bIsCoordinatorNode; }
	void SetCoordinatorNode() { m_bIsCoordinatorNode = true; }
	void SetIpAddr(std::string ip) { m_ipAddress = ip; }
	void SetPort(int port) { m_port = port; }

	int Connect();			// makes the new connection to remote peers
	int HandleConnect(ConnectionPtr pConn);	// handles the connection requests

	void ReceiveMessage(MessagePtr msg);	// receives message from the connection
	int SendMessage(std::string mesg); // sends the message to the peer
	// startup workflow - send 'Online' Message to coordinator
	int SendOnlineMessage();

	static PeerNodePtr CreatePeerNode(std::string ipAddress = "127.0.0.1", int port = 0);
};

#endif