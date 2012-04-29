#ifndef WORK_HIST_H
#define WORK_HIST_H

#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>
#include <boost/enable_shared_from_this.hpp>

#include <string>

class WorkHistory;

typedef boost::shared_ptr<WorkHistory> WorkHistoryPtr;

class WorkHistory {
private:
	static WorkHistoryPtr	m_pWorkHist;

	double				m_totalExecutionTime;
	double				m_avgExecutionTime;
	int					m_currentWorkLoad;
	int					m_numTaskHandled;
	int					m_numTaskFailed;
	
	boost::recursive_mutex		m_mutex;

	WorkHistory();

public:
	static WorkHistoryPtr GetWorkHistory();

public:
	~WorkHistory() {}

	void SetNewFinishTaskInfo(double time, bool bstatus);
	double GetAvgExecuteTime();

	void IncrementWorkLoad();
	void DecrementWorkLoad();
	int GetCurrentWorkLoad();

	int GetNumTaskHandled();
	int GetNumTaskFailed();

};

#endif