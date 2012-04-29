#include <string>
#include <iostream>
#include <stdio.h>

#include "Peer.h"

#define CONFIG_FILE "config.xml"

std::string exec(char* cmd);

void main(int argc, char* argv[])
{
	std::string confg = CONFIG_FILE;
	if(argc > 1)
		confg = argv[1];
	
	PeerPtr ptrCoord = Peer::GetPeer();
	ptrCoord->Start(confg);

	while( std::cin.get() != 'q' ) {
		std::string s;
		getline( std::cin, s );
		//ptrCoord->TestSendMesgs(s);
		ptrCoord->TestTask();
	}

	ptrCoord->Kill();
//	std::string result = exec("grep -irH asd .");
}

std::string exec(char* cmd) {
    FILE* pipe = _popen(cmd, "r");
    if (!pipe) return "ERROR";
    char buffer[128];
    std::string result = "";
    while(!feof(pipe)) {
        if(fgets(buffer, 128, pipe) != NULL)
                result += buffer;
    }
    _pclose(pipe);
    return result;
}
