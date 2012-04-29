#include "Database.h"

int Database::Init()
{
	int result = sqlite3_open("coordinator.db", &m_pDB);
	if(result) {
		Log(ERR, L"Connection to Database Failed!!\n");
		return -1;
	}
	InsertTimestamp();
	return result;
}

int Database::InsertTimestamp()
{
	boost::recursive_mutex::scoped_lock L(m_mutex);
	int result = 0;
	std::stringstream ss;

	unsigned long ts = ::GetTickCount();

	unsigned long oldts = this->GetTimestamp();
	if(oldts == 0) {
		ss << "insert into timestamp values (" << 1 << "," << ts << ");";
	} else {
		ss << "update timestamp set ts = " << ts << " where id=1";
	}

	std::string insertStr = ss.str();
	result = executeQuery(insertStr);
	if(result != 0) {
		Log(ERR, L"DataBase:: Timestamp Could not be added!!\n");
		return -1;
	}

	return result;
}

int Database::UpdateTimestamp()
{
	boost::recursive_mutex::scoped_lock L(m_mutex);
	int result = 0;
	std::stringstream ss;

	unsigned long ts = ::GetTickCount();

	ss << "update timestamp set ts = " << ts << " where id=1";

	std::string insertStr = ss.str();
	result = executeQuery(insertStr);
	if(result != 0) {
		Log(ERR, L"DataBase:: Timestamp Could not be updated!!\n");
		return -1;
	}

	return result;
}

long Database::GetTimestamp()
{
	boost::recursive_mutex::scoped_lock L(m_mutex);
	int result = 0;
	sqlite3_stmt *stmt = 0;
	char* tail = NULL;
	std::stringstream ss;
	ss << "select ts from timestamp where id=1";

	std::string selStr = ss.str();

	result = sqlite3_prepare_v2(m_pDB, selStr.c_str(), strlen(selStr.c_str())+1, &stmt, (const char**)&tail);

	if(result != 0) {
		Log(ERR, L"DataBase:: Error executing query!!\n");
		return 0;
	}
	
	unsigned long ts = 0;
	int cols = sqlite3_column_count(stmt);
	while(true) {
		result = sqlite3_step(stmt);
		if(result == SQLITE_ROW) {
			vector<string> values;
			for(int col = 0; col < cols; col++) {
				ts = sqlite3_column_int64(stmt, col);
			}
		} else {
			break;
		}
	}
	sqlite3_finalize(stmt);

	return ts;
}

int Database::AddPeer(RemotePeers peer) { // also check whether it exists/update
	boost::recursive_mutex::scoped_lock L(m_mutex);
	int result = 0;
	std::stringstream ss;

	//[TODO] frst check whether its already there?
	RemotePeers rpeer = this->GetPeer(peer.peer_id);
	if(rpeer.peer_id == 0) {
		ss << "insert into Users values (" << peer.peer_id << "," << "\"" << peer.ip << "\"," << peer.port << ");";
	} else {
		ss << "update Users set ip = \"" << peer.ip << "\"," << "port=" << peer.port << " where id=" << peer.peer_id;
	}

	std::string insertStr = ss.str();
	result = executeQuery(insertStr);
	if(result != 0) {
		Log(ERR, L"DataBase:: Peer Could not be added!!\n");
		return -1;
	}

	// update the timestamp with new value
	this->UpdateTimestamp();

	return result;
}

int Database::RemovePeer(RemotePeers peer)
{
	boost::recursive_mutex::scoped_lock L(m_mutex);
	int result = 0;
	std::stringstream ss;
	ss << "delete from Users where id = "<< peer.peer_id;

	std::string delStr = ss.str();
	result = this->executeQuery(delStr);
	if(result != 0) {
		Log(ERR, L"DataBase:: Peer Could not be removed!!\n");
		return -1;
	}

	this->UpdateTimestamp();

	return result;
}

RemotePeers Database::GetPeer(int peerid)
{
	boost::recursive_mutex::scoped_lock L(m_mutex);
	RemotePeers peer;
	int result = 0;
	sqlite3_stmt *stmt = 0;
	char* tail = NULL;
	std::stringstream ss;
	ss << "select * from Users where id = " << peerid;

	std::string selStr = ss.str();

	result = sqlite3_prepare_v2(m_pDB, selStr.c_str(), strlen(selStr.c_str())+1, &stmt, (const char**)&tail);

	if(result != 0) {
		Log(ERR, L"DataBase:: Error executing query!!\n");
		return peer;
	}
	
	vector<string> res;
	int cols = sqlite3_column_count(stmt);
	while(true) {
		result = sqlite3_step(stmt);
		if(result == SQLITE_ROW) {
			vector<string> values;
			for(int col = 0; col < cols; col++) {
				values.push_back((char*)sqlite3_column_text(stmt, col));
			}
			res = values;
		} else {
			break;
		}
	}
	sqlite3_finalize(stmt);

	// process results
	if(res.empty() == false) {
		peer.peer_id = atoi(res[0].c_str());
		peer.ip = (res[1]);
		peer.port = atoi(res[2].c_str());
	}

	return peer;
}

std::vector<RemotePeers> Database::GetPeerList(unsigned long& ts) 
{ 
	std::vector<RemotePeers> peers;
	boost::recursive_mutex::scoped_lock L(m_mutex);
	int result = 0;
	sqlite3_stmt *stmt = 0;
	char* tail = NULL;
	std::stringstream ss;
	ss << "select * from Users";

	std::string selStr = ss.str();

	result = sqlite3_prepare_v2(m_pDB, selStr.c_str(), strlen(selStr.c_str())+1, &stmt, (const char**)&tail);

	if(result != 0) {
		Log(ERR, L"DataBase:: Error executing query!!\n");
		return peers;
	}
	
	vector<vector<string> > results;
	int cols = sqlite3_column_count(stmt);
	while(true) {
		result = sqlite3_step(stmt);
		if(result == SQLITE_ROW) {
			vector<string> values;
			for(int col = 0; col < cols; col++) {
				values.push_back((char*)sqlite3_column_text(stmt, col));
			}
			results.push_back(values);
		} else {
			break;
		}
	}
	sqlite3_finalize(stmt);

	// process results
	for(int i = 0; i < results.size(); i++) {
		RemotePeers peer;
		peer.peer_id = atoi(results[i][0].c_str());
		peer.ip = (results[i][1]);
		peer.port = atoi(results[i][2].c_str());
		peers.push_back(peer);
	}

	ts = this->GetTimestamp();

	return peers;
}

int Database::executeQuery(std::string query)
{
	boost::recursive_mutex::scoped_lock L(m_mutex);
	int result = 0;
	sqlite3_stmt *stmt = 0;
	char* tail = NULL;

	result = sqlite3_prepare_v2(m_pDB, query.c_str(), strlen(query.c_str())+1, &stmt, (const char**)&tail);

	if(result != 0) {
		Log(ERR, L"DataBase:: Error executing query!!\n");
		return -1;
	}

	if(sqlite3_step(stmt) != SQLITE_DONE) {
		Log(ERR, L"DataBase:: Error executing query!!\n");
		return -1;
	}

	sqlite3_finalize(stmt);
	return result;
}