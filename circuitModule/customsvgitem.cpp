#include "customsvgitem.h"
#include <QFile>
#include <QGraphicsSceneHoverEvent>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QSvgRenderer>
#include <QVector2D>
#include <QDebug>
#include <QToolTip>
#include "svgmodel.h"

#ifndef _USE_MATH_DEFINES
#define _USE_MATH_DEFINES
#endif
#include <math.h>

CustomSvgItem::CustomSvgItem(const QString& fileName, QGraphicsItem* parent )
	: QGraphicsSvgItem(fileName, parent)
{
	//setAcceptHoverEvents(true);
	//LoadSvg(fileName);
}

CustomSvgItem::CustomSvgItem(QGraphicsItem* parent)
	:QGraphicsSvgItem(parent)
{
	//setAcceptHoverEvents(true);
}

void CustomSvgItem::LoadSvg(const QString& fileName)
{
	m_originalSvgData = LoadSvgData(fileName);
	pugi::xml_parse_result res = m_doc.load(m_originalSvgData.data());
	if (!res)
	{
		qDebug() << QString::fromLocal8Bit("svg 导入错误");
	}

	m_originalSvgData = xmlDocumentToByteArray();
	// 更新QSvgRenderer
	//QSvgRenderer* renderer = this->renderer();
	//if (!renderer)
	//{
	//	this->setSharedRenderer(new QSvgRenderer());
	//	renderer = this->renderer();
	//}

	//if (!renderer->load(m_originalSvgData))
	//{
	//	qDebug() << QString::fromLocal8Bit("svg 渲染错误");
	//}

	this->update();
}

QByteArray CustomSvgItem::LoadSvgData(const QString& fileName)
{
	QFile file(fileName);
	if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		return QByteArray();
	}
	QByteArray data = file.readAll();
	file.close();
	return data;
}

void CustomSvgItem::wheelEvent(QGraphicsSceneWheelEvent* event)
{
	QGraphicsSvgItem::wheelEvent(event);
}

bool CustomSvgItem::isPointOnPath(const QPointF& pos, QLineF& line, quint8 width)
{
	QPainterPath path;
	QPainterPathStroker stroker;
	path.moveTo(line.p1());
	path.lineTo(line.p2());
	stroker.setWidth(width);
	return stroker.createStroke(path).contains(pos);
}

pugi::xml_node CustomSvgItem::GetSvgNodeByPos(const QPointF& pos)
{
	if (m_doc)
	{
		// IED
		// <g fill="none" stroke="#ff0000" stroke-opacity="1" stroke-width="1" stroke-linecap="square" stroke-linejoin="bevel" transform="matrix(1,0,0,1,0,0)" font-family="SimSun" font-size="15" font-weight="400" font-style="normal" iedname="PL2205NA" type="ied">
		pugi::xpath_node_set iedNodeSet = m_doc.select_nodes("//g[@type='ied']");
		for (nodeSetConstIterator iedIt = iedNodeSet.begin(); iedIt != iedNodeSet.end(); ++iedIt)
		{
			pugi::xml_node iedNode = iedIt->node();
			// <path vector-effect="non-scaling-stroke" fill-rule="evenodd" d="M820,490 L1100,490 L1100,590 L820,590 L820,490" />
			pugi::xpath_node pathNode = iedNode.select_node(".//path");
			QRectF iedRect = GetRectFromGNode(pathNode.node());
			if (iedRect.contains(pos))
			{
				return iedNode;
			}
		}

		// circuit
		pugi::xpath_node_set circuitNodeSet = m_doc.select_nodes("//g[@type='circuit']");
		for (nodeSetConstIterator it = circuitNodeSet.begin(); it != circuitNodeSet.end(); ++it)
		{
			pugi::xml_node circuitNode = it->node();
			QList<QLineF> lines = GetLinesFromGNode(circuitNode);
			foreach(QLineF line, lines)
			{
				if (isPointOnPath(pos, line))
				{
					return circuitNode;
				}
			}
		}
	}
	return pugi::xml_node();
}

QList<QLineF> CustomSvgItem::GetLinesFromGNode(pugi::xml_node& gNode)
{
	QList<QLineF> lines;
	pugi::xpath_node_set polylineNodeSet = gNode.select_nodes(".//polyline");
	// 链路svg中，最后两个polyline是链路的箭头，不需要读取
	for (int i = 0; i < polylineNodeSet.size() - 2; ++i)
	{
		pugi::xml_node polylineNode = polylineNodeSet[i].node();
		QString pointsAttr = polylineNode.attribute("points").as_string();
		QLineF line = GetLineFromStr(pointsAttr);
		lines.append(line);
	}
	//for (pugi::xpath_node_set::const_iterator polylineIt = polylineNodeSet.begin(); polylineIt != polylineNodeSet.end(); ++polylineIt)
	//{
	//	pugi::xml_node polylineNode = polylineIt->node();
	//	QString pointsAttr = polylineNode.attribute("points").as_string();
	//	QLineF line = GetLineFromStr(pointsAttr);
	//	lines.append(line);
	//}
	return lines;
}

QRectF CustomSvgItem::GetRectFromGNode(pugi::xml_node& gNode)
{
	if (!gNode) return QRectF();
	QList<QPointF> points;
	// 此处IED是矩形，只需要获取四个路径点即可. 第四个L点为矩形左上角，第二个L点为矩形右下角
	// d="M820,490 L1100,490 L1100,590 L820,590 L820,490"
	QString attr_d = gNode.attribute("d").as_string();
	QRegExp lineToPattern("L\\s*(-?\\d+\\.?\\d*)\\s*,\\s*(-?\\d+\\.?\\d*)");
	int pos = 0;
	while ((pos = lineToPattern.indexIn(attr_d, pos)) != -1)
	{
		//qDebug() << lineToPattern.cap(0);
		qreal x = lineToPattern.cap(1).toDouble();
		qreal y = lineToPattern.cap(2).toDouble();
		points.append(QPointF(x, y));
		pos += lineToPattern.matchedLength();
	}
	if (points.size() != 4)
	{
		qDebug() << QString::fromLocal8Bit("IED矩形读取出错，当前读取矩形为：%1").arg(gNode.parent().attribute("iedname").as_string());
		return QRectF();
	}
	return QRectF(points[3], points[1]);
}

QLineF CustomSvgItem::GetLineFromStr(const QString& str)
{
	// points="820,550 670,550 "
	QString strPoint_1 = str.split(" ")[0];
	QString strPoint_2 = str.split(" ")[1];
	QPointF point_1(strPoint_1.split(",")[0].toFloat(), strPoint_1.split(",")[1].toFloat());
	QPointF point_2(strPoint_2.split(",")[0].toFloat(), strPoint_2.split(",")[1].toFloat());
	return QLineF(point_1, point_2);
}

qreal CustomSvgItem::DegreeToRadians(qreal degree)
{
	return degree * M_PI / 180.0;
}

QPainterPath CustomSvgItem::ParseSvgPath(const QString& d)
{
	QPainterPath path;
	QRegExp cmdExp("([MLZmlz])([^MLZmlz]*)");
	int pos = 0;
	QPointF lastPoint;
	while ((pos = cmdExp.indexIn(d, pos)) != -1)
	{
		QString cmd = cmdExp.cap(1);
		QStringList params = cmdExp.cap(2).split(QRegExp("[ ,]"));
		params.removeAll("");
		if (cmd == "M" || cmd == "m")
		{
			float x = params[0].toFloat(), y = params[1].toFloat();
			path.moveTo(x, y);
			lastPoint = QPointF(x, y);
		}
		else if (cmd == "L" || cmd == "l")
		{
			float x = params[0].toFloat(), y = params[1].toFloat();
			path.lineTo(x, y);
			lastPoint = QPointF(x, y);
		}
		else if (cmd == "Z" || cmd == "z")
		{
			path.closeSubpath();
		}
		pos += cmdExp.matchedLength();
	}
	return path;
}

QPainterPath CustomSvgItem::ParseSvgPolyLine(const QString& points)
{
	QPainterPath path;
	QStringList pointList = points.split(' ', QString::SkipEmptyParts);
	if (pointList.size() < 2) return path;
	for (int i = 0; i < pointList.size(); ++i)
	{
		QStringList xy = pointList[i].split(',');
		if (xy.size() != 2)
		{
			continue;
		}
		float x = xy[0].toFloat(), y = xy[1].toFloat();
		if (i == 0)
			path.moveTo(x, y);
		else
			path.lineTo(x, y);
	}
	return path;
}

// 批量加载交互线
void CustomSvgItem::LoadInteractiveLines(const QString& fileName, QGraphicsScene* scene)
{
	pugi::xml_document doc;
	doc.load_file(fileName.toLocal8Bit());

	// 支持三种类型
	struct TypeInfo
	{
		const char* type;
		InteractiveLineItem::LineType ltype;
		QColor color;
	};
	TypeInfo g_types[] = {
		{ "virtual", InteractiveLineItem::Virtual, QColor(0, 200, 0) },
		{ "logic",   InteractiveLineItem::Logic,   QColor(60, 90, 200) },
		{ "optical", InteractiveLineItem::Optical, QColor(200, 80, 180) }
	};
	const int g_types_count = sizeof(g_types) / sizeof(g_types[0]);
	for (int i = 0; i < g_types_count; ++i)
	{
		QString xpath = QString("//g[@type='%1']//path | //g[@type='%1']//polyline").arg(g_types[i].type);
		QByteArray xpathBytes = xpath.toLocal8Bit();
		pugi::xpath_node_set paths = doc.select_nodes(xpathBytes.constData());
		for (pugi::xpath_node_set::const_iterator it = paths.begin(); it != paths.end(); ++it)
		{
			pugi::xml_node n = it->node();
			QPainterPath path;
			if (QString(n.name()) == "path")
			{
				QString d = n.attribute("d").as_string();
				path = ParseSvgPath(d);
			}
			else if (QString(n.name()) == "polyline")
			{
				QString points = n.attribute("points").as_string();
				path = ParseSvgPolyLine(points);
			}
			InteractiveLineItem* item = new InteractiveLineItem(path, g_types[i].ltype, g_types[i].color);
			item->setZValue(1);
			scene->addItem(item);
			m_interactiveLines.append(item);
		}
	}
}
