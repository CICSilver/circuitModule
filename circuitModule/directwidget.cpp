#include "directwidget.h"
#include "svgmodel.h"
#include "directitems.h"
#include "SvgUtils.h"
#include "circuitconfig.h"
#include "SvgTransformer.h"
#include <QApplication>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QResizeEvent>
#include <QFrame>
#include <stdlib.h>
#include <QMenu>
#include <QAction>
#include <QScrollBar>
#include <QTimer>
#include <QVBoxLayout>
#include <string.h>
static double dw_to_double(const char* val)
{
	return (val && val[0] != '\0') ? atof(val) : 0.0;
}

static bool fill_optical_ports(const OpticalCircuit* oc, const QString& iedName, QString& iedPort, QString& swPort, QString& swName)
{
	if (!oc) return false;
	if (oc->srcIedName == iedName) {
		iedPort = oc->srcIedPort;
		swPort = oc->destIedPort;
		swName = oc->destIedName;
		return true;
	}
	if (oc->destIedName == iedName) {
		iedPort = oc->destIedPort;
		swPort = oc->srcIedPort;
		swName = oc->srcIedName;
		return true;
	}
	return false;
}
static QString build_plate_tooltip(const DirectPlateItem* item)
{
	QString line1 = item->desc().isEmpty() ? QString::fromLocal8Bit("压板") : item->desc();
	QString line2 = item->ref();
	QString line3 = item->code() == 0 ? QString() : QString::number(item->code());
	if (line2.isEmpty()) return line1;
	QString sep = "\n";
	return line1 + sep + line2 + (line3.isEmpty() ? QString() : sep + line3);
}

static QString build_virtual_tooltip(const VirtualCircuitLine* line)
{
	QString tip = QString::fromLocal8Bit("虚回路");
	QString optLine;
	if (line && line->pVirtualCircuit) {
		const VirtualCircuit* vc = line->pVirtualCircuit;
		CircuitConfig* cfg = CircuitConfig::Instance();
		OpticalCircuit* left = cfg ? cfg->getOpticalByCode(vc->leftOpticalCode) : NULL;
		OpticalCircuit* right = cfg ? cfg->getOpticalByCode(vc->rightOpticalCode) : NULL;
		if (left && right) {
			QString leftIedPort, leftSwPort, rightSwPort, rightIedPort;
			QString swName1, swName2;
			if (fill_optical_ports(left, vc->srcIedName, leftIedPort, leftSwPort, swName1) &&
				fill_optical_ports(right, vc->destIedName, rightIedPort, rightSwPort, swName2)) {
				QString swName = !vc->switchIedName.isEmpty() ? vc->switchIedName : (!swName1.isEmpty() ? swName1 : swName2);
				optLine = leftIedPort + " <-> " + leftSwPort + QString::fromLocal8Bit(" ：") + swName + QString::fromLocal8Bit("：") + rightSwPort + " <-> " + rightIedPort;
			}
		} else if (left) {
			QString iedPort1, iedPort2;
			if (left->srcIedName == vc->srcIedName) {
				iedPort1 = left->srcIedPort;
				iedPort2 = left->destIedPort;
			} else {
				iedPort1 = left->destIedPort;
				iedPort2 = left->srcIedPort;
			}
			optLine = iedPort1 + " <-> " + iedPort2;
		}
	}
	if (!optLine.isEmpty()) tip += "\n" + optLine;
	if (line && !line->circuitDesc.isEmpty()) tip += "\n" + line->circuitDesc;
	return tip;
}

class DirectRelationWindow : public QWidget
{
public:
	DirectRelationWindow(QWidget* parent)
		: QWidget(parent)
		, m_directWidget(new DirectWidget(this))
	{
		setAttribute(Qt::WA_DeleteOnClose, true);
		QVBoxLayout* layout = new QVBoxLayout(this);
		layout->setContentsMargins(0, 0, 0, 0);
		layout->addWidget(m_directWidget);
		resize(1200, 800);
	}

	bool LoadByOpticalCode(const QString& currentIedName, quint64 opticalCode)
	{
		CircuitConfig* circuitConfig = CircuitConfig::Instance();
		if (!circuitConfig || currentIedName.isEmpty() || opticalCode == 0)
		{
			return false;
		}

		QList<VirtualCircuit*> relatedCircuitList = circuitConfig->GetVirtualCircuitListByOpticalCode(opticalCode);
		if (relatedCircuitList.isEmpty())
		{
			return false;
		}

		QString displayIedName = currentIedName;
		if (!circuitConfig->GetIedByName(displayIedName))
		{
			OpticalCircuit* opticalCircuit = circuitConfig->getOpticalByCode(opticalCode);
			if (opticalCircuit)
			{
				bool srcIsSwitch = opticalCircuit->srcIedName.contains("SW", Qt::CaseInsensitive);
				bool destIsSwitch = opticalCircuit->destIedName.contains("SW", Qt::CaseInsensitive);
				if (srcIsSwitch && !destIsSwitch)
				{
					displayIedName = opticalCircuit->destIedName;
				}
				else
				{
					displayIedName = opticalCircuit->srcIedName;
				}
			}
			if (displayIedName.isEmpty() && !relatedCircuitList.isEmpty() && relatedCircuitList.first())
			{
				displayIedName = relatedCircuitList.first()->srcIedName;
			}
		}
		if (displayIedName.isEmpty())
		{
			return false;
		}

		QSet<quint64> circuitCodeSet;
		QSet<QString> peerIedNameSet;
		for (int i = 0; i < relatedCircuitList.size(); ++i)
		{
			VirtualCircuit* virtualCircuit = relatedCircuitList.at(i);
			if (!virtualCircuit)
			{
				continue;
			}
			circuitCodeSet.insert(virtualCircuit->code);
			if (virtualCircuit->srcIedName == displayIedName)
			{
				peerIedNameSet.insert(virtualCircuit->destIedName);
			}
			else if (virtualCircuit->destIedName == displayIedName)
			{
				peerIedNameSet.insert(virtualCircuit->srcIedName);
			}
		}

		SvgTransformer transformer;
		VirtualSvg* virtualSvg = transformer.BuildVirtualModelByIedName(displayIedName);
		if (!virtualSvg)
		{
			return false;
		}

		m_directWidget->ParseFromVirtualSvg(*virtualSvg, circuitCodeSet);
		delete virtualSvg;

		if (peerIedNameSet.size() == 1)
		{
			QSet<QString>::const_iterator peerIt = peerIedNameSet.constBegin();
			setWindowTitle(QString("Virtual Circuits - %1 <-> %2").arg(displayIedName, *peerIt));
		}
		else if (!peerIedNameSet.isEmpty())
		{
			setWindowTitle(QString("Virtual Circuits - %1 (%2 peers)").arg(displayIedName).arg(peerIedNameSet.size()));
		}
		else
		{
			setWindowTitle(QString("Virtual Circuits - %1").arg(displayIedName));
		}
		return true;
	}

private:
	DirectWidget* m_directWidget;
};

DirectView::DirectView(QGraphicsScene* scene, QWidget* parent)
	: QGraphicsView(scene, parent)
	, m_dragging(false)
	, m_leftPressed(false)
{
}

void DirectView::mousePressEvent(QMouseEvent* event)
{
	if (event->button() == Qt::LeftButton)
	{
		m_leftPressed = true;
		m_dragging = false;
		m_pressPos = event->pos();
		m_lastViewPos = event->pos();
		QList<QGraphicsItem*> items = scene()->items();
		for (int i = 0; i < items.size(); ++i)
		{
			DirectVirtualLineItem* line = dynamic_cast<DirectVirtualLineItem*>(items[i]);
			if (line)
			{
				line->setHighlighted(false);
			}
		}
		event->accept();
		return;
	}
	QGraphicsView::mousePressEvent(event);
}

void DirectView::mouseMoveEvent(QMouseEvent* event)
{
	if (m_leftPressed)
	{
		if (!m_dragging)
		{
			QPoint deltaPoint = event->pos() - m_pressPos;
			if (deltaPoint.manhattanLength() >= QApplication::startDragDistance())
			{
				m_dragging = true;
				setInteractive(false);
				setCursor(Qt::ClosedHandCursor);
			}
		}
		if (m_dragging)
		{
			QPoint delta = event->pos() - m_lastViewPos;
			m_lastViewPos = event->pos();
			QScrollBar* hbar = horizontalScrollBar();
			QScrollBar* vbar = verticalScrollBar();
			if (hbar) hbar->setValue(hbar->value() - delta.x());
			if (vbar) vbar->setValue(vbar->value() - delta.y());
			event->accept();
			return;
		}
	}
	QGraphicsView::mouseMoveEvent(event);
}

void DirectView::mouseReleaseEvent(QMouseEvent* event)
{
	if (event->button() == Qt::LeftButton)
	{
		if (!m_dragging && m_leftPressed)
		{
			QGraphicsItem* item = itemAt(event->pos());
			DirectOpticalLineItem* opticalLine = dynamic_cast<DirectOpticalLineItem*>(item);
			if (opticalLine)
			{
				quint64 opticalCode = opticalLine->data(0).toULongLong();
				if (opticalCode != 0)
				{
					emit opticalLineClicked(opticalCode);
				}
			}
		}
		m_leftPressed = false;
		m_dragging = false;
		setInteractive(true);
		setCursor(Qt::ArrowCursor);
		event->accept();
		return;
	}
	QGraphicsView::mouseReleaseEvent(event);
}

void DirectView::wheelEvent(QWheelEvent* event)
{
	const double factor = (event->delta() > 0) ? 1.15 : 1.0 / 1.15;
	double sX = transform().m11();
	double sY = transform().m22();
	double minTarget = 0.5;
	double maxTarget = 2.0;
	double targetX = sX * factor;
	double targetY = sY * factor;
	if ((event->delta() > 0 && (targetX > maxTarget || targetY > maxTarget)) ||
		(event->delta() <= 0 && (targetX < minTarget || targetY < minTarget))) {
		event->accept();
		return;
	}
	setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
	scale(factor, factor);
	setTransformationAnchor(QGraphicsView::AnchorViewCenter);
	event->accept();
}

void DirectView::contextMenuEvent(QContextMenuEvent* event)
{
	QGraphicsItem* item = itemAt(event->pos());
	QMenu menu;
	DirectPlateItem* plate = dynamic_cast<DirectPlateItem*>(item);
	if (plate) {
		QAction* act = plate->isClosed()
			? menu.addAction(QString::fromLocal8Bit("断开"))
			: menu.addAction(QString::fromLocal8Bit("合上"));
		QAction* chosen = menu.exec(event->globalPos());
		if (chosen == act) {
			plate->setClosed(!plate->isClosed());
			RtdbClient& rtdb = RtdbClient::Instance();
			stuRtdbStatus* plateEle = rtdb.getRyb(plate->code());
			if (plateEle) {
				const char* newVal = plate->isClosed() ? "1" : "0";
				strcpy(plateEle->val, newVal);
			}
		}
		return;
	}
	QAction* actAllClose = menu.addAction(QString::fromLocal8Bit("全部合上"));
	QAction* actAllOpen  = menu.addAction(QString::fromLocal8Bit("全部断开"));
	QAction* chosen = menu.exec(event->globalPos());
	if (chosen == actAllClose || chosen == actAllOpen) {
		bool toClosed = (chosen == actAllClose);
		QList<QGraphicsItem*> items = scene()->items();
		for (int i = 0; i < items.size(); ++i) {
			DirectPlateItem* p = dynamic_cast<DirectPlateItem*>(items[i]);
			if (p) p->setClosed(toClosed);
		}
	}
}

void DirectView::mouseDoubleClickEvent(QMouseEvent* event)
{
	QGraphicsItem* item = itemAt(event->pos());
	DirectVirtualLineItem* line = dynamic_cast<DirectVirtualLineItem*>(item);
	if (line) {
		line->setBlinking(!line->isBlinking());
	}
	QGraphicsView::mouseDoubleClickEvent(event);
}

DirectWidget::DirectWidget(QWidget *parent)
	: QWidget(parent)
	, m_scene(NULL)
	, m_view(NULL)
	, m_rtdb(RtdbClient::Instance())
	, m_statusTimer(NULL)
	, m_blinkTimer(NULL)
	, m_blinkOn(false)
{
	initView();
	connect(m_view, SIGNAL(opticalLineClicked(quint64)), this, SLOT(OnOpticalLineClicked(quint64)));
	m_statusTimer = new QTimer(this);
	connect(m_statusTimer, SIGNAL(timeout()), this, SLOT(OnStatusTimeout()));
	m_statusTimer->start(1000);
	m_blinkTimer = new QTimer(this);
	connect(m_blinkTimer, SIGNAL(timeout()), this, SLOT(OnBlinkTimeout()));
	m_blinkTimer->start(500);
}

DirectWidget::~DirectWidget()
{}

void DirectWidget::ParseFromLogicSvg(const LogicSvg& svg)
{
	if (!m_scene) return;
	ClearScene();
	m_currentIedName = svg.mainIedRect ? svg.mainIedRect->iedName : QString();
	m_scene->setSceneRect(0, 0, svg.viewBoxWidth, svg.viewBoxHeight);
	// IEDs
	if (svg.mainIedRect) {
		IedItem* item = new IedItem();
		item->setFromIedRect(*svg.mainIedRect, false);
		m_scene->addItem(item);
	}
	for (int i = 0; i < svg.leftIedRectList.size(); ++i) {
		IedItem* item = new IedItem();
		item->setFromIedRect(*svg.leftIedRectList[i], false);
		m_scene->addItem(item);
	}
	for (int i = 0; i < svg.rightIedRectList.size(); ++i) {
		IedItem* item = new IedItem();
		item->setFromIedRect(*svg.rightIedRectList[i], false);
		m_scene->addItem(item);
	}
	// Logic lines
	QList<IedRect*> all;
	all += svg.leftIedRectList;
	all += svg.rightIedRectList;
	for (int i = 0; i < all.size(); ++i) {
		IedRect* rect = all[i];
		for (int j = 0; j < rect->logic_line_list.size(); ++j) {
			LogicCircuitLine* line = rect->logic_line_list[j];
			QVector<QPointF> pts;
			pts.append(line->startPoint);
			pts.append(line->midPoint);
			pts.append(line->endPoint);
			LineItem* l = new LineItem();
			QColor color = (line->pLogicCircuit && line->pLogicCircuit->type == SV) ?
				utils::ColorHelper::Color(utils::ColorHelper::line_smv) :
				utils::ColorHelper::Color(utils::ColorHelper::line_gse);
			l->setColor(color);
			l->setWidth(1);
			l->setPoints(pts);
			m_scene->addItem(l);
		}
	}
}

void DirectWidget::ParseFromOpaticalSvg()
{
}

void DirectWidget::ParseFromVirtualSvg()
{
}

void DirectWidget::initView()
{
	m_scene = new QGraphicsScene(this);
	m_view = new DirectView(m_scene, this);
	m_view->setFrameShape(QFrame::NoFrame);
	m_view->setRenderHint(QPainter::Antialiasing, true);
	m_view->setRenderHint(QPainter::TextAntialiasing, true);
	m_view->setAlignment(Qt::AlignCenter);
	m_view->setBackgroundBrush(Qt::black);
	m_scene->setBackgroundBrush(Qt::black);
	// keep view in widget bounds
	m_view->setGeometry(rect());
}

void DirectWidget::resizeEvent(QResizeEvent* event)
{
	QWidget::resizeEvent(event);
	if (m_view) m_view->setGeometry(rect());
}

void DirectWidget::ClearScene()
{
	if (m_scene) m_scene->clear();
	m_plateItems.clear();
	m_virtualLines.clear();
}

void DirectWidget::ParseFromVirtualSvg(const VirtualSvg& svg)
{
	ParseFromVirtualSvg(svg, QSet<quint64>());
}

void DirectWidget::ParseFromVirtualSvg(const VirtualSvg& svg, const QSet<quint64>& circuitCodeSet)
{
	if (!m_scene) return;
	ClearScene();
	m_currentIedName = svg.mainIedRect ? svg.mainIedRect->iedName : QString();
	m_scene->setSceneRect(0, 0, svg.viewBoxWidth, svg.viewBoxHeight);
	bool hasFilter = !circuitCodeSet.isEmpty();
	QSet<QString> visibleIedNameSet;
	QSet<QString> visiblePlateRefSet;
	QList<IedRect*> allRectList;
	allRectList += svg.leftIedRectList;
	allRectList += svg.rightIedRectList;
	if (hasFilter)
	{
		for (int i = 0; i < allRectList.size(); ++i)
		{
			IedRect* rect = allRectList.at(i);
			for (int j = 0; j < rect->logic_line_list.size(); ++j)
			{
				LogicCircuitLine* logic = rect->logic_line_list.at(j);
				for (int k = 0; k < logic->virtual_line_list.size(); ++k)
				{
					VirtualCircuitLine* virtualLine = logic->virtual_line_list.at(k);
					VirtualCircuit* virtualCircuit = virtualLine ? virtualLine->pVirtualCircuit : NULL;
					if (!virtualCircuit)
					{
						continue;
					}
					if (!circuitCodeSet.contains(virtualCircuit->code))
					{
						continue;
					}
					visibleIedNameSet.insert(rect->iedName);
					if (svg.mainIedRect)
					{
						visibleIedNameSet.insert(svg.mainIedRect->iedName);
					}
					if (!virtualCircuit->srcSoftPlateRef.isEmpty())
					{
						visiblePlateRefSet.insert(virtualCircuit->srcSoftPlateRef);
					}
					if (!virtualCircuit->destSoftPlateRef.isEmpty())
					{
						visiblePlateRefSet.insert(virtualCircuit->destSoftPlateRef);
					}
				}
			}
		}
	}
	if (svg.mainIedRect && (!hasFilter || visibleIedNameSet.contains(svg.mainIedRect->iedName)))
	{
		IedItem* item = new IedItem();
		item->setFromIedRect(*svg.mainIedRect, false);
		m_scene->addItem(item);
	}
	for (int i = 0; i < svg.leftIedRectList.size(); ++i)
	{
		IedRect* rect = svg.leftIedRectList.at(i);
		if (hasFilter && !visibleIedNameSet.contains(rect->iedName))
		{
			continue;
		}
		IedItem* item = new IedItem();
		item->setFromIedRect(*rect, false);
		m_scene->addItem(item);
	}
	for (int i = 0; i < svg.rightIedRectList.size(); ++i)
	{
		IedRect* rect = svg.rightIedRectList.at(i);
		if (hasFilter && !visibleIedNameSet.contains(rect->iedName))
		{
			continue;
		}
		IedItem* item = new IedItem();
		item->setFromIedRect(*rect, false);
		m_scene->addItem(item);
	}
	for (int i = 0; i < allRectList.size(); ++i)
	{
		IedRect* rect = allRectList.at(i);
		if (hasFilter && !visibleIedNameSet.contains(rect->iedName))
		{
			continue;
		}
		for (int j = 0; j < rect->logic_line_list.size(); ++j)
		{
			LogicCircuitLine* logic = rect->logic_line_list.at(j);
			for (int k = 0; k < logic->virtual_line_list.size(); ++k)
			{
				VirtualCircuitLine* virtualLine = logic->virtual_line_list.at(k);
				VirtualCircuit* virtualCircuit = virtualLine ? virtualLine->pVirtualCircuit : NULL;
				if (hasFilter)
				{
					if (!virtualCircuit || !circuitCodeSet.contains(virtualCircuit->code))
					{
						continue;
					}
				}
				DirectVirtualLineItem* lineItem = new DirectVirtualLineItem();
				int virtualType = virtualCircuit ? virtualCircuit->type : 0;
				lineItem->setFromVirtualLine(*virtualLine, virtualType);
				lineItem->setVirtualType(virtualType);
				if (virtualCircuit)
				{
					lineItem->setCircuitCode(virtualCircuit->code);
				}
				lineItem->setLineColor(utils::ColorHelper::Color(utils::ColorHelper::pure_green));
				lineItem->setArrowColor(utils::ColorHelper::Color(utils::ColorHelper::pure_green));
				lineItem->setValueVisible(true);
				lineItem->setToolTip(build_virtual_tooltip(virtualLine));
				m_scene->addItem(lineItem);
				m_virtualLines.append(lineItem);
			}
		}
	}
	if (svg.plateRectHash.isEmpty())
	{
		qWarning() << "plateRectHash is empty";
	}
	QHash<QString, PlateRect>::const_iterator plateIt = svg.plateRectHash.constBegin();
	for (; plateIt != svg.plateRectHash.constEnd(); ++plateIt)
	{
		if (hasFilter && !visiblePlateRefSet.contains(plateIt.value().ref))
		{
			continue;
		}
		DirectPlateItem* item = new DirectPlateItem();
		item->setFromPlateRect(plateIt.value(), plateIt.key());
		item->setClosed(true);
		item->setToolTip(build_plate_tooltip(item));
		m_scene->addItem(item);
		m_plateItems.insert(plateIt.key(), item);
	}
	UpdatePlateStatuses();
	UpdateLineStatuses();
	QRectF itemsRect = m_scene->itemsBoundingRect();
	if (!itemsRect.isNull())
	{
		m_scene->setSceneRect(itemsRect.adjusted(-20.0, -20.0, 20.0, 20.0));
	}
}

void DirectWidget::ParseFromOpticalSvg(const OpticalSvg& svg)
{
	if (!m_scene) return;
	ClearScene();
	m_currentIedName = svg.mainIedRect ? svg.mainIedRect->iedName : QString();
	m_scene->setSceneRect(0, 0, svg.viewBoxWidth, svg.viewBoxHeight);
	if (svg.mainIedRect) {
		IedItem* item = new IedItem();
		item->setFromIedRect(*svg.mainIedRect, false);
		m_scene->addItem(item);
	}
	for (int i = 0; i < svg.iedRectList.size(); ++i) {
		IedItem* item = new IedItem();
		item->setFromIedRect(*svg.iedRectList[i], false);
		m_scene->addItem(item);
	}
	for (int i = 0; i < svg.switcherRectList.size(); ++i) {
		IedItem* item = new IedItem();
		item->setFromIedRect(*svg.switcherRectList[i], true);
		m_scene->addItem(item);
	}
	for (int i = 0; i < svg.opticalCircuitLineList.size(); ++i) {
		OpticalCircuitLine* line = svg.opticalCircuitLineList[i];
		DirectOpticalLineItem* lineItem = new DirectOpticalLineItem();
		lineItem->setFromOpticalLine(*line);
		if (line && line->pOpticalCircuit)
		{
			lineItem->setData(0, QVariant((qulonglong)line->pOpticalCircuit->code));
		}
		m_scene->addItem(lineItem);
	}
	QRectF itemsRect = m_scene->itemsBoundingRect();
	if (!itemsRect.isNull())
	{
		m_scene->setSceneRect(itemsRect.adjusted(-20.0, -20.0, 20.0, 20.0));
	}
	if (m_view)
	{
		m_view->resetTransform();
		m_view->centerOn(m_scene->sceneRect().center());
	}
}

void DirectWidget::OnOpticalLineClicked(quint64 opticalCode)
{
	if (m_currentIedName.isEmpty() || opticalCode == 0)
	{
		return;
	}
	DirectRelationWindow* relationWindow = new DirectRelationWindow(NULL);
	if (!relationWindow->LoadByOpticalCode(m_currentIedName, opticalCode))
	{
		delete relationWindow;
		return;
	}
	relationWindow->show();
	relationWindow->raise();
	relationWindow->activateWindow();
}

void DirectWidget::UpdatePlateStatuses()
{
	if (!m_rtdb.isOpen())
		m_rtdb.refresh();
	QMap<QString, DirectPlateItem*>::iterator it = m_plateItems.begin();
	for (; it != m_plateItems.end(); ++it) {
		DirectPlateItem* item = it.value();
		if (!item) continue;
		quint64 code = item->code();
		if (code == 0) continue;
		stuRtdbStatus* plateEle = m_rtdb.getRyb(code);
		if (plateEle) {
			int value = plateEle->val[0] != '\0' ? atoi(plateEle->val) : 0;
			item->setClosed(value != 0);
		}
	}
}
void DirectWidget::UpdateLineStatuses()
{
	if (!m_rtdb.isOpen())
		m_rtdb.refresh();
	for (int i = 0; i < m_virtualLines.size(); ++i) {
		DirectVirtualLineItem* line = m_virtualLines[i];
		if (!line) continue;
		quint64 code = line->circuitCode();
		if (code == 0) continue;
		if (line->virtualType() == GOOSE) {
			stuRtdbGseCircuit* gseCircuit = m_rtdb.getGseCircuit(code);
			if (gseCircuit) {
				stuRtdbStatus* inChl = static_cast<stuRtdbStatus*>(gseCircuit->m_pInChl);
				stuRtdbStatus* outChl = static_cast<stuRtdbStatus*>(gseCircuit->m_pOutChl);
				QString outVal = outChl ? QString::number(dw_to_double(outChl->val), 'f', 0) : QString();
				QString inVal  = inChl ? QString::number(dw_to_double(inChl->val), 'f', 0) : QString();
				line->setValues(outVal, inVal);
			}
		} else {
			stuRtdbSvCircuit* svCircuit = m_rtdb.getSvCircuit(code);
			if (svCircuit) {
				stuRtdbAnalog* inChl = static_cast<stuRtdbAnalog*>(svCircuit->m_pInChl);
				stuRtdbAnalog* outChl = static_cast<stuRtdbAnalog*>(svCircuit->m_pOutChl);
				QString outVal = outChl ? QString::number(dw_to_double(outChl->val), 'f', 0) : QString();
				QString inVal  = inChl ? QString::number(dw_to_double(inChl->val), 'f', 0) : QString();
				line->setValues(outVal, inVal);
			}
		}
	}
}

void DirectWidget::OnStatusTimeout()
{
	UpdatePlateStatuses();
	UpdateLineStatuses();
}

void DirectWidget::OnBlinkTimeout()
{
	m_blinkOn = !m_blinkOn;
	for (int i = 0; i < m_virtualLines.size(); ++i) {
		DirectVirtualLineItem* line = m_virtualLines[i];
		if (line) line->setBlinkOn(m_blinkOn);
	}
}
