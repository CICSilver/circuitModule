#include "directlinehelpers.h"
#include "svgmodel.h"
#include "SvgUtils.h"
#include <QFontMetrics>
#include <QPainter>
#include <QPolygonF>
#include <qmath.h>

namespace
{
	const qreal DIRECT_DOUBLE_ARROW_GAP_RATIO = 1.5;
	const int DIRECT_PORT_TEXT_OFFSET = 12;

	double direct_angle_to_radians(double angle)
	{
		return angle * M_PI / 180.0;
	}

	QPointF direct_build_arrow_point_on_line(const QPointF& connPoint, const QPointF& outwardVec, int connRadius, int arrowLen, int offset)
	{
		qreal vectorLength = qSqrt(outwardVec.x() * outwardVec.x() + outwardVec.y() * outwardVec.y());
		if (vectorLength < 0.001)
		{
			return connPoint;
		}
		qreal totalOffset = connRadius * 2 + arrowLen + offset;
		return QPointF(
			connPoint.x() + outwardVec.x() / vectorLength * totalOffset,
			connPoint.y() + outwardVec.y() / vectorLength * totalOffset);
	}

	QPointF direct_offset_point_by_vec(const QPointF& point, const QPointF& vec, qreal offset)
	{
		qreal vectorLength = qSqrt(vec.x() * vec.x() + vec.y() * vec.y());
		if (vectorLength < 0.001)
		{
			return point;
		}
		return QPointF(
			point.x() + vec.x() / vectorLength * offset,
			point.y() + vec.y() / vectorLength * offset);
	}

	void direct_draw_single_port_text(QPainter* painter, const QPoint& connPoint, bool isTopSide, bool isSwitch, const QString& port, int connRadius)
	{
		if (port.isEmpty())
		{
			return;
		}
		Q_UNUSED(isSwitch);
		int offset = connRadius * 2 + 10 + DIRECT_PORT_TEXT_OFFSET;
		QPen pen;
		pen.setColor(Qt::white);
		QFont font = painter->font();
		font.setPointSize(12);
		painter->setPen(pen);
		painter->setFont(font);
		int portPointY = isTopSide ? connPoint.y() - offset : connPoint.y() + offset;
		QFontMetrics fontMetrics(painter->font());
		int textWidth = fontMetrics.width(port);
		QPoint leftTopPoint(connPoint.x() - textWidth / 2, portPointY - 5);
		QPoint rightBottomPoint(connPoint.x() + textWidth / 2, portPointY + fontMetrics.height());
		painter->drawText(QRect(leftTopPoint, rightBottomPoint), port);
	}
}

namespace directlinehelpers
{
	double AngleByVec(const QPointF& vec)
	{
		double direction = atan2(-vec.y(), vec.x());
		return direction * (180.0 / M_PI);
	}

	void DrawArrow(QPainter* painter, const QPointF& endPoint, double angle, const QColor& color, int arrowLen)
	{
		QPointF leftArrowPoint = endPoint + QPointF(
			arrowLen * cos(direct_angle_to_radians(angle + 150.0)),
			-arrowLen * sin(direct_angle_to_radians(angle + 150.0)));
		QPointF rightArrowPoint = endPoint + QPointF(
			arrowLen * cos(direct_angle_to_radians(angle - 150.0)),
			-arrowLen * sin(direct_angle_to_radians(angle - 150.0)));
		int offset = 8;
		QPointF offsetPoint = QPointF(
			endPoint.x() + offset * cos(direct_angle_to_radians(angle)),
			endPoint.y() - offset * sin(direct_angle_to_radians(angle)));
		QBrush brush(color, Qt::SolidPattern);
		QPen pen(Qt::NoPen);
		painter->setPen(pen);
		painter->setBrush(brush);
		QVector<QPointF> points;
		points << leftArrowPoint << offsetPoint << rightArrowPoint << endPoint;
		painter->drawPolygon(points.data(), 4);
	}

	QRectF BuildPolylineBoundingRect(const QVector<QPointF>& points, qreal padX, qreal padY)
	{
		if (points.isEmpty())
		{
			return QRectF();
		}
		qreal minX = points[0].x();
		qreal maxX = points[0].x();
		qreal minY = points[0].y();
		qreal maxY = points[0].y();
		for (int i = 1; i < points.size(); ++i)
		{
			const QPointF& point = points[i];
			if (point.x() < minX)
			{
				minX = point.x();
			}
			if (point.x() > maxX)
			{
				maxX = point.x();
			}
			if (point.y() < minY)
			{
				minY = point.y();
			}
			if (point.y() > maxY)
			{
				maxY = point.y();
			}
		}
		return QRectF(QPointF(minX - padX, minY - padY), QPointF(maxX + padX, maxY + padY));
	}

	QPainterPath BuildPolylineShape(const QVector<QPointF>& points, qreal width)
	{
		QPainterPath path;
		if (points.size() < 2)
		{
			return path;
		}
		path.moveTo(points[0]);
		for (int i = 1; i < points.size(); ++i)
		{
			path.lineTo(points[i]);
		}
		QPainterPathStroker stroker;
		stroker.setWidth(width);
		return stroker.createStroke(path);
	}

	int BuildPaintLineWidth(int lineWidth, bool highlighted)
	{
		int paintWidth = lineWidth;
		if (highlighted)
		{
			paintWidth = lineWidth * 3;
			if (paintWidth < 2)
			{
				paintWidth = 2;
			}
		}
		else if (paintWidth < 1)
		{
			paintWidth = 1;
		}
		return paintWidth;
	}

	void DrawPolyline(QPainter* painter, const QVector<QPointF>& points)
	{
		for (int i = 1; i < points.size(); ++i)
		{
			painter->drawLine(points[i - 1], points[i]);
		}
	}

	void DrawPolylineArrows(QPainter* painter, const QVector<QPointF>& points, int connRadius, int arrowOffset,
		quint8 startArrowState, quint8 endArrowState, const QColor& color, int arrowLen)
	{
		if (points.size() < 2)
		{
			return;
		}
		QPointF startVec = points[1] - points[0];
		QPointF endVec = points[points.size() - 2] - points[points.size() - 1];
		if (qFuzzyIsNull(startVec.x()) && qFuzzyIsNull(startVec.y()) &&
			qFuzzyIsNull(endVec.x()) && qFuzzyIsNull(endVec.y()))
		{
			return;
		}
		QPointF startArrowPoint = direct_build_arrow_point_on_line(points[0], startVec, connRadius, arrowLen, arrowOffset);
		QPointF endArrowPoint = direct_build_arrow_point_on_line(points[points.size() - 1], endVec, connRadius, arrowLen, arrowOffset);
		qreal doubleArrowGap = arrowLen * DIRECT_DOUBLE_ARROW_GAP_RATIO;
		QPointF startOutArrowPoint = startArrowPoint;
		QPointF startInArrowPoint = startArrowPoint;
		QPointF endOutArrowPoint = endArrowPoint;
		QPointF endInArrowPoint = endArrowPoint;
		if ((startArrowState & Arrow_InOut) == Arrow_InOut)
		{
			startOutArrowPoint = direct_offset_point_by_vec(startArrowPoint, startVec, doubleArrowGap * 0.5);
			startInArrowPoint = direct_offset_point_by_vec(startArrowPoint, startVec, -doubleArrowGap * 0.5);
		}
		if ((endArrowState & Arrow_InOut) == Arrow_InOut)
		{
			endOutArrowPoint = direct_offset_point_by_vec(endArrowPoint, endVec, doubleArrowGap * 0.5);
			endInArrowPoint = direct_offset_point_by_vec(endArrowPoint, endVec, -doubleArrowGap * 0.5);
		}
		if ((startArrowState & Arrow_Out) && (!qFuzzyIsNull(startVec.x()) || !qFuzzyIsNull(startVec.y())))
		{
			DrawArrow(painter, startOutArrowPoint, AngleByVec(startVec), color, arrowLen);
		}
		if ((startArrowState & Arrow_In) && (!qFuzzyIsNull(startVec.x()) || !qFuzzyIsNull(startVec.y())))
		{
			DrawArrow(painter, startInArrowPoint, AngleByVec(-startVec), color, arrowLen);
		}
		if ((endArrowState & Arrow_Out) && (!qFuzzyIsNull(endVec.x()) || !qFuzzyIsNull(endVec.y())))
		{
			DrawArrow(painter, endOutArrowPoint, AngleByVec(endVec), color, arrowLen);
		}
		if ((endArrowState & Arrow_In) && (!qFuzzyIsNull(endVec.x()) || !qFuzzyIsNull(endVec.y())))
		{
			DrawArrow(painter, endInArrowPoint, AngleByVec(-endVec), color, arrowLen);
		}
	}

	void DrawConnCircle(QPainter* painter, const QPoint& point, int radius, bool isCircleUnderPt)
	{
		QBrush brush(utils::ColorHelper::Color(utils::ColorHelper::pure_green));
		painter->setBrush(brush);
		painter->setPen(Qt::NoPen);
		int centerY = isCircleUnderPt ? point.y() + radius : point.y() - radius;
		painter->drawEllipse(QPoint(point.x(), centerY), radius, radius);
	}

	void DrawPortText(QPainter* painter, const QPoint& startPoint, const QPoint& endPoint,
		bool startAtTop, bool endAtTop, bool startIsSwitch, bool endIsSwitch,
		const QString& startPort, const QString& endPort, int connRadius)
	{
		direct_draw_single_port_text(painter, startPoint, startAtTop, startIsSwitch, startPort, connRadius);
		direct_draw_single_port_text(painter, endPoint, endAtTop, endIsSwitch, endPort, connRadius);
	}

	bool IsTopAnchor(const QPoint& point, const IedRect* rect)
	{
		if (!rect)
		{
			return true;
		}
		int topDistance = qAbs(point.y() - (int)rect->y);
		int bottomDistance = qAbs(point.y() - (int)(rect->y + rect->height));
		return topDistance <= bottomDistance;
	}

	bool IsPointAttachedToRect(const QPoint& point, const IedRect* rect)
	{
		if (!rect)
		{
			return false;
		}
		int left = rect->x;
		int right = rect->x + rect->width;
		int top = rect->y;
		int bottom = rect->y + rect->height;
		bool withinHorizontal = point.x() >= left && point.x() <= right;
		bool withinVertical = point.y() >= top && point.y() <= bottom;
		if (withinHorizontal && (point.y() == top || point.y() == bottom))
		{
			return true;
		}
		if (withinVertical && (point.x() == left || point.x() == right))
		{
			return true;
		}
		return false;
	}
}
