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

enum SYS_MSG {
	NONE = 0
};

enum MSG_TYPE {
	REQ = 0,
	RESP
};

enum REQ_TYPE {
	PING = 0,
	GET_STATE,
	TASK_EXECUTE,
	TASK_STATE,
	TASK_REQUEST,
	TASK_RESULT,
	TASK_CANCEL,
	PEER_LIST,
	GET_OFFLINE,
	GET_ONLINE
};

enum RESP_TYPE {
	OK = 0,
	GET_STATE_RESP,
	TASK_EXECUTE_RESP,
	TASK_STATE_RESP,
	TASK_REQUEST_RESP,
	PEER_LIST_RESP,
	GET_OFFLINE_RESP,
	GET_ONLINE_RESP
};

struct RemotePeers {
	int		peer_id;
	std::string ip;
	int port;
	RemotePeers() { peer_id=0, port=0;}
};

class WMessage;
class Message;
class ResponseMessage;
class RequestMessage;

typedef boost::shared_ptr<WMessage> WMessagePtr;
typedef boost::shared_ptr<Message> MessagePtr;
typedef boost::shared_ptr<RequestMessage> RequestMessagePtr;
typedef boost::shared_ptr<ResponseMessage> ResponseMessagePtr;

// the message that the system uses
class WMessage {
public:
	MessagePtr				m_msg;
	SYS_MSG					m_sysMsg;

	void SetSysMsg(SYS_MSG msg) { m_sysMsg = msg; }
	SYS_MSG GetSysMsg() { return m_sysMsg; }
	void SetMsg(MessagePtr pMsg) { m_msg = pMsg; }
	MessagePtr GetMsg() { return m_msg; }
};

class Message {
public:
	MSG_TYPE				m_mesgType;
	bool					m_status;		// tells the status of the response (success/failure)
	REQ_TYPE				m_reqType;
	RESP_TYPE				m_respType;
	int						m_fromPeerId;
	int						m_taskId;
	int						m_listenPort;
	std::string				m_mesg;
	ConnectionPtr			m_conn;
	MessageMap				m_taskMap;
	MessageMap				m_stateMap;
	unsigned long					m_timestamp;	// only used for peerlist
	std::vector<RemotePeers>	m_peerList;

	std::string				m_xml;

	Message() : m_taskId(0), m_fromPeerId(0), m_listenPort(0) {}
	void SetTaskId(int id) { m_taskId = id; }

	bool GetStatus() { return m_status; }
	bool IsTaskMessage() {
		if(this->m_mesgType == REQ) {
			if(this->m_reqType == TASK_EXECUTE || this->m_reqType ==TASK_STATE || this->m_reqType == TASK_REQUEST
									|| this->m_reqType == TASK_CANCEL)
				return true;
		}
		else {
			if(this->m_reqType == TASK_EXECUTE_RESP || this->m_reqType ==TASK_STATE_RESP 
														|| this->m_reqType == TASK_REQUEST_RESP)
				return true;
		}
		return false;
	}
	
	static std::vector<RemotePeers> GetPeerList(pugi::xml_node& node);
	static MessagePtr ParseMessage(std::string xml);
};

class RequestMessage : public Message {
public:
	void AddGeneralHeader(REQ_TYPE type, int fromPeerId);
	void AddTaskInfo(MessageMap& valMaps);
	void AddStateInfo(MessageMap& valMaps);

	std::string GetXML();

	static MessagePtr ParseRequestMessage(std::string xml);
};

class ResponseMessage : public Message {
public:
	void AddGeneralHeader(RESP_TYPE type, int fromPeerId, bool status);
	void AddPeerListTimestamp(unsigned long ts);
	void AddPeerList(std::vector<RemotePeers>& peers);
	void AddTaskInfo(MessageMap& valMaps);
	void AddStateInfo(MessageMap& valMaps);

	std::string GetXML();

	static MessagePtr ParseResponseMessage(std::string xml);
};

#endif