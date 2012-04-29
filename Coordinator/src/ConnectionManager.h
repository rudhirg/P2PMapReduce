#ifndef CONNECTION_MGR_H
#define CONNECTION_MGR_H

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>

#include "Connection.h"
#include "Log.h"

#define CONN_LISTEN_BACKLOG 5

class ConnectionManager;
class Coordinator;

typedef boost::shared_ptr<Coordinator> CoordinatorPtr;
typedef boost::shared_ptr<ConnectionManager> ConnectionManagerPtr;

// Singleton Class
class ConnectionManager {
	
private:
	static ConnectionManagerPtr		m_ptrConnMgr;
	CoordinatorPtr					m_pCoordinator;
	boost::thread					m_connListenThread;

	SOCKET							m_listenSocket;
	std::string						m_ipAddress;
	int								m_listenPort;

	std::vector<ConnectionPtr>		m_connectionList;	// for temp now
	boost::recursive_mutex					m_mutex;

	ConnectionManager();

	void run();
	int ListenConnection();
	int MakeConnection(SOCKET peerSocket);		// makes a new connection

	void CleanUp();

public:
	static ConnectionManagerPtr GetConnectionManager();

public:
	~ConnectionManager();

	void SetListenPort(int port) { m_listenPort = port; }
	void SetCoordinator(CoordinatorPtr Coordinator) { m_pCoordinator = Coordinator; }
	CoordinatorPtr GetCoordinator() { return m_pCoordinator; }

	void Start();
	void Kill();

	ConnectionPtr Connect(std::string ip, int port);			// creates a new connection for the Coordinator

};

#endif