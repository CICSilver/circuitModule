#pragma once

#include <QColor>
#include <QPainterPath>
#include <QPoint>
#include <QPointF>
#include <QRectF>
#include <QString>
#include <QVector>
#include <QtGlobal>

struct IedRect;
class QPainter;

namespace directlinehelpers
{
	//************************************
	// 函数名称:	AngleByVec
	// 函数全名:	directlinehelpers::AngleByVec
	// 访问权限:	public
	// 函数说明:	根据方向向量计算箭头绘制所需的平面角度
	// 输入参数:	const QPointF& vec
	// 输出参数:	无
	// 返回值:	double
	//************************************
	double AngleByVec(const QPointF& vec);
	//************************************
	// 函数名称:	DrawArrow
	// 函数全名:	directlinehelpers::DrawArrow
	// 访问权限:	public
	// 函数说明:	在给定终点按指定角度绘制实心箭头
	// 输入参数:	QPainter* painter, const QPointF& endPoint, double angle, const QColor& color, int arrowLen
	// 输出参数:	无
	// 返回值:	void
	//************************************
	void DrawArrow(QPainter* painter, const QPointF& endPoint, double angle, const QColor& color, int arrowLen);
	//************************************
	// 函数名称:	BuildPolylineBoundingRect
	// 函数全名:	directlinehelpers::BuildPolylineBoundingRect
	// 访问权限:	public
	// 函数说明:	根据折线点集和额外留白计算包围矩形
	// 输入参数:	const QVector<QPointF>& points, qreal padX, qreal padY
	// 输出参数:	无
	// 返回值:	QRectF
	//************************************
	QRectF BuildPolylineBoundingRect(const QVector<QPointF>& points, qreal padX, qreal padY);
	//************************************
	// 函数名称:	BuildPolylineShape
	// 函数全名:	directlinehelpers::BuildPolylineShape
	// 访问权限:	public
	// 函数说明:	构造折线的命中区域，供 shape 和 hover 检测复用
	// 输入参数:	const QVector<QPointF>& points, qreal width
	// 输出参数:	无
	// 返回值:	QPainterPath
	//************************************
	QPainterPath BuildPolylineShape(const QVector<QPointF>& points, qreal width);
	//************************************
	// 函数名称:	BuildPaintLineWidth
	// 函数全名:	directlinehelpers::BuildPaintLineWidth
	// 访问权限:	public
	// 函数说明:	根据基础线宽和高亮状态计算实际绘制线宽
	// 输入参数:	int lineWidth, bool highlighted
	// 输出参数:	无
	// 返回值:	int
	//************************************
	int BuildPaintLineWidth(int lineWidth, bool highlighted);
	//************************************
	// 函数名称:	DrawPolyline
	// 函数全名:	directlinehelpers::DrawPolyline
	// 访问权限:	public
	// 函数说明:	按点集顺序逐段绘制折线
	// 输入参数:	QPainter* painter, const QVector<QPointF>& points
	// 输出参数:	无
	// 返回值:	void
	//************************************
	void DrawPolyline(QPainter* painter, const QVector<QPointF>& points);
	//************************************
	// 函数名称:	DrawPolylineArrows
	// 函数全名:	directlinehelpers::DrawPolylineArrows
	// 访问权限:	public
	// 函数说明:	根据首尾箭头状态在折线两端绘制单向或双向箭头
	// 输入参数:	QPainter* painter, const QVector<QPointF>& points, int connRadius, int arrowOffset, quint8 startArrowState, quint8 endArrowState, const QColor& color, int arrowLen
	// 输出参数:	无
	// 返回值:	void
	//************************************
	void DrawPolylineArrows(QPainter* painter, const QVector<QPointF>& points, int connRadius, int arrowOffset,
		quint8 startArrowState, quint8 endArrowState, const QColor& color, int arrowLen);
	//************************************
	// 函数名称:	DrawConnCircle
	// 函数全名:	directlinehelpers::DrawConnCircle
	// 访问权限:	public
	// 函数说明:	在连接点上方或下方绘制绿色连接圆
	// 输入参数:	QPainter* painter, const QPoint& point, int radius, bool isCircleUnderPt
	// 输出参数:	无
	// 返回值:	void
	//************************************
	void DrawConnCircle(QPainter* painter, const QPoint& point, int radius, bool isCircleUnderPt);
	//************************************
	// 函数名称:	DrawPortText
	// 函数全名:	directlinehelpers::DrawPortText
	// 访问权限:	public
	// 函数说明:	在折线起点和终点附近绘制端口文字
	// 输入参数:	QPainter* painter, const QPoint& startPoint, const QPoint& endPoint, bool startAtTop, bool endAtTop, bool startIsSwitch, bool endIsSwitch, const QString& startPort, const QString& endPort, int connRadius
	// 输出参数:	无
	// 返回值:	void
	//************************************
	void DrawPortText(QPainter* painter, const QPoint& startPoint, const QPoint& endPoint,
		bool startAtTop, bool endAtTop, bool startIsSwitch, bool endIsSwitch,
		const QString& startPort, const QString& endPort, int connRadius);
	//************************************
	// 函数名称:	IsTopAnchor
	// 函数全名:	directlinehelpers::IsTopAnchor
	// 访问权限:	public
	// 函数说明:	判断连接点更接近IED上边还是下边
	// 输入参数:	const QPoint& point, const IedRect* rect
	// 输出参数:	无
	// 返回值:	bool
	//************************************
	bool IsTopAnchor(const QPoint& point, const IedRect* rect);
	//************************************
	// 函数名称:	IsPointAttachedToRect
	// 函数全名:	directlinehelpers::IsPointAttachedToRect
	// 访问权限:	public
	// 函数说明:	判断连接点是否落在IED矩形边界上
	// 输入参数:	const QPoint& point, const IedRect* rect
	// 输出参数:	无
	// 返回值:	bool
	//************************************
	bool IsPointAttachedToRect(const QPoint& point, const IedRect* rect);
}
