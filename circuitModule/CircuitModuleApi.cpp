#include "../CircuitModuleApi.h"

#include "mainwindow.h"
#include "circuitconfig.h"

#include <QApplication>
#include <QCoreApplication>
#include <QDir>
#include <QMutex>
#include <QMutexLocker>
#include <QString>
#include <QMessageBox>
namespace
{
	CircuitModuleWidget* g_widget = nullptr;
	QMutex g_mainWindowMutex;
	bool g_useSvgBytes = false;
	bool g_configLoaded = false;
}

static QString NormalizeCimeDirectory(const QString& directory)
{
	QString trimmed = directory.trimmed();
	if (trimmed.isEmpty())
	{
		return QString();
	}

	QString cleaned = QDir::fromNativeSeparators(trimmed);
	QDir dir(cleaned);
	if (dir.isAbsolute())
	{
		return QDir::cleanPath(dir.absolutePath());
	}

	QDir base(QCoreApplication::applicationDirPath());
	QString absolute = base.absoluteFilePath(cleaned);
	return QDir::cleanPath(absolute);
}

static void ResetMainWindow()
{
	if (!g_widget)
	{
		return;
	}

	g_widget->clear();
	delete g_widget;
	g_widget = nullptr;
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
	if (!g_widget)
	{
		g_widget = new CircuitModuleWidget(useSvgBytes, parent);
		g_useSvgBytes = useSvgBytes;
		return g_widget;
	}

	if (g_useSvgBytes != useSvgBytes)
	{
		ResetMainWindow();
		g_widget = new CircuitModuleWidget(useSvgBytes, parent);
		g_useSvgBytes = useSvgBytes;
		return g_widget;
	}

	if (parent && g_widget->parentWidget() != parent)
	{
		g_widget->setParent(parent);
	}

	g_useSvgBytes = useSvgBytes;
	return g_widget;
}

extern "C" CIRCUITMODULE_API QWidget* CM_CreateModuleWidget(bool useSvgBytes, QWidget* parent)
{
	if (!QApplication::instance())
	{
		return nullptr;
	}
	if (!EnsureCircuitConfigLoaded())
	{
		QMessageBox::critical(nullptr, QObject::tr("Error"), QObject::tr("ÊµÊ±¿â¶ÁÈ¡Ê§°Ü"));
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
	g_widget->Refresh();
	return true;
}
