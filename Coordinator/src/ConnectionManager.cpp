#include "ConnectionManager.h"
#include "Coordinator.h"
#include "Log.h"

ConnectionManagerPtr ConnectionManager::m_ptrConnMgr;

ConnectionManager::ConnectionManager()
{
	// Initialize Winsock
    WSADATA wsaData;
    int iResult = 0;
	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);

	if (iResult != NO_ERROR) {
		Log(ERR, L"WSAStartup() failed with error: %d\n", iResult);
    }

	m_ipAddress		= "127.0.0.1";
	m_listenPort	= 0;

}

ConnectionManager::~ConnectionManager()
{
	CleanUp();
}

ConnectionManagerPtr ConnectionManager::GetConnectionManager()
{
	if( m_ptrConnMgr == NULL ) {
		ConnectionManagerPtr ptr(new ConnectionManager());
		m_ptrConnMgr = ptr;
	}

	return m_ptrConnMgr;
}

void ConnectionManager::Start()
{
	Log(CONSOLE, L"Starting Connection Manager\n");
	m_connListenThread = boost::thread(&ConnectionManager::run, this);
}

void ConnectionManager::Kill()
{
	Log(CONSOLE, L"Killing Connection Manager\n");
	m_connListenThread.interrupt();
}

ConnectionPtr ConnectionManager::Connect(std::string ip, int port)
{
	ConnectionPtr pConn(new Connection);
	int result = pConn->Connect(ip, port);
	if( result < 0 ) {
		//pConn.
		return ConnectionPtr();	// returns NULL
	}
	// [TODO] store the connection here too
	return pConn;
}

void ConnectionManager::CleanUp()
{
	shutdown(m_listenSocket, SD_BOTH);
	closesocket(m_listenSocket);
    WSACleanup();
}

// connection listen
void ConnectionManager::run()
{
	this->ListenConnection();
}

int ConnectionManager::ListenConnection()
{
	int iResult = 0;

	// Create a SOCKET for listening for incoming connection requests.
	m_listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (m_listenSocket == INVALID_SOCKET) {
		Log(ERR,L"socket function failed with error: %ld\n", WSAGetLastError());
		WSACleanup();
		return -1;
	}

	// The sockaddr_in structure specifies the address family,
	// IP address, and port for the socket that is being bound.
	sockaddr_in service;
	service.sin_family = AF_INET;
	service.sin_addr.s_addr = inet_addr(m_ipAddress.c_str());
	service.sin_port = htons(m_listenPort);

	iResult = bind(m_listenSocket, (SOCKADDR *) & service, sizeof (service));
	if (iResult == SOCKET_ERROR) {
		Log(ERR,L"bind function failed with error %d\n", WSAGetLastError());
		iResult = closesocket(m_listenSocket);
		if (iResult == SOCKET_ERROR)
			Log(ERR,L"closesocket function failed with error %d\n", WSAGetLastError());
		WSACleanup();
		return 1;
	}

	// Listen for incoming connection requests 
	// on the created socket
	if (listen(m_listenSocket, CONN_LISTEN_BACKLOG) == SOCKET_ERROR)
		Log(ERR,L"listen function failed with error: %d\n", WSAGetLastError());

	// Create a socket for accepting incoming requests.
	while(true) {
		SOCKET AcceptSocket;

		// Accept the connection.
		AcceptSocket = accept(m_listenSocket, NULL, NULL);
		if (AcceptSocket == INVALID_SOCKET) {
			Log(ERR,L"accept failed with error: %ld\n", WSAGetLastError());
			closesocket(m_listenSocket);
			return -1;
		}

		// create a new thread to get the msgs from the accepted connection
		this->MakeConnection(AcceptSocket);
	}

    // No longer need server socket
    this->CleanUp();
}

int ConnectionManager::MakeConnection(SOCKET peerSocket)
{
	ConnectionPtr pconn(new Connection(peerSocket));
	//pconn->StartConnection();
	m_pCoordinator->HandleConnectionRequest(pconn);
	return 0;
}