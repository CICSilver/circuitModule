#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QtGui/QWidget>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QStringListModel>
#include "circuitconfig.h"
#include <QEvent>

#include "ui_mainwindow.h"

class MainWindow : public QWidget
{
	Q_OBJECT

public:
	// 默认使用文件路径初始化
	MainWindow(QWidget *parent = 0);
	// 新增：可指定使用内存 svgBytes 初始化
	explicit MainWindow(bool useSvgBytes, QWidget* parent = 0);
	~MainWindow();

protected:
	// 统一在视图层限制缩放范围
	bool eventFilter(QObject* watched, QEvent* event);

protected:
	void InitView();
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
	void InitInteractiveView(QGraphicsView* view, const QString& svgPath);
	void InitInteractiveView(QGraphicsView* view, const QByteArray& svgBytes);

private:
    void SetupInteractiveView(QGraphicsView* view, class InteractiveSvgMapItem* item);

private slots:
	void onIedListItemClicked(const QModelIndex& index);

private:
	Ui::MainWindowClass ui;
	CircuitConfig* m_circuitConfig;
	QStringListModel* m_iedListModel;
	// 是否使用内存 SVG 字节流初始化交互视图
	bool m_useSvgBytes;
};

#endif // MAINWINDOW_H
