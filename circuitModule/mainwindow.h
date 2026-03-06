#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QtGui/QWidget>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QStringListModel>
#include <QString>
#include <QEvent>
#include <QStandardItemModel>
#include <QHash>
#include <QByteArray>

#include "circuitconfig.h"
#include "../circuitmodule_global.h"
#include "ui_mainwindow.h"

class CIRCUITMODULE_API CircuitModuleWidget : public QWidget
{
	Q_OBJECT

public:
	// 默认使用文件路径初始化
	CircuitModuleWidget(QWidget *parent = 0);
	// 新增：可指定使用内存 svgBytes 初始化
	explicit CircuitModuleWidget(bool useSvgBytes, QWidget* parent = 0);
	~CircuitModuleWidget();

	void clear();
	// 刷新界面连接数据
	bool UpdateConnectionData();
	// 刷新实时库数据及界面显示
	bool Refresh();
	static void* operator new(size_t size);
	static void  operator delete(void* p) throw();

protected:
	// 统一在视图层限制缩放范围
	bool eventFilter(QObject* watched, QEvent* event);

protected:
	void InitView();
	void InitSvg();
	void InitTree();

	QString opticalSvgPath(const QString& iedName)
	{
		return QCoreApplication::applicationDirPath() + "/optical/" + iedName + "_optical_circuit.svg";
	}
	QString logicSvgPath(const QString& iedName)
	{
		return QCoreApplication::applicationDirPath() + "/logic/" + iedName + "_logic_circuit.svg";
	}
	QString wholeSvgPath(const QString& iedName)
	{
		return QCoreApplication::applicationDirPath() + "/whole/" + iedName + "_whole_circuit.svg";
	}
	QString virtualSvgPath(const QString& iedName)
	{
		return QCoreApplication::applicationDirPath() + "/virtual/" + iedName + "_virtual_circuit.svg";
	}
	void InitInteractiveView(QGraphicsView* view, const QString& svgPath);
	void InitInteractiveView(QGraphicsView* view, const QByteArray& svgBytes);

private:
	bool InitializeCircuitData(const QString& cimeDirectory);
	void InitializeWindow();
	void SetupInteractiveView(QGraphicsView* view, class InteractiveSvgMapItem* item);
	void DisplayIed(const QString& iedName);
	void clearView(QGraphicsView* view);

private slots:
	void onStationTreeItemClicked(const QModelIndex& index);

private:
	Ui::MainWindowClass ui;
	CircuitConfig* m_circuitConfig;
	QStringListModel* m_iedListModel;
	QStandardItemModel* m_stationModel;
	// 是否使用内存 SVG 字节流初始化交互视图
	struct SvgByteData
	{
		QByteArray logic;
		QByteArray optical;
		QByteArray virtualCircuit;
	};
	QHash<QString, SvgByteData> m_svgByteCache;
	bool m_useSvgBytes;
};

#endif // MAINWINDOW_H
