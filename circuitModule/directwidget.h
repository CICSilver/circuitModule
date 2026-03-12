#pragma once

#include <QWidget>
#include <QMap>
#include <QVector>
#include <QSet>
#include <QTimer>
#include <QGraphicsView>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QContextMenuEvent>
#include "directitems.h"
#include "svgmodel.h"
#include "RtdbClient.h"
// 图元注册类，存储图元信息的结构体，但不负责图元清理（由图元所属窗口负责）
class QGraphicsScene;
class QGraphicsView;
class DirectView : public QGraphicsView
{
	Q_OBJECT

public:
	//************************************
	// 函数名称:	DirectView
	// 函数全名:	DirectView::DirectView
	// 访问权限:	public
	// 函数说明:	构造交互视图
	// 函数参数:	QGraphicsScene* scene
	// 函数参数:	QWidget* parent
	// 返回值:	void
	//************************************
	DirectView(QGraphicsScene* scene, QWidget* parent = NULL);

protected:
	//************************************
	// 函数名称:	mousePressEvent
	// 函数全名:	DirectView::mousePressEvent
	// 访问权限:	protected
	// 函数说明:	鼠标按下事件
	// 函数参数:	QMouseEvent* event
	// 返回值:	void
	//************************************
	void mousePressEvent(QMouseEvent* event);
	//************************************
	// 函数名称:	mouseMoveEvent
	// 函数全名:	DirectView::mouseMoveEvent
	// 访问权限:	protected
	// 函数说明:	鼠标移动事件
	// 函数参数:	QMouseEvent* event
	// 返回值:	void
	//************************************
	void mouseMoveEvent(QMouseEvent* event);
	//************************************
	// 函数名称:	mouseReleaseEvent
	// 函数全名:	DirectView::mouseReleaseEvent
	// 访问权限:	protected
	// 函数说明:	鼠标释放事件
	// 函数参数:	QMouseEvent* event
	// 返回值:	void
	//************************************
	void mouseReleaseEvent(QMouseEvent* event);
	//************************************
	// 函数名称:	wheelEvent
	// 函数全名:	DirectView::wheelEvent
	// 访问权限:	protected
	// 函数说明:	滚轮缩放事件
	// 函数参数:	QWheelEvent* event
	// 返回值:	void
	//************************************
	void wheelEvent(QWheelEvent* event);
	//************************************
	// 函数名称:	contextMenuEvent
	// 函数全名:	DirectView::contextMenuEvent
	// 访问权限:	protected
	// 函数说明:	右键菜单事件
	// 函数参数:	QContextMenuEvent* event
	// 返回值:	void
	//************************************
	void contextMenuEvent(QContextMenuEvent* event);
	//************************************
	// 函数名称:	mouseDoubleClickEvent
	// 函数全名:	DirectView::mouseDoubleClickEvent
	// 访问权限:	protected
	// 函数说明:	鼠标双击事件
	// 函数参数:	QMouseEvent* event
	// 返回值:	void
	//************************************
	void mouseDoubleClickEvent(QMouseEvent* event);

signals:
	void opticalLineClicked(quint64 opticalCode, const QString& srcIedName, const QString& destIedName);
	void logicLineClicked(LineItem* lineItem);
	void maintainPlateToggled(const QString& iedName, int value);

private:
	bool m_dragging;
	bool m_leftPressed;
	QPoint m_lastViewPos;
	QPoint m_pressPos;
};

class ItemRegistry
{
};
class DirectWidget  : public QWidget
{
	Q_OBJECT

public:
	DirectWidget(QWidget *parent);
	~DirectWidget();
	// 将GenerateXXXByIed生成的图元信息解析为结构体
	void ParseFromLogicSvg(const LogicSvg& svg);
	void ParseFromOpaticalSvg();
	void ParseFromOpticalSvg(const OpticalSvg& svg);
	void ParseFromVirtualSvg();
	void ParseFromVirtualSvg(const VirtualSvg& svg);
	void ParseFromVirtualSvg(const VirtualSvg& svg, const QSet<quint64>& circuitCodeSet);
	void SetMaintainPlateText(const QString& iedName, const QString& text);
	void SetMaintainPlateState(const QString& iedName, int value);

protected:
	void resizeEvent(QResizeEvent* event);

private:
	void initView();
	//************************************
	// 函数名称:	UpdatePlateStatuses
	// 函数全名:	DirectWidget::UpdatePlateStatuses
	// 访问权限:	private
	// 函数说明:	更新压板状态显示
	// 函数参数:	void
	// 返回值:	void
	//************************************
	void UpdatePlateStatuses();
	//************************************
	// 函数名称:	ClearScene
	// 函数全名:	DirectWidget::ClearScene
	// 访问权限:	private
	// 函数说明:	清理场景并重置缓存
	// 函数参数:	void
	// 返回值:	void
	//************************************
	void ClearScene();
	//************************************
	// 函数名称:	UpdateLineStatuses
	// 函数全名:	DirectWidget::UpdateLineStatuses
	// 访问权限:	private
	// 函数说明:	更新虚回路状态数值
	// 函数参数:	void
	// 返回值:	void
	//************************************
	void UpdateLineStatuses();

private slots:
	void OnOpticalLineClicked(quint64 opticalCode, const QString& srcIedName, const QString& destIedName);
	void OnLogicLineClicked(LineItem* lineItem);
	void OnMaintainPlateToggled(const QString& iedName, int value);
	//************************************
	// 函数名称:	OnStatusTimeout
	// 函数全名:	DirectWidget::OnStatusTimeout
	// 访问权限:	private slots
	// 函数说明:	状态刷新定时处理
	// 函数参数:	void
	// 返回值:	void
	//************************************
	void OnStatusTimeout();
	//************************************
	// 函数名称:	OnBlinkTimeout
	// 函数全名:	DirectWidget::OnBlinkTimeout
	// 访问权限:	private slots
	// 函数说明:	闪烁状态定时处理
	// 函数参数:	void
	// 返回值:	void
	//************************************
	void OnBlinkTimeout();

private:
	QGraphicsScene* m_scene;
	DirectView* m_view;
	QMap<QString, DirectPlateItem*> m_plateItems;
	QMap<QString, DirectMaintainPlateItem*> m_maintPlateItems;
	QMap<QString, QString> m_maintPlateTextByIed;
	QMap<QString, int> m_maintPlateStateByIed;
	QVector<DirectVirtualLineItem*> m_virtualLines;
	RtdbClient& m_rtdb;
	QTimer* m_statusTimer;
	QTimer* m_blinkTimer;
	bool m_blinkOn;
	QString m_currentIedName;
};
