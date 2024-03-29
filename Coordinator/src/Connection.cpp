#include "Connection.h"
#include "ConnectionManager.h"
#include "PeerNode.h"
#include "Coordinator.h"
#include "MessageQueue.h"
#include "Message.h"


Connection::Connection()
{
	m_remotePort = 0;
	m_recvbuf = (char*)malloc(DEFAULT_MSG_BUFLEN);
}

Connection::Connection(SOCKET sock)
{
	m_connSocket = sock;
	m_recvbuf = (char*)malloc(DEFAULT_MSG_BUFLEN);
}

Connection::~Connection()
{
	free (m_recvbuf);
}

void Connection::StartConnection()
{
	// get ip and port information
	SOCKADDR_IN sock;
	int sz = sizeof(sock);
	int result = getpeername(m_connSocket, (SOCKADDR *)&sock, &sz);
	m_remoteIpAddress = inet_ntoa(sock.sin_addr);
	m_remotePort = htons(sock.sin_port);

	Log(CONSOLE, L"Starting Connection, socket : %d\n", m_connSocket);
	m_connThread = boost::thread(&Connection::run, this);
}

int Connection::Connect( std::string ip, int port )
{
	boost::recursive_mutex::scoped_lock L(m_mutex);

	int result = 0;
	if( ip.empty() || port <= 0 ) return -1;

	SOCKET ConnectSocket = INVALID_SOCKET;
    struct sockaddr_in clientService; 

	// Create a SOCKET for connecting to server
    ConnectSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (ConnectSocket == INVALID_SOCKET) {
        Log(ERR, L"Error at socket(): %ld\n", WSAGetLastError() );
        return -1;
    }

    // The sockaddr_in structure specifies the address family,
    // IP address, and port of the server to be connected to.
    clientService.sin_family = AF_INET;
	clientService.sin_addr.s_addr = inet_addr( ip.c_str() );
    clientService.sin_port = htons( port );

    // Connect to server.
    result = connect( ConnectSocket, (SOCKADDR*) &clientService, sizeof(clientService) );
    if ( result == SOCKET_ERROR) {
        closesocket (ConnectSocket);
        Log(ERR, L"Unable to connect to remote peer: %ld\n", WSAGetLastError());
        return -1;
    }

	// connected
	m_connSocket = ConnectSocket;
	m_remoteIpAddress = ip;
	m_remotePort = port;

	return result;
}

int Connection::SendMessage(std::string mesg)
{
	int result = 0;

	result = send( m_connSocket, mesg.c_str(), (int)mesg.size(), 0 );
    if (result == SOCKET_ERROR) {
        Log(ERR, L"Unable to send message to remote peer: %ld\n", WSAGetLastError());
        return -1;
    }
	return result;
}

void Connection::run()
{
	MsgListener();
}

void Connection::MsgListener()
{
	while(true) {
		int iResult = recv(m_connSocket, m_recvbuf, DEFAULT_MSG_BUFLEN, 0);
		m_recvbuf[iResult] = NULL;
		if( iResult > 0 ) {
			Log(CONSOLE, L"Received %d bytes on socket: %d\n", iResult, m_connSocket, m_recvbuf);
		} else if( iResult == 0 ) {
			Log(CONSOLE, L"Connection on socket: %d closed\n", m_connSocket);
			break;
		} else {
			Log(ERR, L"Connection on socket: %d closed - unexpectedly, error - %d\n", m_connSocket, WSAGetLastError());
			break;
		}

		this->ProcessMessages(m_recvbuf);
	}

}

void Connection::ProcessMessages(std::string mesg)
{
	// parse message
	MessagePtr pMsg = Message::ParseMessage(mesg);
	if( pMsg == NULL ) {
		Log(ERR, L"Message : %s could not be parsed\n", mesg.c_str());
		return;
	}

	pMsg->m_conn = shared_from_this();		// set to 'this'

	// send to the peer node to which this connection belongs
	if( m_pPeerNode == NULL ) {
		Log(ERR, L"Connection:: ProcessMessages - PeerNode found NULL\n");
		return;
	}
	m_pPeerNode->ReceiveMessage(pMsg);
	// put the message in to the queue
/*	ConnectionManagerPtr pConnMgr = ConnectionManager::GetConnectionManager();
	if( pConnMgr == NULL ) return;

	CoordinatorPtr pCoordinator = pConnMgr->GetCoordinator();
	if( pCoordinator == NULL ) return;

	MessageQueuePtr pMsgQ = pCoordinator->GetMessageQueue();
	if( pMsgQ == NULL ) return;

	pMsgQ->PutMessage(pMsg);
*/
}

void Connection::Start()
{
	m_connThread = boost::thread(&Connection::run, this);
}

void Connection::Kill()
{
	Log(CONSOLE, L"Killing Connection thread, socket : %d\n", m_connSocket);
	m_connThread.interrupt();
}