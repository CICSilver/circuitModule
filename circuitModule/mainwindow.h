#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QtGui/QWidget>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QStringListModel>
#include "circuitconfig.h"
#include <QEvent>

#include "ui_mainwindow.h"

class CustomSvgItem;
class MainWindow : public QWidget
{
	Q_OBJECT

public:
	MainWindow(QWidget *parent = 0);
	~MainWindow();

protected:
	// 统一在视图层限制缩放范围
	bool eventFilter(QObject* watched, QEvent* event);

protected:
	void InitView();
	void InitView(QGraphicsView* view, CustomSvgItem* item, QString svgPath);
	void InitList();

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
	// 使用交互式视图加载器（InteractiveSvgMapItem）初始化/刷新某个 QGraphicsView
	void InitInteractiveView(QGraphicsView* view, const QString& svgPath);

private slots:
	void onIedListItemClicked(const QModelIndex& index);

private:
	Ui::MainWindowClass ui;
	CircuitConfig* m_circuitConfig;
	QStringListModel* m_iedListModel;
	CustomSvgItem* m_logicSvgItem;
	CustomSvgItem* m_opticalSvgItem;
	CustomSvgItem* m_wholeSvgItem;
};

#endif // MAINWINDOW_H
