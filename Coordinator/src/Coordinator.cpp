#include "Coordinator.h"

CoordinatorPtr Coordinator::m_ptrCoordinator;

Coordinator::Coordinator()
{
	m_port = 0;
	m_pMsgQ = MessageQueuePtr(new MessageQueue);	
	m_db = DatabasePtr(new Database);
	m_db->Init();
	m_lastRandomPeerId = 0;
}

Coordinator::~Coordinator()
{

}

CoordinatorPtr Coordinator::GetCoordinator()
{
	if( m_ptrCoordinator == NULL ) {
		CoordinatorPtr ptr(new Coordinator());
		m_ptrCoordinator = ptr;
	}

	return m_ptrCoordinator;
}

void Coordinator::Start(std::string configFile)
{
	int result = this->ReadConfigFile(configFile);

	if(result < 0) {
		Log(ERR, L"Config File Error!!\n");
		return;
	}

	// start the connection manager
	m_ptrConnMgr = ConnectionManager::GetConnectionManager();
	m_ptrConnMgr->SetListenPort(m_port);
	m_ptrConnMgr->SetCoordinator(shared_from_this());
	m_ptrConnMgr->Start();
	
	m_coordThread = boost::thread(&Coordinator::run, this);
}

int Coordinator::ReadConfigFile(std::string configFile)
{
	int ret = 0;

	pugi::xml_document doc;
	pugi::xml_parse_result result = doc.load_file(configFile.c_str());

	if( result.status != pugi::status_ok ) {
		Log(ERR, L"Startup Configuration file error\n");
		return -1;
	}

	pugi::xml_node root = doc.child("Root");
	pugi::xml_node node = root.child("Coordinator");
	if( node == NULL )
		return -1;

	pugi::xml_node cnode = node;
	cnode = node.child("port");
	if( cnode == NULL )
		return -1;

	std::string port = cnode.child_value();

//	pCoordNode->SetIpAddr(ip);
	this->SetPort(atoi(port.c_str()));
 
	return result;	
}

void Coordinator::Kill()
{
	if(m_ptrConnMgr != NULL)
		m_ptrConnMgr->Kill();
	m_coordThread.interrupt();
}

void Coordinator::ProcessMessages(WMessagePtr pWMsg)
{
	MessagePtr pMsg = pWMsg->GetMsg();
	if( pWMsg->GetSysMsg() == NONE && pMsg != NULL) {
	Log(CONSOLE, L"Message Processing fro peer : %d, type : %d\n", pMsg->m_fromPeerId, pMsg->m_mesgType);
	if( pMsg->m_mesgType == REQ )
		this->HandleRequests(pMsg);
	else if( pMsg->m_mesgType == RESP )
		this->HandleResponses(pMsg);
	} else {
		//[TODO] handle the sys messages
	}
}

void Coordinator::HandleResponses(MessagePtr pMsg)
{
}

void Coordinator::HandleRequests(MessagePtr pMsg)
{
	switch(pMsg->m_reqType) {
		case GET_ONLINE:
			HandlePeerOnline(pMsg);
			break;
		case GET_OFFLINE:
			HandlePeerOffline(pMsg);
			break;
		default:
			break;
	}
}

void Coordinator::HandlePeerOnline(MessagePtr pMsg)
{
	//[TODO] see if the peer is already in the table

	// get the new peer id (no generation now)
//	long peerId = this->CreateRandomPeerId();

	// save the new peer in the list
	RemotePeers peer;
	peer.peer_id = pMsg->m_fromPeerId;
	peer.ip = pMsg->m_conn->GetPeerNode()->GetIpAddr();
	peer.port = pMsg->m_listenPort;//pMsg->m_conn->GetPeerNode()->GetPort();

	m_db->AddPeer(peer);
	
	// give out the current peer list
	unsigned long ts = 0;
	std::vector<RemotePeers> peers = this->GetPeerList(ts);
	
	ConnectionPtr pConn = pMsg->m_conn;
	if( pConn == NULL ) {
		Log(ERR, L"No connection on the request message\n");
		return;
	}

	PeerNodePtr pNode = pConn->GetPeerNode();
	if( pNode == NULL ) {
		Log(ERR, L"No PeerNode associated with the request message\n");
		return;
	}

	Log(CONSOLE, L"Peer: %d ONLINE\n", pMsg->m_fromPeerId);
	pNode->RespondOnline(/*peerId*/0, peers, ts);
}

void Coordinator::HandlePeerOffline(MessagePtr pMsg)
{
	// save the new peer in the list
	RemotePeers peer;
	peer.peer_id = pMsg->m_fromPeerId;
	peer.ip = pMsg->m_conn->GetPeerNode()->GetIpAddr();
	peer.port = pMsg->m_listenPort;//pMsg->m_conn->GetPeerNode()->GetPort();

	Log(CONSOLE, L"Peer: %d OFFLINE\n", pMsg->m_fromPeerId);
	m_db->RemovePeer(peer);
}

// process message queue
void Coordinator::run()
{
	while (true) {
		WMessagePtr pMesg = m_pMsgQ->GetMessage();
		// if empty wait for queue to get full
		if( pMesg == NULL ) {
			m_pMsgQ->WaitForFull();
			continue;
		}
		ProcessMessages(pMesg);
	}
}

void Coordinator::HandleConnectionRequest(ConnectionPtr pConn)
{
	// make the peer node
	PeerNodePtr pNode = PeerNode::CreatePeerNode(-1);
	pNode->HandleConnect(pConn);
}

long Coordinator::CreateRandomPeerId()
{
	return ++m_lastRandomPeerId;
}

std::vector<RemotePeers> Coordinator::GetPeerList(unsigned long& ts) 
{ 
	return m_db->GetPeerList(ts); 
}