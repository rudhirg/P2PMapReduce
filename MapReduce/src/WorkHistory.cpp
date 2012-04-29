#include "WorkHistory.h"

WorkHistoryPtr WorkHistory::m_pWorkHist;

WorkHistoryPtr WorkHistory::GetWorkHistory()
{
	if(m_pWorkHist == NULL) {
		WorkHistoryPtr ptr(new WorkHistory());
		m_pWorkHist = ptr;
	}
	return m_pWorkHist;
}

WorkHistory::WorkHistory()
{
	m_avgExecutionTime		= 0.0;
	m_currentWorkLoad		= 0;
	m_numTaskHandled		= 0;
	m_totalExecutionTime	= 0.0;
	m_numTaskFailed			= 0;
}	

void WorkHistory::SetNewFinishTaskInfo(double time, bool bstatus){
	boost::recursive_mutex::scoped_lock L(m_mutex);
	m_totalExecutionTime += time;
	m_numTaskHandled++;
	if(!bstatus) m_numTaskFailed++;
}

double WorkHistory::GetAvgExecuteTime()
{
	boost::recursive_mutex::scoped_lock L(m_mutex);
	return (m_numTaskHandled == 0.0 ? 0.0 : m_totalExecutionTime/m_numTaskHandled);
}

void WorkHistory::IncrementWorkLoad()
{
	boost::recursive_mutex::scoped_lock L(m_mutex);
	m_currentWorkLoad++;
}

void WorkHistory::DecrementWorkLoad()
{
	boost::recursive_mutex::scoped_lock L(m_mutex);
	m_currentWorkLoad--;
}

int WorkHistory::GetCurrentWorkLoad()
{
	boost::recursive_mutex::scoped_lock L(m_mutex);
	return m_currentWorkLoad;
}

int WorkHistory::GetNumTaskHandled()
{
	boost::recursive_mutex::scoped_lock L(m_mutex);
	return m_numTaskHandled;
}

int WorkHistory::GetNumTaskFailed()
{
	boost::recursive_mutex::scoped_lock L(m_mutex);
	return m_numTaskFailed;
}
