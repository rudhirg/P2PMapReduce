#ifndef MESG_H
#define MESG_H

#include <pugixml.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>

#include "Log.h"

#include <string>
#include <sstream>

using namespace std;

class Connection;
typedef boost::shared_ptr<Connection> ConnectionPtr;
typedef std::map<std::string, std::string> MessageMap;

enum MSG_TYPE {
	REQ = 0,
	RESP
};

enum REQ_TYPE {
	PING = 0,
	GET_STATE,
	TASK_EXECUTE,
	TASK_STATE,
	TASK_DETAILS,
	PEER_LIST,
	GET_OFFLINE,
	GET_ONLINE
};

enum RESP_TYPE {
	OK = 0,
	GET_STATE_RESP,
	TASK_EXECUTE_RESP,
	TASK_STATE_RESP,
	TASK_DETAILS_RESP,
	PEER_LIST_RESP,
	GET_OFFLINE_RESP,
	GET_ONLINE_RESP
};

struct RemotePeers {
	int		peer_id;
	std::string ip;
};

class Message;
class ResponseMessage;
class RequestMessage;

typedef boost::shared_ptr<Message> MessagePtr;
typedef boost::shared_ptr<RequestMessage> RequestMessagePtr;
typedef boost::shared_ptr<ResponseMessage> ResponseMessagePtr;

class Message {
public:
	MSG_TYPE				m_mesgType;
	int						m_fromPeerId;
	std::string				m_mesg;
	ConnectionPtr			m_conn;
	MessageMap				m_taskMap;
	MessageMap				m_stateMap;
	std::list<RemotePeers>	m_peerList;

	std::string				m_xml;

	static std::list<RemotePeers> GetPeerList(pugi::xml_node& node);

	static MessagePtr ParseMessage(std::string xml);
};

class RequestMessage : public Message {
public:
	REQ_TYPE				m_type;
	std::string				data;

	void AddGeneralHeader(REQ_TYPE type, int fromPeerId);
	void AddTaskInfo(MessageMap& valMaps);
	void AddStateInfo(MessageMap& valMaps);

	std::string GetXML();

	static MessagePtr ParseRequestMessage(std::string xml);
};

class ResponseMessage : public Message {
public:
	RESP_TYPE				m_type;
	std::string				data;

	void AddGeneralHeader(RESP_TYPE type, int fromPeerId);
	void AddTaskInfo(MessageMap& valMaps);
	void AddStateInfo(MessageMap& valMaps);

	std::string GetXML();

	static MessagePtr ParseResponseMessage(std::string xml);
};

#endif