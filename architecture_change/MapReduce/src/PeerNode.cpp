#include "PeerNode.h"
#include "MessageQueue.h"


PeerNode::PeerNode()
{
	m_bConnected			= false;
	m_bIsCoordinatorNode	= false;
}

PeerNodePtr PeerNode::CreatePeerNode(std::string ipAddress, int port)
{
	PeerNodePtr pNode(new PeerNode);
	pNode->SetPort(port);
	pNode->SetIpAddr(ipAddress);

	return pNode;
}

int PeerNode::Connect()
{
	int result = 0;
	boost::mutex::scoped_lock L(m_mutex);

	// if already connected then return
	if( m_bConnected == true ) 
		return 0;
	
	// else connects
	// makes connection via connection manager interface
	ConnectionManagerPtr pConnMgr = ConnectionManager::GetConnectionManager();
	ConnectionPtr pConn = pConnMgr->Connect(m_ipAddress, m_port);
	if( pConn == NULL ) {
		Log(ERROR, L"Could not make connection to Peer Node with ip - %s, port - %d\n", m_ipAddress, m_port);
		return -1;
	}

	m_pConn = pConn;
	m_pConn->SetPeerNode(shared_from_this());
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

void PeerNode::ReceiveMessage(MessagePtr pMsg)
{
	if( pMsg == NULL ) return;

	// put the message in to the queue
	ConnectionManagerPtr pConnMgr = ConnectionManager::GetConnectionManager();
	if( pConnMgr == NULL ) return;

	PeerPtr pPeer = pConnMgr->GetPeer();
	if( pPeer == NULL ) return;

	MessageQueuePtr pMsgQ = pPeer->GetMessageQueue();
	if( pMsgQ == NULL ) return;

	pMsgQ->PutMessage(pMsg);
}

int PeerNode::SendMessage(std::string mesg)
{
	int result = 0;
	// if not connected then try to connect
	if( m_bConnected == false ) {
		result = this->Connect();
		if( result < 0 ) {
			Log(ERROR, L"Message not sent to Peer with ip - %s, port - %d (Could not Connect)\n",
																				m_ipAddress, m_port);
			return result;
		}
	}

	// now send the message
	if( m_pConn == NULL ) {
		Log(ERROR, L"Unavailable Connection to Peer with ip - %s, port - %d (Could not Connect)\n",
																					m_ipAddress, m_port);
		return -1;
	}

	result = m_pConn->SendMessage(mesg);
	return result;
}

int PeerNode::SendOnlineMessage()
{
	int result = 0;
	RequestMessagePtr pReq(new RequestMessage);
	pReq->AddGeneralHeader((REQ_TYPE)GET_ONLINE, 0);
	std::string xml = pReq->GetXML();
	result = this->SendMessage(xml);

	return result;
}