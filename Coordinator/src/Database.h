#ifndef DB_H
#define DB_H

#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>
#include "Message.h"
extern "C" {
#include "sqlite3.h"
}

class Database;

typedef boost::shared_ptr<Database> DatabasePtr;

//struct PeerList {
//	std::vector<RemotePeers>	m_peerList;
//	unsigned long				m_timestamp;
//};

class Database {
	sqlite3*				m_pDB;
	//PeerList				m_peerList;
	boost::recursive_mutex	m_mutex;

public:
	int Init();

	std::vector<RemotePeers> GetPeerList(unsigned long& ts);/* { 
		boost::recursive_mutex::scoped_lock L(m_mutex);
		ts = m_peerList.m_timestamp;
		return m_peerList.m_peerList; 
	}*/

	int AddPeer(RemotePeers peer); /*{ // also check whether it exists/update
		boost::recursive_mutex::scoped_lock L(m_mutex);
		bool bfound = false;
		for(int i = 0;i < m_peerList.m_peerList.size();i++)
			if( m_peerList.m_peerList[i].peer_id == peer.peer_id ) {
				m_peerList.m_peerList[i] = peer;
				bfound = true;
				break;
			}
		if(!bfound)	m_peerList.m_peerList.push_back(peer); 
		unsigned long ts = ::GetTickCount();
		m_peerList.m_timestamp = ts;
	}*/

	int RemovePeer(RemotePeers peer); /*{
		boost::recursive_mutex::scoped_lock L(m_mutex);
		unsigned long ts = ::GetTickCount();
		m_peerList.m_timestamp = ts;
		bool bfound = false;
		int index = -1;
		for(int i = 0;i < m_peerList.m_peerList.size();i++)
			if( m_peerList.m_peerList[i].peer_id == peer.peer_id ) {
				m_peerList.m_peerList[i] = peer;
				bfound = true;
				index = i;
				break;
			}
			if(bfound)	m_peerList.m_peerList.erase(m_peerList.m_peerList.begin() + index); 
	}*/

	RemotePeers GetPeer(int peerid);

	int InsertTimestamp();
	int UpdateTimestamp();
	long GetTimestamp();

	int executeQuery(std::string query);
};

#endif