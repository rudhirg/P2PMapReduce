#include "Peer.h"

PeerPtr Peer::m_ptrPeer;

Peer::Peer()
{
	m_port = 0;
	m_pMsgQ = MessageQueuePtr(new MessageQueue);	
	m_db = DatabasePtr(new Database);
}

Peer::~Peer()
{
	
}

PeerPtr Peer::GetPeer()
{
	if( m_ptrPeer == NULL ) {
		PeerPtr ptr(new Peer());
		m_ptrPeer = ptr;
	}

	return m_ptrPeer;
}

void Peer::Start(std::string configFile)
{
	// Parse startup configuration and create coordinator node
	PeerNodePtr pCoordNode = PeerNode::CreatePeerNode();
	int result = this->ReadConfigFile(configFile, pCoordNode);

	if(result < 0) {
		Log(ERROR, L"Config File Error!!\n");
		return;
	}

	// start the connection manager
	m_ptrConnMgr = ConnectionManager::GetConnectionManager();
	m_ptrConnMgr->SetListenPort(m_port);
	m_ptrConnMgr->SetPeer(shared_from_this());
	m_ptrConnMgr->Start();
	
	m_peerThread = boost::thread(&Peer::run, this);

	// contact coordinator
	m_pCoordinatorNode = pCoordNode;
	m_pCoordinatorNode->SetCoordinatorNode();
	m_pCoordinatorNode->Connect();
	m_pCoordinatorNode->SendOnlineMessage();
}

int Peer::ReadConfigFile(std::string configFile, PeerNodePtr pCoordNode)
{
	int ret = 0;

	pugi::xml_document doc;
	pugi::xml_parse_result result = doc.load_file(configFile.c_str());

	if( result.status != pugi::status_ok ) {
		Log(ERROR, L"Startup Configuration file error\n");
		return -1;
	}

	pugi::xml_node root = doc.child("Root");
	pugi::xml_node node = root.child("Coordinator");
	if( node == NULL )
		return -1;

	pugi::xml_node cnode = node;
	cnode = node.child("ipaddress");
	if( cnode == NULL )
		return -1;

	std::string ip = cnode.child_value();
	
	cnode = node.child("port");
	if( cnode == NULL )
		return -1;

	std::string port = cnode.child_value();

	pCoordNode->SetIpAddr(ip);
	pCoordNode->SetPort(atoi(port.c_str()));

	// get me port
	node = root.child("Me");
	if( node == NULL )
		return -1;

	cnode = node;
	cnode = node.child("port");
	if( cnode == NULL )
		return -1;

	port = cnode.child_value();
	this->SetPort(atoi(port.c_str()));	

	// set the config info in db
	m_db->SetCoordinatorIp(ip);
	m_db->SetCoordinatorPort(atoi(port.c_str()));
 
	return result;	
}

void Peer::Kill()
{
	if(m_ptrConnMgr != NULL)
		m_ptrConnMgr->Kill();
	m_peerThread.interrupt();
}

void Peer::ProcessMessages(MessagePtr pMsg)
{
	Log(CONSOLE, L"Message Processed %s\n", pMsg->m_mesg);
}

// process message queue
void Peer::run()
{
	while (true) {
		MessagePtr pMesg = m_pMsgQ->GetMessage();
		// if empty wait for queue to get full
		if( pMesg == NULL ) {
			m_pMsgQ->WaitForFull();
			continue;
		}
		
	}
}

void Peer::HandleConnectionRequest(ConnectionPtr pConn)
{
	// make the peer node
	PeerNodePtr pNode = PeerNode::CreatePeerNode();
	pNode->HandleConnect(pConn);
}

// Test Methods
void Peer::TestSendMesgs(std::string msg)
{
	RequestMessagePtr pReq(new RequestMessage);
	pReq->AddGeneralHeader((REQ_TYPE)0, 1);
	std::string xml = pReq->GetXML();
	m_pCoordinatorNode->SendMessage(xml);
}