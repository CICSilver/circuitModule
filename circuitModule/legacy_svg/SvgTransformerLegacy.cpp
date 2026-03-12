#include "SvgTransformer.h"
#include "qcoreapplication.h"
#include <QDebug>
#include <include/pugixml/pugixml.hpp>
#include "PainterStateGuard.h"
#include "SvgUtils.h"
#include <QBuffer>
#include <sstream>

using utils::ColorHelper;


void SvgTransformer::GenerateSvgByIedName(const QString& iedName)
{
	VirtualSvg* pSvg = BuildVirtualModelByIedName(iedName);
	if (!pSvg)
	{
		return;
	}
	QString virtualFileName = QCoreApplication::applicationDirPath() + "/" + iedName + "_virtual_circuit.svg";
	m_svgGenerator->setFileName(virtualFileName);
	m_svgGenerator->setViewBox(QRect(0, 0, 2160, 1440));
	m_painter->begin(m_svgGenerator);
	DrawVirtualSvg(*pSvg);
	m_painter->end();
	m_svgGenerator->setFileName(QString());
	ReSignSvg(virtualFileName, *pSvg);
	delete pSvg;
	pSvg = NULL;
	if (!m_errStr.isEmpty())
	{
		qDebug() << m_errStr;
	}
}

void SvgTransformer::RenderLogicByIedName(const QString& iedName, QPaintDevice* device)
{
	if (!device)
	{
		return;
	}
	LogicSvg* pSvg = BuildLogicModelByIedName(iedName);
	if (!pSvg)
	{
		return;
	}
	QPainter painter;
	if (!painter.begin(device))
	{
		delete pSvg;
		return;
	}
	QPainter* oldPainter = m_painter;
	m_painter = &painter;
	DrawLogicSvg(*pSvg);
	m_painter = oldPainter;
	painter.end();
	delete pSvg;
}

void SvgTransformer::RenderLogicByIedName(const QString& iedName, QPainter* activePainter)
{
	if (!activePainter)
	{
		return;
	}
	LogicSvg* pSvg = BuildLogicModelByIedName(iedName);
	if (!pSvg)
	{
		return;
	}
	QPainter* oldPainter = m_painter;
	m_painter = activePainter;
	DrawLogicSvg(*pSvg);
	m_painter = oldPainter;
	delete pSvg;
}

void SvgTransformer::RenderOpticalByIedName(const QString& iedName, QPaintDevice* device)
{
	if (!device)
	{
		return;
	}
	OpticalSvg* pSvg = BuildOpticalModelByIedName(iedName);
	if (!pSvg)
	{
		return;
	}
	QPainter painter;
	if (!painter.begin(device))
	{
		delete pSvg;
		return;
	}
	QPainter* oldPainter = m_painter;
	m_painter = &painter;
	DrawOpticalSvg(*pSvg);
	m_painter = oldPainter;
	painter.end();
	delete pSvg;
}

void SvgTransformer::RenderOpticalByIedName(const QString& iedName, QPainter* activePainter)
{
	if (!activePainter)
	{
		return;
	}
	OpticalSvg* pSvg = BuildOpticalModelByIedName(iedName);
	if (!pSvg)
	{
		return;
	}
	QPainter* oldPainter = m_painter;
	m_painter = activePainter;
	DrawOpticalSvg(*pSvg);
	m_painter = oldPainter;
	delete pSvg;
}

void SvgTransformer::RenderVirtualByIedName(const QString& iedName, QPaintDevice* device)
{
	if (!device)
	{
		return;
	}
	VirtualSvg* pSvg = BuildVirtualModelByIedName(iedName);
	if (!pSvg)
	{
		return;
	}
	QPainter painter;
	if (!painter.begin(device))
	{
		delete pSvg;
		return;
	}
	QPainter* oldPainter = m_painter;
	m_painter = &painter;
	DrawVirtualSvg(*pSvg);
	m_painter = oldPainter;
	painter.end();
	delete pSvg;
}

void SvgTransformer::RenderVirtualByIedName(const QString& iedName, QPainter* activePainter)
{
	if (!activePainter)
	{
		return;
	}
	VirtualSvg* pSvg = BuildVirtualModelByIedName(iedName);
	if (!pSvg)
	{
		return;
	}
	QPainter* oldPainter = m_painter;
	m_painter = activePainter;
	DrawVirtualSvg(*pSvg);
	m_painter = oldPainter;
	delete pSvg;
}

void SvgTransformer::RenderWholeCircuitByIedName(const QString& iedName, QPaintDevice* device)
{
	if (!device)
	{
		return;
	}
	WholeCircuitSvg* pSvg = BuildWholeCircuitModelByIedName(iedName);
	if (!pSvg)
	{
		return;
	}
	QPainter painter;
	if (!painter.begin(device))
	{
		delete pSvg;
		return;
	}
	QPainter* oldPainter = m_painter;
	m_painter = &painter;
	DrawWholeSvg(*pSvg);
	m_painter = oldPainter;
	painter.end();
	delete pSvg;
}

void SvgTransformer::RenderWholeCircuitByIedName(const QString& iedName, QPainter* activePainter)
{
	if (!activePainter)
	{
		return;
	}
	WholeCircuitSvg* pSvg = BuildWholeCircuitModelByIedName(iedName);
	if (!pSvg)
	{
		return;
	}
	QPainter* oldPainter = m_painter;
	m_painter = activePainter;
	DrawWholeSvg(*pSvg);
	m_painter = oldPainter;
	delete pSvg;
}

QImage SvgTransformer::RenderLogicToImage(const QString& iedName, const QSize& size)
{
	LogicSvg* pSvg = BuildLogicModelByIedName(iedName);
	if (!pSvg)
	{
		return QImage();
	}
	QSize target = size.isValid() ? size : QSize(SVG_VIEWBOX_WIDTH, SVG_VIEWBOX_HEIGHT);
	QImage img(target, QImage::Format_ARGB32_Premultiplied);
	img.fill(Qt::black);
	QPainter painter(&img);
	QPainter* oldPainter = m_painter;
	m_painter = &painter;
	DrawLogicSvg(*pSvg);
	m_painter = oldPainter;
	painter.end();
	delete pSvg;
	return img;
}

QImage SvgTransformer::RenderOpticalToImage(const QString& iedName, const QSize& size)
{
	OpticalSvg* pSvg = BuildOpticalModelByIedName(iedName);
	if (!pSvg)
	{
		return QImage();
	}
	QSize target = size.isValid() ? size : QSize(SVG_VIEWBOX_WIDTH, SVG_VIEWBOX_HEIGHT);
	QImage img(target, QImage::Format_ARGB32_Premultiplied);
	img.fill(Qt::black);
	QPainter painter(&img);
	QPainter* oldPainter = m_painter;
	m_painter = &painter;
	DrawOpticalSvg(*pSvg);
	m_painter = oldPainter;
	painter.end();
	delete pSvg;
	return img;
}

QImage SvgTransformer::RenderVirtualToImage(const QString& iedName, const QSize& size)
{
	VirtualSvg* pSvg = BuildVirtualModelByIedName(iedName);
	if (!pSvg)
	{
		return QImage();
	}
	QSize target = size.isValid() ? size : QSize(SVG_VIEWBOX_WIDTH, SVG_VIEWBOX_HEIGHT);
	QImage img(target, QImage::Format_ARGB32_Premultiplied);
	img.fill(Qt::black);
	QPainter painter(&img);
	QPainter* oldPainter = m_painter;
	m_painter = &painter;
	DrawVirtualSvg(*pSvg);
	m_painter = oldPainter;
	painter.end();
	delete pSvg;
	return img;
}

QImage SvgTransformer::RenderWholeCircuitToImage(const QString& iedName, const QSize& size)
{
	WholeCircuitSvg* pSvg = BuildWholeCircuitModelByIedName(iedName);
	if (!pSvg)
	{
		return QImage();
	}
	QSize target = size.isValid() ? size : QSize(SVG_VIEWBOX_WIDTH, SVG_VIEWBOX_HEIGHT);
	QImage img(target, QImage::Format_ARGB32_Premultiplied);
	img.fill(Qt::black);
	QPainter painter(&img);
	QPainter* oldPainter = m_painter;
	m_painter = &painter;
	DrawWholeSvg(*pSvg);
	m_painter = oldPainter;
	painter.end();
	delete pSvg;
	return img;
}

void SvgTransformer::GenerateLogicSvg(const IED* pIed, const QString& filePath)
{
	if (!pIed || filePath.isEmpty())
	{
		return;
	}
	LogicSvg* pSvg = BuildLogicModelByIedName(pIed->name);
	if (!pSvg)
	{
		return;
	}
	m_svgGenerator->setFileName(filePath);
	m_svgGenerator->setViewBox(QRect(0, 0, SVG_VIEWBOX_WIDTH, SVG_VIEWBOX_HEIGHT));
	m_painter->begin(m_svgGenerator);
	DrawLogicSvg(*pSvg);
	m_painter->end();
	m_svgGenerator->setFileName(QString());
	ReSignSvg(filePath, *pSvg);
	delete pSvg;
}

void SvgTransformer::GenerateOpticalSvg(const IED* pIed, const QString& filePath)
{
	if (!pIed || filePath.isEmpty())
	{
		return;
	}
	OpticalSvg* pSvg = BuildOpticalModelByIedName(pIed->name);
	if (!pSvg)
	{
		return;
	}
	m_svgGenerator->setFileName(filePath);
	m_svgGenerator->setViewBox(QRect(0, 0, SVG_VIEWBOX_WIDTH, SVG_VIEWBOX_HEIGHT));
	m_painter->begin(m_svgGenerator);
	DrawOpticalSvg(*pSvg);
	m_painter->end();
	m_svgGenerator->setFileName(QString());
	ReSignSvg(filePath, *pSvg);
	delete pSvg;
}

void SvgTransformer::GenerateVirtualSvg(const IED* pIed, const QString& filePath)
{
	if (!pIed || filePath.isEmpty())
	{
		return;
	}
	VirtualSvg* pSvg = BuildVirtualModelByIedName(pIed->name);
	if (!pSvg)
	{
		return;
	}
	m_svgGenerator->setFileName(filePath);
	m_svgGenerator->setViewBox(QRect(0, 0, SVG_VIEWBOX_WIDTH, SVG_VIEWBOX_HEIGHT));
	m_painter->begin(m_svgGenerator);
	DrawVirtualSvg(*pSvg);
	m_painter->end();
	m_svgGenerator->setFileName(QString());
	ReSignSvg(filePath, *pSvg);
	delete pSvg;
}

QByteArray SvgTransformer::GenerateLogicSvgBytes(const QString& iedName)
{
	LogicSvg* pSvg = BuildLogicModelByIedName(iedName);
	if (!pSvg)
	{
		return QByteArray();
	}
	QByteArray bytes;
	QBuffer buffer(&bytes);
	buffer.open(QIODevice::WriteOnly);
	QSvgGenerator generator;
	generator.setOutputDevice(&buffer);
	generator.setViewBox(QRect(0, 0, SVG_VIEWBOX_WIDTH, SVG_VIEWBOX_HEIGHT));
	QPainter painter;
	painter.begin(&generator);
	QPainter* oldPainter = m_painter;
	m_painter = &painter;
	DrawLogicSvg(*pSvg);
	m_painter = oldPainter;
	painter.end();
	buffer.close();
	pugi::xml_document doc;
	doc.load_buffer(bytes.constData(), bytes.size());
	ReSignSvgDoc(doc, *pSvg);
	delete pSvg;
	std::ostringstream oss;
	doc.save(oss, "", pugi::format_raw, pugi::encoding_utf8);
	std::string svgText = oss.str();
	return QByteArray(svgText.data(), int(svgText.size()));
}

QByteArray SvgTransformer::GenerateOpticalSvgBytes(const QString& iedName)
{
	OpticalSvg* pSvg = BuildOpticalModelByIedName(iedName);
	if (!pSvg)
	{
		return QByteArray();
	}
	QByteArray bytes;
	QBuffer buffer(&bytes);
	buffer.open(QIODevice::WriteOnly);
	QSvgGenerator generator;
	generator.setOutputDevice(&buffer);
	generator.setViewBox(QRect(0, 0, SVG_VIEWBOX_WIDTH, SVG_VIEWBOX_HEIGHT));
	QPainter painter;
	painter.begin(&generator);
	QPainter* oldPainter = m_painter;
	m_painter = &painter;
	DrawOpticalSvg(*pSvg);
	m_painter = oldPainter;
	painter.end();
	buffer.close();
	pugi::xml_document doc;
	doc.load_buffer(bytes.constData(), bytes.size());
	ReSignSvgDoc(doc, *pSvg);
	delete pSvg;
	std::ostringstream oss;
	doc.save(oss, "", pugi::format_raw, pugi::encoding_utf8);
	std::string svgText = oss.str();
	return QByteArray(svgText.data(), int(svgText.size()));
}

QByteArray SvgTransformer::GenerateVirtualSvgBytes(const QString& iedName, const QString& swName)
{
	Q_UNUSED(swName);
	VirtualSvg* pSvg = BuildVirtualModelByIedName(iedName);
	if (!pSvg)
	{
		return QByteArray();
	}
	QByteArray bytes;
	QBuffer buffer(&bytes);
	buffer.open(QIODevice::WriteOnly);
	QSvgGenerator generator;
	generator.setOutputDevice(&buffer);
	generator.setViewBox(QRect(0, 0, SVG_VIEWBOX_WIDTH, SVG_VIEWBOX_HEIGHT));
	QPainter painter;
	painter.begin(&generator);
	QPainter* oldPainter = m_painter;
	m_painter = &painter;
	DrawVirtualSvg(*pSvg);
	m_painter = oldPainter;
	painter.end();
	buffer.close();
	pugi::xml_document doc;
	doc.load_buffer(bytes.constData(), bytes.size());
	ReSignSvgDoc(doc, *pSvg);
	delete pSvg;
	std::ostringstream oss;
	doc.save(oss, "", pugi::format_raw, pugi::encoding_utf8);
	std::string svgText = oss.str();
	return QByteArray(svgText.data(), int(svgText.size()));
}

QByteArray SvgTransformer::GenerateWholeCircuitSvgBytes(const QString& iedName)
{
	WholeCircuitSvg* pSvg = BuildWholeCircuitModelByIedName(iedName);
	if (!pSvg)
	{
		return QByteArray();
	}
	QByteArray bytes;
	QBuffer buffer(&bytes);
	buffer.open(QIODevice::WriteOnly);
	QSvgGenerator generator;
	generator.setOutputDevice(&buffer);
	generator.setViewBox(QRect(0, 0, SVG_VIEWBOX_WIDTH, SVG_VIEWBOX_HEIGHT));
	QPainter painter;
	painter.begin(&generator);
	QPainter* oldPainter = m_painter;
	m_painter = &painter;
	DrawWholeSvg(*pSvg);
	m_painter = oldPainter;
	painter.end();
	buffer.close();
	pugi::xml_document doc;
	doc.load_buffer(bytes.constData(), bytes.size());
	ReSignSvgDoc(doc, *pSvg);
	delete pSvg;
	std::ostringstream oss;
	doc.save(oss, "", pugi::format_raw, pugi::encoding_utf8);
	std::string svgText = oss.str();
	return QByteArray(svgText.data(), int(svgText.size()));
}

void SvgTransformer::DrawLogicSvg(LogicSvg& svg)
{
	// 主IED
	DrawIedRect(svg.mainIedRect);		// 主体
	DrawExternalRect(svg.mainIedRect, QString::fromLocal8Bit("检修域"));	// 外部虚线框
	// 关联IED
	DrawIedRect(svg.leftIedRectList);	// 左侧
	DrawIedRect(svg.rightIedRectList);	// 右侧
	DrawExternalRect(svg.leftIedRectList, QString::fromLocal8Bit("影响域"), true);	// 左侧外部虚线框，上方描述矩形
	DrawExternalRect(svg.rightIedRectList, QString::fromLocal8Bit("影响域"));	// 右侧外部虚线框
	// 关联链路
	DrawLogicCircuitLine(svg.leftIedRectList);
	DrawLogicCircuitLine(svg.rightIedRectList);
}

void SvgTransformer::DrawOpticalSvg(OpticalSvg& svg)
{
	if(svg.mainIedRect->iedName == "PL2205NA")
	{
		int a = 1;
	}
	// 主IED
	DrawIedRect(svg.mainIedRect);
	// 下方关联IED
	foreach(IedRect * pRect, svg.iedRectList)
	{
		DrawIedRect(pRect);
	}
	// 光纤链路
	foreach(OpticalCircuitLine* optLine, svg.opticalCircuitLineList)
	{
		DrawOpticalLine(optLine);
	}
	// 交换机
	foreach(IedRect* pRect, svg.switcherRectList)
	{
		DrawSwitcherRect(pRect);
	}
	//端口号
	foreach(OpticalCircuitLine* pLine, svg.opticalCircuitLineList)
	{
		DrawPortText(pLine, CONN_R);
	}
}

void SvgTransformer::DrawVirtualSvg(VirtualSvg& svg)
{
	// 绘制IED矩形
	DrawIedRect(svg.mainIedRect);
	DrawIedRect(svg.leftIedRectList);
	DrawIedRect(svg.rightIedRectList);
	// 绘制链路
	DrawVirtualCircuitLine(svg.leftIedRectList);
	DrawVirtualCircuitLine(svg.rightIedRectList);
	// 绘制压板
	DrawPlate(svg.plateRectHash);
}

void SvgTransformer::DrawWholeSvg(WholeCircuitSvg& svg)
{
	// 绘制IED矩形
	DrawIedRect(svg.mainIedRect);
	DrawIedRect(svg.leftIedRectList);
	DrawIedRect(svg.rightIedRectList);
	// 绘制链路
	foreach(IedRect * pIedRect, svg.leftIedRectList)
	{
		QPoint iconStartPos = QPoint(pIedRect->x + pIedRect->width - ICON_LENGTH * 2, pIedRect->y + pIedRect->height + ICON_LENGTH);
		foreach(LogicCircuitLine* pLogicCircuit, pIedRect->logic_line_list)
		{
		}
	}
}

void SvgTransformer::DrawConnCircle(const QPoint& pt, int radius, bool isCircleUnderPt)
{
	//m_painter->save();
	PAINTER_STATE_GUARD(m_painter);
	QBrush brush(ColorHelper::Color(ColorHelper::pure_green));
	m_painter->setBrush(brush);
	int y = isCircleUnderPt ? pt.y() + radius : pt.y() - radius;
	m_painter->drawEllipse(QPoint(pt.x(), y), radius, radius);
	// m_painter->restore();
}

void SvgTransformer::drawArrowHeader(const QPoint& endPoint, double arrowAngle, QColor color, int arrowLen)
{
	//m_painter->save();
	PAINTER_STATE_GUARD(m_painter);
	double arrowHeadLineAngle = 150;
	QPointF leftArrowPoint = endPoint + QPointF(
		arrowLen * cos(AngleToRadians(arrowAngle + arrowHeadLineAngle)),
		-arrowLen * sin(AngleToRadians(arrowAngle + arrowHeadLineAngle)));
	QPointF rightArrowPoint = endPoint + QPointF(
		arrowLen * cos(AngleToRadians(arrowAngle - arrowHeadLineAngle)),
		-arrowLen * sin(AngleToRadians(arrowAngle - arrowHeadLineAngle)));
	int offset = 8;
	// 考虑当箭头方向不是垂直时的偏移量，offset偏移仅为箭头方向的偏移量
	// 箭头方向减少偏移，假设箭头点位置为endPoint，则偏移点位置为
	QPointF offsetPoint = QPointF(
		endPoint.x() + offset * cos(AngleToRadians(arrowAngle)),
		endPoint.y() - offset * sin(AngleToRadians(arrowAngle)));
	QBrush brush(color, Qt::SolidPattern);
	QPen pen(Qt::NoPen);
	m_painter->setPen(pen);
	m_painter->setBrush(brush);
	QVector<QPointF> points;
	points << leftArrowPoint << offsetPoint << rightArrowPoint << endPoint;
	m_painter->drawPolygon(points.data(), 4);
	// m_painter->restore();
}

double SvgTransformer::GetAngleByVec(const QPointF& vec) const
{
	double direction = atan2(-vec.y(), vec.x());
	double angle = direction * (180 / M_PI);
	return angle;
}

QPoint SvgTransformer::GetArrowPt(const QPoint& pt, int arrowLen, int conn_r, double angle, bool isUnderConnpt, int offset /* 箭头偏移量，向前兼容 */)
{
	//int offset = 10;
	// 保留原有垂直方向的特殊处理逻辑，确保向前兼容
	if (abs(angle - 90) < 0.001 || abs(angle + 90) < 0.001)
	{
		// 原有垂直方向的处理，保持不变
		if (isUnderConnpt)
		{
			return QPoint(
				pt.x(),
				pt.y() + 2 * conn_r + (angle > 0 ? 1 : (2 * arrowLen + 5)) + offset
			);
		}
		else
		{
			return QPoint(
				pt.x(),
				pt.y() - 2 * conn_r - (angle > 0 ? (2 * arrowLen + 5) : 1) - offset
			);
		}
	}
	// 任意方向的箭头偏移计算
	double angleRad = AngleToRadians(angle);
	// 根据参数计算箭头点在连接点的哪一侧
	int direction = isUnderConnpt ? 1 : -1;
	// 计算总偏移距离
	int totalOffset = 2 * conn_r + arrowLen + offset;
	// 使用三角函数计算任意方向上的偏移点
	return QPoint(
		pt.x() + direction * totalOffset * cos(angleRad),
		pt.y() - direction * totalOffset * sin(angleRad)
	);
}

void SvgTransformer::DrawDescRect(const IedRect& leftTopRect)
{
	DrawDescRect(leftTopRect, m_painter);
}

void SvgTransformer::DrawDescRect(const IedRect& leftTopRect, QPainter* _painter)
{
	QFont descFont;
	descFont.setPointSize(15);
	QFontMetrics fm(descFont);
	quint16 width = 180;
	quint16 height = fm.height() * 2;
	quint16 x = leftTopRect.x;
	quint16 y = leftTopRect.y - height - 20;
	QPen pen;
	//pen.setColor(ColorHelper::Color(ColorHelper::line_gse));
	pen.setColor(Qt::white);
	pen.setStyle(Qt::DashLine);
	_painter->setPen(pen);
	_painter->setFont(descFont);
	quint8 descLineWidth = 160;
	_painter->drawRect(x, y, width, height);
	_painter->drawText(x, y + fm.height(), "GSE");
	_painter->drawText(x, y + fm.height() * 2, "SV");
}

void SvgTransformer::DrawIedRect(IedRect* rect)
{
	DrawIedRect(rect, m_painter);
}

void SvgTransformer::DrawIedRect(IedRect* rect, QPainter* _painter)
{
	PAINTER_STATE_GUARD(_painter);
	QPen pen;
	QBrush brush(rect->underground_color);
	if (rect->extend_height != 0) // 需要绘制额外的外部矩形框
	{
		brush.setColor(ColorHelper::Color(ColorHelper::ied_underground));
		pen.setColor(ColorHelper::Color(ColorHelper::ied_border));
		pen.setStyle(Qt::DashLine);
		int margin = 15;
		_painter->setPen(pen);
		_painter->setBrush(brush);
		QPoint tl(rect->x - rect->inner_gap, rect->y - rect->inner_gap);
		QPoint br(rect->x + rect->inner_gap, rect->y + rect->height + rect->extend_height + rect->inner_gap);
		QRect dashRect(rect->x - rect->inner_gap, rect->y - rect->inner_gap, 2 * rect->inner_gap + rect->width, rect->GetExtendHeight());
		_painter->fillRect(dashRect, brush);
		_painter->setBrush(Qt::NoBrush);
		_painter->drawRect(dashRect);
	}
	pen.setStyle(Qt::SolidLine);
	pen.setColor(ColorHelper::Color(rect->border_color));
	_painter->setPen(pen);
	QFont font;
	font.setFamily(rect->iedName);
	font.setPointSize(TYPE_IED);
	_painter->setFont(font);
	if (rect->underground_color == 0) // 透明
		_painter->drawRect(rect->x, rect->y, rect->width, rect->height);
	else
	{
		_painter->fillRect(rect->x, rect->y, rect->width, rect->height, brush);
	}
	DrawTextInRect(_painter, rect, rect->iedName, rect->iedDesc);
}

void SvgTransformer::DrawIedRect(QList<IedRect*>& rectList)
{
	DrawIedRect(rectList, m_painter);
}

void SvgTransformer::DrawIedRect(QList<IedRect*>& rectList, QPainter* _painter)
{
	if (rectList.isEmpty())
	{
		return;
	}
	foreach(IedRect * rect, rectList)
	{
		DrawIedRect(rect, _painter);
	}
}

void SvgTransformer::DrawSwitcherRect(IedRect* rect)
{
	DrawSwitcherRect(rect, m_painter);
}

void SvgTransformer::DrawSwitcherRect(IedRect* rect, QPainter* _painter)
{
	PAINTER_STATE_GUARD(_painter);
	QPen pen;
	pen.setColor(rect->border_color);
	_painter->setPen(pen);
	QBrush brush(rect->underground_color);
	QFont font;
	font.setFamily(rect->iedName);
	font.setPointSize(TYPE_Switcher);
	_painter->setFont(font);
	_painter->fillRect(rect->x, rect->y, rect->width, rect->height, brush);
	DrawTextInRect(rect, rect->iedName, rect->iedDesc, 18);
}

void SvgTransformer::DrawTextInRect(SvgRect* rect, QString name, QString desc, int pointSize)
{
	DrawTextInRect(m_painter, rect, name, desc, pointSize);
}

void SvgTransformer::DrawTextInRect(QPainter* _painter, SvgRect* rect, QString name, QString desc, int pointSize)
{
	PAINTER_STATE_GUARD(_painter);
	// 设置字体大小
	QString text = desc;
	QPen pen;
	pen.setColor(Qt::white);
	QFont font = _painter->font();
	font.setPointSize(pointSize);
	_painter->setFont(font);
	_painter->setPen(pen);
	// 计算文字位置及换行
	QFontMetrics metrics(_painter->font());
	int maxWidth = rect->width - 2 * rect->padding;
	QStringList lines;
	QString currentLine;
	int currentWidth = 0;
	lines.append(name);
	lines.append("");
	foreach(const QChar & ch, text)
	{
		int charWidth = metrics.width(ch);
		if (currentWidth + charWidth > maxWidth)
		{
			lines.append(currentLine);
			currentLine.clear();
			currentWidth = 0;
		}
		currentLine.append(ch);
		currentWidth += charWidth;
	}
	if (!currentLine.isEmpty())
	{
		lines.append(currentLine);
	}
	int lineHeight = metrics.height();
	int totalTextHeight = lines.size() * lineHeight;
	int text_y = rect->y + (rect->height - totalTextHeight) / 2 + lineHeight - metrics.descent();
	foreach(const QString & line, lines)
	{
		int textWidth = metrics.width(line);
		int text_x = rect->x + (rect->width - textWidth) / 2;
		_painter->drawText(text_x, text_y, line);
		text_y += lineHeight;
	}
}

void SvgTransformer::DrawExternalRect(QList<IedRect*>& rectList, QString title, bool hasDescIedRect)
{
	// 绘制外部虚线框
	if (rectList.isEmpty())
	{
		return;
	}
	IedRect* firstIed = rectList.first();
	IedRect* lastIed = rectList.last();
	QFont font;
	font.setPointSize(15);
	m_painter->setFont(font);
	QFontMetrics fm(font);
	quint16 titleWidth = fm.width(title);
	quint16 titleHeight = fm.height();
	quint16 x = firstIed->x - firstIed->horizontal_margin - titleWidth;
	quint16 y = firstIed->y - firstIed->vertical_margin;
	quint16 width = firstIed->width + 2 * firstIed->horizontal_margin + titleWidth;
	quint16 height = (lastIed->y - y + lastIed->height) + lastIed->vertical_margin;
	QRect externalRect(x, y, width, height);
	QPen pen;
	pen.setStyle(Qt::DashLine);
	pen.setColor(ColorHelper::Color(firstIed->border_color));
	m_painter->setPen(pen);
	m_painter->drawRect(externalRect);
	pen.setColor(Qt::white);
	m_painter->setPen(pen);
	m_painter->drawText(x + 10, y + titleHeight, title);
	// 描述矩形
	if (hasDescIedRect)
	{
		//IedRect* leftTopRect = rectList.first();
		quint16 line_width = 140;
		quint16 desc_width = 180;
		quint16 desc_height = fm.height() * 2 + 10;
		quint16 desc_x = x;
		quint16 desc_y = y - desc_height - 20;
		QPen pen;
		//pen.setColor(ColorHelper::Color(ColorHelper::line_gse));
		pen.setColor(Qt::white);
		pen.setStyle(Qt::DashLine);
		m_painter->setPen(pen);
		m_painter->setFont(font);
		m_painter->drawRect(desc_x, desc_y, desc_width, desc_height);
		m_painter->drawText(desc_x + line_width + 10, desc_y + fm.height(), "GSE");
		m_painter->drawText(desc_x + line_width + 10, desc_y + fm.height() * 2, "SV");
		quint16 line_x = desc_x + 5;
		quint16 line_y = desc_y + fm.height() / 2 + 5;
		pen.setStyle(Qt::SolidLine);
		pen.setColor(ColorHelper::Color(ColorHelper::line_gse));
		m_painter->setPen(pen);
		m_painter->drawLine(line_x, line_y, line_x + line_width, line_y);
		pen.setColor(ColorHelper::Color(ColorHelper::line_smv));
		m_painter->setPen(pen);
		m_painter->drawLine(line_x, line_y + fm.height(), line_x + line_width, line_y + fm.height());
	}
}

void SvgTransformer::DrawExternalRect(IedRect* rect, QString title, bool hasDescIedRect)
{
	DrawExternalRect(QList<IedRect*>() << rect, title, hasDescIedRect);
}

void SvgTransformer::DrawWholeRect(IedRect* rect)
{
	// 绘制矩形
	DrawIedRect(rect);
}

void SvgTransformer::DrawLogicCircuitLine(QList<IedRect*>& rectList)
{
	foreach(IedRect* rect, rectList)
	{
		foreach(LogicCircuitLine* line, rect->logic_line_list)
		{
			//m_painter->save();
			PAINTER_STATE_GUARD(m_painter);
			QPen pen;
			pen.setStyle(Qt::SolidLine);
			QColor color = line->pLogicCircuit->type == SV ? ColorHelper::Color(ColorHelper::line_smv) : ColorHelper::Color(ColorHelper::line_gse);
			pen.setColor(color);
			m_painter->setPen(pen);
			// 仅作链路区分，对图像显示无影响
			QFont font;
			font.setPointSize(TYPE_LogicCircuit);	// 标识该<g>节点为链路父节点
			//font.setWeight(line->id);	// 标识链路id
			QString signStr = joinGroups() <<
				(QString)(joinFields() << m_element_id++) <<
				// srcIedName cbName
				(QString)(joinFields() << line->pLogicCircuit->pSrcIed->name << line->pLogicCircuit->cbName) <<
				// destIedName
				(QString)(joinFields() << line->pLogicCircuit->pDestIed->name);
			font.setFamily(signStr);
			//font.setWeight(m_circuit_id++);	// 标识链路id
			m_painter->setFont(font);
			// 起点为子设备连接点，终点为主设备连接点
			m_painter->drawLine(line->startPoint, line->midPoint);
			m_painter->drawLine(line->midPoint, line->endPoint);
			double angle = GetAngleByVec(line->endPoint - line->midPoint);
			font.setPointSize(TYPE_Circuit_Arrow);
			m_painter->setFont(font);
			drawArrowHeader(GetArrowPt(line->endPoint, ARROW_LEN, 0, angle, false, 0), angle, color);
			// m_painter->restore();
		}
	}
}

void SvgTransformer::DrawVirtualCircuitLine(QList<IedRect*>& rectList)
{
	foreach(IedRect * rect, rectList)
	{
		foreach(LogicCircuitLine * pLogicLine, rect->logic_line_list)
		{
			foreach(VirtualCircuitLine * pVtLine, pLogicLine->virtual_line_list)
			{
				//m_painter->save();
				PAINTER_STATE_GUARD(m_painter);
				QPen pen;
				QColor green = ColorHelper::Color(ColorHelper::pure_green);
				pen.setColor(green);
				m_painter->setPen(pen);
				// 绘制图标，静态绘制时按连通状态绘制
				DrawVirtualIcon(pVtLine->startIconPt, pLogicLine->pLogicCircuit->type, ColorHelper::pure_green);
				DrawVirtualIcon(pVtLine->endIconPt, pLogicLine->pLogicCircuit->type, ColorHelper::pure_green);
				// 绘制值
				pVtLine->startValRect.setHeight(0);
				pVtLine->endValRect.setHeight(0);
				DrawVirtualText(pVtLine->startValRect, pVtLine->valStr, "out");
				DrawVirtualText(pVtLine->endValRect, pVtLine->valStr, "in");
				// 绘制描述信息
				DrawVirtualText(pVtLine->circuitDescRect, pVtLine->circuitDesc, "desc");
				// 记录信息到回路图像
				QFont font;
				font.setPointSize(TYPE_VirtualCircuit);
				//font.setWeight(m_circuit_id++);	// 标识链路id
				// 信息整合
				QString vtLineInfo = joinGroups() <<
					// id 0
					(QString)(joinGroups() << m_element_id++) <<
					// srcIedName:destIedName 1
					(QString)(joinFields() << pVtLine->pVirtualCircuit->srcIedName << pVtLine->pVirtualCircuit->destIedName) <<
					// srcSoftPlateDesc:destSoftPlateDesc 2
					(QString)(joinFields() << pVtLine->pVirtualCircuit->srcSoftPlateDesc << pVtLine->pVirtualCircuit->destSoftPlateDesc) <<
					// srcSoftPlateRef:destSoftPlateRef 3
					(QString)(joinFields() << pVtLine->pVirtualCircuit->srcSoftPlateRef << pVtLine->pVirtualCircuit->destSoftPlateRef) <<
					// srcRef:destRef 4
					(QString)(joinFields() << pVtLine->pVirtualCircuit->srcRef << pVtLine->pVirtualCircuit->destRef) <<
					// srcName:destName 5
					(QString)(joinFields() << pVtLine->pVirtualCircuit->srcName << pVtLine->pVirtualCircuit->destName) <<
					// remoteId:remoteSigId_A:remoteSigId_B 6
					(QString)(joinFields() << pVtLine->pVirtualCircuit->inRemoteCode << pVtLine->pVirtualCircuit->outRemoteCode) <<
					// virtualType 7
					(QString)(joinFields() << (pVtLine->pVirtualCircuit->type == GOOSE ? "gse" : "sv")) <<
					// softPlateCode 8
					(QString)(joinFields() << pVtLine->pVirtualCircuit->srcSoftPlateCode << pVtLine->pVirtualCircuit->destSoftPlateCode) <<
					// line Code 9
					(QString)(joinFields() << pVtLine->pVirtualCircuit->code) <<
					// circuitDesc 10
					(QString)(joinFields() << pVtLine->circuitDesc);
				font.setFamily(vtLineInfo);
				m_painter->setFont(font);
				m_painter->drawLine(pVtLine->startPoint, pVtLine->endPoint);
				font.setPointSize(TYPE_Circuit_Arrow);
				m_painter->setFont(font);
				drawArrowHeader(pVtLine->endPoint, GetAngleByVec(pVtLine->endPoint - pVtLine->startPoint));
				// m_painter->restore();
			}
		}
	}
}

static QPoint BuildArrowTipPoint(const QPoint& endpoint, const QPoint& innerPoint, int offset)
{
	QPointF direction = innerPoint - endpoint;
	double length = sqrt(direction.x() * direction.x() + direction.y() * direction.y());
	if (length < 0.001)
	{
		return endpoint;
	}
	double scale = offset / length;
	QPointF tipPoint(endpoint.x() + direction.x() * scale, endpoint.y() + direction.y() * scale);
	return QPoint(qRound(tipPoint.x()), qRound(tipPoint.y()));
}

static QPoint OffsetArrowPointByVec(const QPoint& point, const QPoint& vec, double offset)
{
	double length = sqrt((double)vec.x() * (double)vec.x() + (double)vec.y() * (double)vec.y());
	if (length < 0.001)
	{
		return point;
	}
	QPointF shiftedPoint(point.x() + vec.x() / length * offset, point.y() + vec.y() / length * offset);
	return QPoint(qRound(shiftedPoint.x()), qRound(shiftedPoint.y()));
}

static bool IsPointAttachedToRect(const QPoint& point, const IedRect* pRect)
{
	if (!pRect)
	{
		return false;
	}
	int left = pRect->x;
	int right = pRect->x + pRect->width;
	int top = pRect->y;
	int bottom = pRect->y + pRect->height;
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

static QPoint ResolveArrowAdjacentPoint(const QVector<QPoint>& pointList, int endpointIndex)
{
	if (pointList.isEmpty() || endpointIndex < 0 || endpointIndex >= pointList.size())
	{
		return QPoint();
	}
	QPoint endpoint = pointList.at(endpointIndex);
	if (endpointIndex == 0)
	{
		for (int pointIndex = 1; pointIndex < pointList.size(); ++pointIndex)
		{
			if (pointList.at(pointIndex) != endpoint)
			{
				return pointList.at(pointIndex);
			}
		}
	}
	else
	{
		for (int pointIndex = endpointIndex - 1; pointIndex >= 0; --pointIndex)
		{
			if (pointList.at(pointIndex) != endpoint)
			{
				return pointList.at(pointIndex);
			}
		}
	}
	return endpoint;
}

void SvgTransformer::DrawOpticalLine(OpticalCircuitLine* optLine)
{
	QPen pen;
	pen.setColor(ColorHelper::Color(ColorHelper::pure_green));
	//m_painter->save();
	PAINTER_STATE_GUARD(m_painter);
	m_painter->setPen(pen);
	// 记录链路信息到SVG文件
	QFont font;
	QString connStatus = optLine->pOpticalCircuit->connStatus ? "true" : "false";	// true/false
	QString optInfo = joinGroups() <<
		(QString)(joinFields() << m_element_id++) <<
		(QString)(joinFields() << optLine->pOpticalCircuit->loopCode << optLine->pOpticalCircuit->code) <<
		(QString)(joinFields() << optLine->pOpticalCircuit->srcIedName << optLine->pOpticalCircuit->srcIedPort) <<
		(QString)(joinFields() << optLine->pOpticalCircuit->destIedName << optLine->pOpticalCircuit->destIedPort) <<
		(QString)(joinFields() << optLine->pOpticalCircuit->connStatus) <<
		(QString)(joinFields() << optLine->pOpticalCircuit->remoteId);
		//optLine->pOpticalCircuit->cableDesc << connStatus << optLine->pOpticalCircuit->code;
	font.setFamily(optInfo);
	font.setPointSize(TYPE_OpticalCircuit);
	//font.setWeight(m_circuit_id++);	// 标识链路id
	m_painter->setFont(font);
	// 绘制光纤链路折线
	QVector<QPoint> points;
	points << optLine->startPoint;
	foreach(const QPoint& pt, optLine->midPoints)
	{
		points << pt;
	}
	points << optLine->endPoint;
	m_painter->drawPolyline(points.data(), points.size());
	// 绘制连接点圆
	font.setPointSize(TYPE_Optical_ConnCircle);
	//font.setFamily(DEFAULT_FONT_FAMILY);
	m_painter->setFont(font);
	QPoint& upPoint = optLine->startPoint.y() < optLine->endPoint.y() ? optLine->startPoint : optLine->endPoint;
	QPoint& downPoint = optLine->startPoint.y() < optLine->endPoint.y() ? optLine->endPoint : optLine->startPoint;
	DrawConnCircle(upPoint, CONN_R);
	DrawConnCircle(downPoint, CONN_R, false);
	font.setPointSize(TYPE_Circuit_Arrow);
	m_painter->setFont(font);
	if (points.size() < 2 || !optLine->pSrcRect || !optLine->pDestRect)
	{
		return;
	}
	bool startIsSrc = IsPointAttachedToRect(optLine->startPoint, optLine->pSrcRect);
	bool endIsSrc = IsPointAttachedToRect(optLine->endPoint, optLine->pSrcRect);
	int srcPointIndex = 0;
	int destPointIndex = points.size() - 1;
	if (!startIsSrc && endIsSrc)
	{
		srcPointIndex = points.size() - 1;
		destPointIndex = 0;
	}
	QPoint srcPoint = points.at(srcPointIndex);
	QPoint destPoint = points.at(destPointIndex);
	QPoint srcAdjacentPoint = ResolveArrowAdjacentPoint(points, srcPointIndex);
	QPoint destAdjacentPoint = ResolveArrowAdjacentPoint(points, destPointIndex);
	if (srcPoint == srcAdjacentPoint || destPoint == destAdjacentPoint)
	{
		return;
	}
	int arrowOffset = 2 * CONN_R + ARROW_LEN + OPTICAL_ARROW_EXTRA_OFFSET;
	QPoint srcArrowPoint = BuildArrowTipPoint(srcPoint, srcAdjacentPoint, arrowOffset);
	QPoint destArrowPoint = BuildArrowTipPoint(destPoint, destAdjacentPoint, arrowOffset);
	QPoint srcVec = srcAdjacentPoint - srcPoint;
	QPoint destVec = destAdjacentPoint - destPoint;
	double srcOutAngle = GetAngleByVec(srcVec);
	double srcInAngle = GetAngleByVec(srcPoint - srcAdjacentPoint);
	double destOutAngle = GetAngleByVec(destVec);
	double destInAngle = GetAngleByVec(destPoint - destAdjacentPoint);
	double doubleArrowGap = ARROW_LEN * OPTICAL_DOUBLE_ARROW_GAP_RATIO;
	QPoint srcOutArrowPoint = srcArrowPoint;
	QPoint srcInArrowPoint = srcArrowPoint;
	QPoint destOutArrowPoint = destArrowPoint;
	QPoint destInArrowPoint = destArrowPoint;
	if ((optLine->srcArrowState & Arrow_InOut) == Arrow_InOut)
	{
		srcOutArrowPoint = OffsetArrowPointByVec(srcArrowPoint, srcVec, doubleArrowGap * 0.5);
		srcInArrowPoint = OffsetArrowPointByVec(srcArrowPoint, srcVec, -doubleArrowGap * 0.5);
	}
	if ((optLine->destArrowState & Arrow_InOut) == Arrow_InOut)
	{
		destOutArrowPoint = OffsetArrowPointByVec(destArrowPoint, destVec, doubleArrowGap * 0.5);
		destInArrowPoint = OffsetArrowPointByVec(destArrowPoint, destVec, -doubleArrowGap * 0.5);
	}
	if (optLine->srcArrowState & Arrow_Out)
	{
		drawArrowHeader(srcOutArrowPoint, srcOutAngle);
	}
	if (optLine->srcArrowState & Arrow_In)
	{
		drawArrowHeader(srcInArrowPoint, srcInAngle);
	}
	if (optLine->destArrowState & Arrow_Out)
	{
		drawArrowHeader(destOutArrowPoint, destOutAngle);
	}
	if (optLine->destArrowState & Arrow_In)
	{
		drawArrowHeader(destInArrowPoint, destInAngle);
	}
}

void SvgTransformer::DrawPortText(OpticalCircuitLine* line, int conn_r)
{
	PAINTER_STATE_GUARD(m_painter);
	int offset = conn_r * 2 + 10;
	QPen pen;
	pen.setColor(ColorHelper::Color(ColorHelper::pure_white));
	QFont font;
	font.setPointSize(18);
	m_painter->setPen(pen);
	m_painter->setFont(font);
	QPoint& topPoint = line->startPoint.y() < line->endPoint.y() ? line->startPoint : line->endPoint;
	QPoint& bottomPoint = line->startPoint.y() < line->endPoint.y() ? line->endPoint : line->startPoint;
	IedRect* topRect = line->pSrcRect->y < line->pDestRect->y ? line->pSrcRect : line->pDestRect;
	IedRect* bottomRect = line->pSrcRect->y < line->pDestRect->y ? line->pDestRect : line->pSrcRect;
	int topPtY = topRect->iedName.contains("SW") ? topPoint.y() - offset : topPoint.y() + offset;
	int bottomPtY = bottomRect->iedName.contains("SW") ? bottomPoint.y() + offset : bottomPoint.y() - offset;
	QString topPort = line->pSrcRect->y > line->pDestRect->y ?
		line->pOpticalCircuit->destIedPort :
		line->pOpticalCircuit->srcIedPort;
	QString bottomPort = line->pSrcRect->y > line->pDestRect->y ?
		line->pOpticalCircuit->srcIedPort :
		line->pOpticalCircuit->destIedPort;
	QFontMetrics fm(m_painter->font());
	int upperPortStrWidth = fm.width(topPort);
	int bottomPortStrWidth = fm.width(bottomPort);
	QPoint topPort_lt_pt(topPoint.x() - upperPortStrWidth / 2, topPtY - 5);
	QPoint topPort_rb_pt(topPoint.x() + upperPortStrWidth / 2, topPtY + fm.height());
	QPoint bottomPort_lt_pt(bottomPoint.x() - bottomPortStrWidth / 2, bottomPtY - 5);
	QPoint bottomPort_rb_pt(bottomPoint.x() + bottomPortStrWidth / 2, bottomPtY + fm.height());
	m_painter->drawText(QRect(topPort_lt_pt, topPort_rb_pt), topPort);
	m_painter->drawText(QRect(bottomPort_lt_pt, bottomPort_rb_pt), bottomPort);
}

void SvgTransformer::DrawVirtualIcon(const QPoint& pt, VirtualType _type, const quint32 color)
{
	if(_type == GOOSE)
	{
		DrawGseIcon(pt, color);
	}
	else if(_type == SV)
	{
		DrawSvIcon(pt, color);
	}
	else
	{
		qWarning() << "SvgTransformer::DrawVirtualIcon: Unsupported virtual type";
	}
}

void SvgTransformer::DrawVirtualText(const QRect& rect, QString val, QString typeStr)
{
	//m_painter->save();
	PAINTER_STATE_GUARD(m_painter);
	if (rect.height() == 0)
	{
		// 使用 font-family 携带占位信息：id 以及矩形位置尺寸
		// group0: id，仅用流水 m_element_id
		// group1: x y w h（这里 h==0 只是标记用途，仍按数值写入）
		QString payload = joinGroups()
			<< (QString)(joinFields() << m_element_id)
			<< (QString)(joinFields() << rect.x() << rect.y() << rect.width() << rect.height())
			<< (QString)(joinFields() << typeStr);
		QFont font;
		font.setFamily(payload);
		font.setPointSize(TYPE_Virtual_Value); // 用特殊字号标注节点类型
		m_painter->setFont(font);
		// 画一个 1px 的透明矩形作为占位（既能落到 SVG，又不会可见）
		QPen pen(Qt::NoPen);
		QBrush brush(Qt::NoBrush);
		m_painter->setPen(pen);
		m_painter->setBrush(brush);
		m_painter->drawRect(rect);
		// m_painter->restore();
		return;
	}
	// 其他文本（如描述）仍保持原有绘制
	QPen pen;
	pen.setColor(Qt::white);
	QFont font;
	font.setPointSize(18);
	m_painter->setPen(pen);
	m_painter->setFont(font);
	m_painter->drawText(rect, val);
	// m_painter->restore();
}

void SvgTransformer::DrawPlate(const QHash<QString, PlateRect>& hash)
{
	foreach(const PlateRect& plateRect, hash)
	{
		//m_painter->save();
		PAINTER_STATE_GUARD(m_painter);
		// 绘制压板矩形
		QPen pen;
		QFont font;
		pen.setColor(ColorHelper::Color(ColorHelper::pure_white));
		//pen.setColor(Qt::transparent);
		QBrush brush(ColorHelper::Color(ColorHelper::ied_underground));
		pen.setWidth(2);
		pen.setStyle(Qt::DashDotLine);
		QVector<qreal> dashPattern;
		dashPattern << 3 << 3;
		pen.setDashPattern(dashPattern);
		m_painter->setPen(pen);
		m_painter->setBrush(brush);
		QString plateInfo = joinGroups() <<
			(QString)(joinFields() << m_element_id++) <<
			(QString)(joinFields() << plateRect.iedName << plateRect.desc << plateRect.ref << plateRect.code);
		font.setPointSize(TYPE_Plate_RECT);
		font.setFamily(plateInfo);
		m_painter->setFont(font);
		m_painter->drawRect(plateRect.rect);
		// 绘制压板图标
		QPoint centerPt(plateRect.rect.x() + plateRect.rect.width() / 2 - PLATE_WIDTH / 2 + PLATE_CIRCLE_RADIUS + PLATE_GAP, plateRect.rect.y() + plateRect.rect.height() / 2);
		DrawPlateIcon(centerPt, true, plateInfo);	// 根据压板描述判断闭合状态
		// 绘制hitbox
		m_painter->setPen(Qt::NoPen);
		m_painter->setBrush(Qt::NoBrush);
		font.setPointSize(TYPE_Plate_HITBOX);
		m_painter->setFont(font);
		m_painter->drawRect(QRect(QPoint(centerPt.x() - PLATE_CIRCLE_RADIUS, centerPt.y() - PLATE_CIRCLE_RADIUS), QSize(PLATE_WIDTH, PLATE_HEIGHT)));
		// m_painter->restore();
	}
}

void SvgTransformer::DrawGseIcon(const QPoint& pt, const quint32 color)
{
	PAINTER_STATE_GUARD(m_painter);
	// 绘制边框
	//QPen pen;
	//pen.setColor(ColorHelper::Color(color));
	//pen.setWidth(3);
	//m_painter->setPen(pen);
	//m_painter->drawRect(pt.x(), pt.y(), ICON_LENGTH, ICON_LENGTH);
	//// 绘制内容
	//qreal contentMargin = 3;
	//qreal contentVerticalMargin = 7;
	//qreal lineWidth = (ICON_LENGTH - contentMargin * 2) / 3;
	//int firstLineWidth_bottom = lineWidth * 0.9;
	//int secondLineWidth_bottom = lineWidth * 1.25;
	//int thirdLineWidth_bottom = lineWidth * 0.9;
	//int contentTopY = pt.y() + contentVerticalMargin;
	//int contentBottomY = pt.y() + ICON_LENGTH - contentVerticalMargin;
	//QPointF points[6] = {

	//	QPointF(pt.x() + contentMargin, contentBottomY),
	//	QPointF(pt.x() + contentMargin + firstLineWidth_bottom, contentBottomY),
	//	QPointF(pt.x() + contentMargin + firstLineWidth_bottom, contentTopY),
	//	QPointF(pt.x() + contentMargin + firstLineWidth_bottom + secondLineWidth_bottom, contentTopY),
	//	QPointF(pt.x() + contentMargin + firstLineWidth_bottom + secondLineWidth_bottom, contentBottomY),
	//	QPointF(pt.x() + contentMargin + firstLineWidth_bottom + secondLineWidth_bottom + thirdLineWidth_bottom + 0.5, contentBottomY)
	//};
	//pen.setWidth(2);
	//m_painter->setPen(pen);
	//m_painter->drawPolyline(points, 6);
	utils::drawGseIcon(m_painter, pt, ICON_LENGTH, ColorHelper::Color(color));
}

void SvgTransformer::DrawSvIcon(const QPoint& pt, const quint32 color)
{
	PAINTER_STATE_GUARD(m_painter);
	// 绘制边框
	//QPen pen;
	//pen.setColor(ColorHelper::Color(color));
	//pen.setWidth(3);
	//m_painter->setPen(pen);
	//m_painter->drawRect(pt.x(), pt.y(), ICON_LENGTH, ICON_LENGTH);
	//// 绘制内容
	//qreal contentMargin = 2;
	//qreal contentWidth = ICON_LENGTH - contentMargin * 2;
	//qreal contentY = (pt.y() + static_cast<qreal>(ICON_LENGTH)) / 2;
	//qreal contentTopY = pt.y() + contentMargin;
	//qreal contentBottomY = pt.y() + ICON_LENGTH - contentMargin;
	//QPointF startPoint(pt.x() + contentMargin, pt.y() + ICON_LENGTH / 2);
	//QPointF endPoint(pt.x() + ICON_LENGTH - contentMargin, pt.y() + ICON_LENGTH / 2);
	//QPointF controlPt1(pt.x() + contentWidth / 2 - 1, startPoint.y() - 5);
	//QPointF controlPt2(pt.x() + contentWidth / 4 * 3 + 1, startPoint.y() + 5);
	//QPainterPath path;
	//path.moveTo(startPoint);
	//path.cubicTo(controlPt1, controlPt1, QPointF((startPoint.x() + endPoint.x()) / 2, startPoint.y()));
	//path.cubicTo(controlPt2, controlPt2, endPoint);
	//pen.setWidth(2);
	//m_painter->setPen(pen);
	//m_painter->drawPath(path);
	utils::drawSvIcon(m_painter, pt, ICON_LENGTH, ColorHelper::Color(color));
}

void SvgTransformer::DrawPlateIcon(const QPoint& centerPt, bool status, const QString& info)
{
	PAINTER_STATE_GUARD(m_painter);
	quint32 color = status ? ColorHelper::pure_green : ColorHelper::pure_red; // 根据闭合状态使用默认颜色
	QPen pen;
	QFont font;
	pen.setWidth(2);
	pen.setColor(ColorHelper::Color(color));
	font.setPointSize(TYPE_Plate_ICON);
	font.setFamily(info);
	m_painter->setFont(font);
	m_painter->setPen(pen);
	int distance = PLATE_WIDTH - PLATE_CIRCLE_RADIUS * 4;	// 两个圆之间的距离 = 压板图形总宽度 - 两个圆的直径
	// 直径20的圆
	m_painter->drawEllipse(centerPt, PLATE_CIRCLE_RADIUS, PLATE_CIRCLE_RADIUS);
	m_painter->drawEllipse(centerPt + QPoint(distance, 0), PLATE_CIRCLE_RADIUS, PLATE_CIRCLE_RADIUS);
	// 矩形连接左侧圆
	//int rectWidth = 50;
	int rectHeight = PLATE_CIRCLE_RADIUS * 2;
	//m_painter->translate(centerPt);
	QPoint ptList[4] = {
		QPoint(0, -rectHeight / 2),
		QPoint(distance, -rectHeight / 2),
		QPoint(distance, rectHeight / 2),
		QPoint(0, rectHeight / 2),
	};
	// 闭合状态矩形
	pen.setColor(Qt::transparent);
	m_painter->setPen(pen);
	m_painter->drawLine(centerPt + ptList[0], centerPt + ptList[1]);
	m_painter->drawLine(centerPt + ptList[2], centerPt + ptList[3]);
}

void SvgTransformer::ReSignIedRect(pugi::xml_document& doc)
{
	pugi::xpath_node_set iedRectNodeSet = doc.select_nodes(QString("//g[@font-size='%1']").arg(TYPE_IED).toLocal8Bit());
	for (pugi::xpath_node_set::const_iterator it = iedRectNodeSet.begin(); it != iedRectNodeSet.end(); ++it)
	{
		pugi::xml_node iedRectNode = it->node();
		QString iedName = iedRectNode.attribute("font-family").value();
		iedRectNode.append_attribute("iedname");
		iedRectNode.append_attribute("type");
		iedRectNode.attribute("iedname").set_value(iedName.toUtf8().constData());
		iedRectNode.attribute("type").set_value("ied");
		iedRectNode.attribute("font-family").set_value("SimSun");
		iedRectNode.attribute("font-size").set_value("15");
	}
}

void SvgTransformer::ReSignSvg(const QString& filename, BaseSvg& svg)
{
	pugi::xml_document doc;
	pugi::xml_parse_result res = doc.load_file(filename.toLocal8Bit());
	if (!res)
	{
		qDebug() << __FILE__ << __LINE__ << "load file failed";
		return;
	}
	ReSignSvgDoc(doc, svg);
	if (!doc.save_file(filename.toLocal8Bit()))
	{
		qDebug() << "save file failed";
	}
}

void SvgTransformer::ReSignSvgDoc(pugi::xml_document& doc, BaseSvg& svg)
{
	ReSignIedRect(doc);
	ReSignCircuitLine(doc);
	ReSignPlate(doc);
	ReSignVirtualValuePlaceholders(doc);
	ReSignSvgViewBox(doc, svg.viewBoxX, svg.viewBoxY, svg.viewBoxWidth, svg.viewBoxHeight);
}

void SvgTransformer::ReSignCircuitLine(pugi::xml_document& doc)
{
	// 逻辑回路
	pugi::xpath_node_set circuitLineNodeSet = doc.select_nodes(QString("//g[@font-size='%1']").arg(TYPE_LogicCircuit).toLocal8Bit());
	for (nodeSetConstIterator it = circuitLineNodeSet.begin(); it != circuitLineNodeSet.end(); ++it)
	{
		pugi::xml_node lineNode = it->node();
		QString circuitDesc = lineNode.attribute("font-family").value();
		QList<QStringList> circuitDescGrp = splitGroupAndFields(circuitDesc);
		// (srcIedName, cbName) & (destIedName)
		QString id = getField(circuitDescGrp, 0, 0);
		QString srcIedName = getField(circuitDescGrp, 1, 0);
		QString cbName = getField(circuitDescGrp, 1, 1);
		QString destIedName = getField(circuitDescGrp, 2, 0);
		QString circuitId = lineNode.attribute("font-weight").value();
		lineNode.append_attribute("id");
		lineNode.append_attribute("src-iedname");
		lineNode.append_attribute("src-cbname");
		lineNode.append_attribute("dest-iedname");
		lineNode.append_attribute("type");
		lineNode.append_attribute("circuit-code");
		lineNode.attribute("id").set_value(id.toUtf8().constData());
		lineNode.attribute("src-iedname").set_value(srcIedName.toUtf8().constData());
		lineNode.attribute("src-cbname").set_value(cbName.toUtf8().constData());
		lineNode.attribute("dest-iedname").set_value(destIedName.toUtf8().constData());
		lineNode.attribute("type").set_value("logic");
		lineNode.attribute("circuit-code").set_value(circuitId.toUtf8().constData());
		// 重置不相关属性为默认值
		lineNode.attribute("font-family").set_value("SimSun");
		lineNode.attribute("font-size").set_value("15");
		lineNode.attribute("font-weight").set_value("400");
	}
	// 光纤链路
	pugi::xpath_node_set opticalLineNodeSet = doc.select_nodes(QString("//g[@font-size='%1']").arg(TYPE_OpticalCircuit).toLocal8Bit());
	for (nodeSetConstIterator it = opticalLineNodeSet.begin(); it != opticalLineNodeSet.end(); ++it)
	{
		pugi::xml_node lineNode = it->node();
		QString attrStr = lineNode.attribute("font-family").value();
		QList<QStringList> strList = splitGroupAndFields(attrStr);
		QString id = getField(strList, 0, 0);	// id
		QString loopCode = getField(strList, 1, 0);	// loopCode
		QString code = getField(strList, 1, 1);	// code
		QString srcIedName = getField(strList, 2, 0);	// srcIedName
		QString srcPort = getField(strList, 2, 1);	// srcPort
		QString destIedName = getField(strList, 3, 0);	// destIedName
		QString destPort = getField(strList, 3, 1);	// destPort
		QString connStatus = getField(strList, 4, 0);	// connStatus
		QString remoteId = getField(strList, 5, 0);	// remoteId
		lineNode.append_attribute("src-ied").set_value(srcIedName.toUtf8().constData());
		lineNode.append_attribute("src-port").set_value(srcPort.toUtf8().constData());
		lineNode.append_attribute("dest-ied").set_value(destIedName.toUtf8().constData());
		lineNode.append_attribute("dest-port").set_value(destPort.toUtf8().constData());
		lineNode.append_attribute("loopCode").set_value(loopCode.toUtf8().constData());
		lineNode.append_attribute("status").set_value(connStatus.toUtf8().constData());
		lineNode.append_attribute("remote-id").set_value(remoteId.toUtf8().constData());
		lineNode.append_attribute("code").set_value(code.toUtf8().constData());
		lineNode.append_attribute("type").set_value("optical");
		lineNode.append_attribute("id").set_value(id.toUtf8().constData());
		// 重置不相关属性为默认值
		lineNode.attribute("font-family").set_value("SimSun");
		lineNode.attribute("font-size").set_value("15");
		lineNode.attribute("font-weight").set_value("400");
	}
	pugi::xpath_node_set opticalConnNodeSet = doc.select_nodes(QString("//g[@font-size='%1']").arg(TYPE_Optical_ConnCircle).toLocal8Bit());
	for (nodeSetConstIterator it = opticalConnNodeSet.begin(); it != opticalConnNodeSet.end(); ++it)
	{
		pugi::xml_node connNode = it->node();
		// 连接点只记录id，用于配对
		connNode.append_attribute("id");
		QString id = connNode.attribute("font-weight").value();	// id
		connNode.attribute("id").set_value(id.toUtf8().constData());
		// 重置不相关属性为默认值
		connNode.attribute("font-family").set_value("SimSun");
		connNode.attribute("font-size").set_value("15");
		connNode.attribute("font-weight").set_value("400");
	}
	// 虚回路
	pugi::xpath_node_set virtualLineNodeSet = doc.select_nodes(QString("//g[@font-size='%1']").arg(TYPE_VirtualCircuit).toLocal8Bit());
	for (nodeSetConstIterator it = virtualLineNodeSet.begin(); it != virtualLineNodeSet.end(); ++it)
	{
		pugi::xml_node lineNode = it->node();
		QString attrStr = QString::fromUtf8(lineNode.attribute("font-family").value());
		QList<QStringList> strList = splitGroupAndFields(attrStr);
		QString id = getField(strList, 0, 0);	// id
		QString srcIedName = getField(strList, 1, 0);
		QString destIedName = getField(strList, 1, 1);
		QString srcSoftPlateDesc = getField(strList, 2, 0);
		QString destSoftPlateDesc = getField(strList, 2, 1);
		QString srcSoftPlateRef = getField(strList, 3, 0);
		QString destSoftPlateRef = getField(strList, 3, 1);
		QString srcRef = getField(strList, 4, 0);	// srcRef
		QString destRef = getField(strList, 4, 1);	// destRef
		QString srcName = getField(strList, 5, 0);	// srcName
		QString destName = getField(strList, 5, 1);	// destName
		QString remoteSigId_A = getField(strList, 6, 0);	// remoteSigId_A
		QString remoteSigId_B = getField(strList, 6, 1);	// remoteSigId_B
		QString vType = getField(strList, 7, 0);	// vType (gse/sv)
		QString srcSoftPlateCode = getField(strList, 8, 0);
		QString destSoftPlateCode = getField(strList, 8, 1);
		QString code = getField(strList, 9, 0);	// code
		QString circuitDesc = getField(strList, 10, 0);	// circuitDesc
		lineNode.append_attribute("id").set_value(id.toUtf8().constData());
		lineNode.append_attribute("code").set_value(code.toUtf8().constData());
		lineNode.append_attribute("srcIedName").set_value(srcIedName.toUtf8().constData());
		lineNode.append_attribute("destIedName").set_value(destIedName.toUtf8().constData());
		lineNode.append_attribute("srcSoftPlateCode").set_value(srcSoftPlateCode.toUtf8().constData());
		lineNode.append_attribute("destSoftPlateCode").set_value(destSoftPlateCode.toUtf8().constData());
		lineNode.append_attribute("srcSoftPlateDesc").set_value(srcSoftPlateDesc.toUtf8().constData());
		lineNode.append_attribute("destSoftPlateDesc").set_value(destSoftPlateDesc.toUtf8().constData());
		lineNode.append_attribute("srcSoftPlateRef").set_value(srcSoftPlateRef.toUtf8().constData());
		lineNode.append_attribute("destSoftPlateRef").set_value(destSoftPlateRef.toUtf8().constData());
		lineNode.append_attribute("circuitDesc").set_value(circuitDesc.toUtf8().constData());
		lineNode.append_attribute("remoteSigId_A").set_value(remoteSigId_A.toUtf8().constData());
		lineNode.append_attribute("remoteSigId_B").set_value(remoteSigId_B.toUtf8().constData());
		lineNode.append_attribute("type").set_value("virtual");
		lineNode.append_attribute("virtual-type").set_value(vType.toUtf8().constData());
		// 重置不相关属性
		lineNode.attribute("font-family").set_value("SimSun");
		lineNode.attribute("font-size").set_value("15");
		lineNode.attribute("font-weight").set_value("400");
	}
	// 回路箭头，绑定id
	pugi::xpath_node_set arrowNodeSet = doc.select_nodes(QString("//g[@font-size='%1']").arg(TYPE_Circuit_Arrow).toLocal8Bit());
	for (nodeSetConstIterator it = arrowNodeSet.begin(); it != arrowNodeSet.end(); ++it)
	{
		pugi::xml_node arrowNode = it->node();
		QString arrowDesc = arrowNode.attribute("font-family").value();
		QString id = getField(splitGroupAndFields(arrowDesc), 0, 0);
		arrowNode.append_attribute("id");
		arrowNode.append_attribute("type");
		arrowNode.attribute("id").set_value(id.toUtf8().constData());
		arrowNode.attribute("type").set_value("arrow");
		// 重置不相关属性
		arrowNode.attribute("font-family").set_value("SimSun");
		arrowNode.attribute("font-size").set_value("15");
		arrowNode.attribute("font-weight").set_value("400");
	}
}

void SvgTransformer::ReSignPlate(pugi::xml_document& doc)
{
	pugi::xpath_node_set plateNodeSet = doc.select_nodes(QString("//g[@font-size='%1']").arg(TYPE_Plate_HITBOX).toLocal8Bit());
	for (nodeSetConstIterator it = plateNodeSet.begin(); it != plateNodeSet.end(); ++it)
	{
		pugi::xml_node plateNode = it->node();
		plateNode.append_attribute("ied-name");
		plateNode.append_attribute("plate-desc");
		plateNode.append_attribute("plate-ref");
		plateNode.append_attribute("plate-code");
		plateNode.append_attribute("type");
		plateNode.append_attribute("id");
		QString plateDesc = QString::fromUtf8(plateNode.attribute("font-family").value());
		QList<QStringList> plateDescGrp = splitGroupAndFields(plateDesc);
		QString id = getField(plateDescGrp, 0, 0);	// id
		QString iedName = getField(plateDescGrp, 1, 0);	// ied-name
		QString desc = getField(plateDescGrp, 1, 1);	// plate-desc
		QString ref = getField(plateDescGrp, 1, 2);	// plate-ref
		QString code = getField(plateDescGrp, 1, 3);	// plate-code
		plateNode.attribute("id").set_value(id.toUtf8().constData());
		plateNode.attribute("ied-name").set_value(iedName.toUtf8().constData());
		plateNode.attribute("plate-desc").set_value(desc.toUtf8().constData());
		plateNode.attribute("plate-ref").set_value(ref.toUtf8().constData());
		plateNode.attribute("plate-code").set_value(code.toUtf8().constData());
		plateNode.attribute("type").set_value("plate");
		// 重置不相关属性
		plateNode.attribute("font-family").set_value("SimSun");
		plateNode.attribute("font-size").set_value("15");
	}
	pugi::xpath_node_set plateRectNodeSet = doc.select_nodes(QString("//g[@font-size='%1' or @font-size='%2']").arg(TYPE_Plate_RECT).arg(TYPE_Plate_ICON).toLocal8Bit());
	for (nodeSetConstIterator it = plateRectNodeSet.begin(); it != plateRectNodeSet.end(); ++it)
	{
			pugi::xml_node plateRectNode = it->node();
			plateRectNode.append_attribute("type");
			plateRectNode.append_attribute("id");
			QString plateDesc = plateRectNode.attribute("font-family").value();
			QList<QStringList> plateDescGrp = splitGroupAndFields(plateDesc);
			QString id = getField(plateDescGrp, 0, 0);	// id
			plateRectNode.attribute("type").set_value("plate-component");
			plateRectNode.attribute("id").set_value(id.toUtf8().constData());
			plateRectNode.attribute("font-family").set_value("SimSun");
			plateRectNode.attribute("font-size").set_value("15");
	}
}

void SvgTransformer::ReSignVirtualValuePlaceholders(pugi::xml_document& doc)
{
	// 查找所有以 TYPE_Virtual_Value 标注的占位元素，将其转换为具名属性
	pugi::xpath_node_set nodeSet = doc.select_nodes(QString("//g[@font-size='%1']").arg(TYPE_Virtual_Value).toLocal8Bit());
	for (nodeSetConstIterator it = nodeSet.begin(); it != nodeSet.end(); ++it)
	{
		pugi::xml_node node = it->node();
		QString packed = node.attribute("font-family").value();
		QList<QStringList> groups = splitGroupAndFields(packed);
		QString id = getField(groups, 0, 0);
		QString x = getField(groups, 1, 0);
		QString y = getField(groups, 1, 1);
		QString w = getField(groups, 1, 2);
		QString h = getField(groups, 1, 3);
		QString textType = getField(groups, 2, 0);
		node.append_attribute("type").set_value("virtual-value");
		node.append_attribute("id").set_value(id.toUtf8().constData());
		node.append_attribute("x").set_value(x.toUtf8().constData());
		node.append_attribute("y").set_value(y.toUtf8().constData());
		node.append_attribute("w").set_value(w.toUtf8().constData());
		node.append_attribute("h").set_value(h.toUtf8().constData());
		node.append_attribute("textType").set_value(textType.toUtf8().constData());
		// 重置不相关属性
		node.attribute("font-family").set_value("SimSun");
		node.attribute("font-size").set_value("15");
		node.attribute("font-weight").set_value("400");
	}
}

void SvgTransformer::ReSignSvgViewBox(pugi::xml_document& doc, int x, int y, int width, int height)
{
	pugi::xml_node svgNode = doc.child("svg");
	svgNode.attribute("viewBox").set_value(QString("%1 %2 %3 %4").arg(x).arg(y).arg(width).arg(height).toUtf8().constData());
}
