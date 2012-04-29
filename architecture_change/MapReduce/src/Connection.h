#ifndef CONNECTION_H
#define CONNECTION_H

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include "Log.h"

#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>
#include <boost/enable_shared_from_this.hpp>

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>

#include <stdlib.h>

#define DEFAULT_MSG_BUFLEN	1000

class Connection;
class PeerNode;

typedef boost::shared_ptr<PeerNode> PeerNodePtr;
typedef boost::shared_ptr<Connection> ConnectionPtr;

class Connection : public boost::enable_shared_from_this<Connection> {
	boost::thread				m_connThread;
	std::string					m_remoteIpAddress;
	int							m_remotePort;
	SOCKET						m_connSocket;
	char*						m_recvbuf;
	PeerNodePtr					m_pPeerNode;

	boost::mutex				m_mutex;

public:
	Connection(SOCKET sock);
	Connection();
	~Connection();

	boost::shared_ptr<Connection> f()
    {
        return shared_from_this();
    }

	void SetPeerNode(PeerNodePtr pNode) { m_pPeerNode = pNode; }
	std::string GetRemoteIpAddr() { return m_remoteIpAddress; }
	int GetRemotePort() { return m_remotePort; }

	void Start();		// thread start and kill operations
	void Kill();

	void StartConnection();	// starts the connection thread
	int Connect( std::string ip, int port );	// creates the connection for the peer

	int SendMessage(std::string mesg);

private:
	void run();
	void MsgListener();		// recvs messages over the connection
	void ProcessMessages(std::string mesg);
};

#endif