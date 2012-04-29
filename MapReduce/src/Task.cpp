#include "Peer.h"
#include "Task.h"
#include <boost/lexical_cast.hpp>

Task::Task(int taskId)
{
	m_taskId = taskId;
	m_numRequiredPeers = 1;
	m_numMaxToTryPeers = 3;
	m_numTotalPeersTried = 0;

	m_highThreshold	= 2.0;
	m_mediumThreshold = 1.0;
	m_currentThreshold = m_highThreshold;
	m_numResultsRecvd = 0;

	m_bStop = false;

	m_numRemainingPeers = m_numRequiredPeers;
	m_pMsgQ = MessageQueuePtr(new MessageQueue);	
}

int Task::Start()
{
	// Ask some peer nodes for executing tasks
	// [TODO] whom to select? right now just ask/get first 2 nodes
	Log(CONSOLE, L"Task::Start - Starting New Task, id : %d\n", m_taskId);
	
	m_taskThread = boost::thread(&Task::run, this);
	AskPeers();

	return 0;
}

void Task::AddTaskParams(std::vector<std::string> params) 
{ 
	m_paramList.clear(); 
	m_paramList = params; 
	m_numRequiredPeers = params.size();
	m_numRemainingPeers = m_numRequiredPeers;
	for(int i = 0; i < params.size(); i++)
		m_unAllotParamList.push_back(i);
}

int Task::AskPeers()
{
	//[TODO] make some good strategy to know how many peers to contact and how to try
	// other peers when some nodes do not respond
	// [TODO] handle situation when some peers do not handle in some specified time
	// right now handling only situation where they give success or false

	Log(CONSOLE, L"Task::AskPeers - Asking Peers for Task, id : %d\n", m_taskId);

	// get peer list
	PeerPtr pPeer = Peer::GetPeer();
	unsigned long ts = 0;
	std::vector<RemotePeers> peerList = pPeer->GetPeerList(ts);
	m_numMaxToTryPeers = (peerList.size() - 1);
//	m_numMaxToTryPeers = 2 * m_numRemainingPeers;

	// ask peers
	m_currentThreshold = m_highThreshold;
	std::vector<PeerNodePtr> toTryNodes;
	int numAsking = 0;

	for(int i = 0; i < peerList.size(); i++) {
		if(m_numTotalPeersTried >= m_numMaxToTryPeers) {
			if(m_currentThreshold <= m_mediumThreshold) {
				Log(ERR, L"Number of Eligible Peers not found for Task: %d, Cannot execute task\n", m_taskId);
				this->TaskCancel();
				return -1;
			}

			m_currentThreshold = m_mediumThreshold;
			Log(CONSOLE, L"Number of Requests exceeded the threshold for task: %d, lowering threshold to %0.1f\n", m_taskId,
				m_currentThreshold);
			m_tryNodes.clear();
			m_numTotalPeersTried = 0;
			i = 0;	// restart
		}
		int paramIndx = 0;	// always go for 1st index
		
		// see if myself
		if(peerList[i].peer_id == pPeer->GetPeerId()) continue;
		// see if already asked
		if(this->IsInTryNodes(peerList[i].peer_id) == true) continue;

		PeerNodePtr pNode = PeerNode::CreatePeerNode(peerList[i]);
		if( pNode != NULL ) {
			// try connecting it with the peer node so we can know whether its alive or not
			m_numTotalPeersTried ++;
			int result = pNode->Connect();
			if( result < 0 ) continue;

			// if it connects, then add to possible contact list
			numAsking ++;
			m_tryNodes.push_back(pNode);
//			m_tryPeerParamMap[pNode] = m_unAllotParamList[paramIndx];
			m_tryPeerParamMap[m_unAllotParamList[paramIndx]] = pNode;
			pNode->SetParamIndex(m_unAllotParamList[paramIndx]);
			//toTryNodes.push_back(pNode);
			result = pNode->TaskRequest(m_taskId, m_paramList[m_unAllotParamList[paramIndx]]);
			// if req successfully sent then erase param from the unalloted param list
			if(result > 0) 
				m_unAllotParamList.erase(m_unAllotParamList.begin() + paramIndx);
		}

		if(numAsking >= m_numRemainingPeers) break;
	}

	if(numAsking < m_numRemainingPeers) {
		Log(ERR, L"Task - %d, Cannot be completed (Required Number of Peers Not Available\n", m_taskId);
		this->TaskCancel();
		return 0;
	}
	m_numRemainingPeers -= numAsking;

/*	// lower the threshold
	if(m_numTotalPeersTried >= m_numMaxToTryPeers) {
		if(m_currentThreshold <= m_mediumThreshold) {
			Log(ERR, L"Number of Eligible Peers not found for Task: %d, Cannot execute task\n", m_taskId);
			this->TaskCancel();
			return -1;
		}
		
		m_currentThreshold = m_mediumThreshold;
		Log(CONSOLE, L"Number of Requests exceeded the threshold for task: %d, lowering threshold to %0.1f\n", m_taskId,
					m_currentThreshold);
		m_tryNodes.clear();
		m_numTotalPeersTried = 0;
	}

	for(int i = 0; i < peerList.size(); i++) {
		// see if myself
		if(peerList[i].peer_id == pPeer->GetPeerId()) continue;
		// see if already asked
		if(this->IsInTryNodes(peerList[i].peer_id) == true) continue;

		PeerNodePtr pNode = PeerNode::CreatePeerNode(peerList[i]);
		if( pNode != NULL ) {
			// try connecting it with the peer node so we can know whether its alive or not
			int result = pNode->Connect();
			if( result < 0 ) continue;

			// if it connects, then add to possible contact list
			numAsking ++;
			m_tryNodes.push_back(pNode);
			toTryNodes.push_back(pNode);
		}

		if(numAsking >= m_numRemainingPeers) break;
	}

	if( numAsking < m_numRemainingPeers ) {
		Log(ERR, L"Task - %d, Cannot be completed (Required Number of Peers Not Available\n", m_taskId);
		this->TaskCancel();
		return 0;
	}

	for(int i = 0; i < toTryNodes.size(); i++) {
		int result = 0;
		// send request
		result = toTryNodes[i]->Connect();
		result = toTryNodes[i]->TaskRequest(m_taskId);
		m_numTotalPeersTried ++;
	}
*/
	return 0;
}

void Task::ProcessMessages(WMessagePtr pWMsg)
{
	MessagePtr pMsg = pWMsg->GetMsg();
	if( pMsg != NULL && pMsg->m_mesgType != INTERNAL) {
		Log(CONSOLE, L"Task::ProcessMessages Task Message Processed %d\n", pMsg->m_mesgType);
	} else {
		this->HandleSysMessages(pWMsg);
		return;
	}

	if(pMsg->m_mesgType == RESP) {
		switch(pMsg->m_respType) {
			case TASK_REQUEST_RESP:
				HandleTaskRequestResponse(pMsg);
				break;
			case TASK_EXECUTE_RESP:
				HandleTaskExecuteResponse(pMsg);
				break;
			case TASK_RESULT:
				HandleTaskResult(pMsg);
				break;
		}
	}
	else if(pMsg->m_mesgType == REQ) {
		switch(pMsg->m_reqType) {
			case TASK_RESULT:
				HandleTaskResult(pMsg);
				break;
		}
	}

}

void Task::HandleSysMessages(WMessagePtr pMsg)
{
	switch(pMsg->m_sysMsg) {
		case CONNECTION_FAILURE_ON_PEER:
			HandlePeerConnFailure(pMsg);
			break;
	}
}

void Task::HandlePeerConnFailure(WMessagePtr pMsg)
{
	int remotePeer = pMsg->GetPeerId();

	// see if the peer has been alloted any task or not
	PeerNodePtr pPeer = pMsg->GetMsg()->m_conn->GetPeerNode();
	if( IsPeerInstanceBelongToTask(pPeer) == false ) 
		return;

	PeerNodePtr pNode = PeerNodePtr();
	int indx = -1;
	for(int i = 0; i < m_taskNodes.size(); i++) {
		if(m_taskNodes[i]->GetPeerId() == remotePeer) {
			if(m_peerResultAvailMap.find(/*m_taskNodes[i]*/pPeer) != m_peerResultAvailMap.end() ) 
				continue;
			if( pPeer != m_taskNodes[i] )
				continue;
			pNode = m_taskNodes[i];
			indx = i;
			break;
		}
	}
	if(pNode == NULL) return;

	int paramIndx = pNode->GetParamIndex();
	if(paramIndx < 0) {
		Log(ERR, L"Task::HandlePeerConnFailure - PeerNode: %d, has ParamIndex: %d\n", pNode->GetPeerId(),
			paramIndx);
	}

	Log(CONSOLE, L"A Peer: %d, got disconnected for Task: %d, Parameter: %s, Handling\n", remotePeer, m_taskId,
		m_paramList[paramIndx].c_str());

	// get the param to unalloted list
	if(m_tryPeerParamMap.find(paramIndx) != m_tryPeerParamMap.end())
		m_tryPeerParamMap.erase(m_tryPeerParamMap.find(paramIndx));
	if(m_peerParamMap.find(paramIndx) != m_peerParamMap.end())
		m_peerParamMap.erase(m_peerParamMap.find(paramIndx));
	if(find(m_unAllotParamList.begin(), m_unAllotParamList.end(), paramIndx) == m_unAllotParamList.end())
		m_unAllotParamList.push_back(paramIndx);
	
	// cancel its task and send this task to some other peer.
	pNode->TaskCancel(m_taskId);
	m_taskNodes.erase(m_taskNodes.begin() + indx);

	// remove from the try nodes too
	this->RemovePeerFromTryNodes(pNode);

	m_numRemainingPeers++;
//	m_numTotalPeersTried = 0;

	this->AskPeers();
}

// process message queue
void Task::run()
{
	while (!m_bStop) {
		WMessagePtr pMesg = m_pMsgQ->GetMessage();
		// if empty wait for queue to get full
		if( pMesg == NULL ) {
			m_pMsgQ->WaitForFull();
			continue;
		}
		ProcessMessages(pMesg);
	}
}

void Task::HandleTaskRequestResponse(MessagePtr pMsg)
{
	PeerNodePtr pNode = pMsg->m_conn->GetPeerNode();
	bool status = pMsg->m_status;

	Log(CONSOLE, L"Task Request Response Received with status: %d, taskId: %d\n", status, pMsg->m_taskId);

	int paramIndx = pNode->GetParamIndex();
	if(paramIndx < 0) {
		Log(ERR, L"Task::RequestResponse - PeerNode: %d, has ParamIndex: %d\n", pNode->GetPeerId(),
			paramIndx);
	}

	if(status == true) {
		// see the peers history to decide
		double elig = CalculatePeerEligibility(pMsg);
		if( elig >= m_currentThreshold ) {
			Log(CONSOLE, L"Eligible Peer: %d Found for task: %d, eligibility level: %0.1f\n", pNode->GetPeerId(),
				m_taskId, elig);
		} else {
			Log(CONSOLE, L"Peer: %d Found Not Eligible for task: %d, eligibility level: %0.1f\n", pNode->GetPeerId(),
				m_taskId, elig);
			status = false;
		}
	}

	// success
	if(status == true) {
		if( m_numRemainingPeers <= 0) {
			//[TODO] if m_numRemainingPeers gets < 0 i.e we spawned more succesful requests than required
			// cancel these requests
		}

		m_numRemainingPeers --;

		// add in final nodes
		m_taskNodes.push_back(pNode);
		
		// add param to final list
		m_peerParamMap[paramIndx] = pNode;

		Log(CONSOLE, L"Task- Allocating Task: %d, with param: %s, to PeerNode: %d\n", m_taskId, 
			m_paramList[paramIndx], pNode->GetPeerId());
		
		if(m_tryPeerParamMap.find(paramIndx) != m_tryPeerParamMap.end())
			m_tryPeerParamMap.erase(m_tryPeerParamMap.find(paramIndx));

		// send task exec req
		pNode->TaskExecute(m_taskId);
	} else {	// failure; peer doesnot want to execute task or not eligible
		Log(ERR, L"Task Request Response Received with Failure or Not Eligible, taskId: %d\n", pMsg->m_taskId);
		pNode->TaskCancel(m_taskId);

		// re-add param to unallot list
		m_tryPeerParamMap.erase(m_tryPeerParamMap.find(paramIndx));

		if(find(m_unAllotParamList.begin(), m_unAllotParamList.end(), paramIndx) == m_unAllotParamList.end())
			m_unAllotParamList.push_back(paramIndx);

		// again ask some peers
		this->AskPeers();
	}
}

void Task::HandleTaskResult(MessagePtr pMsg)
{
	Log(CONSOLE, L"Task Execute Result Received from Peer: %d, taskId: %d\n", pMsg->m_fromPeerId, pMsg->m_taskId);
	std::map<std::string, std::string> taskMap = pMsg->m_taskMap;
	std::vector<std::string> result = pMsg->m_resultList;

	if(taskMap.size() == 0) {
		Log(ERR, L"Task Execute Result Received from Peer: %d is Invalid, taskId: %d\n", 
			pMsg->m_fromPeerId, pMsg->m_taskId);
		return;
	}

	PeerNodePtr pNode = pMsg->m_conn->GetPeerNode();
	int paramIndx = -1;
	std::string param = taskMap["taskParam"];
	// find param indx
	for(int i = 0; i < m_paramList.size(); i++) {
		if(m_paramList[i] == param) {
			paramIndx = i;
			break;
		}
	}

	this->RemovePeerFromFinalNodes(pNode);
	this->RemovePeerFromTryNodes(pNode);

	m_numResultsRecvd ++;
	bool status = false;
	status = boost::lexical_cast<bool>(taskMap["status"]);
	
	// success
	if(status == true) {
		m_resultList.push_back(result);
		m_paramResultMap[paramIndx] = result;

		m_peerResultAvailMap[pNode] = true;

		Log(CONSOLE, L"Task Execute Result Received - Success!!, taskId: %d\n", pMsg->m_taskId);
		Log(CONSOLE, L"Results:\n");
		for(int i = 0; i < result.size(); i++) {
			Log(CONSOLE, L"Result line #%d: %s\n", i, result[i].c_str());
		}
	} else {	// failure
		Log(ERR, L"Task Execute Result Received - Failure!!, taskId: %d\n", pMsg->m_taskId);
	}

	PeerNode::DestroyPeerNode(pNode);
}

void Task::HandleTaskExecuteResponse(MessagePtr pMsg)
{
	bool status = pMsg->m_status;

	Log(CONSOLE, L"Task Execute Response Received with status: %d, taskId: %d\n", status, pMsg->m_taskId);

	PeerNodePtr pNode = pMsg->m_conn->GetPeerNode();
	int paramIndx = pNode->GetParamIndex();
	if(paramIndx < 0) {
		Log(ERR, L"Task::RequestResponse - PeerNode: %d, has ParamIndex: %d\n", pNode->GetPeerId(),
			paramIndx);
	}

	// success
	if(status == true) {
		 
	} else {	// failure; peer doesnot want to execute task
		Log(ERR, L"Task Execute Response Received with Failure, taskId: %d\n", pMsg->m_taskId);
		PeerNodePtr pNode = pMsg->m_conn->GetPeerNode();
		m_numRemainingPeers ++;

		// remove from final nodes
		RemovePeerFromFinalNodes(pNode);

		// re-add param to unallot list
		if(m_peerParamMap.find(paramIndx) != m_peerParamMap.end())
			m_peerParamMap.erase(m_peerParamMap.find(paramIndx));

		if(find(m_unAllotParamList.begin(), m_unAllotParamList.end(), paramIndx) == m_unAllotParamList.end())
			m_unAllotParamList.push_back(paramIndx);

		// again ask some peers
		this->AskPeers();
	}
}

void Task::TaskCancel()
{
	Log(CONSOLE, L"Cancelling Task: %d\n", m_taskId);
	for(int i = 0;i < m_tryNodes.size(); i++) {
		m_tryNodes[i]->TaskCancel(this->m_taskId);
	}
	m_tryNodes.clear();
	m_peerParamMap.clear();
	m_tryPeerParamMap.clear();

	for(int i = 0;i < m_taskNodes.size(); i++) {
		m_taskNodes[i]->TaskCancel(this->m_taskId);
	}
	m_taskNodes.clear();

	this->Stop();
}

bool Task::IsPeerInstanceBelongToTask(PeerNodePtr pPeer)
{
	bool bfound = false;
	for(unsigned int i = 0; i < m_taskNodes.size(); i++) {
		PeerNodePtr p = m_taskNodes[i];
		if( p == pPeer ) {
			bfound = true;
			break;
		}
	}
	return bfound;
}

bool Task::IsTaskComplete()
{
	if(m_numResultsRecvd >= m_numRequiredPeers && m_taskNodes.size() == 0)
		return true;
	return false;
}

void Task::RemovePeerFromTryNodes(PeerNodePtr pNode)
{
	int toRemove = -1;
	for(unsigned int i = 0; i < m_tryNodes.size(); i++) {
		PeerNodePtr p = m_tryNodes[i];
		if( p->GetPeerId() == pNode->GetPeerId() ) {
			toRemove = i;
			break;
		}
	}

	if( toRemove >= 0 )
		m_tryNodes.erase( m_tryNodes.begin() + toRemove );
}

void Task::RemovePeerFromFinalNodes(PeerNodePtr pNode)
{
	int toRemove = -1;
	for(unsigned int i = 0; i < m_taskNodes.size(); i++) {
		PeerNodePtr p = m_taskNodes[i];
		if( p->GetPeerId() == pNode->GetPeerId() ) {
			toRemove = i;
			break;
		}
	}

	if( toRemove >= 0 )
		m_taskNodes.erase( m_taskNodes.begin() + toRemove );
}

bool Task::IsInTryNodes(int peerId)
{
	bool bfound = false;
	for(unsigned int i = 0; i < m_tryNodes.size(); i++) {
		PeerNodePtr p = m_tryNodes[i];
		if( p->GetPeerId() == peerId ) {
			bfound = true;
			break;
		}
	}
	return bfound;
}

double Task::CalculatePeerEligibility(MessagePtr pMsg)
{
	std::map<std::string, std::string> histMap = pMsg->m_sysHistory;
	if( histMap.size() <= 0) return 0.0;
	double workload = boost::lexical_cast<double>(histMap["workload"]);
	double avgExec = boost::lexical_cast<double>(histMap["avgexecution"]);
	double totalExec = boost::lexical_cast<double>(histMap["totalexecuted"]);
	double totalFail = boost::lexical_cast<double>(histMap["totalfailure"]);

	double iWload = (workload <= 0.0 ? 0.0 : (1.0/workload));
	if(totalExec == 0.0) return (m_mediumThreshold + 0.1);

	//double elig = ( (1.0/workload) * (1.0/avgExec) * (totalExec/7.0) - ((totalFail/totalExec)*2.0) );
	double elig = ( (/*(1.0/avgExec) + */(totalExec/1.0)) - ((totalFail/1.0)*2.0 + iWload) );
	return elig;
}