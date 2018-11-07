#include "QTManager.h"
#include <QtWidgets/QApplication>

int main(int argc, char *argv[])
{
	QApplication a(argc, argv);
	QTManager w;
	w.show();
	return a.exec();
}
