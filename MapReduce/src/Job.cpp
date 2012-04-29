#include "Peer.h"
#include "Job.h"

#include <boost/timer.hpp>

#include <cmath>

Job::Job(int JobId)
{
	m_JobId = JobId;
	m_status = JOB_INIT;
}

int Job::Start()
{
	m_JobThread = boost::thread(&Job::run, this);
	return 0;
}

void Job::Stop()
{
	Log(CONSOLE, L"Cancelling Job: %d, for Peer: %d (taskId: %d)\n", m_JobId, m_remotePeerId, m_taskId);
	m_JobThread.interrupt();
}

JOB_STATUS Job::GetStatus() 
{ 
	boost::recursive_mutex::scoped_lock L(m_mutex);
	return m_status; 
}

// process message queue
void Job::run()
{
	{
		boost::recursive_mutex::scoped_lock L(m_mutex);
		m_status = JOB_RUNNING;
	}

	Log(CONSOLE, L"Running Job: %d, for Peer: %d (taskId: %d) with Parameters: %s\n", m_JobId, 
		m_remotePeerId, m_taskId, m_param.c_str());
	
	// start the timer here
	boost::timer t; // start timing

	// run the job here

	std::vector<string> result;
	std::string commandStr = "grep -irH peernode " + m_param;
	int status = exec((char*)commandStr.c_str(), result);

	Sleep(5000);

	m_bResult = (status >= 0 ? true : false);
	m_result.clear();
	if(result.size() > 0)
		m_result = result;
	{
		boost::recursive_mutex::scoped_lock L(m_mutex);
		m_status = JOB_FINSHED;
	}

	//end timer here
	double elapsed_time = t.elapsed();
	m_executionTime = elapsed_time;
	
	// when it gets done, it message to main thread (peer) that it finished with the results
	// the main thread sends the job results to the remote peer
	Log(CONSOLE, L"Successfully Executed Job: %d, for Peer: %d (taskId: %d)\n", m_JobId, m_remotePeerId, m_taskId);

	PeerPtr pPeer = Peer::GetPeer();
	WMessagePtr pWMsg(new WMessage);
	pWMsg->SetSysMsg(TASK_EXECUTION_SUCCESS);
	pWMsg->SetJobId(m_JobId);
	pPeer->PutMessage(pWMsg);
}

int Job::exec(char* cmd, std::vector<string>& result) {
    FILE* pipe = _popen(cmd, "r");
    if (!pipe) return -1;
    char buffer[128];
    while(!feof(pipe)) {
        if(fgets(buffer, 128, pipe) != NULL)
			result.push_back(buffer);
    }
    _pclose(pipe);
    return 0;
}



