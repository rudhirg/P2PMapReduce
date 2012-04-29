#ifndef DB_H
#define DB_H

#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>
#include "Message.h"

class Database;

typedef boost::shared_ptr<Database> DatabasePtr;

struct PeerList {
	std::vector<RemotePeers>	m_peerList;
	unsigned long					m_timestamp;

	PeerList() { m_timestamp = 0; }
};

class Database {
	PeerList				m_peerList;
	std::string				m_coordIp;
	int						m_coordPort;
	boost::recursive_mutex			m_mutex;

public:
	void AddPeer(RemotePeers peer) { m_peerList.m_peerList.push_back(peer); }
	void SetPeerList(std::vector<RemotePeers> list) { 
		boost::recursive_mutex::scoped_lock L(m_mutex);
		m_peerList.m_peerList = list; 
	}
	void SetPeerList(std::vector<RemotePeers> list, unsigned long ts) { 
		boost::recursive_mutex::scoped_lock L(m_mutex);
		m_peerList.m_peerList = list; 
		m_peerList.m_timestamp=ts; 
	}
	std::vector<RemotePeers>& GetPeerList(unsigned long& ts) { 
		boost::recursive_mutex::scoped_lock L(m_mutex);
		ts = m_peerList.m_timestamp;
		return m_peerList.m_peerList; 
	}

	void SetCoordinatorIp(std::string ip) { m_coordIp = ip; }
	std::string GetCoordinatorIp() { return m_coordIp; }

	void SetCoordinatorPort(int ip) { m_coordPort = ip; }
	int GetCoordinatorPort() { return m_coordPort; }

	unsigned long GetPeerListTimestamp() { 
		boost::recursive_mutex::scoped_lock L(m_mutex);
		return m_peerList.m_timestamp; 
	}
};

#endif