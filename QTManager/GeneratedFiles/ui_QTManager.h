/********************************************************************************
** Form generated from reading UI file 'QTManager.ui'
**
** Created by: Qt User Interface Compiler version 5.11.2
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_QTMANAGER_H
#define UI_QTMANAGER_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QTableView>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_QTManagerClass
{
public:
    QWidget *centralWidget;
    QTableView *tableView;
    QStatusBar *statusBar;

    void setupUi(QMainWindow *QTManagerClass)
    {
        if (QTManagerClass->objectName().isEmpty())
            QTManagerClass->setObjectName(QStringLiteral("QTManagerClass"));
        QTManagerClass->resize(870, 402);
        centralWidget = new QWidget(QTManagerClass);
        centralWidget->setObjectName(QStringLiteral("centralWidget"));
        tableView = new QTableView(centralWidget);
        tableView->setObjectName(QStringLiteral("tableView"));
        tableView->setGeometry(QRect(20, 20, 831, 321));
        QTManagerClass->setCentralWidget(centralWidget);
        statusBar = new QStatusBar(QTManagerClass);
        statusBar->setObjectName(QStringLiteral("statusBar"));
        QTManagerClass->setStatusBar(statusBar);

        retranslateUi(QTManagerClass);

        QMetaObject::connectSlotsByName(QTManagerClass);
    } // setupUi

    void retranslateUi(QMainWindow *QTManagerClass)
    {
        QTManagerClass->setWindowTitle(QApplication::translate("QTManagerClass", "QTManager", nullptr));
    } // retranslateUi

};

namespace Ui {
    class QTManagerClass: public Ui_QTManagerClass {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_QTMANAGER_H
