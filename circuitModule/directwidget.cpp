#include "directwidget.h"
#include "svgmodel.h"
#include "directitems.h"
#include "directlinehelpers.h"
#include "SvgUtils.h"
#include "circuitconfig.h"
#include "CircuitDiagramProxy.h"
#include <QApplication>
#include "ControlBlockListWindow.h"
#include <QFontMetrics>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QResizeEvent>
#include <QFrame>
#include <stdlib.h>
#include <QMenu>
#include <QAction>
#include <QAbstractItemView>
#include <QHeaderView>
#include <QScrollBar>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QTimer>
#include <QVBoxLayout>
#include <QtAlgorithms>
#include <string.h>
static double dw_to_double(const char* val)
{
	return (val && val[0] != '\0') ? atof(val) : 0.0;
}

static bool fill_optical_ports(const OpticalCircuit* oc, const QString& iedName, QString& iedPort, QString& swPort, QString& swName)
{
	if (!oc) return false;
	if (oc->srcIedName == iedName)
	{
		iedPort = oc->srcIedPort;
		swPort = oc->destIedPort;
		swName = oc->destIedName;
		return true;
	}
	if (oc->destIedName == iedName)
	{
		iedPort = oc->destIedPort;
		swPort = oc->srcIedPort;
		swName = oc->srcIedName;
		return true;
	}
	return false;
}

static bool fill_optical_endpoint_info(const OpticalCircuitLine* line, bool isStartPoint, QString& iedName, QString& peerIedName, bool& isSwitch, bool& atTop)
{
	if (!line || !line->pSrcRect || !line->pDestRect || !line->pOpticalCircuit)
	{
		return false;
	}
	bool startIsSrc = directlinehelpers::IsPointAttachedToRect(line->startPoint, line->pSrcRect);
	bool endIsSrc = directlinehelpers::IsPointAttachedToRect(line->endPoint, line->pSrcRect);
	bool useStartAsSrc = true;
	if (!startIsSrc && endIsSrc)
	{
		useStartAsSrc = false;
	}
	const IedRect* pRect = NULL;
	const IedRect* pPeerRect = NULL;
	QPoint point;
	if (isStartPoint)
	{
		point = line->startPoint;
		if (useStartAsSrc)
		{
			pRect = line->pSrcRect;
			pPeerRect = line->pDestRect;
		}
		else
		{
			pRect = line->pDestRect;
			pPeerRect = line->pSrcRect;
		}
	}
	else
	{
		point = line->endPoint;
		if (useStartAsSrc)
		{
			pRect = line->pDestRect;
			pPeerRect = line->pSrcRect;
		}
		else
		{
			pRect = line->pSrcRect;
			pPeerRect = line->pDestRect;
		}
	}
	if (!pRect || !pPeerRect)
	{
		return false;
	}
	iedName = pRect->iedName;
	peerIedName = pPeerRect->iedName;
	isSwitch = iedName.contains("SW", Qt::CaseInsensitive);
	atTop = directlinehelpers::IsTopAnchor(point, pRect);
	return true;
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

static QString build_maint_plate_default_text()
{
	return QString::fromLocal8Bit("检修压板");
}

static QString build_virtual_tooltip(const VirtualCircuitLine* line)
{
	QString tip = QString::fromLocal8Bit("虚回路");
	QString optLine;
	if (line && line->pVirtualCircuit)
	{
		const VirtualCircuit* vc = line->pVirtualCircuit;
		CircuitConfig* cfg = CircuitConfig::Instance();
		OpticalCircuit* left = cfg ? cfg->getOpticalByCode(vc->leftOpticalCode) : NULL;
		OpticalCircuit* right = cfg ? cfg->getOpticalByCode(vc->rightOpticalCode) : NULL;
		if (left && right)
		{
			QString leftIedPort, leftSwPort, rightSwPort, rightIedPort;
			QString swName1, swName2;
			if (fill_optical_ports(left, vc->srcIedName, leftIedPort, leftSwPort, swName1) &&
				fill_optical_ports(right, vc->destIedName, rightIedPort, rightSwPort, swName2))
				{
				QString swName = !vc->switchIedName.isEmpty() ? vc->switchIedName : (!swName1.isEmpty() ? swName1 : swName2);
				optLine = leftIedPort + " <-> " + leftSwPort + QString::fromLocal8Bit(" ：") + swName + QString::fromLocal8Bit("：") + rightSwPort + " <-> " + rightIedPort;
			}
		}
		else if (left)
		{
			QString iedPort1, iedPort2;
			if (left->srcIedName == vc->srcIedName)
			{
				iedPort1 = left->srcIedPort;
				iedPort2 = left->destIedPort;
			}
			else
			{
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

static QString build_logic_review_title()
{
	return QString::fromLocal8Bit("检修域");
}

static QString build_logic_effect_title()
{
	return QString::fromLocal8Bit("影响域");
}

static QRectF build_logic_external_rect(const IedRect* rect, const QString& title)
{
	if (!rect)
	{
		return QRectF();
	}
	QFont font = QApplication::font();
	font.setPointSize(LogicFrameItem::TitleFontPointSize());
	QFontMetrics metrics(font);
	int titleWidth = metrics.width(title);
	return QRectF(
		rect->x - rect->horizontal_margin - titleWidth,
		rect->y - rect->vertical_margin,
		rect->width + rect->horizontal_margin * 2 + titleWidth,
		rect->height + rect->vertical_margin * 2);
}

static QRectF build_logic_external_rect(const QList<IedRect*>& rectList, const QString& title)
{
	if (rectList.isEmpty())
	{
		return QRectF();
	}
	IedRect* firstIed = rectList.first();
	IedRect* lastIed = rectList.last();
	QFont font = QApplication::font();
	font.setPointSize(LogicFrameItem::TitleFontPointSize());
	QFontMetrics metrics(font);
	int titleWidth = metrics.width(title);
	int x = firstIed->x - firstIed->horizontal_margin - titleWidth;
	int y = firstIed->y - firstIed->vertical_margin;
	int width = firstIed->width + firstIed->horizontal_margin * 2 + titleWidth;
	int height = (lastIed->y - y + lastIed->height) + lastIed->vertical_margin;
	return QRectF(x, y, width, height);
}

static QRectF build_virtual_ied_outer_rect(const IedRect* rect)
{
	if (!rect)
	{
		return QRectF();
	}
	if (rect->extend_height > 0)
	{
		return QRectF(rect->x - rect->inner_gap,
			rect->y - rect->inner_gap,
			rect->width + rect->inner_gap * 2,
			rect->height + rect->extend_height + rect->inner_gap * 2);
	}
	return QRectF(rect->x, rect->y, rect->width, rect->height);
}

static void logic_add_frame_item(QGraphicsScene* scene, const QRectF& rect, const QString& title, const QColor& borderColor, bool showLegend)
{
	if (!scene || rect.isNull())
	{
		return;
	}
	LogicFrameItem* frameItem = new LogicFrameItem();
	frameItem->setFrame(rect, title, borderColor, showLegend);
	frameItem->setZValue(-2.0);
	scene->addItem(frameItem);
}

static void logic_add_ied_item(QGraphicsScene* scene, const IedRect* rect)
{
	if (!scene || !rect)
	{
		return;
	}
	IedItem* item = new IedItem();
	item->setFromIedRect(*rect, false);
	item->setZValue(-1.0);
	scene->addItem(item);
}

static void logic_add_ied_item_list(QGraphicsScene* scene, const QList<IedRect*>& rectList)
{
	for (int i = 0; i < rectList.size(); ++i)
	{
		logic_add_ied_item(scene, rectList.at(i));
	}
}

static QString whole_build_elided_label_text(const QString& text, const QFontMetrics& metrics, int width)
{
	if (text.isEmpty() || width <= 0)
	{
		return QString();
	}
	QString suffix = QString::fromLatin1("..");
	int suffixWidth = metrics.width(suffix);
	if (width < suffixWidth)
	{
		return QString();
	}
	if (metrics.width(text) <= width)
	{
		return text;
	}
	QString result = text;
	while (!result.isEmpty() && metrics.width(result + suffix) > width)
	{
		result.chop(1);
	}
	if (result.isEmpty())
	{
		return suffix;
	}
	return result + suffix;
}

static void whole_build_side_label_layout(const VirtualCircuitLine* virtualLine, const WholeGroupDecor* groupDecor, QString& leftText, QRectF& leftRect, QString& rightText, QRectF& rightRect)
{
	leftText.clear();
	rightText.clear();
	leftRect = QRectF();
	rightRect = QRectF();
	if (!virtualLine || !virtualLine->pVirtualCircuit || !groupDecor || !virtualLine->pSrcIedRect || !virtualLine->pDestIedRect)
	{
		return;
	}
	const VirtualCircuit* virtualCircuit = virtualLine->pVirtualCircuit;
	QFont font = QApplication::font();
	font.setPointSize(DirectVirtualLineItem::LabelFontPointSize());
	QFontMetrics metrics(font);
	bool srcOnLeft = virtualLine->pSrcIedRect->x <= virtualLine->pDestIedRect->x;
	const IedRect* leftIedRect = srcOnLeft ? virtualLine->pSrcIedRect : virtualLine->pDestIedRect;
	const IedRect* rightIedRect = srcOnLeft ? virtualLine->pDestIedRect : virtualLine->pSrcIedRect;
	QString leftName = srcOnLeft ? virtualCircuit->srcDesc : virtualCircuit->destDesc;
	QString rightName = srcOnLeft ? virtualCircuit->destDesc : virtualCircuit->srcDesc;
	qreal verticalPadding = DirectVirtualLineItem::SideLabelVerticalPadding();
	qreal labelHeight = metrics.lineSpacing() + verticalPadding + verticalPadding;
	qreal labelTop = virtualLine->startPoint.y() - labelHeight - DirectVirtualLineItem::SideLabelLineGap();
	qreal textWidth = DirectVirtualLineItem::SideLabelTextWidth();
	qreal leftLabelStartX = leftIedRect->x + leftIedRect->width;
	qreal leftLabelEndLimit = groupDecor->leftBraceRect.left();
	qreal rightLabelStartX = groupDecor->rightBraceRect.right();
	qreal rightLabelEndLimit = rightIedRect->x;
	qreal leftAvailableWidth = leftLabelEndLimit - leftLabelStartX;
	qreal rightAvailableWidth = rightLabelEndLimit - rightLabelStartX;
	if (leftAvailableWidth > 0 && !leftName.isEmpty())
	{
		qreal leftWidth = qMin(textWidth, leftAvailableWidth);
		leftText = whole_build_elided_label_text(leftName, metrics, (int)leftWidth);
		if (!leftText.isEmpty())
		{
			leftRect = QRectF(leftLabelStartX, labelTop, leftWidth, labelHeight);
		}
	}
	if (rightAvailableWidth > 0 && !rightName.isEmpty())
	{
		qreal rightWidth = qMin(textWidth, rightAvailableWidth);
		rightText = whole_build_elided_label_text(rightName, metrics, (int)rightWidth);
		if (!rightText.isEmpty())
		{
			rightRect = QRectF(rightLabelStartX, labelTop, rightWidth, labelHeight);
		}
	}
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

	bool LoadByOpticalCode(const QString& currentIedName, quint64 opticalCode, const QString& srcIedName, const QString& destIedName)
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

		QString pairIedName1 = srcIedName;
		QString pairIedName2 = destIedName;
		if (pairIedName1.contains("SW", Qt::CaseInsensitive))
		{
			pairIedName1.clear();
		}
		if (pairIedName2.contains("SW", Qt::CaseInsensitive))
		{
			pairIedName2.clear();
		}
		bool hasPairFilter = !pairIedName1.isEmpty() && !pairIedName2.isEmpty() && pairIedName1 != pairIedName2;

		QSet<quint64> circuitCodeSet;
		QSet<QString> peerIedNameSet;
		for (int i = 0; i < relatedCircuitList.size(); ++i)
		{
			VirtualCircuit* virtualCircuit = relatedCircuitList.at(i);
			if (!virtualCircuit)
			{
				continue;
			}
			if (hasPairFilter)
			{
				bool isPairMatched = (virtualCircuit->srcIedName == pairIedName1 && virtualCircuit->destIedName == pairIedName2) ||
					(virtualCircuit->srcIedName == pairIedName2 && virtualCircuit->destIedName == pairIedName1);
				if (!isPairMatched)
				{
					continue;
				}
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
		if (hasPairFilter && circuitCodeSet.isEmpty())
		{
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
		}

		CircuitDiagramProxy diagramProxy;
		VirtualDiagramModel* virtualDiagram = NULL;
		if (hasPairFilter)
		{
			QString mainIedName = displayIedName;
			QString peerIedName;
			if (mainIedName == pairIedName1)
			{
				peerIedName = pairIedName2;
			}
			else if (mainIedName == pairIedName2)
			{
				peerIedName = pairIedName1;
			}
			else
			{
				mainIedName = pairIedName1;
				peerIedName = pairIedName2;
			}
			virtualDiagram = diagramProxy.BuildVirtualDiagramByIedPair(mainIedName, peerIedName);
		}
		else
		{
			virtualDiagram = diagramProxy.BuildVirtualDiagramByIedName(displayIedName);
		}
		if (!virtualDiagram)
		{
			return false;
		}

		m_directWidget->ParseFromVirtualSvg(*virtualDiagram, circuitCodeSet);
		delete virtualDiagram;

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
	bool LoadByLogicDirection(const QString& srcIedName, const QString& destIedName, const QSet<quint64>& circuitCodeSet, const QString& preferredMainIedName)
	{
		if (circuitCodeSet.isEmpty())
		{
			return false;
		}
		QString displayIedName = preferredMainIedName;
		if (displayIedName.isEmpty())
		{
			displayIedName = srcIedName;
		}
		if (displayIedName.isEmpty())
		{
			displayIedName = destIedName;
		}
		if (displayIedName.isEmpty())
		{
			return false;
		}
		CircuitDiagramProxy diagramProxy;
		VirtualDiagramModel* virtualDiagram = NULL;
		if (!srcIedName.isEmpty() && !destIedName.isEmpty() && srcIedName != destIedName)
		{
			QString mainIedName = displayIedName;
			QString peerIedName;
			if (mainIedName == srcIedName)
			{
				peerIedName = destIedName;
			}
			else if (mainIedName == destIedName)
			{
				peerIedName = srcIedName;
			}
			else
			{
				mainIedName = srcIedName;
				peerIedName = destIedName;
			}
			virtualDiagram = diagramProxy.BuildVirtualDiagramByIedPair(mainIedName, peerIedName);
		}
		else
		{
			virtualDiagram = diagramProxy.BuildVirtualDiagramByIedName(displayIedName);
		}
		if (!virtualDiagram)
		{
			return false;
		}
		m_directWidget->ParseFromVirtualSvg(*virtualDiagram, circuitCodeSet);
		delete virtualDiagram;
		if (!srcIedName.isEmpty() && !destIedName.isEmpty())
		{
			setWindowTitle(QString("Virtual Circuits - %1 -> %2").arg(srcIedName, destIedName));
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
			DirectOpticalConnPointItem* opticalConnPoint = dynamic_cast<DirectOpticalConnPointItem*>(item);
			if (opticalConnPoint)
			{
				quint64 opticalCode = opticalConnPoint->data(0).toULongLong();
				if (opticalCode != 0)
				{
					QString iedName = opticalConnPoint->data(1).toString();
					QString peerIedName = opticalConnPoint->data(2).toString();
					emit opticalConnPointClicked(opticalCode, iedName, peerIedName);
				}
			}
			else
			{
				DirectOpticalLineItem* opticalLine = dynamic_cast<DirectOpticalLineItem*>(item);
				if (opticalLine)
				{
					quint64 opticalCode = opticalLine->data(0).toULongLong();
					if (opticalCode != 0)
					{
						QString srcIedName = opticalLine->data(1).toString();
						QString destIedName = opticalLine->data(2).toString();
						emit opticalLineClicked(opticalCode, srcIedName, destIedName);
					}
				}
			}
			LineItem* logicLine = dynamic_cast<LineItem*>(item);
			if (logicLine)
			{
				emit logicLineClicked(logicLine);
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
		(event->delta() <= 0 && (targetX < minTarget || targetY < minTarget)))
	{
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
	DirectMaintainPlateItem* maintainPlate = dynamic_cast<DirectMaintainPlateItem*>(item);
	if (maintainPlate)
	{
		QAction* act = maintainPlate->isClosed()
			? menu.addAction(QString::fromLatin1("Open"))
			: menu.addAction(QString::fromLatin1("Close"));
		QAction* chosen = menu.exec(event->globalPos());
		if (chosen == act)
		{
			bool nextClosed = !maintainPlate->isClosed();
			maintainPlate->setClosed(nextClosed);
			emit maintainPlateToggled(maintainPlate->iedName(), nextClosed ? 1 : 0);
		}
		return;
	}
	DirectPlateItem* plate = dynamic_cast<DirectPlateItem*>(item);
	if (plate)
	{
		QAction* act = plate->isClosed()
			? menu.addAction(QString::fromLocal8Bit("断开"))
			: menu.addAction(QString::fromLocal8Bit("合上"));
		QAction* chosen = menu.exec(event->globalPos());
		if (chosen == act)
		{
			plate->setClosed(!plate->isClosed());
			RtdbClient& rtdb = RtdbClient::Instance();
			stuRtdbStatus* plateEle = rtdb.getRyb(plate->code());
			if (plateEle)
			{
				const char* newVal = plate->isClosed() ? "1" : "0";
				strcpy(plateEle->val, newVal);
			}
		}
		return;
	}
	QAction* actAllClose = menu.addAction(QString::fromLocal8Bit("全部合上"));
	QAction* actAllOpen  = menu.addAction(QString::fromLocal8Bit("全部断开"));
	QAction* chosen = menu.exec(event->globalPos());
	if (chosen == actAllClose || chosen == actAllOpen)
	{
		bool toClosed = (chosen == actAllClose);
		QList<QGraphicsItem*> items = scene()->items();
		for (int i = 0; i < items.size(); ++i)
		{
			DirectPlateItem* p = dynamic_cast<DirectPlateItem*>(items[i]);
			if (p) p->setClosed(toClosed);
		}
	}
}

void DirectView::mouseDoubleClickEvent(QMouseEvent* event)
{
	QGraphicsItem* item = itemAt(event->pos());
	DirectVirtualLineItem* line = dynamic_cast<DirectVirtualLineItem*>(item);
	if (line)
	{
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
	connect(m_view, SIGNAL(opticalLineClicked(quint64, const QString&, const QString&)), this, SLOT(OnOpticalLineClicked(quint64, const QString&, const QString&)));
	connect(m_view, SIGNAL(opticalConnPointClicked(quint64, const QString&, const QString&)), this, SLOT(OnOpticalConnPointClicked(quint64, const QString&, const QString&)));
	connect(m_view, SIGNAL(logicLineClicked(LineItem*)), this, SLOT(OnLogicLineClicked(LineItem*)));
	connect(m_view, SIGNAL(maintainPlateToggled(const QString&, int)), this, SLOT(OnMaintainPlateToggled(const QString&, int)));
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
	if (!m_scene)
	{
		return;
	}
	ClearScene();
	m_currentIedName = svg.mainIedRect ? svg.mainIedRect->iedName : (svg.centerIedRectList.isEmpty() ? QString() : svg.centerIedRectList.first()->iedName);
	m_scene->setSceneRect(0, 0, svg.viewBoxWidth, svg.viewBoxHeight);
	QString reviewTitle = build_logic_review_title();
	QString effectTitle = build_logic_effect_title();
	bool hasLeftFrame = !svg.leftIedRectList.isEmpty();
	bool hasCenterFrame = false;
	QRectF centerFrameRect;
	QColor centerFrameColor;
	if (!svg.centerIedRectList.isEmpty())
	{
		centerFrameRect = build_logic_external_rect(svg.centerIedRectList, reviewTitle);
		centerFrameColor = utils::ColorHelper::Color(utils::ColorHelper::pure_red);
		hasCenterFrame = true;
	}
	else if (svg.mainIedRect)
	{
		centerFrameRect = build_logic_external_rect(svg.mainIedRect, reviewTitle);
		centerFrameColor = utils::ColorHelper::Color(svg.mainIedRect->border_color);
		hasCenterFrame = true;
	}
	if (hasCenterFrame)
	{
		logic_add_frame_item(m_scene, centerFrameRect, reviewTitle, centerFrameColor, !hasLeftFrame);
	}
	if (hasLeftFrame)
	{
		logic_add_frame_item(m_scene, build_logic_external_rect(svg.leftIedRectList, effectTitle), effectTitle, utils::ColorHelper::Color(svg.leftIedRectList.first()->border_color), true);
	}
	logic_add_ied_item(m_scene, svg.mainIedRect);
	logic_add_ied_item_list(m_scene, svg.centerIedRectList);
	logic_add_ied_item_list(m_scene, svg.leftIedRectList);
	logic_add_ied_item_list(m_scene, svg.rightIedRectList);
	QSet<QString> centerIedNameSet;
	for (int centerIndex = 0; centerIndex < svg.centerIedRectList.size(); ++centerIndex)
	{
		IedRect* centerRect = svg.centerIedRectList.at(centerIndex);
		if (!centerRect)
		{
			continue;
		}
		centerIedNameSet.insert(centerRect->iedName);
	}
	if (svg.mainIedRect)
	{
		centerIedNameSet.insert(svg.mainIedRect->iedName);
	}
	QList<IedRect*> all;
	all += svg.centerIedRectList;
	all += svg.leftIedRectList;
	all += svg.rightIedRectList;
	for (int i = 0; i < all.size(); ++i)
	{
		IedRect* rect = all[i];
		for (int j = 0; j < rect->logic_line_list.size(); ++j)
		{
			LogicCircuitLine* line = rect->logic_line_list[j];
			if (!line)
			{
				continue;
			}
			QVector<QPointF> pts;
			pts.append(line->startPoint);
			if (!line->turnPointList.isEmpty())
			{
				for (int ptIndex = 0; ptIndex < line->turnPointList.size(); ++ptIndex)
				{
					pts.append(line->turnPointList.at(ptIndex));
				}
			}
			else
			{
				pts.append(line->midPoint);
			}
			pts.append(line->endPoint);
			QSet<quint64> circuitCodeSet;
			QString srcIedName;
			QString destIedName;
			if (line->pLogicCircuit)
			{
				if (line->pLogicCircuit->pSrcIed)
				{
					srcIedName = line->pLogicCircuit->pSrcIed->name;
				}
				if (line->pLogicCircuit->pDestIed)
				{
					destIedName = line->pLogicCircuit->pDestIed->name;
				}
				for (int circuitIndex = 0; circuitIndex < line->pLogicCircuit->circuitList.size(); ++circuitIndex)
				{
					VirtualCircuit* virtualCircuit = line->pLogicCircuit->circuitList.at(circuitIndex);
					if (!virtualCircuit)
					{
						continue;
					}
					circuitCodeSet.insert(virtualCircuit->code);
				}
			}
			if (srcIedName.isEmpty() && line->pSrcIedRect)
			{
				srcIedName = line->pSrcIedRect->iedName;
			}
			if (destIedName.isEmpty() && line->pDestIedRect)
			{
				destIedName = line->pDestIedRect->iedName;
			}
			QString preferredMainIedName;
			if (centerIedNameSet.contains(srcIedName) && !centerIedNameSet.contains(destIedName))
			{
				preferredMainIedName = srcIedName;
			}
			else if (centerIedNameSet.contains(destIedName) && !centerIedNameSet.contains(srcIedName))
			{
				preferredMainIedName = destIedName;
			}
			else if (centerIedNameSet.contains(srcIedName))
			{
				preferredMainIedName = srcIedName;
			}
			else if (centerIedNameSet.contains(destIedName))
			{
				preferredMainIedName = destIedName;
			}
			LineItem* l = new LineItem();
			QColor color = (line->pLogicCircuit && line->pLogicCircuit->type == SV) ?
				utils::ColorHelper::Color(utils::ColorHelper::line_smv) :
				utils::ColorHelper::Color(utils::ColorHelper::line_gse);
			l->setColor(color);
			l->setArrowColor(color);
			l->setArrowState(line->srcArrowState, line->destArrowState);
			l->setWidth(1);
			l->setPoints(pts);
			l->setDirectionIedNames(srcIedName, destIedName);
			l->setRelatedVirtualCodes(circuitCodeSet);
			l->setPreferredMainIedName(preferredMainIedName);
			m_scene->addItem(l);
		}
	}
	QRectF itemsRect = m_scene->itemsBoundingRect();
	if (!itemsRect.isNull())
	{
		m_scene->setSceneRect(itemsRect.adjusted(-20.0, -20.0, 20.0, 20.0));
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
	m_maintPlateItems.clear();
	m_virtualLines.clear();
}

void DirectWidget::ParseFromVirtualSvg(const VirtualSvg& svg)
{
	ParseFromVirtualSvg(svg, QSet<quint64>());
}

void DirectWidget::SetMaintainPlateText(const QString& iedName, const QString& text)
{
	if (iedName.isEmpty())
	{
		return;
	}
	QString displayText = text;
	if (displayText.isEmpty())
	{
		displayText = build_maint_plate_default_text();
	}
	m_maintPlateTextByIed.insert(iedName, displayText);
	DirectMaintainPlateItem* item = m_maintPlateItems.value(iedName, NULL);
	if (!item)
	{
		return;
	}
	item->setDisplayText(displayText);
	item->setToolTip(displayText);
}

void DirectWidget::SetMaintainPlateState(const QString& iedName, int value)
{
	if (iedName.isEmpty())
	{
		return;
	}
	int stateValue = value == 0 ? 0 : 1;
	m_maintPlateStateByIed.insert(iedName, stateValue);
	DirectMaintainPlateItem* item = m_maintPlateItems.value(iedName, NULL);
	if (!item)
	{
		return;
	}
	item->setStateByValue(stateValue);
}

void DirectWidget::ParseFromVirtualSvg(const VirtualSvg& svg, const QSet<quint64>& circuitCodeSet)
{
	if (!m_scene)
	{
		return;
	}
	ClearScene();
	m_currentIedName = svg.mainIedRect ? svg.mainIedRect->iedName : (svg.centerIedRectList.isEmpty() ? QString() : svg.centerIedRectList.first()->iedName);
	m_scene->setSceneRect(0, 0, svg.viewBoxWidth, svg.viewBoxHeight);
	bool hasFilter = !circuitCodeSet.isEmpty();
	QSet<QString> visibleIedNameSet;
	QSet<QString> visiblePlateRefSet;
	QSet<quint64> visiblePlateCodeSet;
	QMap<QString, QRectF> maintainPlateRectHash;
	QList<IedRect*> allRectList;
	allRectList += svg.centerIedRectList;
	allRectList += svg.leftIedRectList;
	allRectList += svg.rightIedRectList;
	if (hasFilter)
	{
		for (int i = 0; i < allRectList.size(); ++i)
		{
			IedRect* rect = allRectList.at(i);
			if (!rect)
			{
				continue;
			}
			for (int j = 0; j < rect->logic_line_list.size(); ++j)
			{
				LogicCircuitLine* logic = rect->logic_line_list.at(j);
				if (!logic)
				{
					continue;
				}
				for (int k = 0; k < logic->virtual_line_list.size(); ++k)
				{
					VirtualCircuitLine* virtualLine = logic->virtual_line_list.at(k);
					if (!virtualLine)
					{
						continue;
					}
					VirtualCircuit* virtualCircuit = virtualLine->pVirtualCircuit;
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
					if (virtualCircuit->srcSoftPlateCode != 0)
					{
						visiblePlateCodeSet.insert(virtualCircuit->srcSoftPlateCode);
					}
					if (!virtualCircuit->srcSoftPlateRef.isEmpty())
					{
						visiblePlateRefSet.insert(virtualCircuit->srcSoftPlateRef);
					}
					if (virtualCircuit->destSoftPlateCode != 0)
					{
						visiblePlateCodeSet.insert(virtualCircuit->destSoftPlateCode);
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
		maintainPlateRectHash.insert(svg.mainIedRect->iedName, build_virtual_ied_outer_rect(svg.mainIedRect));
	}
	for (int i = 0; i < svg.centerIedRectList.size(); ++i)
	{
		IedRect* rect = svg.centerIedRectList.at(i);
		if (!rect)
		{
			continue;
		}
		IedItem* item = new IedItem();
		item->setFromIedRect(*rect, false);
		m_scene->addItem(item);
		maintainPlateRectHash.insert(rect->iedName, build_virtual_ied_outer_rect(rect));
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
		maintainPlateRectHash.insert(rect->iedName, build_virtual_ied_outer_rect(rect));
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
		maintainPlateRectHash.insert(rect->iedName, build_virtual_ied_outer_rect(rect));
	}
	QMap<QString, QRectF>::const_iterator maintainIt = maintainPlateRectHash.constBegin();
	for (; maintainIt != maintainPlateRectHash.constEnd(); ++maintainIt)
	{
		DirectMaintainPlateItem* maintainItem = new DirectMaintainPlateItem();
		maintainItem->setIedName(maintainIt.key());
		QString displayText = m_maintPlateTextByIed.contains(maintainIt.key()) ?
			m_maintPlateTextByIed.value(maintainIt.key()) :
			build_maint_plate_default_text();
		int stateValue = m_maintPlateStateByIed.contains(maintainIt.key()) ?
			m_maintPlateStateByIed.value(maintainIt.key()) :
			1;
		maintainItem->setDisplayText(displayText);
		maintainItem->setStateByValue(stateValue);
		maintainItem->setAnchorRect(maintainIt.value());
		maintainItem->setToolTip(displayText);
		maintainItem->setZValue(1.0);
		m_scene->addItem(maintainItem);
		m_maintPlateItems.insert(maintainIt.key(), maintainItem);
	}
	for (int i = 0; i < allRectList.size(); ++i)
	{
		IedRect* rect = allRectList.at(i);
		if (!rect)
		{
			continue;
		}
		if (hasFilter && !visibleIedNameSet.contains(rect->iedName))
		{
			continue;
		}
		for (int j = 0; j < rect->logic_line_list.size(); ++j)
		{
			LogicCircuitLine* logic = rect->logic_line_list.at(j);
			if (!logic)
			{
				continue;
			}
			for (int k = 0; k < logic->virtual_line_list.size(); ++k)
			{
				VirtualCircuitLine* virtualLine = logic->virtual_line_list.at(k);
				if (!virtualLine)
				{
					continue;
				}
				VirtualCircuit* virtualCircuit = virtualLine->pVirtualCircuit;
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
	// if (svg.plateRectHash.isEmpty())
	// {
	// 	qWarning() << "plateRectHash is empty";
	// }
	QHash<QString, PlateRect>::const_iterator plateIt = svg.plateRectHash.constBegin();
	for (; plateIt != svg.plateRectHash.constEnd(); ++plateIt)
	{
		if (hasFilter)
		{
			const PlateRect& plateRect = plateIt.value();
			if (plateRect.code != 0)
			{
				if (!visiblePlateCodeSet.contains(plateRect.code))
				{
					continue;
				}
			}
			else if (!visiblePlateRefSet.contains(plateRect.ref))
			{
				continue;
			}
		}
		DirectPlateItem* item = new DirectPlateItem();
		item->setFromPlateRect(plateIt.value(), plateIt.key());
		item->setClosed(true);
		item->setToolTip(build_plate_tooltip(item));
		item->setZValue(2.0);
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

void DirectWidget::ParseFromWholeSvg(const WholeCircuitSvg& svg)
{
	if (!m_scene)
	{
		return;
	}
	ClearScene();
	m_currentIedName = svg.mainIedRect ? svg.mainIedRect->iedName : (svg.centerIedRectList.isEmpty() ? QString() : svg.centerIedRectList.first()->iedName);
	m_scene->setSceneRect(0, 0, svg.viewBoxWidth, svg.viewBoxHeight);
	QMap<QString, QRectF> maintainPlateRectHash;
	QList<IedRect*> allRectList;
	allRectList += svg.centerIedRectList;
	allRectList += svg.leftIedRectList;
	allRectList += svg.rightIedRectList;
	if (svg.mainIedRect)
	{
		IedItem* item = new IedItem();
		item->setFromIedRect(*svg.mainIedRect, false);
		m_scene->addItem(item);
		maintainPlateRectHash.insert(svg.mainIedRect->iedName, build_virtual_ied_outer_rect(svg.mainIedRect));
	}
	for (int i = 0; i < svg.centerIedRectList.size(); ++i)
	{
		IedRect* rect = svg.centerIedRectList.at(i);
		if (!rect)
		{
			continue;
		}
		IedItem* item = new IedItem();
		item->setFromIedRect(*rect, false);
		m_scene->addItem(item);
		maintainPlateRectHash.insert(rect->iedName, build_virtual_ied_outer_rect(rect));
	}
	for (int i = 0; i < svg.leftIedRectList.size(); ++i)
	{
		IedRect* rect = svg.leftIedRectList.at(i);
		if (!rect)
		{
			continue;
		}
		IedItem* item = new IedItem();
		item->setFromIedRect(*rect, false);
		m_scene->addItem(item);
		maintainPlateRectHash.insert(rect->iedName, build_virtual_ied_outer_rect(rect));
	}
	for (int i = 0; i < svg.rightIedRectList.size(); ++i)
	{
		IedRect* rect = svg.rightIedRectList.at(i);
		if (!rect)
		{
			continue;
		}
		IedItem* item = new IedItem();
		item->setFromIedRect(*rect, false);
		m_scene->addItem(item);
		maintainPlateRectHash.insert(rect->iedName, build_virtual_ied_outer_rect(rect));
	}
	QMap<QString, QRectF>::const_iterator maintainIt = maintainPlateRectHash.constBegin();
	for (; maintainIt != maintainPlateRectHash.constEnd(); ++maintainIt)
	{
		DirectMaintainPlateItem* maintainItem = new DirectMaintainPlateItem();
		maintainItem->setIedName(maintainIt.key());
		QString displayText = m_maintPlateTextByIed.contains(maintainIt.key()) ?
			m_maintPlateTextByIed.value(maintainIt.key()) :
			build_maint_plate_default_text();
		int stateValue = m_maintPlateStateByIed.contains(maintainIt.key()) ?
			m_maintPlateStateByIed.value(maintainIt.key()) :
			1;
		maintainItem->setDisplayText(displayText);
		maintainItem->setStateByValue(stateValue);
		maintainItem->setAnchorRect(maintainIt.value());
		maintainItem->setToolTip(displayText);
		maintainItem->setZValue(1.0);
		m_scene->addItem(maintainItem);
		m_maintPlateItems.insert(maintainIt.key(), maintainItem);
	}
	QMap<quintptr, const WholeGroupDecor*> groupDecorMap;
	for (int i = 0; i < svg.groupDecorList.size(); ++i)
	{
		const WholeGroupDecor* groupDecor = svg.groupDecorList.at(i);
		if (!groupDecor || !groupDecor->pLogicLine)
		{
			continue;
		}
		groupDecorMap.insert((quintptr)groupDecor->pLogicLine, groupDecor);
		if (groupDecor->groupMode == WholeGroupMode_Fallback)
		{
			continue;
		}
		DirectWholeGroupItem* groupItem = new DirectWholeGroupItem();
		groupItem->setFromWholeGroupDecor(*groupDecor);
		groupItem->setLineColor(utils::ColorHelper::Color(utils::ColorHelper::pure_green));
		if (groupDecor->hasSwitchIcon && !groupDecor->switchIedName.isEmpty())
		{
			groupItem->setToolTip(groupDecor->switchIedName);
		}
		groupItem->setZValue(0.5);
		m_scene->addItem(groupItem);
	}
	for (int i = 0; i < allRectList.size(); ++i)
	{
		IedRect* rect = allRectList.at(i);
		if (!rect)
		{
			continue;
		}
		for (int j = 0; j < rect->logic_line_list.size(); ++j)
		{
			LogicCircuitLine* logic = rect->logic_line_list.at(j);
			if (!logic)
			{
				continue;
			}
			const WholeGroupDecor* groupDecor = groupDecorMap.value((quintptr)logic, NULL);
			for (int k = 0; k < logic->virtual_line_list.size(); ++k)
			{
				VirtualCircuitLine* virtualLine = logic->virtual_line_list.at(k);
				if (!virtualLine)
				{
					continue;
				}
				VirtualCircuit* virtualCircuit = virtualLine->pVirtualCircuit;
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
				lineItem->setLineStyle(Qt::SolidLine);
				lineItem->setValueVisible(true);
				if (groupDecor && groupDecor->groupMode != WholeGroupMode_Fallback)
				{
					QString leftText;
					QString rightText;
					QRectF leftRect;
					QRectF rightRect;
					lineItem->setArrowVisible(true);
					lineItem->setMiddleGap(groupDecor->gapStartX, groupDecor->gapEndX);
					whole_build_side_label_layout(virtualLine, groupDecor, leftText, leftRect, rightText, rightRect);
					lineItem->setSideLabels(leftText, leftRect, rightText, rightRect);
				}
				else
				{
					lineItem->setArrowVisible(true);
					lineItem->clearMiddleGap();
					lineItem->clearSideLabels();
				}
				lineItem->setToolTip(build_virtual_tooltip(virtualLine));
				lineItem->setZValue(1.0);
				m_scene->addItem(lineItem);
				m_virtualLines.append(lineItem);
			}
		}
	}
	QHash<QString, PlateRect>::const_iterator plateIt = svg.plateRectHash.constBegin();
	for (; plateIt != svg.plateRectHash.constEnd(); ++plateIt)
	{
		DirectPlateItem* item = new DirectPlateItem();
		item->setFromPlateRect(plateIt.value(), plateIt.key());
		item->setClosed(true);
		item->setToolTip(build_plate_tooltip(item));
		item->setZValue(2.0);
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
	if (!m_scene)
	{
		return;
	}
	ClearScene();
	m_currentIedName = svg.mainIedRect ? svg.mainIedRect->iedName : QString();
	m_scene->setSceneRect(0, 0, svg.viewBoxWidth, svg.viewBoxHeight);
	if (svg.mainIedRect)
	{
		IedItem* item = new IedItem();
		item->setFromIedRect(*svg.mainIedRect, false);
		m_scene->addItem(item);
	}
	for (int i = 0; i < svg.iedRectList.size(); ++i)
	{
		IedItem* item = new IedItem();
		item->setFromIedRect(*svg.iedRectList[i], false);
		m_scene->addItem(item);
	}
	for (int i = 0; i < svg.switcherRectList.size(); ++i)
	{
		IedItem* item = new IedItem();
		item->setFromIedRect(*svg.switcherRectList[i], true);
		m_scene->addItem(item);
	}
	for (int i = 0; i < svg.opticalCircuitLineList.size(); ++i)
	{
		OpticalCircuitLine* line = svg.opticalCircuitLineList[i];
		DirectOpticalLineItem* lineItem = new DirectOpticalLineItem();
		lineItem->setFromOpticalLine(*line);
		if (line && line->pOpticalCircuit)
		{
			lineItem->setData(0, QVariant((qulonglong)line->pOpticalCircuit->code));
			lineItem->setData(1, QVariant(line->pSrcRect ? line->pSrcRect->iedName : QString()));
			lineItem->setData(2, QVariant(line->pDestRect ? line->pDestRect->iedName : QString()));
		}
		m_scene->addItem(lineItem);
		if (line && line->pOpticalCircuit)
		{
			QString startIedName;
			QString startPeerIedName;
			QString endIedName;
			QString endPeerIedName;
			bool startIsSwitch = false;
			bool endIsSwitch = false;
			bool startAtTop = true;
			bool endAtTop = false;
			if (fill_optical_endpoint_info(line, true, startIedName, startPeerIedName, startIsSwitch, startAtTop))
			{
				DirectOpticalConnPointItem* startPointItem = new DirectOpticalConnPointItem();
				startPointItem->setConnPoint(line->startPoint, startAtTop, lineItem);
				startPointItem->setData(0, QVariant((qulonglong)line->pOpticalCircuit->code));
				startPointItem->setData(1, QVariant(startIedName));
				startPointItem->setData(2, QVariant(startPeerIedName));
				startPointItem->setZValue(lineItem->zValue() + 1.0);
				m_scene->addItem(startPointItem);
			}
			if (fill_optical_endpoint_info(line, false, endIedName, endPeerIedName, endIsSwitch, endAtTop))
			{
				DirectOpticalConnPointItem* endPointItem = new DirectOpticalConnPointItem();
				endPointItem->setConnPoint(line->endPoint, endAtTop, lineItem);
				endPointItem->setData(0, QVariant((qulonglong)line->pOpticalCircuit->code));
				endPointItem->setData(1, QVariant(endIedName));
				endPointItem->setData(2, QVariant(endPeerIedName));
				endPointItem->setZValue(lineItem->zValue() + 1.0);
				m_scene->addItem(endPointItem);
			}
		}
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

void DirectWidget::OnOpticalLineClicked(quint64 opticalCode, const QString& srcIedName, const QString& destIedName)
{
	if (m_currentIedName.isEmpty() || opticalCode == 0)
	{
		return;
	}
	DirectRelationWindow* relationWindow = new DirectRelationWindow(NULL);
	if (!relationWindow->LoadByOpticalCode(m_currentIedName, opticalCode, srcIedName, destIedName))
	{
		delete relationWindow;
		return;
	}
	relationWindow->show();
	relationWindow->raise();
	relationWindow->activateWindow();
}

void DirectWidget::OnOpticalConnPointClicked(quint64 opticalCode, const QString& iedName, const QString& peerIedName)
{
	Q_UNUSED(peerIedName);
	if (opticalCode == 0 || iedName.isEmpty() || iedName.contains("SW", Qt::CaseInsensitive))
	{
		return;
	}
	ControlBlockListWindow* controlBlockListWindow = new ControlBlockListWindow(NULL);
	if (!controlBlockListWindow->LoadByOpticalConnPoint(opticalCode, iedName))
	{
		delete controlBlockListWindow;
		return;
	}
	controlBlockListWindow->show();
	controlBlockListWindow->raise();
	controlBlockListWindow->activateWindow();
}

void DirectWidget::OnLogicLineClicked(LineItem* lineItem)
{
	if (!lineItem)
	{
		return;
	}
	const QSet<quint64>& circuitCodeSet = lineItem->relatedVirtualCodes();
	if (circuitCodeSet.isEmpty())
	{
		return;
	}
	QString srcIedName = lineItem->srcIedName();
	QString destIedName = lineItem->destIedName();
	QString preferredMainIedName = lineItem->preferredMainIedName();
	DirectRelationWindow* relationWindow = new DirectRelationWindow(NULL);
	if (!relationWindow->LoadByLogicDirection(srcIedName, destIedName, circuitCodeSet, preferredMainIedName))
	{
		delete relationWindow;
		return;
	}
	relationWindow->show();
	relationWindow->raise();
	relationWindow->activateWindow();
}
void DirectWidget::OnMaintainPlateToggled(const QString& iedName, int value)
{
	SetMaintainPlateState(iedName, value);
}


void DirectWidget::UpdatePlateStatuses()
{
	if (!m_rtdb.isOpen())
		m_rtdb.refresh();
	QMap<QString, DirectPlateItem*>::iterator it = m_plateItems.begin();
	for (; it != m_plateItems.end(); ++it)
	{
		DirectPlateItem* item = it.value();
		if (!item)
		{
			continue;
		}
		quint64 code = item->code();
		if (code == 0)
		{
			continue;
		}
		stuRtdbStatus* plateEle = m_rtdb.getRyb(code);
		if (plateEle)
		{
			int value = plateEle->val[0] != '\0' ? atoi(plateEle->val) : 0;
			item->setClosed(value != 0);
		}
	}
}
void DirectWidget::UpdateLineStatuses()
{
	if (!m_rtdb.isOpen())
		m_rtdb.refresh();
	for (int i = 0; i < m_virtualLines.size(); ++i)
	{
		DirectVirtualLineItem* line = m_virtualLines[i];
		if (!line)
		{
			continue;
		}
		quint64 code = line->circuitCode();
		if (code == 0)
		{
			continue;
		}
		if (line->virtualType() == GOOSE)
		{
			stuRtdbGseCircuit* gseCircuit = m_rtdb.getGseCircuit(code);
			if (gseCircuit)
			{
				stuRtdbStatus* inChl = static_cast<stuRtdbStatus*>(gseCircuit->m_pInChl);
				stuRtdbStatus* outChl = static_cast<stuRtdbStatus*>(gseCircuit->m_pOutChl);
				QString outVal = outChl ? QString::number(dw_to_double(outChl->val), 'f', 0) : QString();
				QString inVal  = inChl ? QString::number(dw_to_double(inChl->val), 'f', 0) : QString();
				line->setValues(outVal, inVal);
			}
		}
		else
		{
			stuRtdbSvCircuit* svCircuit = m_rtdb.getSvCircuit(code);
			if (svCircuit)
			{
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
	for (int i = 0; i < m_virtualLines.size(); ++i)
	{
		DirectVirtualLineItem* line = m_virtualLines[i];
		if (line) line->setBlinkOn(m_blinkOn);
	}
}
