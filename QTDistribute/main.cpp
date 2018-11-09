#include <QtCore/QCoreApplication>
#include "Distribute.h"
int main(int argc, char *argv[])
{
	QCoreApplication q(argc, argv);
	Distribute distribute;
	distribute.Run();
	return 0;
}
