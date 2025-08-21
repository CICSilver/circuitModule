#include "mainwindow.h"
#include "customsvgitem.h"
#include "InteractiveSvgItem.h"
#include <QSvgRenderer>
#include <QScrollBar>
#include <QWheelEvent>
MainWindow::MainWindow(QWidget *parent)
    : m_circuitConfig(CircuitConfig::Instance()),
      m_iedListModel(new QStringListModel(this)),
      m_logicSvgItem(new CustomSvgItem()), m_opticalSvgItem(new CustomSvgItem()), m_wholeSvgItem(new CustomSvgItem()),
      QWidget(parent)
{
    ui.setupUi(this);
    setObjectName("MainWindow");

    InitList();
    InitView();
    this->resize(1920, 1080);

    // Install event filter on the VIEWPORTS (wheel events go to viewport in Qt4)
    ui.logicView->viewport()->installEventFilter(this);
    ui.opticalView->viewport()->installEventFilter(this);
    ui.wholeView->viewport()->installEventFilter(this);

    // Ensure the views accept wheel focus so the viewport receives wheel events
    ui.logicView->setFocusPolicy(Qt::WheelFocus);
    ui.opticalView->setFocusPolicy(Qt::WheelFocus);
    ui.wholeView->setFocusPolicy(Qt::WheelFocus);

    connect(ui.iedList, SIGNAL(clicked(const QModelIndex&)), this, SLOT(onIedListItemClicked(const QModelIndex&)));
}

MainWindow::~MainWindow()
{
    if (m_logicSvgItem)
    {
        delete m_logicSvgItem;
        m_logicSvgItem = NULL;
    }
    if (m_opticalSvgItem)
    {
        delete m_opticalSvgItem;
        m_opticalSvgItem;
    }
    if (m_wholeSvgItem)
    {
        delete m_wholeSvgItem;
        m_wholeSvgItem = NULL;
    }


}

void MainWindow::InitView()
{
	QModelIndex index = m_iedListModel->index(0);
	QString iedName = index.data().toString();
	// 初始化三个标签页均为交互式视图
	InitInteractiveView(ui.logicView,   logicSvgPath(iedName));
	InitInteractiveView(ui.opticalView, opticalSvgPath(iedName));
	InitInteractiveView(ui.wholeView,   virtualSvgPath(iedName));
}

bool MainWindow::eventFilter(QObject* watched, QEvent* event)
{
	// 在 Qt4 中，滚轮事件发送到 QGraphicsView 的 viewport（QWidget），
	// 因此这里要从 watched 的父级获取 QGraphicsView
	if (event->type() == QEvent::Wheel) {
		//QGraphicsView* view = qobject_cast<QGraphicsView*>(watched);
		//if (!view) {
		//	QWidget* vw = qobject_cast<QWidget*>(watched);
		//	if (vw) view = qobject_cast<QGraphicsView*>(vw->parent());
		//}
		//if (view) {
		//	QWheelEvent* we = static_cast<QWheelEvent*>(event);
		//	const double factor = (we->delta() > 0) ? 1.15 : (1.0 / 1.15);
		//	const double minS = 1.0;
		//	const double maxS = 2.0;

		//	// 当前缩放
		//	const QTransform t = view->transform();
		//	const double sx = t.m11();
		//	const double sy = t.m22();
		//	const double tx = sx * factor;
		//	const double ty = sy * factor;

		//	// 如果缩放将超限，则直接拦截
		//	if ((we->delta() > 0 && (tx > maxS || ty > maxS)) ||
		//		(we->delta() <= 0 && (tx < minS || ty < minS))) {
		//		return true;
		//	}

		//	view->setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
		//	view->scale(factor, factor);
		//	view->setTransformationAnchor(QGraphicsView::AnchorViewCenter);
			return false;
		//}
	}
	return QWidget::eventFilter(watched, event);
}

void MainWindow::InitView(QGraphicsView* view, CustomSvgItem* item, QString svgPath)
{
	item->setSharedRenderer(new QSvgRenderer(svgPath));
	QGraphicsScene* scene = new QGraphicsScene(this);
	scene->setSceneRect(0, 0, 2160, 1440);
	scene->addItem(item);

	view->setScene(scene);
	view->setScene(scene);
	view->setRenderHint(QPainter::Antialiasing);
	view->setViewportUpdateMode(QGraphicsView::BoundingRectViewportUpdate);
	view->setBackgroundBrush(Qt::black);
	view->resize(1920, 1080);
	view->setDragMode(QGraphicsView::ScrollHandDrag);
	view->scene()->setItemIndexMethod(QGraphicsScene::NoIndex);
}

void MainWindow::InitList()
{
	ui.iedList->setEditTriggers(QAbstractItemView::NoEditTriggers);
	QList<IED*> ied_list = m_circuitConfig->GetIedList();
	QStringList iedNameList;
	foreach(IED * pIed, ied_list)
	{
		iedNameList << pIed->name;
	}
	m_iedListModel->setStringList(iedNameList);
	ui.iedList->setModel(m_iedListModel);
}

void MainWindow::InitInteractiveView(QGraphicsView* view, const QString& svgPath)
{
	if (!view) return;
	if (view->scene()) {
		delete view->scene();
	}
	QGraphicsScene* scene = new QGraphicsScene(view);
	InteractiveSvgMapItem* item = new InteractiveSvgMapItem(svgPath);
	scene->addItem(item);
	scene->setSceneRect(item->boundingRect());
	view->setScene(scene);
	view->setRenderHint(QPainter::Antialiasing);
	view->setViewportUpdateMode(QGraphicsView::BoundingRectViewportUpdate);
	view->setBackgroundBrush(Qt::black);
	view->setDragMode(QGraphicsView::NoDrag); // 由 Item 自己通过滚动条实现平移
	view->resetTransform(); // 初始缩放为 1.0，配合 InteractiveSvgMapItem 的 min/max 限制
	if (view->verticalScrollBar()) view->verticalScrollBar()->setValue(0);
	if (view->horizontalScrollBar()) view->horizontalScrollBar()->setValue(0);
}
void MainWindow::onIedListItemClicked(const QModelIndex& index)
{
	if (index.isValid())
	{
		QString iedName = index.data().toString();
		IED* pIed = m_circuitConfig->GetIedByName(iedName);
		//qDebug() << "iedName:" << iedName;
		QString logicPath = QCoreApplication::applicationDirPath() + "/logic";
		QString opticalPath = QCoreApplication::applicationDirPath() + "/optical";
		//QString wholePath = QCoreApplication::applicationDirPath() + "/whole";

	// 同时刷新三个视图（逻辑/光纤/虚回路）。Tab 控制显示哪个
	InitInteractiveView(ui.logicView,   logicSvgPath(iedName));
	InitInteractiveView(ui.opticalView, opticalSvgPath(iedName));
	InitInteractiveView(ui.wholeView,   virtualSvgPath(iedName));
	}
}
