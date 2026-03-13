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
	double AngleByVec(const QPointF& vec);
	void DrawArrow(QPainter* painter, const QPointF& endPoint, double angle, const QColor& color, int arrowLen);
	QRectF BuildPolylineBoundingRect(const QVector<QPointF>& points, qreal padX, qreal padY);
	QPainterPath BuildPolylineShape(const QVector<QPointF>& points, qreal width);
	int BuildPaintLineWidth(int lineWidth, bool highlighted);
	void DrawPolyline(QPainter* painter, const QVector<QPointF>& points);
	void DrawPolylineArrows(QPainter* painter, const QVector<QPointF>& points, int connRadius, int arrowOffset,
		quint8 startArrowState, quint8 endArrowState, const QColor& color, int arrowLen);
	void DrawConnCircle(QPainter* painter, const QPoint& point, int radius, bool isCircleUnderPt);
	void DrawPortText(QPainter* painter, const QPoint& startPoint, const QPoint& endPoint,
		bool startAtTop, bool endAtTop, bool startIsSwitch, bool endIsSwitch,
		const QString& startPort, const QString& endPort, int connRadius);
	bool IsTopAnchor(const QPoint& point, const IedRect* rect);
	bool IsPointAttachedToRect(const QPoint& point, const IedRect* rect);
}
