#include "Coordinator.h"

#define CONFIG_FILE "config.xml"

void main()
{
	CoordinatorPtr ptrCoord = Coordinator::GetCoordinator();
	ptrCoord->Start(CONFIG_FILE);

	while( std::cin.get() != 'q' );
	ptrCoord->Kill();

}