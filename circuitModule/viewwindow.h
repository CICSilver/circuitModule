#pragma once

#include <QMainWindow>
#include "ui_viewwindow.h"

class ViewWindow : public QMainWindow
{
	Q_OBJECT

public:
	ViewWindow(QWidget *parent = nullptr);
	~ViewWindow();

private:
	Ui::ViewWindowClass ui;
};
