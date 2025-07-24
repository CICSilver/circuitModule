#include "mainwindow.h"
#include "customsvgitem.h"
#include <QSvgRenderer>
MainWindow::MainWindow(QWidget *parent)
	: m_circuitConfig(CircuitConfig::Instance()), 
	m_iedListModel(new QStringListModel(this)), 
	m_logicSvgItem(new CustomSvgItem()), m_opticalSvgItem(new CustomSvgItem()), m_wholeSvgItem(new CustomSvgItem()),
	QWidget(parent)
{
	ui.setupUi(this);

	// 初始化列表
	InitList();
	// 初始化视图
	InitView();
	this->resize(1920, 1080);
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
	InitView(ui.logicView, m_logicSvgItem, logicSvgPath(iedName));
	InitView(ui.opticalView, m_opticalSvgItem, opticalSvgPath(iedName));
	InitView(ui.wholeView, m_wholeSvgItem, wholeSvgPath(iedName));
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

		// 初始化SVG
		m_logicSvgItem->LoadSvg(logicSvgPath(iedName));
		m_opticalSvgItem->LoadSvg(opticalSvgPath(iedName));
		//m_wholeSvgItem->LoadSvg(wholePath + "/" + pIed->name + "_whole_circuit.svg");

		// 强制刷新视图
		ui.logicView->viewport()->update();
		ui.opticalView->viewport()->update();
	}
}
