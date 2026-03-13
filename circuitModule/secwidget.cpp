#include "secwidget.h"

#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsTextItem>
#include <QVBoxLayout>
#include <QFont>
#include "svgmodel.h"
#include "SvgUtils.h"
enum SecWidgetConfig
{
	SECWIDGET_WIDTH = 800,	// 二次回路窗口默认宽度
	SECWIDGET_HEIGHT = 600	// 二次回路窗口默认高度
};
SecWidget::SecWidget(QWidget *parent)
	: QWidget(parent)
	, m_view(new QGraphicsView(this))
	, m_scene(new QGraphicsScene(this))
{
	setWindowTitle(tr("IED回路"));
	//setAttribute(Qt::WA_DeleteOnClose, true);

	m_view->setScene(m_scene);
	m_view->setRenderHint(QPainter::Antialiasing);
	m_view->setViewportUpdateMode(QGraphicsView::BoundingRectViewportUpdate);
	m_view->setBackgroundBrush(Qt::black);
	m_view->setDragMode(QGraphicsView::NoDrag);

	QVBoxLayout* layout = new QVBoxLayout(this);
	layout->setContentsMargins(0, 0, 0, 0);
	layout->addWidget(m_view);

	resize(800, 600);
}

SecWidget::~SecWidget()
{
}

void SecWidget::displayCircuit(const QString& srcIed, const QString& destIed)
{
	if (!m_scene)
	{
		return;
	}

	m_scene->clear();
	const QString label = tr("IED回路: %1 - %2").arg(srcIed.isEmpty() ? tr("未知") : srcIed,
		destIed.isEmpty() ? tr("未知") : destIed);
	QGraphicsTextItem* textItem = m_scene->addText(label, QFont("Microsoft YaHei", 16, QFont::Bold));
	textItem->setDefaultTextColor(Qt::white);
	textItem->setPos(20, 20);

	QRectF itemRect = textItem->boundingRect().translated(textItem->pos()).adjusted(-20, -20, 20, 20);
	m_scene->setSceneRect(itemRect);
	setWindowTitle(label);
}

void SecWidget::displayCircuit(const MapLine& line)
{
	const MapLine::OpticalAttrs* optAttrs =
		line.attrs.dynamicCast<MapLine::OpticalAttrs>().data();
	if (!optAttrs)
	{
		IED* pSrcIed = CircuitConfig::Instance()->GetIedByName(optAttrs->srcIed);
		IED* pDestIed = CircuitConfig::Instance()->GetIedByName(optAttrs->destIed);
		IedRect* pSrcRect = utils::GetIedRect(pSrcIed->name, pSrcIed->desc, 
			(SECWIDGET_WIDTH - RECT_DEFAULT_WIDTH) / 2, 
			(SECWIDGET_HEIGHT - RECT_DEFAULT_HEIGHT) / 2,
			RECT_DEFAULT_WIDTH, RECT_DEFAULT_HEIGHT,
			utils::ColorHelper::ied_border,
			utils::ColorHelper::ied_underground);
	}
}
