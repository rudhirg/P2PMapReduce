#include "Peer.h"

PeerPtr Peer::m_ptrPeer;

Peer::Peer()
{
	m_peerId = 0;
	m_lastTaskId = 0;
	m_lastJobId = 0;
	m_port = 0;
	m_pMsgQ = MessageQueuePtr(new MessageQueue);	
	m_db = DatabasePtr(new Database);
	m_pTimerMessages		= TimerMessagePtr(new TimerMessage());
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

void Peer::MessageEvent(SYS_MSG msg)
{
	switch(msg) {
		case CONNECT_COORD:
			this->ConnectToCoordinator();
			break;
	}
}

void Peer::Start(std::string configFile)
{
	m_pTimerMessages->Init(shared_from_this());
	// Parse startup configuration and create coordinator node
	// coordinator is '0' peer id
	PeerNodePtr pCoordNode = PeerNode::CreatePeerNode(0);
	int result = this->ReadConfigFile(configFile, pCoordNode);

	if(result < 0) {
		Log(ERR, L"Config File Error!!\n");
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

	this->ConnectToCoordinator();
}

int Peer::ConnectToCoordinator()
{
	Log(CONSOLE, L"Trying to Connect to Coordinator\n");
	int result = m_pCoordinatorNode->Connect();
	if( result != 0 ) {
		m_pTimerMessages->CreateTimer(CONNECT_COORD, 4);
		return result;
	}
	m_pCoordinatorNode->SendOnlineMessage();
	return result;
}

int Peer::ReadConfigFile(std::string configFile, PeerNodePtr pCoordNode)
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
	cnode = node.child("ipaddress");
	if( cnode == NULL )
		return -1;

	std::string ip = cnode.child_value();
	
	cnode = node.child("port");
	if( cnode == NULL )
		return -1;

	std::string port = cnode.child_value();

	pCoordNode->SetIpAddr(ip);
	pCoordNode->SetListenPort(atoi(port.c_str()));

	// get backup coordinators
	node = root.child("Me");

	// get me port
	pugi::xml_node backupNode = root.child("Backup");
	if( backupNode != NULL ) {
		for (pugi::xml_node pnode = backupNode.child("Coordinator"); pnode; pnode = pnode.next_sibling("Coordinator")){
			pugi::xml_node cnode = pnode;
			cnode = pnode.child("ipaddress");
			std::string ip = cnode.child_value();
			cnode = pnode.child("port");
			std::string port = cnode.child_value();
			pCoordNode->AddBackupServer(ip, atoi(port.c_str()));
		}
	}

	cnode = node;
	cnode = node.child("port");
	if( cnode == NULL )
		return -1;

	port = cnode.child_value();
	this->SetPort(atoi(port.c_str()));	

	cnode = node;
	cnode = node.child("id");
	if( cnode == NULL )
		return -1;

	string peerid = cnode.child_value();
	this->SetPeerId(atoi(peerid.c_str()));	

	// set the config info in db
	m_db->SetCoordinatorIp(ip);
	m_db->SetCoordinatorPort(atoi(port.c_str()));
 
	return result;	
}

void Peer::Kill()
{
	// send offline message to coordinator
	m_pCoordinatorNode->SendOfflineMessage();
	Sleep(1000);

	//if(m_ptrConnMgr != NULL)
	//	m_ptrConnMgr->Kill();
	//m_peerThread.interrupt();
}

void Peer::ProcessMessages(WMessagePtr pWMsg)
{
	MessagePtr pMsg = pWMsg->GetMsg();
	if( pWMsg->GetSysMsg() == NONE && pMsg != NULL) {
		Log(CONSOLE, L"Message Processed\n");
		if( pMsg->m_mesgType == REQ )
			this->HandleRequests(pWMsg);
		else if( pMsg->m_mesgType == RESP )
			this->HandleResponses(pWMsg);
	} else {
		//[TODO] handle the sys messages
		this->HandleSysMessages(pWMsg);
	}
}

void Peer::HandleSysMessages(WMessagePtr pMsg)
{
	SYS_MSG sysMsg = pMsg->GetSysMsg();
	switch(sysMsg) {
		case TASK_EXECUTION_SUCCESS:
			HandleJobExecution(pMsg);
			break;
		case CONNECTION_FAILURE_ON_PEER:
			HandleConnectionFailure(pMsg);
			break;
	}
}

void Peer::HandleJobExecution(WMessagePtr pMsg)
{
	bool bJobResult = false;
	SYS_MSG sysMsg = pMsg->GetSysMsg();
	if(sysMsg == TASK_EXECUTION_SUCCESS) {
		bJobResult = true;
		int jobId = pMsg->GetJobId();
		if(jobId <= 0) {
			Log(ERR, L"Peer::HandleJobExecution Message from Invalid Job.. Ignoring it\n");
			return;
		}
		// find the job
		if(m_jobList.find(jobId) == m_jobList.end()) {
			Log(ERR, L"Peer::HandleJobExecution Message from Invalid Job.. Ignoring it\n");
			return;
		}

		JobPtr pJob = m_jobList[jobId];
		// send the response to remote peer
		RemotePeers rpeer = this->GetRemotePeerById(pJob->GetRemotePeerId());

		// get the stats and put in workhistory
		double execTime = pJob->GetExecutionTime();
		WorkHistoryPtr pWH = WorkHistory::GetWorkHistory();
		pWH->SetNewFinishTaskInfo(execTime, true);

		// not found in db
		// [TODO] handle when peer not found in db
		if(rpeer.peer_id < 0) {
			Log(ERR, L"Peer::HandleJobExecution Peer: %d, Not found in DB for job: %d, still sending the result\n", 
				pJob->GetRemotePeerId(), jobId);
		}

		//PeerNodePtr pNode = PeerNode::CreatePeerNode(rpeer);
		PeerNodePtr pNode = pJob->GetPeerNode();
		pNode->TaskExecuteResult(pJob->GetTaskId(), bJobResult, pJob->GetResult(), pJob->GetParam());
	}
}

void Peer::HandleResponses(WMessagePtr pWMsg)
{
	MessagePtr pMsg = pWMsg->GetMsg();

	// get the peer list if given in message
	std::vector<RemotePeers> peerList = pMsg->m_peerList;
	if( peerList.size() > 0 ) {
		long ts = pMsg->m_timestamp;
		this->HandlePeerList(peerList, ts);
	}

	// see if it is a task message
	if( pMsg->IsTaskMessage() ) {
		int msgTaskId = pMsg->m_taskId;
		// get the task and forward the message
		std::map<int, TaskPtr>::iterator it;
		it = m_taskList.find(msgTaskId);
		if( it != m_taskList.end() ) {
			TaskPtr pTask = (*it).second;
			pTask->GetMessageQueue()->PutMessage(pWMsg);
		}
		else {
			Log(CONSOLE, L"No Task Related to Task Response Message Received for taskId: %d\n", msgTaskId);
		}
		return;
	}

	switch(pMsg->m_respType) {
		case GET_ONLINE_RESP:
			HandleOnlineResp(pMsg);
			break;
		default:
			break;
	}
}

void Peer::HandleRequests(WMessagePtr pWMsg)
{
	MessagePtr pMsg = pWMsg->GetMsg();

	// see if it is a task message
	/*if( pMsg->IsTaskMessage() ) {
		int msgTaskId = pMsg->m_taskId;
		// get the task and forward the message
		std::map<int, TaskPtr>::iterator it;
		it = m_taskList.find(msgTaskId);
		if( it != m_taskList.end() ) {
			TaskPtr pTask = (*it).second;
			pTask->GetMessageQueue()->PutMessage(pWMsg);
			return;
		}
		else {
			Log(CONSOLE, L"No Task Related to Request Message Received for taskId: %d, Forwarding to Main Queue\n",
																	msgTaskId);
		}
	}*/

	switch(pMsg->m_reqType) {
		case TASK_REQUEST:
			HandleTaskRequest(pMsg);
			break;
		case TASK_EXECUTE:
			HandleTaskExecute(pMsg);
			break;
		case TASK_STATE:
			break;
		case TASK_RESULT:
			HandleTaskResult(pWMsg);
			break;
		case TASK_CANCEL:
			HandleTaskCancel(pMsg);
			break;
		default:
			break;
	}
}

void Peer::HandleTaskResult(WMessagePtr pWMsg)
{
	MessagePtr pMsg = pWMsg->GetMsg();
	Log(CONSOLE, L"Handling Task Result from Peer: %d, taskid: %d\n", 
											pMsg->m_fromPeerId, pMsg->m_taskId);

	PeerNodePtr pNode = pMsg->m_conn->GetPeerNode();
	// get the task and forward the message
	std::map<int, TaskPtr>::iterator it;
	it = m_taskList.find(pMsg->m_taskId);
	if( it != m_taskList.end() ) {
		TaskPtr pTask = (*it).second;
		pTask->GetMessageQueue()->PutMessage(pWMsg);
	}
	else {
		Log(CONSOLE, L"No Task Related to Task Result Message Received for taskId: %d\n", pMsg->m_taskId);
	}
}

void Peer::HandleTaskRequest(MessagePtr pMsg)
{
	Log(CONSOLE, L"Handling Task request from Peer: %d, taskid: %d\n", 
											pMsg->m_fromPeerId, pMsg->m_taskId);

	int frompeerid = pMsg->m_fromPeerId;
	PeerNodePtr pNode = pMsg->m_conn->GetPeerNode();
	// check whether the remote peer is in peer list- if not then add [TODO] implement more secure way
	RemotePeers rpeer = this->GetRemotePeerById(frompeerid);
	if(rpeer.peer_id < 0) {
		Log(CONSOLE, L"Received Task Request from Unresolved Peer, id: %d, taskId: %d\n", frompeerid,
			pMsg->m_taskId);
		rpeer.peer_id = frompeerid;
		rpeer.ip = pNode->GetIpAddr();
		rpeer.port = pNode->GetListenPort();

		// add
		m_db->AddPeer(rpeer);
	}

	//[TODO] check details of task and our capability

	// create job
	CreateJob(pMsg->m_fromPeerId, pMsg->m_taskId, pMsg->m_taskMap, pNode);

	// send the response
	pNode->TaskResponse(pMsg->m_taskId, true);
}

void Peer::HandleTaskExecute(MessagePtr pMsg)
{
	bool status = true;
	PeerNodePtr pPeer = pMsg->m_conn->GetPeerNode();

	JobPtr pJob = this->GetJob(pMsg->m_fromPeerId, pMsg->m_taskId, pPeer);

	if(pJob == NULL) {
		Log(ERR, L"Peer::HandleTaskExecute- Task Execute request received for Non-Existing Job from Peer: %d, taskid: %d\n", 
											pMsg->m_fromPeerId, pMsg->m_taskId);
		status = false;
	}
	else {
		Log(CONSOLE, L"Handling Task Execute from Peer: %d, taskid: %d\n", 
			pMsg->m_fromPeerId, pMsg->m_taskId);
		// start the job
		pJob->SetPeerNode(pMsg->m_conn->GetPeerNode());
		int result = pJob->Start();

		//[TODO] handle job failure to execute
		if(result < 0) {
			Log(ERR, L"Peer::HandleTaskExecute- Task Execute request could not be completed from Peer: %d, taskid: %d\n", 
											pMsg->m_fromPeerId, pMsg->m_taskId);
			status = false;
		}
	}

	// send response
	PeerNodePtr pNode = pMsg->m_conn->GetPeerNode();
	pNode->TaskExecuteResponse(pMsg->m_taskId, status);
}

void Peer::HandleTaskCancel(MessagePtr pMsg)
{
	bool status = true;
	JobPtr pJob = this->GetJob(pMsg->m_fromPeerId, pMsg->m_taskId);

	if(pJob == NULL) {
		Log(ERR, L"Peer::HandleTaskCancel- Task Cancel request received for Non-Existing Job from Peer: %d, taskid: %d\n", 
											pMsg->m_fromPeerId, pMsg->m_taskId);
		status = false;
	}
	else {
		Log(CONSOLE, L"Handling Task Cancel from Peer: %d, taskid: %d\n", pMsg->m_fromPeerId, pMsg->m_taskId);
		// cancel the job
		pJob->Stop();
		m_jobList.erase(pJob->GetJobId());
	}
}

void Peer::HandleOnlineResp(MessagePtr pMsg)
{
	// [TODO] handle failed (status) msgs by reconnecting

	// get peer id - peer id is the task id
//	m_peerId = pMsg->m_taskId;

	// get the peers list
	m_db->SetPeerList(pMsg->m_peerList, pMsg->m_timestamp);

	Log(CONSOLE, L"Online Response Message\n");
	Log(CONSOLE, L"Peer ID: %d\n", m_peerId);
	Log(CONSOLE, L"Peer List:\n");
	unsigned long ts = 0;
	std::vector<RemotePeers>& peers = m_db->GetPeerList(ts);
	for(unsigned int i = 0; i < peers.size(); i++) {
		Log(CONSOLE, L"Peer id - %d, ip - %s\n", peers[i].peer_id, peers[i].ip.c_str());
	}

	// disconnect coord
	PeerNodePtr pNode = pMsg->m_conn->GetPeerNode();
	if(pNode != NULL)
		PeerNode::DestroyPeerNode(pNode);
}

void Peer::HandlePeerList(std::vector<RemotePeers> plist, long timestamp)
{
	if(plist.size() <= 0 || timestamp <= 0)
		return;

	// get the peers list
	unsigned long pTs = m_db->GetPeerListTimestamp();
	if( pTs >= timestamp ) {
		Log(CONSOLE, L"New PeerList received is Stale, Ignoring it!!\n");
		return;
	}

	m_db->SetPeerList(plist, timestamp);

	Log(CONSOLE, L"New Peer List Received\n");
	Log(CONSOLE, L"Peer ID: %d\n", m_peerId);
	Log(CONSOLE, L"Peer List:\n");
	unsigned long ts = 0;
	std::vector<RemotePeers>& peers = m_db->GetPeerList(ts);
	for(unsigned int i = 0; i < peers.size(); i++) {
		Log(CONSOLE, L"Peer id - %d, ip - %s\n", peers[i].peer_id, peers[i].ip.c_str());
	}

}

// handle the connection failures
void Peer::HandleConnectionFailure(WMessagePtr pMsg)
{
	// see to which peer-node the connection failed
	// see if there is any task been alloted to that peer
	// see if there is any job been executed for that peer

	int failedConnPeer = pMsg->GetPeerId();
	// get the task and forward the failure message
	std::map<int, TaskPtr>::iterator it = m_taskList.begin();
	for( ;it != m_taskList.end(); it++ ) {
		TaskPtr pTask = (*it).second;
		pTask->GetMessageQueue()->PutMessage(pMsg);
	}
}

// process message queue
void Peer::run()
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

void Peer::HandleConnectionRequest(ConnectionPtr pConn)
{
	// make the peer node, we do not know at the start the peer id
	PeerNodePtr pNode = PeerNode::CreatePeerNode(-1);
	pNode->HandleConnect(pConn);
}

int Peer::CreateJob(int remotePeer, int taskId, std::map<std::string, std::string> taskInfo, PeerNodePtr pNode)
{
	int jobId = GetRandomJobId();
	JobPtr pJob(new Job(jobId));
	pJob->SetRemotePeerId(remotePeer);
	pJob->SetTaskId(taskId);
	pJob->SetParam(taskInfo["taskParam"]);
	pJob->SetPeerNode(pNode);

	m_jobList[jobId] = pJob;

	Log(CONSOLE, L"Peer::CreateJob - Creating New Job, job-id : %d\n", jobId);
	return 0;
}

JobPtr Peer::GetJob(int peerId, int taskId, PeerNodePtr pNode)
{
	std::map<int, JobPtr>::iterator it;
	for(it = m_jobList.begin(); it != m_jobList.end(); it++) {
		JobPtr pJob = (*it).second;
		if(pJob->GetTaskId() == taskId && pJob->GetRemotePeerId() == peerId)
			if(pNode == NULL)
				return pJob;
			else if(pNode == pJob->GetPeerNode())
				return pJob;
	}
	return JobPtr();
}

int Peer::AddTask()
{
	boost::recursive_mutex::scoped_lock L(m_taskMutex);

	Log(CONSOLE, L"Creating New Task\n");

	// get task config
	std::vector<std::string> paramList = this->ReadTaskConfig("Task-config.xml");

	int taskId = GetRandomTaskId();
	// create task
	TaskPtr pTask(new Task(taskId));
	m_taskList[taskId] = pTask;

	pTask->AddTaskParams(paramList);
	pTask->Start();

	return 0;
}

std::vector<std::string> Peer::ReadTaskConfig(std::string configFile)
{
	boost::recursive_mutex::scoped_lock L(m_taskMutex);
	Log(CONSOLE, L"Reading Task Config\n");

	int ret = 0;

	std::vector<std::string> paramList;

	pugi::xml_document doc;
	pugi::xml_parse_result result = doc.load_file(configFile.c_str());

	if( result.status != pugi::status_ok ) {
		Log(ERR, L"Task Configuration file error\n");
		return paramList;
	}

	pugi::xml_node root = doc.child("Root");

	// get me port
	for (pugi::xml_node pnode = root.child("param"); pnode; pnode = pnode.next_sibling("param")){
		std::string param = pnode.child_value();
		paramList.push_back(param);
	}

	// set the config info in db
	return paramList;	

}

RemotePeers Peer::GetRemotePeerById(int peerId)
{
	RemotePeers r;
	r.peer_id = -1;

	boost::recursive_mutex::scoped_lock L(m_mutex);

	unsigned long ts = 0;
	std::vector<RemotePeers> peers = this->GetPeerList(ts);
	for(int i = 0;i < peers.size(); i++) {
		if( peers[i].peer_id == peerId )
			return peers[i];
	}

	return r;
}

void Peer::AddPeerNode(PeerNodePtr pNode)
{
	boost::recursive_mutex::scoped_lock L(m_mutex);
	m_peerNodeList.push_back(pNode);
}

void Peer::RemovePeerNode(PeerNodePtr pNode)
{
	boost::recursive_mutex::scoped_lock L(m_mutex);
	int toRemove = -1;
	for(unsigned int i = 0; i < m_peerNodeList.size(); i++) {
		PeerNodePtr p = m_peerNodeList[i];
		if( p->GetPeerId() == pNode->GetPeerId() ||
			( p->GetIpAddr() == pNode->GetIpAddr() && p->GetPort() == pNode->GetPort())) {
			toRemove = i;
			break;
		}
	}

	if( toRemove >= 0 )
		m_peerNodeList.erase( m_peerNodeList.begin() + toRemove );
}

// Test Methods
void Peer::TestSendMesgs(std::string msg)
{
	/*RequestMessagePtr pReq(new RequestMessage);
	pReq->AddGeneralHeader((REQ_TYPE)0, 1);
	std::string xml = pReq->GetXML();
	m_pCoordinatorNode->SendMessage(xml);*/
}

void Peer::TestTask()
{
	this->AddTask();
}