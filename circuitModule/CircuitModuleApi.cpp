#include "../CircuitModuleApi.h"

#include "mainwindow.h"
#include "circuitconfig.h"

#include <QApplication>
#include <QMutex>
#include <QMutexLocker>
#include <QMessageBox>

namespace
{
	CircuitModuleWidget* g_widget = 0;
	QMutex g_mainWindowMutex;
	bool g_configLoaded = false;
}

static void ResetMainWindow()
{
	if (!g_widget)
	{
		return;
	}

	g_widget->clear();
	delete g_widget;
	g_widget = 0;
}

static bool EnsureCircuitConfigLoaded()
{
	CircuitConfig* config = CircuitConfig::Instance();
	if (!config)
	{
		return false;
	}

	if (!config->LoadRTDB())
	{
		return false;
	}

	g_configLoaded = true;
	return true;
}

static CircuitModuleWidget* EnsureMainWindow(bool useSvgBytes, QWidget* parent)
{
	Q_UNUSED(useSvgBytes);

	if (!g_widget)
	{
		g_widget = new CircuitModuleWidget(parent);
		return g_widget;
	}

	if (parent && g_widget->parentWidget() != parent)
	{
		g_widget->setParent(parent);
	}
	return g_widget;
}

extern "C" CIRCUITMODULE_API QWidget* CM_CreateModuleWidget(bool useSvgBytes, QWidget* parent)
{
	if (!QApplication::instance())
	{
		return 0;
	}
	if (!EnsureCircuitConfigLoaded())
	{
		QMessageBox::critical(0, QObject::tr("Error"), QObject::tr("RTDB load failed"));
	}
	QMutexLocker locker(&g_mainWindowMutex);
	return EnsureMainWindow(useSvgBytes, parent);
}

extern "C" CIRCUITMODULE_API QWidget* CM_GetModuleWidget()
{
	QMutexLocker locker(&g_mainWindowMutex);
	return g_widget;
}

extern "C" CIRCUITMODULE_API void CM_Destroy()
{
	QMutexLocker locker(&g_mainWindowMutex);
	if (g_widget)
	{
		ResetMainWindow();
		g_configLoaded = false;
	}
}

CIRCUITMODULE_API bool CM_Refresh()
{
	if (!g_widget)
	{
		return false;
	}
	return g_widget->Refresh();
}
