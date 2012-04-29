#include "Peer.h"

#define CONFIG_FILE "config.xml"

void main()
{
	PeerPtr ptrCoord = Peer::GetPeer();
	ptrCoord->Start(CONFIG_FILE);

	while( std::cin.get() != 'q' ) {
		std::string s;
		getline( std::cin, s );
		ptrCoord->TestSendMesgs(s);
	}

	ptrCoord->Kill();

}