#include "mainwindow.h"

#include <QApplication>

int main(int argc, char* argv[])
{
	QApplication app(argc, argv);

	CircuitModuleWidget mainWindow;
	mainWindow.resize(1920, 1080);
	mainWindow.show();
	return app.exec();
}
