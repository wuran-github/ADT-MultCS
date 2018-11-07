#pragma once

#include <QtWidgets/QMainWindow>
#include "ui_QTManager.h"

class QTManager : public QMainWindow
{
	Q_OBJECT

public:
	QTManager(QWidget *parent = Q_NULLPTR);

private:
	Ui::QTManagerClass ui;
};
