#include "Coordinator.h"

CoordinatorPtr Coordinator::m_ptrCoordinator;

Coordinator::Coordinator()
{
	m_pMsgQ = MessageQueuePtr(new MessageQueue);	
}

Coordinator::~Coordinator()
{

}

CoordinatorPtr Coordinator::GetCoordinator()
{
	if( m_ptrCoordinator == NULL ) {
		CoordinatorPtr ptr(new Coordinator());
		m_ptrCoordinator = ptr;
	}

	return m_ptrCoordinator;
}

void Coordinator::Start(std::string configFile)
{
	int result = this->ReadConfigFile(configFile);

	if(result < 0) {
		Log(ERROR, L"Config File Error!!\n");
		return;
	}

	// start the connection manager
	m_ptrConnMgr = ConnectionManager::GetConnectionManager();
	m_ptrConnMgr->SetListenPort(m_port);
	m_ptrConnMgr->SetCoordinator(shared_from_this());
	m_ptrConnMgr->Start();
	
	m_coordThread = boost::thread(&Coordinator::run, this);
}

int Coordinator::ReadConfigFile(std::string configFile)
{
	int ret = 0;

	pugi::xml_document doc;
	pugi::xml_parse_result result = doc.load_file(configFile.c_str());

	if( result.status != pugi::status_ok ) {
		Log(ERROR, L"Startup Configuration file error\n");
		return -1;
	}

	pugi::xml_node root = doc.child("Root");
	pugi::xml_node node = root.child("Coordinator");
	if( node == NULL )
		return -1;

	pugi::xml_node cnode = node;
	cnode = node.child("port");
	if( cnode == NULL )
		return -1;

	std::string port = cnode.child_value();

//	pCoordNode->SetIpAddr(ip);
	this->SetPort(atoi(port.c_str()));
 
	return result;	
}

void Coordinator::Kill()
{
	if(m_ptrConnMgr != NULL)
		m_ptrConnMgr->Kill();
	m_coordThread.interrupt();
}

void Coordinator::ProcessMessages(MessagePtr pMsg)
{
	Log(CONSOLE, L"Message Processed %s\n", pMsg->m_mesg);
}

// process message queue
void Coordinator::run()
{
	while (true) {
		MessagePtr pMesg = m_pMsgQ->GetMessage();
		// if empty wait for queue to get full
		if( pMesg == NULL ) {
			m_pMsgQ->WaitForFull();
			continue;
		}
		
	}
}