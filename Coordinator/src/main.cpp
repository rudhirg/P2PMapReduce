#include "Coordinator.h"

#define CONFIG_FILE "config.xml"

void main(int argc, char* argv[])
{
	std::string confg = CONFIG_FILE;
	if(argc > 1)
		confg = argv[1];

	CoordinatorPtr ptrCoord = Coordinator::GetCoordinator();
	ptrCoord->Start(confg);

	while( std::cin.get() != 'q' );
	ptrCoord->Kill();

}