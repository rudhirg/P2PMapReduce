#ifndef JOB_H
#define JOB_H

#include "Log.h"
#include "Database.h"

#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>
#include <boost/enable_shared_from_this.hpp>

#include <string>

enum JOB_STATUS{
	JOB_INIT,
	JOB_RUNNING,
	JOB_FINSHED
};

class Job;

typedef boost::shared_ptr<Job> JobPtr;

class Job : public boost::enable_shared_from_this<Job> {
	
private:
	int							m_JobId;
	int							m_remotePeerId;
	int							m_taskId;
	JOB_STATUS					m_status;
	bool						m_bResult;
	double						m_executionTime;
	std::vector<std::string>	m_result;

	PeerNodePtr					m_pPeer;

	boost::thread				m_JobThread;
	boost::recursive_mutex				m_mutex;
	std::string					m_param;

	void run();
	int exec(char* cmd, std::vector<string>& result);

public:
	Job(int JobId);
	~Job() { Log(CONSOLE, L"Deleting Job: %d\n", m_JobId); }

	int GetJobId() { return m_JobId; }
	JOB_STATUS GetStatus();
	int GetRemotePeerId() { return m_remotePeerId; }
	void SetRemotePeerId(int id) { m_remotePeerId = id; }
	int GetTaskId() { return m_taskId; }
	void SetTaskId(int id) { m_taskId = id; }
	double GetExecutionTime() { return m_executionTime; }
	bool GetSuccessStatus() { return m_bResult; }
	std::vector<std::string>& GetResult() { return m_result; }
	void SetParam(std::string param) { m_param = param; }
	std::string GetParam() { return m_param; }
	PeerNodePtr GetPeerNode() { return m_pPeer; }
	void SetPeerNode(PeerNodePtr peer) { m_pPeer = peer; } 

	// starts the Job process by first getting all peers
	int Start();
	// stops
	void Stop();
};
#endif