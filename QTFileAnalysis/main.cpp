#include <QtCore/QCoreApplication>
#include "FileAnalysis.h"
int main(int argc, char *argv[])
{
	QCoreApplication q(argc, argv);
	FileAnalysis analysis;
	analysis.Run();
	return 0;
}
