#include "InteractiveSvgItem.h"
#include "SvgUtils.h"
#include <QPainter>
#include <QGraphicsSceneHoverEvent>
#include <QFile>
#include <QDebug>
#include "svgmodel.h"
#include <QMap>
#include <QTransform>
#include <cmath>
#include <QMenu>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QGraphicsSceneContextMenuEvent>
#include <QScrollBar>
#include <QSet>
#include <QtAlgorithms>
#include <string>
#include <sstream>
#include <QToolTip>
#include <QTimer>
// 闪烁间隔
#define BLINK_CYCLE_MS 1000
InteractiveSvgMapItem::InteractiveSvgMapItem(const QString& svgPath)
	: m_highlightedLineIdx(-1)
	, m_hoverPlateIdx(-1)
	, m_dragging(false)
	, m_fittedOnce(false)
	, m_blinkOn(false)
{
	initCommon();
	parseSvgAndInit(svgPath);
}

InteractiveSvgMapItem::InteractiveSvgMapItem(const QByteArray& svgBytes)
	: m_highlightedLineIdx(-1)
	, m_hoverPlateIdx(-1)
	, m_dragging(false)
	, m_fittedOnce(false)
	, m_blinkOn(false)
{
	initCommon();
	parseSvgAndInit(svgBytes);
}

void InteractiveSvgMapItem::initCommon() {
	setAcceptHoverEvents(true);
	m_circuitConfig = CircuitConfig::Instance();
	m_blinkTimer = new QTimer(this);
	connect(m_blinkTimer, SIGNAL(timeout()), this, SLOT(onBlinkTimeout()));
	m_blinkTimer->start(500);
}

QRectF InteractiveSvgMapItem::boundingRect() const
{
	return QRectF(0, 0, m_bgPixmap.width(), m_bgPixmap.height());
}

void InteractiveSvgMapItem::paint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*)
{
	painter->drawPixmap(0, 0, m_bgPixmap);
	// 先画回路
	for (int i = 0; i < m_allLines.size(); ++i) {
		const MapLine& line = m_allLines[i];
		paintLine(painter, line, i == m_highlightedLineIdx);
	}
	// 后画压板
	paintPlates(painter);
	// 最后绘制虚拟数值文本，覆盖在最上层
	paintVirtualValues(painter);
}
void InteractiveSvgMapItem::mousePressEvent(QGraphicsSceneMouseEvent* event)
{
	if (event->button() == Qt::LeftButton) {
		m_dragging = true;
		if (scene() && !scene()->views().isEmpty()) {
			QGraphicsView* v = scene()->views().first();
			// 记录按下时的视图坐标位置（像素）
			m_lastViewPos = v->mapFromScene(event->scenePos());
		} else {
			m_lastViewPos = event->screenPos();
		}
		event->accept();
		return;
	}
	QGraphicsItem::mousePressEvent(event);
}

void InteractiveSvgMapItem::mouseMoveEvent(QGraphicsSceneMouseEvent* event)
{
	if (m_dragging) {
		if (scene() && !scene()->views().isEmpty()) {
			QGraphicsView* v = scene()->views().first();
			// 当前鼠标对应的视图坐标
			QPoint curViewPos = v->mapFromScene(event->scenePos());
			QPoint delta = curViewPos - m_lastViewPos;
			m_lastViewPos = curViewPos;
			// 用滚动条平移
			QScrollBar* hbar = v->horizontalScrollBar();
			QScrollBar* vbar = v->verticalScrollBar();
			if (hbar) hbar->setValue(hbar->value() - delta.x());
			if (vbar) vbar->setValue(vbar->value() - delta.y());
		}
		event->accept();
		return;
	}
	QGraphicsItem::mouseMoveEvent(event);
}

void InteractiveSvgMapItem::mouseReleaseEvent(QGraphicsSceneMouseEvent* event)
{
	if (event->button() == Qt::LeftButton) {
		m_dragging = false;
		event->accept();
		return;
	}
	QGraphicsItem::mouseReleaseEvent(event);
}

void InteractiveSvgMapItem::wheelEvent(QGraphicsSceneWheelEvent* event)
{
	if (scene() && !scene()->views().isEmpty()) {
		QGraphicsView* v = scene()->views().first();
		
		// 独立窗口中的缩放处理
		const double factor = (event->delta() > 0) ? 1.15 : 1.0 / 1.15;
		double sX = v->transform().m11();
		double sY = v->transform().m22();
		double minTarget = 0.5;
		double maxTarget = 2.0;
		double targetX = sX * factor;
		double targetY = sY * factor;
		if ((event->delta() > 0 && (targetX > maxTarget || targetY > maxTarget)) ||
			(event->delta() <= 0 && (targetX < minTarget || targetY < minTarget))) {
			event->accept();
			return;
		}
		v->setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
		v->scale(factor, factor);
		v->setTransformationAnchor(QGraphicsView::AnchorViewCenter);
	}
	event->accept();
}

void InteractiveSvgMapItem::onBlinkTimeout()
{
	m_blinkOn = !m_blinkOn;
	update();
}

void InteractiveSvgMapItem::fitToViewIfPossible()
{
	if (!scene() || scene()->views().isEmpty()) return;
	QGraphicsView* v = scene()->views().first();
	if (!v) return;
	QRectF rect = boundingRect();
	if (rect.isEmpty()) return;
}

// 设置高亮线路
void InteractiveSvgMapItem::setHighlightedLine(int idx)
{
	if (m_highlightedLineIdx != idx) {
		m_highlightedLineIdx = idx;
		update();
	}
}

void InteractiveSvgMapItem::hoverMoveEvent(QGraphicsSceneHoverEvent* event)
{
	QPointF pos = event->pos();
	// 1) 压板悬停提示（优先级高于线路高亮）
	int plateIdx = hitTestPlate(pos);
	if (plateIdx >= 0) {
		if (plateIdx != m_hoverPlateIdx) {
			m_hoverPlateIdx = plateIdx;
			const PlateItem& plate = m_allPlates[plateIdx];
			const QString tip = buildPlateTooltip(plate);
			// 将场景坐标转成屏幕坐标以在鼠标附近显示
			QPoint globalPos = event->screenPos();
			QToolTip::showText(globalPos, tip);
		}
		// 当悬停在压板上时，不再更新线路高亮
		setHighlightedLine(-1);
		return;
	} else if (m_hoverPlateIdx != -1) {
		// 离开压板区域时，隐藏 tip
		m_hoverPlateIdx = -1;
		QToolTip::hideText();
	}

	// 2) 线路最近高亮
	int closest = -1;
	double minDist = 99999.0;
	for (int i = 0; i < m_allLines.size(); ++i) {
		const QVector<QPointF>& pts = m_allLines[i].points;
		for (int j = 1; j < pts.size(); ++j) {
			QLineF seg(pts[j - 1], pts[j]);
			double dist = seg.p1() == seg.p2() ? QLineF(pos, seg.p1()).length() : utils::pointToSegmentDistance(pos, seg.p1(), seg.p2());
			if (dist < minDist) {
				minDist = dist;
				closest = i;
			}
		}
	}
	if (minDist < 15.0) 
		setHighlightedLine(closest);
	else
		setHighlightedLine(-1);
}

void InteractiveSvgMapItem::hoverLeaveEvent(QGraphicsSceneHoverEvent* event)
{
	Q_UNUSED(event);
	if (m_hoverPlateIdx != -1) {
		m_hoverPlateIdx = -1;
		QToolTip::hideText();
	}
}

void InteractiveSvgMapItem::mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event)
{
	const QPointF pos = event->pos();

	int closest = -1;
	double minDist = 1e100;
	for (int i = 0; i < m_allLines.size(); ++i) {
		const QVector<QPointF>& pts = m_allLines[i].points;
		for (int j = 1; j < pts.size(); ++j) {
			const QLineF seg(pts[j - 1], pts[j]);
			const double dist = (seg.p1() == seg.p2())
				? QLineF(pos, seg.p1()).length()
				: utils::pointToSegmentDistance(pos, seg.p1(), seg.p2());
			if (dist < minDist) { minDist = dist; closest = i; }
		}
	}

	if (closest >= 0 && minDist < 15.0) {
		const MapLine& line = m_allLines[closest];
		if (line.type == LineType_Optical && !line.attrs.isNull()) {
			// 选中光纤链路 跳转到关联设备图
			showOpticalRelatedCircuits(line);
			// 显示光纤链路信息
			const MapLine::OpticalAttrs* a =
				static_cast<const MapLine::OpticalAttrs*>(line.attrs.data());
			QString tip;
			tip += QString::fromLocal8Bit("光纤链路\n");
			tip += QString::fromLocal8Bit("端口①: %1 / %2\n").arg(a->srcIed, a->srcPort);
			tip += QString::fromLocal8Bit("端口②: %1 / %2\n").arg(a->destIed, a->destPort);
			if (!a->code.isEmpty())    tip += QString("Code: %1\n").arg(a->code);
			if (!a->status.isEmpty())  tip += QString("Status: %1\n").arg(a->status);
			if (!a->remoteId.isEmpty())tip += QString("RemoteId: %1\n").arg(a->remoteId);

			QToolTip::showText(event->screenPos(), tip);
		}
		if (line.type == LineType_Virtual && !line.attrs.isNull())
		{
#ifdef _DEBUG
			MapLine* dbgLine = &m_allLines[closest];
			dbgLine->isBlinking = !dbgLine->isBlinking;
#endif
		}
	}

	event->accept();
}


void InteractiveSvgMapItem::contextMenuEvent(QGraphicsSceneContextMenuEvent* event)
{
	if(m_svgType != LineType_Virtual) {
		// 仅虚回路支持压板操作
		event->ignore();
		return;
	}
	const QPointF pos = event->pos();
	int pi = hitTestPlate(pos);
	QMenu menu;
	if (pi >= 0) {
		PlateItem& plate = m_allPlates[pi];
		QAction* act = plate.isClosed
			? menu.addAction(QString::fromLocal8Bit("置分"))
			: menu.addAction(QString::fromLocal8Bit("置合"));
		QAction* chosen = menu.exec(event->screenPos());
		if (chosen == act) {
			plate.isClosed = !plate.isClosed;
			update();
		}
	} else {
		QAction* actAllClose = menu.addAction(QString::fromLocal8Bit("全部置合"));
		QAction* actAllOpen  = menu.addAction(QString::fromLocal8Bit("全部置分"));
		QAction* chosen = menu.exec(event->screenPos());
		if (chosen == actAllClose || chosen == actAllOpen) {
			bool toClosed = (chosen == actAllClose);
			for (int i = 0; i < m_allPlates.size(); ++i) m_allPlates[i].isClosed = toClosed;
			update();
		}
	}
}

int InteractiveSvgMapItem::hitTestPlate(const QPointF& pos) const
{
	for (int i = 0; i < m_allPlates.size(); ++i) {
		if (m_allPlates[i].rect.contains(pos)) return i;
	}
	return -1;
}

QString InteractiveSvgMapItem::buildPlateTooltip(const PlateItem& plate) const
{
	// 两行：名称（desc）与引用（ref）。若缺失则留空/仅显示可用字段。
	QString line1 = plate.attrs.desc.isEmpty() ? QString::fromLocal8Bit("压板") : plate.attrs.desc;
	QString line2 = plate.attrs.ref.isEmpty() ? QString() : plate.attrs.ref;
	if (line2.isEmpty()) return line1; // 只有一行
	return line1 + "\n" + line2;
}

void InteractiveSvgMapItem::showOpticalRelatedCircuits(const MapLine& line)
{
	if (line.type != LineType_Optical || line.attrs.isNull()) return;
	const MapLine::OpticalAttrs* a = static_cast<const MapLine::OpticalAttrs*>(line.attrs.data());
	if (!a) return;
	QString iedName1 = a->srcIed;
	QString iedName2 = a->destIed;
	if (iedName1.isEmpty() && iedName2.isEmpty()) return;
	// 两种情况，直连/交换机
	// 直连, 显示两个设备间所有虚回路
	QList<LogicCircuit*> logicCircuitList = m_circuitConfig->GetCircuitListBySrcAndDest(iedName1, iedName2);
	QList<LogicCircuit*> logicCircuitList2 = m_circuitConfig->GetCircuitListBySrcAndDest(iedName2, iedName1);
	if (logicCircuitList.isEmpty())
	{
		// 有光纤回路无虚回路，解析出错
		return;
	}


}

void InteractiveSvgMapItem::paintLine(QPainter* painter, const MapLine& line, bool isHighLight) const
{
	if(line.isBlinking && !m_blinkOn) {
		// 闪烁状态且当前为隐藏周期，不绘制
		return;
	}
	QColor stroke = colorForLine(line);
	if (!stroke.isValid()) stroke = Qt::green;
	QPen pen(stroke);
	int baseW = int(line.style.strokeWidth);
	int w = baseW;
	if (isHighLight) {
		w = baseW * 3;
		if (w < 2) w = 2;
	}
	else if (w < 1) {
		w = 1;
	}
	pen.setWidth(w);
	painter->setPen(pen);
	for (int j = 1; j < line.points.size(); ++j)
		painter->drawLine(line.points[j - 1], line.points[j]);

	// 画此回路的箭头
	for (int k = 0; k < line.arrows.size(); ++k) {
		const ArrowHead& ah = line.arrows[k];
		if (ah.points.size() < 3) continue;

		QPen apen(pen.color());
		// 高亮时箭头笔宽与线路一致；否则用基础宽
		int aw = (isHighLight)
			? w
			: qMax(1, int(line.style.strokeWidth));
		apen.setWidth(aw);
		apen.setCapStyle(Qt::SquareCap);
		apen.setJoinStyle(Qt::MiterJoin);
		painter->setPen(apen);

		// 高亮时填充同色，未高亮保持空心
		if (isHighLight) {
			QColor fill = apen.color();
			fill.setAlpha(255); // 确保不透明
			painter->setBrush(fill);
		}
		else {
			painter->setBrush(Qt::NoBrush);
		}

		QPolygonF poly;
		for (int pi = 0; pi < ah.points.size(); ++pi) poly << ah.points[pi];

		// 使用 WindingFill 以避免复杂箭头边界的空洞问题
		painter->drawPolygon(poly, Qt::WindingFill);
	}
}

QColor InteractiveSvgMapItem::colorForLine(const MapLine& line) const
{
	// 基于 ref 的关联：任一关联压板非 closed 则灰色，全部 closed 则绿色；无关联则用原始色
	if (line.type == LineType_Logic && line.attrs.isNull())
		return QColor::fromRgba(line.style.strokeRgb);
	if (line.type == LineType_Optical)
	{
		// 光纤链路状态色
		// 连接：绿色 
		// 断开：红色
		// 告警：黄色
		const MapLine::OpticalAttrs* oa = static_cast<const MapLine::OpticalAttrs*>(line.attrs.data());

	}
	if (line.type == LineType_Virtual)
	{
		// 虚回路状态色，断开优先级高于压板
		// 连接：绿色 
		// 断开：红色
		// 告警：黄色
		// 压板断开：灰色
		const MapLine::VirtualAttrs* va = static_cast<const MapLine::VirtualAttrs*>(line.attrs.data());
		bool hasAny = false;
		if (line.status == Status_Disconnected)
		{
			return QColor(Qt::red);
		}
		if (va) {
			if (!va->srcSoftPlateRef.isEmpty()) {
				QMap<QString, PlateItem*>::const_iterator it = m_plateMap.find(va->srcSoftPlateRef);
				if (it != m_plateMap.end() && it.value())
				{
					hasAny = true;
					if (!it.value()->isClosed) return QColor(Qt::gray);
				}
			}
			if (!va->destSoftPlateRef.isEmpty()) {
				QMap<QString, PlateItem*>::const_iterator it = m_plateMap.find(va->destSoftPlateRef);
				if (it != m_plateMap.end() && it.value())
				{
					hasAny = true;
					if (!it.value()->isClosed) return QColor(Qt::gray);
				}
			}
		}
		if (!hasAny) return QColor::fromRgba(line.style.strokeRgb);
	}
	return QColor(Qt::green);
}

void InteractiveSvgMapItem::Clean()
{
    // 通知场景该 Item 的几何可能改变（boundingRect 会变为 0x0）
    prepareGeometryChange();

    // 重置内部数据
    m_bgPixmap = QPixmap();      // 空 pixmap，boundingRect 变为 0
    m_allLines.clear();
    m_allPlates.clear();
    m_highlightedLineIdx = -1;

    update(); // 触发重绘
}

void InteractiveSvgMapItem::parseSvgAndInit(const QString& svgPath)
{
    QFile file(svgPath);
    if (!file.open(QIODevice::ReadOnly)) { qWarning("SVG文件打开失败"); return; }
    QByteArray svgData = file.readAll();
	pugi::xml_document doc;
	if (!doc.load_buffer(svgData.data(), svgData.size())) { qWarning("SVG加载失败"); return; }
	// 释放原始 SVG 字节以降低峰值内存
	svgData.clear(); svgData.squeeze();

	// 先根据 root svg 的 viewBox 计算文档坐标到像素坐标的基础变换：
	// 若 viewBox = [minX, minY, width, height]，我们可以限制背景 pixmap 的最大边，
	// 并把相同的缩放合入 m_docToPix，保证叠加图元与底图一致。
	double vbMinX = 0.0, vbMinY = 0.0, vbW = 0.0, vbH = 0.0;
	double viewScale = 1.0;
	{
		pugi::xml_node root = doc.child("svg");
		if (!root) root = doc.document_element();
		const char* vb = root.attribute("viewBox").value();
		if (vb && *vb) {
			// 解析 viewBox: "minX minY width height"
			QStringList parts = QString::fromLatin1(vb).simplified().split(' ');
			if (parts.size() >= 4) {
				vbMinX = parts[0].toDouble();
				vbMinY = parts[1].toDouble();
				vbW    = parts[2].toDouble();
				vbH    = parts[3].toDouble();
			} else if (parts.size() >= 2) {
				vbMinX = parts[0].toDouble();
				vbMinY = parts[1].toDouble();
			}
		}
		// 背景最大边限制，防止占用过多内存
		const int kMaxBgDim = 4096; // 可按需要调整，例如 3072/4096
		if (vbW > 0.0 && vbH > 0.0) {
			if (vbW > kMaxBgDim || vbH > kMaxBgDim) {
				viewScale = qMin(double(kMaxBgDim) / vbW, double(kMaxBgDim) / vbH);
			}
		}
		m_docToPix = QTransform::fromTranslate(-vbMinX, -vbMinY);
		if (viewScale != 1.0) m_docToPix.scale(viewScale, viewScale);
	}

	// 解析时隐藏链路及箭头的底图显示
	m_lineIdByCode.clear();
    if (svgPath.contains("virtual")) {
	parseVirtualSvg(doc);
        m_svgType = LineType_Virtual;
    } else if (svgPath.contains("logic")) {
	parseLogicSvg(doc);
        m_svgType = LineType_Logic;
    } else if (svgPath.contains("optical")) {
	parseOpticalSvg(doc);
        m_svgType = LineType_Optical;
    }

	std::ostringstream oss;
	doc.save(oss, "", pugi::format_raw, pugi::encoding_utf8);
	const std::string s = oss.str();
	QByteArray modifiedSvg(s.data(), int(s.size()));

   // 修改后的内存 SVG 渲染到底图
	QSvgRenderer renderer;
	if (!renderer.load(modifiedSvg)) {
		// 如果内存加载失败，回退到原文件
		renderer.load(svgPath);
	}
	// 释放中间缓冲
	modifiedSvg.clear(); modifiedSvg.squeeze();

	// 基于 viewBox 与缩放计算背景图尺寸
	QRectF viewBox = renderer.viewBoxF();
	QSize imageSize;
	if (viewBox.isValid()) {
		if (vbW <= 0.0 || vbH <= 0.0) { vbW = viewBox.width(); vbH = viewBox.height(); }
		imageSize = QSize(int(vbW * viewScale), int(vbH * viewScale));
		if (imageSize.width() <= 0 || imageSize.height() <= 0) {
			imageSize = QSize(int(viewBox.width()), int(viewBox.height()));
		}
	} else {
		imageSize = QSize(2000, 2000);
	}

    QPixmap pixmap(imageSize);
    pixmap.fill(Qt::black);
    QPainter painter(&pixmap);
    renderer.render(&painter);
    painter.end();

    prepareGeometryChange();
    m_bgPixmap = pixmap;
}

void InteractiveSvgMapItem::parseSvgAndInit(const QByteArray& svgBytes)
{
	pugi::xml_document doc;
	if (!doc.load_buffer(svgBytes.constData(), svgBytes.size())) { qWarning("SVG加载失败"); return; }

	double vbMinX = 0.0, vbMinY = 0.0, vbW = 0.0, vbH = 0.0;
	double viewScale = 1.0;
	{
		pugi::xml_node root = doc.child("svg");
		if (!root) root = doc.document_element();
		const char* vb = root.attribute("viewBox").value();
		if (vb && *vb) {
			QStringList parts = QString::fromLatin1(vb).simplified().split(' ');
			if (parts.size() >= 4) { vbMinX = parts[0].toDouble(); vbMinY = parts[1].toDouble(); vbW = parts[2].toDouble(); vbH = parts[3].toDouble(); }
			else if (parts.size() >= 2) { vbMinX = parts[0].toDouble(); vbMinY = parts[1].toDouble(); }
		}
		const int kMaxBgDim = 4096;
		if (vbW > 0.0 && vbH > 0.0) {
			if (vbW > kMaxBgDim || vbH > kMaxBgDim) {
				viewScale = qMin(double(kMaxBgDim) / vbW, double(kMaxBgDim) / vbH);
			}
		}
		m_docToPix = QTransform::fromTranslate(-vbMinX, -vbMinY);
		if (viewScale != 1.0) m_docToPix.scale(viewScale, viewScale);
	}

	// 类型识别：根据 type 节点选择分支
	m_lineIdByCode.clear();
	// 这里不依赖文件名，直接探测文档中的类型
	if (!doc.select_nodes("//g[@type='virtual']").empty()) {
		parseVirtualSvg(doc); m_svgType = LineType_Virtual;
	} else if (!doc.select_nodes("//g[@type='logic']").empty()) {
		parseLogicSvg(doc); m_svgType = LineType_Logic;
	} else if (!doc.select_nodes("//g[@type='optical']").empty()) {
		parseOpticalSvg(doc); m_svgType = LineType_Optical;
	}

	std::ostringstream oss;
	doc.save(oss, "", pugi::format_raw, pugi::encoding_utf8);
	const std::string s = oss.str();
	QByteArray modifiedSvg(s.data(), int(s.size()));

	QSvgRenderer renderer;
	if (!renderer.load(modifiedSvg)) {
		// 纯内存失败则直接返回
		qWarning("QSvgRenderer 内存加载失败");
		return;
	}

	QRectF viewBox = renderer.viewBoxF();
	QSize imageSize;
	if (viewBox.isValid()) {
		if (vbW <= 0.0 || vbH <= 0.0) { vbW = viewBox.width(); vbH = viewBox.height(); }
		imageSize = QSize(int(vbW * viewScale), int(vbH * viewScale));
		if (imageSize.width() <= 0 || imageSize.height() <= 0) {
			imageSize = QSize(int(viewBox.width()), int(viewBox.height()));
		}
	} else {
		imageSize = QSize(2000, 2000);
	}

	QPixmap pixmap(imageSize);
	pixmap.fill(Qt::black);
	QPainter painter(&pixmap);
	renderer.render(&painter);
	painter.end();

	prepareGeometryChange();
	m_bgPixmap = pixmap;
}

void InteractiveSvgMapItem::parseVirtualSvg(const pugi::xml_document& doc)
{
	// 先解析回路
	m_allLines += parseCircuitLines(doc, "virtual");
	m_allLines.squeeze();

	// 解析压板（仅 virtual 存在）
	QMap<QString, PlateItem> plateMap;
	pugi::xpath_node_set plateNodeSet = doc.select_nodes("//g[@type='plate-component' or @type='plate']");
	for (int i = 0; i < plateNodeSet.size(); ++i) {
		pugi::xml_node plateNode = plateNodeSet[i].node();
		QString plate_id = plateNode.attribute("id").as_string();
		PlateItem& p = plateMap[plate_id];
		p.svgGrpId = plate_id;
		if (p.rect.isNull()) p.isClosed = true; // 默认置合


		if (strcmp(plateNode.attribute("type").value(), "plate") == 0) {
			// 读取 plate 的语义属性
			const char* refAttr  = plateNode.attribute("plate-ref").value();
			const char* descAttr = plateNode.attribute("plate-desc").value();
			const char* idAttr  = plateNode.attribute("id").value();
			if (refAttr && *refAttr)  p.attrs.ref  = refAttr;
			if (descAttr && *descAttr) p.attrs.desc = QString::fromUtf8(descAttr);
			if (idAttr && *idAttr)  p.attrs.id  = idAttr;
			QRectF bbox;
			if (utils::computePathBoundingRect(plateNode, m_docToPix, bbox)) {
				p.rect = bbox;
			}
		} else if (strcmp(plateNode.attribute("type").value(), "plate-component") == 0) {
			pugi::xml_node pathNode = plateNode.child("path");
			if (pathNode) {
				const char* strokeAttr = plateNode.attribute("stroke").value();
				const char* fillAttr   = plateNode.attribute("fill").value();
				// 1) 外部矩形（覆盖回路）：fill="#233f4f" + stroke="#ffffff"
				if (fillAttr && qstrcmp(fillAttr, "#233f4f") == 0 && strokeAttr && qstrcmp(strokeAttr, "#ffffff") == 0) {
					QRectF bbox;
					if (utils::computePathBoundingRect(plateNode, m_docToPix, bbox)) p.outerRect = bbox;
				}
				// 2) 内部矩形组件：黑色描边
				else if (strokeAttr && qstrcmp(strokeAttr, "#000000") == 0) {
					p.rects = parsePlateRects(plateNode);
				}
				// 3) 圆组件：绿色描边
				else if (strokeAttr && qstrcmp(strokeAttr, "#00ff00") == 0) {
					p.circles = parsePlateCircles(plateNode);
				}
			} else if (plateNode.child("polyline")) {
				p.lines = parsePlateLines(plateNode);
			}
		}
	}

	// 收集压板
	m_allPlates = plateMap.values().toVector();
	// 压缩内部容器容量以节省内存
	for (int i = 0; i < m_allPlates.size(); ++i) {
		m_allPlates[i].circles.squeeze();
		m_allPlates[i].lines.squeeze();
		m_allPlates[i].rects.squeeze();
	}
	m_allPlates.squeeze();
	// ref -> PlateItem* 映射
	m_plateMap.clear();
	for (int i = 0; i < m_allPlates.size(); ++i) {
		PlateItem& p = m_allPlates[i];
		if (!p.attrs.ref.isEmpty()) m_plateMap.insert(p.attrs.ref, &p);
	}

	// 解析虚拟数值文本框
	parseVirtualValueBoxes(doc);
}

void InteractiveSvgMapItem::parseLogicSvg(const pugi::xml_document& doc)
{
	m_allLines += parseCircuitLines(doc, "logic");
	m_allLines.squeeze();
}

void InteractiveSvgMapItem::parseOpticalSvg(const pugi::xml_document& doc)
{
	m_allLines += parseCircuitLines(doc, "optical");
	m_allLines.squeeze();

}

QVector<MapLine> InteractiveSvgMapItem::parseCircuitLines(const pugi::xml_document& doc, const char* type)
{
	QVector<MapLine> linesOut;
	QString xPath = QString("//g[@type='%1']").arg(type);
	pugi::xpath_node_set groups = doc.select_nodes(xPath.toLocal8Bit());
	for (int i = 0; i < groups.size(); ++i) {
		pugi::xml_node g = groups[i].node();
		// 聚合该组内的所有 polyline 段，作为同一条回路
		MapLine line;
		line.isBlinking = false;
		if (qstrcmp(type, "optical") == 0)
			line.type = LineType_Optical;
		else if (qstrcmp(type, "logic") == 0)
			line.type = LineType_Logic;
		else
			line.type = LineType_Virtual;
		line.style = parseLineStyle(g);

	// 统一应用：文档->像素 以及 节点自身 transform
	QTransform tf = m_docToPix * utils::parseTransformMatrix(g);
		bool hasAnyPolyline = false;
		for (pugi::xml_node polyline = g.child("polyline"); polyline; polyline = polyline.next_sibling("polyline")) {
			const char* ptsAttr = polyline.attribute("points").value();
			if (!ptsAttr || !*ptsAttr) continue;
			QVector<QPointF> pts = utils::parsePointsAttr(QString::fromLatin1(ptsAttr));
			for (int pi = 0; pi < pts.size(); ++pi) {
				QPointF mapped = tf.map(pts[pi]);
				if (!line.points.isEmpty() && line.points.last() == mapped) continue; // 去重相邻重复点
				line.points.append(mapped);
			}
			hasAnyPolyline = hasAnyPolyline || !pts.isEmpty();
			g.attribute("stroke-opacity").set_value(0);
		}
		// 兼容极少数 polyline 直接作为组的情况
		if (!hasAnyPolyline && QString::fromLatin1(g.name()) == "polyline") {
			const char* ptsAttr = g.attribute("points").value();
			if (ptsAttr && *ptsAttr) {
				QVector<QPointF> pts = utils::parsePointsAttr(QString::fromLatin1(ptsAttr));
		for (int pi = 0; pi < pts.size(); ++pi) line.points.append(tf.map(pts[pi]));
				hasAnyPolyline = !pts.isEmpty();
			}
		}
		if (!hasAnyPolyline || line.points.size() < 2) continue;

		// 简化并收缩点集，按像素容差（与线宽相关）
		{
			qreal tol = qMax<qreal>(0.5, 0.25 * qreal(line.style.strokeWidth));
			utils::simplifyPolyline(line.points, tol);
			line.points.squeeze();
		}

		// 复制并标准化组上的元信息属性，复用 basemodel 键名
		normalizeAttrsForBaseModel(line, g);
		int grp_id = g.attribute("id").as_int();
		line.svgGrpId = grp_id;
		{
			QVector<ArrowHead> arr = parseArrowHeadsForGroup(doc, grp_id);
			for (int ai = 0; ai < arr.size(); ++ai) line.arrows.append(arr[ai]);
		}

		// 记录 code -> id 映射（若可用）
		if (!line.code.isEmpty()) {
			m_lineIdByCode.insert(line.code, grp_id);
		}
		linesOut.append(line);
	}
	return linesOut;
}

void InteractiveSvgMapItem::parseVirtualValueBoxes(const pugi::xml_document& doc)
{
	m_valuePairs.clear();
	// 选择所有虚拟数值组，每个组内包含两侧的空矩形
	pugi::xpath_node_set groups = doc.select_nodes("//g[@type='virtual-value']");
	for (int i = 0; i < groups.size(); ++i) {
		pugi::xml_node g = groups[i].node();
		int lineId = g.attribute("id").as_int(-1);
		if (lineId < 0) continue;

	QTransform gtf = m_docToPix * utils::parseTransformMatrix(g);
		QVector<QRectF> rects;

		// 直接 rect 元素
		for (pugi::xml_node rn = g.child("rect"); rn; rn = rn.next_sibling("rect")) {
			double x = rn.attribute("x").as_double(0.0);
			double y = rn.attribute("y").as_double(0.0);
			double w = rn.attribute("width").as_double(0.0);
			double h = rn.attribute("height").as_double(0.0);
			QRectF lr(x, y, w, h);
			// 子节点可能也有 transform
			QTransform rtf = gtf * utils::parseTransformMatrix(rn);
			rects.append(rtf.mapRect(lr));
		}
		// 兼容 path 画矩形的情况：取 path 的包围盒，并应用父组与自身变换
		for (pugi::xml_node pn = g.child("path"); pn; pn = pn.next_sibling("path")) {
			const char* d = pn.attribute("d").value();
			if (!d || !*d) continue;
			QVector<QPointF> local = utils::parseSvgPathToPolyline(QString::fromLatin1(d));
			if (local.isEmpty()) continue;
			double minX = 1e100, minY = 1e100, maxX = -1e100, maxY = -1e100;
			for (int i2 = 0; i2 < local.size(); ++i2) {
				const QPointF& p = local[i2];
				if (p.x() < minX) minX = p.x(); if (p.x() > maxX) maxX = p.x();
				if (p.y() < minY) minY = p.y(); if (p.y() > maxY) maxY = p.y();
			}
			if (minX <= maxX && minY <= maxY) {
				QRectF lr(QPointF(minX, minY), QPointF(maxX, maxY));
				QTransform ptf = gtf * utils::parseTransformMatrix(pn);
				rects.append(ptf.mapRect(lr));
			}
		}

		if (rects.isEmpty()) continue;

		// 选择 x 居中最小和最大者为左右框（避免使用 C++11 lambda）
		int leftIdx = 0, rightIdx = 0;
		double minCx = 1e100, maxCx = -1e100;
		for (int ri = 0; ri < rects.size(); ++ri) {
			double cx = rects[ri].center().x();
			if (cx < minCx) { minCx = cx; leftIdx = ri; }
			if (cx > maxCx) { maxCx = cx; rightIdx = ri; }
		}
		QRectF leftR = rects[leftIdx];
		QRectF rightR = rects[rightIdx];

		ValuePair vp;
		if (!leftR.isNull()) { vp.hasLeft = true; vp.left.rect = leftR; }
		if (!rightR.isNull() && rightIdx != leftIdx) { vp.hasRight = true; vp.right.rect = rightR; }
		m_valuePairs.insert(lineId, vp);
	}
}

QVector<ArrowHead> InteractiveSvgMapItem::parseArrowHeadsForGroup(const pugi::xml_document& doc, const int grp_id)
{
	QVector<ArrowHead> arrows;
	pugi::xpath_query query(QString("//g[@id='%1' and @type='arrow']").arg(grp_id).toLocal8Bit());
	pugi::xpath_node_set arrowNodes = doc.select_nodes(query);
	foreach(pugi::xpath_node arrowNode, arrowNodes)
	{
		pugi::xml_node gnode = arrowNode.node();
	QTransform tf = m_docToPix * utils::parseTransformMatrix(gnode);
		for (pugi::xml_node path = gnode.child("path"); path; path = path.next_sibling("path")) {
			const char* dAttr = path.attribute("d").value();
			if (!dAttr || !*dAttr) continue;
			QVector<QPointF> local = utils::parseSvgPathToPolyline(QString::fromLatin1(dAttr));
			if (local.size() >= 3) {
				ArrowHead a;
				for (int i = 0; i < local.size(); ++i) a.points.append(tf.map(local[i]));
				a.points.squeeze();
				arrows.append(a);
			}
		}
		// 隐藏底图箭头组
		gnode.attribute("stroke-opacity").set_value(0);
		gnode.attribute("fill-opacity").set_value(0);
	}
	return arrows;
}

// 统一键名到 basemodel 相关：
// optical: src-ied/src-port/dest-ied/dest-port/code/status/remote-id
// logic:   src-iedname/src-cbname/dest-iedname/circuit-code
// virtual: srcIedName/destIedName/srcSoftPlateDesc/destSoftPlateDesc/srcSoftPlateRef/destSoftPlateRef/remoteId/remoteSigId_A/remoteSigId_B/virtual-type
void InteractiveSvgMapItem::normalizeAttrsForBaseModel(MapLine& line, const pugi::xml_node& g) const
{
	// 填充类型化属性（惰性分配，仅占用一种）
	if (line.type == LineType_Optical) {
		if (line.attrs.isNull()) line.attrs = QSharedPointer<MapLine::AttrBase>(new MapLine::OpticalAttrs);
		MapLine::OpticalAttrs* a = static_cast<MapLine::OpticalAttrs*>(line.attrs.data());
		if (g.attribute("src-ied"))   a->srcIed   = g.attribute("src-ied").value();
		if (g.attribute("dest-ied"))  a->destIed  = g.attribute("dest-ied").value();
		if (g.attribute("src-port"))  a->srcPort  = g.attribute("src-port").value();
		if (g.attribute("dest-port")) a->destPort = g.attribute("dest-port").value();
		if (g.attribute("code"))      { a->code = g.attribute("code").value(); line.code = a->code; }
		if (g.attribute("status"))    a->status   = g.attribute("status").value();
		if (g.attribute("remote-id")) a->remoteId = g.attribute("remote-id").value();
	} else if (line.type == LineType_Logic) {
		if (line.attrs.isNull()) line.attrs = QSharedPointer<MapLine::AttrBase>(new MapLine::LogicAttrs);
		MapLine::LogicAttrs* a = static_cast<MapLine::LogicAttrs*>(line.attrs.data());
		if (g.attribute("src-iedname"))  a->srcIedName  = g.attribute("src-iedname").value();
		if (g.attribute("src-cbname"))   a->srcCbName   = g.attribute("src-cbname").value();
		if (g.attribute("dest-iedname")) a->destIedName = g.attribute("dest-iedname").value();
		if (g.attribute("circuit-code")) { a->circuitCode = g.attribute("circuit-code").value(); line.code = a->circuitCode; }
	} else { // LineType_Virtual
		if (line.attrs.isNull()) line.attrs = QSharedPointer<MapLine::AttrBase>(new MapLine::VirtualAttrs);
		MapLine::VirtualAttrs* a = static_cast<MapLine::VirtualAttrs*>(line.attrs.data());
		if (g.attribute("srcIedName"))        a->srcIedName        = g.attribute("srcIedName").value();
		if (g.attribute("destIedName"))       a->destIedName       = g.attribute("destIedName").value();
		if (g.attribute("srcSoftPlateDesc"))  a->srcSoftPlateDesc  = g.attribute("srcSoftPlateDesc").value();
		if (g.attribute("destSoftPlateDesc")) a->destSoftPlateDesc = g.attribute("destSoftPlateDesc").value();
		if (g.attribute("srcSoftPlateRef"))   a->srcSoftPlateRef   = g.attribute("srcSoftPlateRef").value();
		if (g.attribute("destSoftPlateRef"))  a->destSoftPlateRef  = g.attribute("destSoftPlateRef").value();
		if (g.attribute("remoteId"))          a->remoteId          = g.attribute("remoteId").value();
		if (g.attribute("remoteSigId_A"))     a->remoteSigId_A     = g.attribute("remoteSigId_A").value();
		if (g.attribute("remoteSigId_B"))     a->remoteSigId_B     = g.attribute("remoteSigId_B").value();
		if (g.attribute("virtual-type"))      a->virtualType       = g.attribute("virtual-type").value();
		// 如果虚回路也提供了 code 字段，则记录
		if (g.attribute("code"))              line.code            = g.attribute("code").value();
	}
}

void InteractiveSvgMapItem::setVirtualValuesByCode(const QString& lineCode, double value, int precision)
{
	QMap<QString, int>::const_iterator it = m_lineIdByCode.find(lineCode);
	if (it == m_lineIdByCode.end()) return;
	setVirtualValues(it.value(), value, value, precision);
}

void InteractiveSvgMapItem::setLeftVirtualValue(const QString& lineCode, double value, int precision)
{
	QMap<QString, int>::const_iterator it = m_lineIdByCode.find(lineCode);
	if (it == m_lineIdByCode.end()) return;
	setVirtualValue(it.value(), true, value, precision);
}

void InteractiveSvgMapItem::setRightVirtualValue(const QString& lineCode, double value, int precision)
{
	QMap<QString, int>::const_iterator it = m_lineIdByCode.find(lineCode);
	if (it == m_lineIdByCode.end()) return;
	setVirtualValue(it.value(), false, value, precision);
}

void InteractiveSvgMapItem::drawPlateIcon(QPainter* painter, const QPointF& center) const
{
        painter->save();
        QPen pen(QColor(0x00ff00));
        pen.setWidth(2);
        painter->setPen(pen);
        int distance = PLATE_WIDTH - PLATE_CIRCLE_RADIUS * 4;
        painter->drawEllipse(center, PLATE_CIRCLE_RADIUS, PLATE_CIRCLE_RADIUS);
        painter->drawEllipse(center + QPointF(distance, 0), PLATE_CIRCLE_RADIUS, PLATE_CIRCLE_RADIUS);
        painter->save();
        painter->translate(center);
        int rectHeight = PLATE_CIRCLE_RADIUS * 2;
        QPointF p1(0, -rectHeight / 2), p2(distance, -rectHeight / 2);
        QPointF p3(distance, rectHeight / 2), p4(0, rectHeight / 2);
        painter->drawLine(p1, p2);
        painter->drawLine(p3, p4);
        painter->restore();
        painter->restore();
}

// drawArrows 已内聚到 paint 中 per-line 绘制

QVector<PlateCircleItem> InteractiveSvgMapItem::parsePlateCircles(const pugi::xml_node& plateCircleNode)
{
	QVector<PlateCircleItem> circles;
	// 处理压板组件组
	SvgNodeStyle style = parseNodeStyle(plateCircleNode);
	// 统一与 computePathBoundingRect 的顺序：先节点变换，再文档到像素
	QTransform transform = m_docToPix * utils::parseTransformMatrix(plateCircleNode);

	// d="
	// M980,375 C980,377.761 977.761,380 975,380 
	// C972.239,380 970,377.761 970,375 
	// C970,372.239 972.239,370 975,370 
	// C977.761,370 980,372.239 980,375 "
	for (pugi::xml_node path = plateCircleNode.child("path"); path; path = path.next_sibling("path")) {
		const char* d = path.attribute("d").value();
		if (!d || !*d) continue;

		QVector<QPointF> local = utils::parseSvgPathToPolyline(QString::fromLatin1(d));
		if (local.size() < 4) continue;
		double minX = 1e100, minY = 1e100;
		double maxX = -1e100, maxY = -1e100;
		for (int i = 0; i < local.size(); ++i) {
			double x = local[i].x(), y = local[i].y();
			if (x < minX) minX = x; if (x > maxX) maxX = x;
			if (y < minY) minY = y; if (y > maxY) maxY = y;
		}

		double cx = (minX + maxX) * 0.5;
		double cy = (minY + maxY) * 0.5;
		double rx = (maxX - minX) * 0.5;
		double ry = (maxY - minY) * 0.5;

		PlateCircleItem c;
		c.center = transform.map(QPointF(cx, cy));
		// 为了正确处理缩放，我们变换圆上的一个点来计算新的半径
		QPointF edgePoint = transform.map(QPointF(cx + rx, cy));
		c.radius = QLineF(c.center, edgePoint).length();
		c.style = style;

		circles.push_back(c);
	}
	return circles;
}

QVector<PlateLineItem> InteractiveSvgMapItem::parsePlateLines(const pugi::xml_node& plateLineNode)
{
	QVector<PlateLineItem> lines;
	SvgNodeStyle style = parseNodeStyle(plateLineNode);
	// 统一顺序：先节点变换，再文档到像素
	QTransform transform = m_docToPix * utils::parseTransformMatrix(plateLineNode);

	for (pugi::xml_node polyline = plateLineNode.child("polyline"); polyline; polyline = polyline.next_sibling("polyline"))
	{
		// 本地解析 points（不使用 parsePointsAttr，避免重复应用 m_docToPix）
		QVector<QPointF> points;
		const char* ptsAttr = polyline.attribute("points").as_string();
		if (ptsAttr && *ptsAttr) {
			QStringList ptList = QString::fromLatin1(ptsAttr).split(' ', QString::SkipEmptyParts);
			for (int i = 0; i < ptList.size(); ++i) {
				QStringList xy = ptList[i].split(',');
				if (xy.size() == 2) points.append(QPointF(xy[0].toDouble(), xy[1].toDouble()));
			}
		}
		if (points.size() >= 2)
		{
			PlateLineItem line;
			line.start = transform.map(points.first());
			line.end = transform.map(points.last());
			line.style = style;
			lines.append(line);
		}
	}
	return lines;
}

QVector<PlateRectItem> InteractiveSvgMapItem::parsePlateRects(const pugi::xml_node& plateRectNode)
{
	QVector<PlateRectItem> rects;
	SvgNodeStyle style = parseNodeStyle(plateRectNode);
	//style.fill = 
	// 统一与 computePathBoundingRect 的顺序：先节点变换，再文档到像素
	QTransform transform = m_docToPix * utils::parseTransformMatrix(plateRectNode);

	for (pugi::xml_node path = plateRectNode.child("path"); path; path = path.next_sibling("path"))
	{
		const char* d = path.attribute("d").value();
		if (!d || !*d) continue;

		QVector<QPointF> local = utils::parseSvgPathToPolyline(QString::fromLatin1(d));
		if (local.isEmpty()) continue;
		double minX = 1e100, minY = 1e100;
		double maxX = -1e100, maxY = -1e100;
		for (int i = 0; i < local.size(); ++i) {
			double x = local[i].x(), y = local[i].y();
			if (x < minX) minX = x;
			if (x > maxX) maxX = x;
			if (y < minY) minY = y;
			if (y > maxY) maxY = y;
		}

		if (minX <= maxX && minY <= maxY)
		{
			PlateRectItem r;
			QRectF localRect(QPointF(minX, minY), QPointF(maxX, maxY));
			r.rect = transform.mapRect(localRect);
			r.style = style;
			rects.append(r);
		}
	}
	return rects;
}

void InteractiveSvgMapItem::drawPlateCircles(QPainter* p, const QVector<PlateCircleItem>& cs)
{
	for (int i = 0; i < cs.size(); ++i) {
		const PlateCircleItem& c = cs[i];
		p->save();
		QPen pen(c.style.stroke);
		pen.setWidth(c.style.strokeWidth);
		p->setPen(pen);

		QColor fill = c.style.fill;
		fill.setAlphaF(c.style.fillOpacity);
		p->setBrush(fill);

		p->drawEllipse(c.center, c.radius, c.radius);
		p->restore();
	}
}

void InteractiveSvgMapItem::drawPlateLines(QPainter* p, const QVector<PlateLineItem>& ls)
{
	for (int i = 0; i < ls.size(); ++i) {
		const PlateLineItem& l = ls[i];
		p->save();
		QPen pen(Qt::green);
		pen.setWidth(l.style.strokeWidth);
		p->setPen(pen);

		p->drawLine(l.start, l.end);
		p->restore();
	}
}

void InteractiveSvgMapItem::drawPlateRects(QPainter* p, const QVector<PlateRectItem>& rs)
{
	foreach (const PlateRectItem & r, rs)
	{
		p->save();
		QPen pen(r.style.stroke);
		pen.setWidth(r.style.strokeWidth);

		if (!r.style.dashArray.isEmpty())
		{
			QStringList parts = r.style.dashArray.split(',');
			if (!parts.isEmpty())
			{
				QVector<qreal> dashPattern;
				foreach (const QString& part, parts)
				{
					dashPattern << part.toDouble();
				}
				pen.setDashPattern(dashPattern);
			}
		}

		p->setPen(pen);

		QColor fill = r.style.fill;
		fill.setAlphaF(r.style.fillOpacity);
		p->setBrush(fill);

		p->fillRect(r.rect, fill);
		pen.setColor(Qt::white);
		p->setPen(pen);
		p->drawRect(r.rect);
		p->restore();
	}
}

SvgNodeStyle InteractiveSvgMapItem::parseNodeStyle(const pugi::xml_node& node)
{
    SvgNodeStyle style;
    
    // 解析样式属性
    const char* strokeAttr = node.attribute("stroke").value();
    style.strokeWidth = node.attribute("stroke-width").as_int(1);
    style.strokeOpacity = node.attribute("stroke-opacity").as_double(1.0);
    const char* fillAttr = node.attribute("fill").value();
    style.fillOpacity = node.attribute("fill-opacity").as_double(1.0);
    style.dashArray = node.attribute("stroke-dasharray").as_string("");
    
    // 解析颜色
	style.stroke = utils::parseColor(strokeAttr, style.strokeOpacity);
	style.fill = utils::parseColor(fillAttr, style.fillOpacity);
    
    return style;
}

LineStyle InteractiveSvgMapItem::parseLineStyle(const pugi::xml_node& node)
{
	LineStyle ls;
	const char* strokeAttr = node.attribute("stroke").value();
	QColor stroke = parseColor(strokeAttr, 1.0);
	ls.strokeRgb = stroke.rgba();
	ls.strokeWidth = (unsigned short)node.attribute("stroke-width").as_int(1);
	return ls;
}

void InteractiveSvgMapItem::paintPlates(QPainter* painter)
{
	// 分两步：先铺 outerRect 背景覆盖回路，再绘制组件与边框
	for (int i = 0; i < m_allPlates.size(); ++i) {
		const PlateItem& plate = m_allPlates[i];
		if (!plate.outerRect.isNull()) {
			painter->save();
			QPen pen;
			QBrush brush;
			//pen.setColor(Qt::black);
			//pen.setStyle(Qt::DashLine);
			brush.setColor(QColor(0x23, 0x3f, 0x4f));
			brush.setStyle(Qt::SolidPattern);
			painter->setPen(pen);
			painter->setBrush(brush);
			//painter->drawRect(plate.outerRect);
			painter->fillRect(plate.outerRect, brush);
			painter->restore();
		}
	}
	for (int i = 0; i < m_allPlates.size(); ++i) {
		paintSinglePlate(painter, m_allPlates[i]);
	}
}

void InteractiveSvgMapItem::paintSinglePlate(QPainter* painter, const PlateItem& plate)
{
	// 内部组件
	if (!plate.isClosed) {
		// 置分：线隐藏、圆红色空心，矩形照常
		//drawPlateRects(painter, plate.rects);
		if (!plate.circles.isEmpty()) {
			QVector<PlateCircleItem> red = plate.circles;
			for (int k = 0; k < red.size(); ++k) {
				red[k].style.stroke = Qt::red;
				red[k].style.fill = Qt::NoBrush;
			}
			drawPlateCircles(painter, red);
		}
	} else {
		//drawPlateRects(painter, plate.rects);
		drawPlateLines(painter, plate.lines);
		drawPlateCircles(painter, plate.circles);
	}

	// 外框（白色虚线）绘制在最上层
	//const QRectF frameRect = !plate.outerRect.isNull() ? plate.outerRect : plate.rect;
	//if (!frameRect.isNull()) {
	//	QPen framePen(Qt::white);
	//	framePen.setWidth(2);
	//	framePen.setStyle(Qt::DashDotLine);
	//	QVector<qreal> dashPattern; dashPattern << 3.0; dashPattern << 3.0;
	//	framePen.setDashPattern(dashPattern);
	//	painter->save();
	//	painter->setPen(framePen);
	//	painter->setBrush(Qt::NoBrush);
	//	painter->drawRect(frameRect);
	//	painter->restore();
	//}
}

void InteractiveSvgMapItem::paintVirtualValues(QPainter* painter)
{
	if (m_valuePairs.isEmpty()) return;
	painter->save();
	QFont font = painter->font();
	// 稍微大一点，便于查看；可根据需要调整
	font.setPointSize(qMax(10, font.pointSize()));
	painter->setFont(font);
	painter->setPen(Qt::white);

	QMap<int, ValuePair>::const_iterator it = m_valuePairs.constBegin();
	for (; it != m_valuePairs.constEnd(); ++it) {
		const ValuePair& vp = it.value();
		if (vp.hasLeft) {
			painter->drawText(vp.left.rect, Qt::AlignCenter, vp.left.text);
		}
		if (vp.hasRight) {
			painter->drawText(vp.right.rect, Qt::AlignCenter, vp.right.text);
		}
	}
	painter->restore();
}

void InteractiveSvgMapItem::setVirtualValues(int lineId, double leftValue, double rightValue, int precision)
{
	QMap<int, ValuePair>::iterator it = m_valuePairs.find(lineId);
	if (it == m_valuePairs.end()) return;
	ValuePair& vp = it.value();
	if (vp.hasLeft)  vp.left.text  = QString::number(leftValue, 'f', precision);
	if (vp.hasRight) vp.right.text = QString::number(rightValue, 'f', precision);
	update();
}

void InteractiveSvgMapItem::setVirtualValue(int lineId, bool isLeft, double value, int precision)
{
	QMap<int, ValuePair>::iterator it = m_valuePairs.find(lineId);
	if (it == m_valuePairs.end()) return;
	ValuePair& vp = it.value();
	if (isLeft && vp.hasLeft) vp.left.text = QString::number(value, 'f', precision);
	if (!isLeft && vp.hasRight) vp.right.text = QString::number(value, 'f', precision);
	update();
}

bool InteractiveSvgMapItem::getVirtualValueRects(int lineId, QRectF& leftRect, QRectF& rightRect) const
{
	QMap<int, ValuePair>::const_iterator it = m_valuePairs.find(lineId);
	if (it == m_valuePairs.end()) return false;
	const ValuePair& vp = it.value();
	bool ok = true;
	if (vp.hasLeft) leftRect = vp.left.rect; else ok = false;
	if (vp.hasRight) rightRect = vp.right.rect; else ok = false;
	return ok;
}
