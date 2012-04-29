#ifndef TASK_H
#define TASK_H

#include "Log.h"
#include "Database.h"

#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>
#include <boost/enable_shared_from_this.hpp>

#include <string>

class Task;

typedef boost::shared_ptr<Task> TaskPtr;

class Task : public boost::enable_shared_from_this<Task> {
	
private:
	int							m_taskId;
	int							m_numRequiredPeers;	// total peers to distribute work to
	int							m_numRemainingPeers;	// total peers remaining to be tried
	int							m_numMaxToTryPeers;	// max num of peers to try before threshold lowers
	int							m_numTotalPeersTried;

	double						m_highThreshold;
	double						m_mediumThreshold;
	double						m_currentThreshold;

	std::map<int, PeerNodePtr>	m_peerParamMap;
	std::map<int, PeerNodePtr>	m_tryPeerParamMap;

	std::map<PeerNodePtr, bool>			m_peerResultAvailMap;	// tells whether result from peer has been recvd or not

	//std::vector<RemotePeers>	m_peerNodes;
	std::vector<PeerNodePtr>	m_taskNodes;	// final nodes where task is running
	std::vector<PeerNodePtr>	m_tryNodes;	// nodes to request to

	std::vector<std::string>	m_paramList;
	std::vector<int>			m_unAllotParamList;

	std::vector<std::vector<std::string> >	m_resultList;
	std::map<int, std::vector<std::string> >	m_paramResultMap;

	int							m_numResultsRecvd;	// num of peers from where the results have been recvd

	boost::thread				m_taskThread;
	boost::recursive_mutex		m_mutex;

	MessageQueuePtr				m_pMsgQ;

	bool						m_bStop;

	int AskPeers();

	void run();
	void ProcessMessages(WMessagePtr pMsg);
	void HandleSysMessages(WMessagePtr pMsg);

	void HandlePeerConnFailure(WMessagePtr pMsg);

	// cancels this task, sends task cancel requests
	void TaskCancel();

	void RemovePeerFromTryNodes(PeerNodePtr pPeer);
	void RemovePeerFromFinalNodes(PeerNodePtr pPeer);
	bool IsInTryNodes(int peerId);
	// see whether this instance of peer connection instance belong to this task
	bool IsPeerInstanceBelongToTask(PeerNodePtr pPeer);

	double CalculatePeerEligibility(MessagePtr pMsg);

public:
	Task(int taskId);
	~Task() {}

	int GetTaskId() { return m_taskId; }
	MessageQueuePtr GetMessageQueue() { return m_pMsgQ; }

	bool IsTaskComplete();

	// starts the task process by first getting all peers
	void AddTaskParams(std::vector<std::string> params);
	int Start();
	void Stop() { m_bStop = true; }

	void HandleTaskResult(MessagePtr pMsg);
	void HandleTaskRequestResponse(MessagePtr pMsg);
	void HandleTaskExecuteResponse(MessagePtr pMsg);
};

#endif