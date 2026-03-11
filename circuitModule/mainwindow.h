#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QtGui/QWidget>
#include <QGraphicsView>
#include <QStringListModel>
#include <QString>
#include <QEvent>
#include <QStandardItemModel>

class DirectWidget;

#include "circuitconfig.h"
#include "../circuitmodule_global.h"
#include "ui_mainwindow.h"

class CIRCUITMODULE_API CircuitModuleWidget : public QWidget
{
	Q_OBJECT

public:
	CircuitModuleWidget(QWidget *parent = 0);
	explicit CircuitModuleWidget(bool useSvgBytes, QWidget* parent = 0);
	~CircuitModuleWidget();

	void clear();
	bool UpdateConnectionData();
	bool Refresh();
	static void* operator new(size_t size);
	static void  operator delete(void* p) throw();

protected:
	bool eventFilter(QObject* watched, QEvent* event);

protected:
	void InitView();
	void InitSvg();
	void InitTree();
	void InitializeDirectWidgets();
	void ResetDirectWidgets();
	void DisplayLogicIed(const QString& iedName);
	void DisplayBayLogic(const QString& bayName);
	void DisplayOpticalIed(const QString& iedName);
	void DisplayVirtualIed(const QString& iedName);
	void DisplayStationOptical();
	void DisplayBayOptical(const QString& bayName);

private:
	bool InitializeCircuitData(const QString& cimeDirectory);
	void InitializeWindow();
	void DisplayIed(const QString& iedName);

private slots:
	void onStationTreeItemClicked(const QModelIndex& index);

private:
	Ui::MainWindowClass ui;
	CircuitConfig* m_circuitConfig;
	QStringListModel* m_iedListModel;
	QStandardItemModel* m_stationModel;
	bool m_useSvgBytes;
	DirectWidget* m_directLogicWidget;
	DirectWidget* m_directOpticalWidget;
	DirectWidget* m_directVirtualWidget;
};

#endif // MAINWINDOW_H
