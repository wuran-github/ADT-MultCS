#include <QtCore/QCoreApplication>
#include <iostream>
#include <qfile.h>
#include "WorkStation.h"
int main(int argc, char *argv[])
{
	QCoreApplication q(argc, argv);
	WorkStation station;
	//run
	station.Run();
	return 0;
}
