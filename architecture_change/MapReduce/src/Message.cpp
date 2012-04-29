#include "Connection.h"
#include "Message.h"

void RequestMessage::AddGeneralHeader(REQ_TYPE type, int fromPeerId)
{
	this->m_type = type;
	this->m_fromPeerId = fromPeerId;
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

	std::stringstream ss;
	ss << this->m_type;
	pugi::xml_document doc;
	pugi::xml_node root = doc.append_child("Request");
	// add header
	pugi::xml_node type = root.append_child("type");
	type.append_child(pugi::node_pcdata).set_value(ss.str().c_str());
	ss.clear();
	ss << this->m_fromPeerId;
	pugi::xml_node from = root.append_child("frompeer");
	from.append_child(pugi::node_pcdata).set_value(ss.str().c_str());
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

void ResponseMessage::AddGeneralHeader(RESP_TYPE type, int fromPeerId)
{
	this->m_type = type;
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

std::string ResponseMessage::GetXML()
{
	if(m_xml.empty() == false) return m_xml;

	std::stringstream ss;
	ss << this->m_type;
	pugi::xml_document doc;
	pugi::xml_node root = doc.append_child("Response");
	// add header
	pugi::xml_node type = root.append_child("type");
	type.append_child(pugi::node_pcdata).set_value(ss.str().c_str());
	ss.clear();
	ss << this->m_fromPeerId;
	pugi::xml_node from = root.append_child("frompeer");
	from.append_child(pugi::node_pcdata).set_value(ss.str().c_str());
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
		Log(ERROR, L"Request Parse Error\n");
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

	// make the request and add values
	RequestMessagePtr req(new RequestMessage);
	req->m_mesg = xml;
	req->m_type = (REQ_TYPE)atoi(type.c_str());
	req->m_mesgType = REQ;

	return req;
}

std::list<RemotePeers> Message::GetPeerList(pugi::xml_node& node)
{
	pugi::xml_node pnode = node;
	std::list<RemotePeers> peers;

	while(true) {
		pugi::xml_node cnode = pnode;
		cnode = cnode.child("peer");
		if( cnode == NULL )
			break;
		cnode = cnode.child("peerid");
		if( cnode == NULL )
			break;
		std::string peerid = cnode.child_value();

		cnode = cnode.child("peerip");
		if( cnode == NULL )
			break;
		std::string peerip = cnode.child_value();

		RemotePeers peer;
		peer.ip = peerip;
		peer.peer_id = atoi(peerid.c_str());

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
		Log(ERROR, L"Response Parse Error\n");
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

	std::list<RemotePeers> peers;
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
	resp->m_type = (RESP_TYPE)atoi(type.c_str());
	resp->m_mesgType = RESP;
	resp->m_peerList = peers;

	return resp;
}