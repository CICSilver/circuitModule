/********************************************************************************
** Form generated from reading UI file 'viewwindow.ui'
**
** Created by: Qt User Interface Compiler version 4.8.7
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_VIEWWINDOW_H
#define UI_VIEWWINDOW_H

#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QButtonGroup>
#include <QtGui/QHBoxLayout>
#include <QtGui/QHeaderView>
#include <QtGui/QMainWindow>
#include <QtGui/QTreeView>
#include <QtGui/QWidget>

QT_BEGIN_NAMESPACE

class Ui_ViewWindowClass
{
public:
    QWidget *centralWidget;
    QHBoxLayout *horizontalLayout;
    QTreeView *treeView;
    QWidget *widget;

    void setupUi(QMainWindow *ViewWindowClass)
    {
        if (ViewWindowClass->objectName().isEmpty())
            ViewWindowClass->setObjectName(QString::fromUtf8("ViewWindowClass"));
        ViewWindowClass->resize(638, 462);
        centralWidget = new QWidget(ViewWindowClass);
        centralWidget->setObjectName(QString::fromUtf8("centralWidget"));
        horizontalLayout = new QHBoxLayout(centralWidget);
        horizontalLayout->setSpacing(6);
        horizontalLayout->setContentsMargins(11, 11, 11, 11);
        horizontalLayout->setObjectName(QString::fromUtf8("horizontalLayout"));
        treeView = new QTreeView(centralWidget);
        treeView->setObjectName(QString::fromUtf8("treeView"));

        horizontalLayout->addWidget(treeView);

        widget = new QWidget(centralWidget);
        widget->setObjectName(QString::fromUtf8("widget"));

        horizontalLayout->addWidget(widget);

        horizontalLayout->setStretch(0, 2);
        horizontalLayout->setStretch(1, 8);
        ViewWindowClass->setCentralWidget(centralWidget);

        retranslateUi(ViewWindowClass);

        QMetaObject::connectSlotsByName(ViewWindowClass);
    } // setupUi

    void retranslateUi(QMainWindow *ViewWindowClass)
    {
        ViewWindowClass->setWindowTitle(QApplication::translate("ViewWindowClass", "ViewWindow", 0, QApplication::UnicodeUTF8));
    } // retranslateUi

};

namespace Ui {
    class ViewWindowClass: public Ui_ViewWindowClass {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_VIEWWINDOW_H
