#include "Connection.h"
#include "Message.h"

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

	return req;
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

	//[TODO] parse all other information too

	// make the request and add values
	ResponseMessagePtr resp(new ResponseMessage);
	resp->m_mesg = xml;
	resp->m_type = (RESP_TYPE)atoi(type.c_str());

	return resp;
}