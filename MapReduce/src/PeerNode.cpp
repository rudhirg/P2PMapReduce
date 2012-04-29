#include "PeerNode.h"
#include "MessageQueue.h"
#include <boost/lexical_cast.hpp>


PeerNode::PeerNode()
{
	m_peerId				= -1;
	m_bConnected			= false;
	m_bIsCoordinatorNode	= false;
	m_listenPort 			= 0;
	m_paramIndx				= -1;

	m_pTimerMessages		= TimerMessagePtr(new TimerMessage());
}

PeerNodePtr PeerNode::CreatePeerNode(int peerId, std::string ipAddress, int port)
{
	PeerNodePtr pNode(new PeerNode);
	pNode->SetPort(port);
	pNode->SetListenPort(port);
	pNode->SetIpAddr(ipAddress);
	pNode->SetPeerId(peerId);
	pNode->Init();

	PeerPtr pPeer = Peer::GetPeer();
	pPeer->AddPeerNode(pNode);

	return pNode;
}

PeerNodePtr PeerNode::CreatePeerNode(RemotePeers peer)
{
	Log(CONSOLE, L"Creating Peer Node: IP - %s, Port - %d, PeerId - %d\n", peer.ip.c_str(), peer.port, peer.peer_id);
	PeerNodePtr pPeer = CreatePeerNode(peer.peer_id, peer.ip, peer.port);
	pPeer->SetPeerId(peer.peer_id);
	return pPeer;
}

void PeerNode::DestroyPeerNode(PeerNodePtr pNode)
{
	if( pNode == NULL ) return;

	Log(CONSOLE, L"Destroying Peer Node: IP - %s, Port - %d, PeerId - %d\n", 
						pNode->GetIpAddr().c_str(), pNode->GetPort(), pNode->GetPeerId());
	pNode->DisConnect();
	PeerPtr pPeer = Peer::GetPeer();
	pPeer->RemovePeerNode(pNode);
}

void PeerNode::Init()
{
	m_pTimerMessages->Init(shared_from_this());
}

void PeerNode::MessageEvent(SYS_MSG msg)
{

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
	ConnectionPtr pConn = m_pConn;
	if(pConn == NULL) {
		pConn = pConnMgr->Connect(m_ipAddress, m_listenPort);
		if( pConn == NULL ) {
			Log(ERR, L"Could not make connection to Peer Node with ip - %s, port - %d\n", m_ipAddress, m_port);
			if( this->IsCoordinatorNode() ) {
				// try connecting to backups
				pConn = ConnectBackups();
			}
			if( pConn == NULL ) {
				return -1;
			}
		}
	} else {
		pConn->Connect(m_ipAddress, m_listenPort);
	}

	//m_pConn.reset();
	m_pConn = pConn;
	m_pConn->SetPeerNode(shared_from_this());
	m_pConn->Start();
	m_bConnected = true;
	return result;
}

ConnectionPtr PeerNode::ConnectBackups()
{
	boost::recursive_mutex::scoped_lock L(m_mutex);
	ConnectionManagerPtr pConnMgr = ConnectionManager::GetConnectionManager();
	ConnectionPtr pConn = ConnectionPtr();
	for(int i = 0; i < m_backups.size(); i++) {
		pConn = pConnMgr->Connect(m_backups[i].m_ipAddress, m_backups[i].m_port);
		if( pConn == NULL ) {
			Log(CONSOLE, L"BackUp Coordinator (port: %d), Not UP!!\n", m_backups[i].m_port);
			continue;
		} else {
			Log(CONSOLE, L"BackUp Coordinator (port: %d), Working!!\n", m_backups[i].m_port);
			this->m_ipAddress = m_backups[i].m_ipAddress;
			this->m_listenPort = m_backups[i].m_port;
			break;
		}
	}
	return pConn;
}

void PeerNode::HandleConnectionBroken()
{
	m_bConnected = false;
	ConnectionManagerPtr pConnMgr = ConnectionManager::GetConnectionManager();
	if( pConnMgr == NULL ) return;

	PeerPtr pPeer = pConnMgr->GetPeer();
	if( pPeer == NULL ) return;

	// send the message to the main peer
	MessageQueuePtr pMsgQ = pPeer->GetMessageQueue();
	if( pMsgQ == NULL ) return;

	WMessagePtr pWMsg(new WMessage);
	pWMsg->SetPeerId(this->m_peerId);
	pWMsg->SetSysMsg(CONNECTION_FAILURE_ON_PEER);
	MessagePtr pMsg(new Message);
	pMsg->m_conn = this->m_pConn;
	pMsg->m_mesgType = INTERNAL;
	pWMsg->SetMsg(pMsg);
	pMsgQ->PutMessage(pWMsg);
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

int PeerNode::DisConnect()
{
	if( m_bConnected == false )
		return 0;

	Log(CONSOLE, L"Disconnecting Peer Node: IP - %s, Port - %d, PeerId - %d\n", 
						this->GetIpAddr().c_str(), this->GetPort(), this->GetPeerId());
	// close connection
	m_bConnected = false;
	if( m_pConn == NULL ) return 0;
	m_pConn->DisConnect();

	return 0;
}

void PeerNode::ReceiveMessage(MessagePtr pMsg)
{
	if( pMsg == NULL ) return;

	this->SetPeerId(pMsg->m_fromPeerId);
	this->SetListenPort(pMsg->m_listenPort);

	//[TODO] get task id and send the message to correct task instance

	// put the message in to the main queue
	/*if( pMsg->m_taskId == 0  || pMsg->m_respType == GET_ONLINE_RESP )*/ {
		ConnectionManagerPtr pConnMgr = ConnectionManager::GetConnectionManager();
		if( pConnMgr == NULL ) return;

		PeerPtr pPeer = pConnMgr->GetPeer();
		if( pPeer == NULL ) return;

		MessageQueuePtr pMsgQ = pPeer->GetMessageQueue();
		if( pMsgQ == NULL ) return;

		WMessagePtr pWMsg(new WMessage);
		pWMsg->SetMsg(pMsg);
		pWMsg->SetSysMsg(NONE);
		pMsgQ->PutMessage(pWMsg);
	}
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

// this message is the first mesg to coordiantor
int PeerNode::SendOnlineMessage()
{
	int result = 0;

	Log(CONSOLE, L"Sending PEER ONLINE\n");
	int fromPeerId = Peer::GetPeer()->GetPeerId();
	int listenPort = Peer::GetPeer()->GetPort();
	//m_listenPort = listenPort;
	RequestMessagePtr pReq(new RequestMessage);
	
	pReq->AddGeneralHeader((REQ_TYPE)GET_ONLINE, fromPeerId, listenPort);
	std::string xml = pReq->GetXML();
	result = this->SendMessage(xml);

	return result;
}

int PeerNode::SendOfflineMessage()
{
	int result = 0;

	Log(CONSOLE, L"Sending PEER OFFLINE\n");
	int fromPeerId = Peer::GetPeer()->GetPeerId();
	int listenPort = Peer::GetPeer()->GetPort();
	RequestMessagePtr pReq(new RequestMessage);
	
	pReq->AddGeneralHeader((REQ_TYPE)GET_OFFLINE, fromPeerId, listenPort);
	std::string xml = pReq->GetXML();
	result = this->SendMessage(xml);

	return result;
}

int PeerNode::TaskRequest(int taskId, std::string param)
{
	int result = 0;

	Log(CONSOLE, L"Sending TASK REQUEST, id: %d to Peer: %d, with Parameters: %s\n", taskId, m_peerId,
		param.c_str());
	int fromPeerId = Peer::GetPeer()->GetPeerId();
	int listenPort = Peer::GetPeer()->GetPort();

	string tId = boost::lexical_cast<string>(taskId);
	string pId = boost::lexical_cast<string>(fromPeerId);

	RequestMessagePtr pReq(new RequestMessage);
	pReq->AddGeneralHeader((REQ_TYPE)TASK_REQUEST, fromPeerId, listenPort);

	MessageMap mmap;
	mmap["taskid"] = boost::lexical_cast<string>(tId);
	mmap["taskParam"] = param;

	pReq->AddTaskInfo(mmap);
	std::string xml = pReq->GetXML();
	result = this->SendMessage(xml);

	return result;
}

int PeerNode::TaskResponse(int taskId, bool status)
{
	int result = 0;
	ResponseMessagePtr pResp(new ResponseMessage);

	int fromPeerId = Peer::GetPeer()->GetPeerId();

	Log(CONSOLE, L"Sending TASK RESPONSE, status : %b, task-id: %d to Peer: %d\n", status, taskId, m_peerId);
	
	pResp->AddGeneralHeader((RESP_TYPE)TASK_REQUEST_RESP, fromPeerId, status);
	MessageMap mmap;
	mmap["taskid"] = boost::lexical_cast<string>(taskId);

	pResp->AddTaskInfo(mmap);

	// give out the current peer list
	unsigned long ts = 0;
	std::vector<RemotePeers> peers = Peer::GetPeer()->GetPeerList(ts);
	

	// add peerlist
	pResp->AddPeerListTimestamp(ts);
	pResp->AddPeerList(peers);

	// add work history
	WorkHistoryPtr pWH = WorkHistory::GetWorkHistory();
	MessageMap hmap;
	hmap["workload"] = boost::lexical_cast<string>(pWH->GetCurrentWorkLoad());	
	hmap["avgexecution"] = boost::lexical_cast<string>(pWH->GetAvgExecuteTime());
	hmap["totalexecuted"] = boost::lexical_cast<string>(pWH->GetNumTaskHandled());
	hmap["totalfailure"] = boost::lexical_cast<string>(pWH->GetNumTaskFailed());

	pResp->AddWorkHistory(hmap);

	std::string xml = pResp->GetXML();
	result = this->SendMessage(xml);
	return result;
}

int PeerNode::TaskExecute(int taskId)
{
	int result = 0;

	Log(CONSOLE, L"Sending TASK EXECUTE REQUEST, id: %d to Peer: %d\n", taskId, m_peerId);
	int fromPeerId = Peer::GetPeer()->GetPeerId();
	int listenPort = Peer::GetPeer()->GetPort();

	string tId = boost::lexical_cast<string>(taskId);
	string pId = boost::lexical_cast<string>(fromPeerId);

	RequestMessagePtr pReq(new RequestMessage);
	pReq->AddGeneralHeader((REQ_TYPE)TASK_EXECUTE, fromPeerId, listenPort);

	MessageMap mmap;
	mmap["taskid"] = boost::lexical_cast<string>(tId);

	pReq->AddTaskInfo(mmap);
	std::string xml = pReq->GetXML();
	result = this->SendMessage(xml);

	return result;
}

int PeerNode::TaskCancel(int taskId)
{
	int result = 0;

	Log(CONSOLE, L"Sending TASK CANCEL REQUEST, id: %d to Peer: %d\n", taskId, m_peerId);
	int fromPeerId = Peer::GetPeer()->GetPeerId();
	int listenPort = Peer::GetPeer()->GetPort();

	string tId = boost::lexical_cast<string>(taskId);
	string pId = boost::lexical_cast<string>(fromPeerId);

	RequestMessagePtr pReq(new RequestMessage);
	pReq->AddGeneralHeader((REQ_TYPE)TASK_CANCEL, fromPeerId, listenPort);

	MessageMap mmap;
	mmap["taskid"] = boost::lexical_cast<string>(tId);

	pReq->AddTaskInfo(mmap);
	std::string xml = pReq->GetXML();
	result = this->SendMessage(xml);

	return result;
}

int PeerNode::TaskExecuteResponse(int taskId, bool status)
{
	int result = 0;
	ResponseMessagePtr pResp(new ResponseMessage);

	int fromPeerId = Peer::GetPeer()->GetPeerId();

	Log(CONSOLE, L"Sending TASK EXECUTE RESPONSE, status : %b, task-id: %d to Peer: %d\n", status, taskId, m_peerId);
	
	pResp->AddGeneralHeader((RESP_TYPE)TASK_EXECUTE_RESP, fromPeerId, status);
	MessageMap mmap;
	mmap["taskid"] = boost::lexical_cast<string>(taskId);

	pResp->AddTaskInfo(mmap);

	std::string xml = pResp->GetXML();
	result = this->SendMessage(xml);
	return result;
}

int PeerNode::TaskExecuteResult(int taskId, bool status, std::vector<std::string>& taskresult, std::string param)
{
	int result = 0;

	Log(CONSOLE, L"Sending TASK_RESULT, id: %d to Peer: %d\n", taskId, m_peerId);
	int fromPeerId = Peer::GetPeer()->GetPeerId();
	int listenPort = Peer::GetPeer()->GetPort();

	string tId = boost::lexical_cast<string>(taskId);
	string pId = boost::lexical_cast<string>(fromPeerId);

	RequestMessagePtr pReq(new RequestMessage);
	pReq->AddGeneralHeader((REQ_TYPE)TASK_RESULT, fromPeerId, listenPort);
	pReq->AddResult(taskresult);

	MessageMap mmap;
	mmap["taskid"] = boost::lexical_cast<string>(tId);
	mmap["status"] = boost::lexical_cast<string>(status);
	mmap["taskParam"] = param;

	pReq->AddTaskInfo(mmap);
	std::string xml = pReq->GetXML();
	result = this->SendMessage(xml);

	return result;	
}