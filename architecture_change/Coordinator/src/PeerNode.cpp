#include "PeerNode.h"


PeerNode::PeerNode()
{
	m_bConnected = false;
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
	m_bConnected = true;
	return result;
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