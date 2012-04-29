#ifndef PEER_NODE_H
#define PEER_NODE_H

#include "ConnectionManager.h"
#include "Connection.h"
#include "Message.h"
#include "Peer.h"
#include "Log.h"

#include "TimerMessage.h"

#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>
#include <boost/enable_shared_from_this.hpp>

#include <string>

class PeerNode;

typedef boost::shared_ptr<PeerNode> PeerNodePtr;

struct BackupConfig {
	std::string		m_ipAddress;
	int				m_port;
	BackupConfig(std::string ip, int port) { m_ipAddress=ip, m_port=port; }
};

class PeerNode : public boost::enable_shared_from_this<PeerNode>, public TimerI {
private:
	std::string					m_ipAddress;
	int							m_port;			// remote port of remote peer

	std::vector<BackupConfig>	m_backups;		// backup servers for coordinator

	int							m_listenPort;	// listen port of remote peer
	bool						m_bConnected;
	bool						m_bIsCoordinatorNode;
	ConnectionPtr				m_pConn;
	int							m_peerId;

	int							m_paramIndx;	// indx into the params list of the task it is executing

	TimerMessagePtr				m_pTimerMessages;

	boost::recursive_mutex				m_mutex;

public:
	PeerNode();
	~PeerNode() 
	{
	}

	boost::shared_ptr<PeerNode> f()
    {
        return shared_from_this();
    }

	void Init();

	void MessageEvent(SYS_MSG msg);	// function which receives the timer events

	bool IsCoordinatorNode() { return m_bIsCoordinatorNode; }
	void SetCoordinatorNode() { m_bIsCoordinatorNode = true; }
	void SetIpAddr(std::string ip) { m_ipAddress = ip; }
	void SetPort(int port) { m_port = port; }
	std::string GetIpAddr() { return m_ipAddress; }
	int GetPort() { return m_port; }
	void SetListenPort(int port) { if(port > 0) m_listenPort = port; }
	int GetListenPort() { return m_listenPort; }
	void SetPeerId(int pId) { m_peerId = pId; }
	int GetPeerId() { return m_peerId; }
	int GetParamIndex() { return m_paramIndx; }
	void SetParamIndex(int indx) { m_paramIndx = indx; }

	void AddBackupServer(std::string ip, int port) { 
		BackupConfig config(ip, port);
		m_backups.push_back(config);
	}

	int Connect();			// makes the new connection to remote peers
	int HandleConnect(ConnectionPtr pConn);	// handles the connection requests

	int DisConnect();

	ConnectionPtr ConnectBackups();	// if primary coordinator not working then try contacting backups

	void HandleConnectionBroken();

	void ReceiveMessage(MessagePtr msg);	// receives message from the connection
	int SendMessage(std::string mesg); // sends the message to the peer

	// startup workflow - send 'Online' Message to coordinator
	int SendOnlineMessage();
	// send offline message
	int SendOfflineMessage();

	// send a task request
	int TaskRequest(int taskId, std::string param);
	// send the task request response
	int TaskResponse(int taskId, bool success);	
	// send task execute request
	int TaskExecute(int taskId);
	// send task cancel request
	int TaskCancel(int taskId);
	// send task execute response
	int TaskExecuteResponse(int taskId, bool success);
	// send job executed message
	int TaskExecuteResult(int taskId, bool status, std::vector<std::string>& result, std::string param);

	static PeerNodePtr CreatePeerNode(int peerId, std::string ipAddress = "127.0.0.1", int port = 0);
	static PeerNodePtr CreatePeerNode(RemotePeers peer);
	static void DestroyPeerNode(PeerNodePtr pNode);
};

#endif