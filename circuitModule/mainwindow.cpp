#include "mainwindow.h"
#include "InteractiveSvgItem.h"
#include "SvgTransformer.h"
#include <QSvgRenderer>
#include <QScrollBar>
#include <QWheelEvent>
MainWindow::MainWindow(QWidget *parent)
		: m_circuitConfig(CircuitConfig::Instance()),
			m_iedListModel(new QStringListModel(this)),
			m_useSvgBytes(false),
			QWidget(parent)
{
    ui.setupUi(this);
    setObjectName("MainWindow");

    InitList();
    InitView();
    this->resize(1920, 1080);

    ui.logicView->viewport()->installEventFilter(this);
    ui.opticalView->viewport()->installEventFilter(this);
    ui.wholeView->viewport()->installEventFilter(this);

    ui.logicView->setFocusPolicy(Qt::WheelFocus);
    ui.opticalView->setFocusPolicy(Qt::WheelFocus);
    ui.wholeView->setFocusPolicy(Qt::WheelFocus);

    connect(ui.iedList, SIGNAL(clicked(const QModelIndex&)), this, SLOT(onIedListItemClicked(const QModelIndex&)));
}

MainWindow::MainWindow(bool useSvgBytes, QWidget* parent)
	: m_circuitConfig(CircuitConfig::Instance()),
	  m_iedListModel(new QStringListModel(this)),
	  QWidget(parent), m_useSvgBytes(useSvgBytes)
{
	ui.setupUi(this);
	setObjectName("MainWindow");

	InitList();
	InitView();
	this->resize(1920, 1080);

	ui.logicView->viewport()->installEventFilter(this);
	ui.opticalView->viewport()->installEventFilter(this);
	ui.wholeView->viewport()->installEventFilter(this);

	ui.logicView->setFocusPolicy(Qt::WheelFocus);
	ui.opticalView->setFocusPolicy(Qt::WheelFocus);
	ui.wholeView->setFocusPolicy(Qt::WheelFocus);

	connect(ui.iedList, SIGNAL(clicked(const QModelIndex&)), this, SLOT(onIedListItemClicked(const QModelIndex&)));
}

MainWindow::~MainWindow()
{}

void MainWindow::InitView()
{
	QModelIndex index = m_iedListModel->index(0);
	QString iedName = index.data().toString();
	if (!m_useSvgBytes) {
		// 文件路径方式
		InitInteractiveView(ui.logicView,   logicSvgPath(iedName));
		InitInteractiveView(ui.opticalView, opticalSvgPath(iedName));
		InitInteractiveView(ui.wholeView,   virtualSvgPath(iedName));
	} else {
		// 内存字节方式
		SvgTransformer transformer;
		InitInteractiveView(ui.logicView,   transformer.GenerateLogicSvgBytes(iedName));
		InitInteractiveView(ui.opticalView, transformer.GenerateOpticalSvgBytes(iedName));
		InitInteractiveView(ui.wholeView,   transformer.GenerateVirtualSvgBytes(iedName));
	}
}

bool MainWindow::eventFilter(QObject* watched, QEvent* event)
{
	if (event->type() == QEvent::Wheel) {
			return false;
		//}
	}
	return QWidget::eventFilter(watched, event);
}

void MainWindow::InitList()
{
	ui.iedList->setEditTriggers(QAbstractItemView::NoEditTriggers);
	QList<IED*> ied_list = m_circuitConfig->GetIedList();
	QStringList iedNameList;
	foreach(IED * pIed, ied_list)
	{
		if(pIed->name.contains("SW"))
		{
			// 交换机设备不显示在列表中
			continue;
		}
		iedNameList << pIed->name;
	}
	m_iedListModel->setStringList(iedNameList);
	ui.iedList->setModel(m_iedListModel);
}

void MainWindow::InitInteractiveView(QGraphicsView* view, const QString& svgPath)
{
	if (!view) return;
	InteractiveSvgMapItem* item = new InteractiveSvgMapItem(svgPath);
	SetupInteractiveView(view, item);
}
void MainWindow::InitInteractiveView(QGraphicsView* view, const QByteArray& svgBytes)
{
	if (!view) return;
	InteractiveSvgMapItem* item = new InteractiveSvgMapItem(svgBytes);
	SetupInteractiveView(view, item);
}

void MainWindow::SetupInteractiveView(QGraphicsView* view, InteractiveSvgMapItem* item)
{
	if (!view || !item) return;
	QGraphicsScene* old = view->scene();
	if (old) {
		view->setScene(NULL);
		old->deleteLater();
	}

	QGraphicsScene* scene = new QGraphicsScene(view);
	scene->addItem(item);
	scene->setSceneRect(item->boundingRect());
	view->setScene(scene);
	view->setRenderHint(QPainter::Antialiasing);
	view->setViewportUpdateMode(QGraphicsView::BoundingRectViewportUpdate);
	view->setBackgroundBrush(Qt::black);
	view->setDragMode(QGraphicsView::NoDrag); // 由 Item 自己通过滚动条实现平移
	view->resetTransform(); // 初始缩放为 1.0
	if (view->verticalScrollBar()) view->verticalScrollBar()->setValue(0);
	if (view->horizontalScrollBar()) view->horizontalScrollBar()->setValue(0);
}
void MainWindow::onIedListItemClicked(const QModelIndex& index)
{
	if (index.isValid())
	{
		QString iedName = index.data().toString();
		if (!m_useSvgBytes) {
			// 走文件路径
			InitInteractiveView(ui.logicView,   logicSvgPath(iedName));
			InitInteractiveView(ui.opticalView, opticalSvgPath(iedName));
			InitInteractiveView(ui.wholeView,   virtualSvgPath(iedName));
		} else {
			// 走内存字节
			SvgTransformer transformer;
			InitInteractiveView(ui.logicView,   transformer.GenerateLogicSvgBytes(iedName));
			InitInteractiveView(ui.opticalView, transformer.GenerateOpticalSvgBytes(iedName));
			InitInteractiveView(ui.wholeView,   transformer.GenerateVirtualSvgBytes(iedName));
		}
	}
}
