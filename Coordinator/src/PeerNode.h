#ifndef PEER_NODE_H
#define PEER_NODE_H

#include "ConnectionManager.h"
#include "Connection.h"
#include "Message.h"
#include "Coordinator.h"
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
	int							m_port;			// remote port of remote peer
	int							m_listenPort;	// listen port of remote peer
	bool						m_bConnected;
	ConnectionPtr				m_pConn;
	int							m_peerId;

	boost::recursive_mutex				m_mutex;

public:
	PeerNode();
	~PeerNode() {}

	boost::shared_ptr<PeerNode> f()
    {
        return shared_from_this();
    }

	void SetIpAddr(std::string ip) { m_ipAddress = ip; }
	std::string GetIpAddr() { return m_ipAddress; }
	void SetPort(int port) { m_port = port; }
	int GetPort() { return m_port; }
	void SetListenPort(int port) { m_listenPort = port; }
	int GetListenPort() { return m_listenPort; }
	void SetPeerId(int pId) { m_peerId = pId; }
	int GetPeerId() { return m_peerId; }

	int Connect();			// makes the new connection to remote peers
	int HandleConnect(ConnectionPtr pConn);	// handles the connection requests

	int RespondOnline(int newPeerId, std::vector<RemotePeers>& peers, unsigned long ts);

	void ReceiveMessage(MessagePtr msg);	// receives message from the connection
	int SendMessage(std::string mesg); // sends the message to the peer

	static PeerNodePtr CreatePeerNode(int peerId, std::string ipAddress = "127.0.0.1", int port = 0);
};

#endif