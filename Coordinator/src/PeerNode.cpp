#include "PeerNode.h"
#include "MessageQueue.h"


PeerNode::PeerNode()
{
	m_bConnected = false;
	m_listenPort = 0;
}

PeerNodePtr PeerNode::CreatePeerNode(int peerId, std::string ipAddress, int port)
{
	PeerNodePtr pNode(new PeerNode);
	pNode->SetPort(port);
	pNode->SetListenPort(port);
	pNode->SetIpAddr(ipAddress);
	pNode->SetPeerId(peerId);

	return pNode;
}

int PeerNode::Connect()
{
	int result = 0;
	boost::recursive_mutex::scoped_lock L(m_mutex);

	// if already connected then return
	if( m_bConnected == true ) 
		return 0;
	
	// else connects
	// makes connection via connection manager interface
	ConnectionManagerPtr pConnMgr = ConnectionManager::GetConnectionManager();
	ConnectionPtr pConn = pConnMgr->Connect(m_ipAddress, m_listenPort);
	if( pConn == NULL ) {
		Log(ERR, L"Could not make connection to Peer Node with ip - %s, port - %d\n", m_ipAddress, m_port);
		return -1;
	}

	m_pConn = pConn;
	m_pConn->SetPeerNode(shared_from_this());
	m_pConn->Start();
	m_bConnected = true;
	return result;
}

int PeerNode::HandleConnect(ConnectionPtr pConn)
{
	if( pConn == NULL ) return -1;

	m_pConn = pConn;
	m_bConnected = true;
	m_pConn->SetPeerNode(shared_from_this());
	m_pConn->StartConnection();

	m_ipAddress = m_pConn->GetRemoteIpAddr();
	m_port = m_pConn->GetRemotePort();

	return 0;
}

int PeerNode::RespondOnline(int newPeerId, std::vector<RemotePeers>& peers, unsigned long ts)
{
	int result = 0;
	ResponseMessagePtr pResp(new ResponseMessage);
	
	// [NOTE] not using newpeerid anymore
	pResp->AddGeneralHeader((RESP_TYPE)GET_ONLINE_RESP, 0/*newPeerId*/, true);
	pResp->AddPeerListTimestamp(ts);
	pResp->AddPeerList(peers);

	std::string xml = pResp->GetXML();
	result = this->SendMessage(xml);
	return result;
}

void PeerNode::ReceiveMessage(MessagePtr pMsg)
{
	if( pMsg == NULL ) return;

	this->SetPeerId(pMsg->m_fromPeerId);
	//this->SetListenPort(pMsg->m_listenPort);

	//[TODO] get task id and send the message to correct task instance

	// put the message in to the main queue
	ConnectionManagerPtr pConnMgr = ConnectionManager::GetConnectionManager();
	if( pConnMgr == NULL ) return;

	CoordinatorPtr pCoord = pConnMgr->GetCoordinator();
	if( pCoord == NULL ) return;

	MessageQueuePtr pMsgQ = pCoord->GetMessageQueue();
	if( pMsgQ == NULL ) return;

	WMessagePtr pWMsg(new WMessage);
	pWMsg->SetMsg(pMsg);
	pWMsg->SetSysMsg(NONE);
	pMsgQ->PutMessage(pWMsg);
}

int PeerNode::SendMessage(std::string mesg)
{
	int result = 0;
	// if not connected then try to connect
	if( m_bConnected == false ) {
		result = this->Connect();
		if( result < 0 ) {
			Log(ERR, L"Message not sent to Peer with ip - %s, port - %d (Could not Connect)\n",
																				m_ipAddress, m_port);
			return result;
		}
	}

	// now send the message
	if( m_pConn == NULL ) {
		Log(ERR, L"Unavailable Connection to Peer with ip - %s, port - %d (Could not Connect)\n",
																					m_ipAddress, m_port);
		return -1;
	}

	result = m_pConn->SendMessage(mesg);
	return result;
}