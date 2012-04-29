#include "Connection.h"
#include "Message.h"
#include <boost/lexical_cast.hpp>

void RequestMessage::AddGeneralHeader(REQ_TYPE type, int fromPeerId)
{
	this->m_reqType = type;
	this->m_fromPeerId = fromPeerId;
	this->m_listenPort = 0;
}

void RequestMessage::AddTaskInfo(MessageMap& valMaps)
{
	this->m_taskMap = valMaps;
}

void RequestMessage::AddStateInfo(MessageMap& valMaps)
{
	this->m_stateMap = valMaps;
}

std::string RequestMessage::GetXML()
{
	if(m_xml.empty() == false) return m_xml;

	string ss = "";
	ss = boost::lexical_cast<string>(this->m_reqType);
	pugi::xml_document doc;
	pugi::xml_node root = doc.append_child("Request");
	// add header
	pugi::xml_node type = root.append_child("type");
	type.append_child(pugi::node_pcdata).set_value(ss.c_str());

	ss = boost::lexical_cast<string>( this->m_fromPeerId );
	pugi::xml_node from = root.append_child("frompeer");
	from.append_child(pugi::node_pcdata).set_value(ss.c_str());

	if( m_taskId > 0 ) {
		ss = boost::lexical_cast<string>(this->m_taskId);
		pugi::xml_node from = root.append_child("taskid");
		from.append_child(pugi::node_pcdata).set_value(ss.c_str());
	}

	// add task
	if( m_taskMap.size() > 0 ) {
		pugi::xml_node tasknode = root.append_child("task");
		for( MessageMap::iterator it = m_taskMap.begin(); it != m_taskMap.end(); it++ ) {
			pugi::xml_node info = tasknode.append_child((*it).first.c_str());
			info.append_child(pugi::node_pcdata).set_value((*it).second.c_str());
		}
	}
	// add state
	if( m_stateMap.size() > 0 ) {
		pugi::xml_node statenode = root.append_child("state");
		for( MessageMap::iterator it = m_stateMap.begin(); it != m_stateMap.end(); it++ ) {
			pugi::xml_node info = statenode.append_child((*it).first.c_str());
			info.append_child(pugi::node_pcdata).set_value((*it).second.c_str());
		}
	}

	std::stringstream xml;
	doc.save(xml);
	this->m_xml = xml.str();
	return m_xml;
}

void ResponseMessage::AddGeneralHeader(RESP_TYPE type, int fromPeerId, bool status)
{
	this->m_status = status;
	this->m_respType = type;
	this->m_fromPeerId = fromPeerId;
}

void ResponseMessage::AddTaskInfo(MessageMap& valMaps)
{
	this->m_taskMap = valMaps;
}

void ResponseMessage::AddStateInfo(MessageMap& valMaps)
{
	this->m_stateMap = valMaps;
}

void ResponseMessage::AddPeerListTimestamp(unsigned long ts)
{
	m_timestamp = ts;
}

void ResponseMessage::AddPeerList(std::vector<RemotePeers>& peers)
{
	m_peerList = peers;
}

std::string ResponseMessage::GetXML()
{
	if(m_xml.empty() == false) return m_xml;

	string ss = "";
	ss = boost::lexical_cast<string>(this->m_respType);
	pugi::xml_document doc;
	pugi::xml_node root = doc.append_child("Response");
	// add header
	pugi::xml_node type = root.append_child("type");
	type.append_child(pugi::node_pcdata).set_value(ss.c_str());

	ss = boost::lexical_cast<string>(this->m_status);
	pugi::xml_node status = root.append_child("status");
	status.append_child(pugi::node_pcdata).set_value(ss.c_str());
	
	ss = boost::lexical_cast<string>(this->m_fromPeerId);
	pugi::xml_node from = root.append_child("frompeer");
	from.append_child(pugi::node_pcdata).set_value(ss.c_str());

	if( m_timestamp > 0 ) {
		ss = boost::lexical_cast<string>(this->m_timestamp);
		pugi::xml_node ts = root.append_child("timestamp");
		ts.append_child(pugi::node_pcdata).set_value(ss.c_str());
	}

	// add the peer-list
	if( m_peerList.size() > 0 ) {
		pugi::xml_node peerList = root.append_child("peerlist");
		for (int i = 0; i < m_peerList.size(); i++ ) {
			pugi::xml_node peer = peerList.append_child("peer");
			// add the peer info
			ss = boost::lexical_cast<string>(m_peerList[i].peer_id);
			pugi::xml_node peerId = peer.append_child("peerid");
			peerId.append_child(pugi::node_pcdata).set_value(ss.c_str());
			pugi::xml_node peerIp = peer.append_child("peerip");
			peerIp.append_child(pugi::node_pcdata).set_value(m_peerList[i].ip.c_str());
			ss = boost::lexical_cast<string>(m_peerList[i].port);
			pugi::xml_node peerPort = peer.append_child("peerport");
			peerPort.append_child(pugi::node_pcdata).set_value(ss.c_str());
		}
	}

	if( m_taskId > 0 ) {
		ss = boost::lexical_cast<string>(this->m_taskId);
		pugi::xml_node from = root.append_child("taskid");
		from.append_child(pugi::node_pcdata).set_value(ss.c_str());
	}

	// add task
	if( m_taskMap.size() > 0 ) {
		pugi::xml_node tasknode = root.append_child("task");
		for( MessageMap::iterator it = m_taskMap.begin(); it != m_taskMap.end(); it++ ) {
			pugi::xml_node info = tasknode.append_child((*it).first.c_str());
			info.append_child(pugi::node_pcdata).set_value((*it).second.c_str());
		}
	}
	// add state
	if( m_stateMap.size() > 0 ) {
		pugi::xml_node statenode = root.append_child("state");
		for( MessageMap::iterator it = m_stateMap.begin(); it != m_stateMap.end(); it++ ) {
			pugi::xml_node info = statenode.append_child((*it).first.c_str());
			info.append_child(pugi::node_pcdata).set_value((*it).second.c_str());
		}
	}

	std::stringstream xml;
	doc.save(xml);
	this->m_xml = xml.str();
	return m_xml;
}

MessagePtr Message::ParseMessage(std::string xml)
{
	std::stringstream ss(xml);

	MessagePtr req = RequestMessage::ParseRequestMessage(xml);
	if( req != NULL ) {
		return req;
	}

	req = ResponseMessage::ParseResponseMessage(xml);
	return req;
}

MessagePtr RequestMessage::ParseRequestMessage(std::string xml)
{
	std::stringstream ss(xml);
	
	pugi::xml_document doc;
	pugi::xml_parse_result result = doc.load(ss);

	if( result.status != pugi::status_ok ) {
		Log(ERR, L"Request Parse Error\n");
		return RequestMessagePtr();
	}

	pugi::xml_node node = doc.child("Request");
	if( node == NULL )
		return RequestMessagePtr();

	pugi::xml_node cnode = node;
	cnode = cnode.child("type");
	if( cnode == NULL )
		return RequestMessagePtr();
	std::string type = cnode.child_value();

	//[TODO] parse all other information too

	cnode = node;
	cnode = cnode.child("frompeer");
	if( cnode == NULL )
		return RequestMessagePtr();
	std::string peerid = cnode.child_value();

	cnode = node;
	cnode = cnode.child("taskid");
	std::string taskid = "0";
	if( cnode != NULL ) {
		taskid = cnode.child_value();
	}

	cnode = node;
	cnode = cnode.child("listenport");
	if( cnode == NULL )
		return RequestMessagePtr();
	std::string listenport = cnode.child_value();

	// make the request and add values
	RequestMessagePtr req(new RequestMessage);
	req->m_mesg = xml;
	req->m_reqType = (REQ_TYPE)atoi(type.c_str());
	req->m_mesgType = REQ;
	req->m_fromPeerId = atoi(peerid.c_str());
	req->m_listenPort = atoi(listenport.c_str());
	req->m_taskId = atoi(taskid.c_str());

	return req;
}

std::vector<RemotePeers> Message::GetPeerList(pugi::xml_node& node)
{
	pugi::xml_node pnode = node;
	std::vector<RemotePeers> peers;

	for (pugi::xml_node pnode = node.child("peer"); pnode; pnode = pnode.next_sibling("peer")) {
		pugi::xml_node cnode = pnode.child("peerid");
		if( cnode == NULL )
			break;
		std::string peerid = cnode.child_value();

		cnode = pnode.child("peerip");
		if( cnode == NULL )
			break;
		std::string peerip = cnode.child_value();

		cnode = pnode.child("peerport");
		if( cnode == NULL )
			break;
		std::string peerport = cnode.child_value();

		RemotePeers peer;
		peer.ip = peerip;
		peer.peer_id = atoi(peerid.c_str());
		peer.port = atoi(peerport.c_str());

		peers.push_back(peer);
	}
	return peers;
}

MessagePtr ResponseMessage::ParseResponseMessage(std::string xml)
{
	std::stringstream ss(xml);
	
	pugi::xml_document doc;
	pugi::xml_parse_result result = doc.load(ss);
	if( result.status != pugi::status_ok ) {
		Log(ERR, L"Response Parse Error\n");
		return RequestMessagePtr();
	}

	pugi::xml_node node = doc.child("Response");
	if( node == NULL )
		return ResponseMessagePtr();

	pugi::xml_node cnode = node;
	cnode = cnode.child("type");
	if( cnode == NULL )
		return ResponseMessagePtr();
	std::string type = cnode.child_value();

	std::vector<RemotePeers> peers;
	cnode = node;
	cnode = cnode.child("peerlist");
	if( cnode != NULL ) {
		pugi::xml_node pnode = cnode;
		peers = GetPeerList( pnode );
	}

	//[TODO] parse all other information too

	// make the request and add values
	ResponseMessagePtr resp(new ResponseMessage);
	resp->m_mesg = xml;
	resp->m_respType = (RESP_TYPE)atoi(type.c_str());
	resp->m_mesgType = RESP;
	resp->m_peerList = peers;

	return resp;
}