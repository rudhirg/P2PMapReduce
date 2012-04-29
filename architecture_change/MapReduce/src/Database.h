#ifndef DB_H
#define DB_H

#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>
#include "Message.h"

class Database;

typedef boost::shared_ptr<Database> DatabasePtr;

struct PeerList {
	std::list<RemotePeers>	m_peerList;
	long					m_timestamp;
};

class Database {
	PeerList				m_peerList;
	std::string				m_coordIp;
	int						m_coordPort;

public:
	void SetPeerList(std::list<RemotePeers> list) { m_peerList.m_peerList = list; }
	std::list<RemotePeers>& GetPeerList() { return m_peerList.m_peerList; }

	void SetCoordinatorIp(std::string ip) { m_coordIp = ip; }
	std::string GetCoordinatorIp() { return m_coordIp; }

	void SetCoordinatorPort(int ip) { m_coordPort = ip; }
	int GetCoordinatorPort() { return m_coordPort; }
};

#endif