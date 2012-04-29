#ifndef PEER_H
#define PEER_H

#include "PeerNode.h"

#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>
#include <boost/enable_shared_from_this.hpp>

#include <pugixml.hpp>

#include <string>
#include "MessageQueue.h"
#include "ConnectionManager.h"
#include "Database.h"
#include "Task.h"
#include "Job.h"
#include "WorkHistory.h"
#include "TimerMessage.h"

class Peer;

typedef boost::shared_ptr<Peer> PeerPtr;

class Peer : public boost::enable_shared_from_this<Peer>, public TimerI {
private:
	static PeerPtr				m_ptrPeer;
	int							m_peerId;
	std::string					m_ipAddress;
	int							m_port;
	DatabasePtr					m_db;		// stores config infos
	boost::thread				m_peerThread;

	ConnectionManagerPtr		m_ptrConnMgr;

	MessageQueuePtr				m_pMsgQ;

	PeerNodePtr					m_pCoordinatorNode;
	std::vector<PeerNodePtr>	m_peerNodeList;
	std::map<int, TaskPtr>		m_taskList;		// tasks which are sent to other peers
	std::map<int, JobPtr>		m_jobList;		// tasks which came from others
	//std::map<PeerNodePtr, JobPtr>		m_jobList;		// tasks which came from others

	int							m_lastTaskId;	// just needed to generate task ids
	int							m_lastJobId;	// just needed to generate job ids

	TimerMessagePtr				m_pTimerMessages;

	boost::recursive_mutex				m_mutex;
	boost::recursive_mutex				m_taskMutex;

	Peer();
	void run();
	void ProcessMessages(WMessagePtr pMsg);
	void HandleResponses(WMessagePtr pMsg);
	void HandleRequests(WMessagePtr pMsg);
	void HandleSysMessages(WMessagePtr pMsg);

	void HandleTaskResult(WMessagePtr pMsg);
	void HandleTaskRequest(MessagePtr pMsg);
	void HandleTaskExecute(MessagePtr pMsg);
	void HandleTaskCancel(MessagePtr pMsg);
	void HandleOnlineResp(MessagePtr pMsg);

	void HandleJobExecution(WMessagePtr pMsg);	// called when job executed has result available

	void HandlePeerList(std::vector<RemotePeers> plist, long timestamp);

	void HandleConnectionFailure(WMessagePtr pMsg);

	int ReadConfigFile(std::string configFile, PeerNodePtr pCoordNode);

	int CreateJob(int remotePeer, int taskId, std::map<std::string, std::string> taskInfo, PeerNodePtr pNode);
	JobPtr GetJob(int peerId, int taskId, PeerNodePtr pNode = PeerNodePtr());

	int GetRandomTaskId() { return ++m_lastTaskId; }
	int GetRandomJobId() { return ++m_lastJobId; }
	
public:
	static PeerPtr GetPeer();

public:
	~Peer();

	void MessageEvent(SYS_MSG msg);	// function which receives the timer events

	void SetPort(int port) { m_port = port; }
	int GetPort() { return m_port; }

	long GetPeerListTimestamp() { return m_db->GetPeerListTimestamp(); }
	std::vector<RemotePeers>& GetPeerList(unsigned long& ts) { return m_db->GetPeerList(ts); }
	MessageQueuePtr GetMessageQueue() { return m_pMsgQ; }

	int GetPeerId() { return m_peerId; }
	void SetPeerId(int id) { m_peerId = id; }

	RemotePeers GetRemotePeerById(int peerId);
	void AddPeerNode(PeerNodePtr pNode);
	void RemovePeerNode(PeerNodePtr pNode);

	void PutMessage(WMessagePtr pWMsg) { m_pMsgQ->PutMessage(pWMsg); }
	void Start(std::string configFile);
	void Kill();

	int ConnectToCoordinator();

	// handles the new connection request from remote peers
	void HandleConnectionRequest(ConnectionPtr pConn);

	// creates new task and processes it
	int AddTask();

	std::vector<std::string> ReadTaskConfig(std::string configFile);

	// test methods
	void TestSendMesgs(std::string msg);
	void TestTask();
};

#endif