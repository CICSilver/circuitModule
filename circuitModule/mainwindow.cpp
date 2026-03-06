#include "mainwindow.h"
#include "InteractiveSvgItem.h"
#include "SvgTransformer.h"
#include <QSvgRenderer>
#include <QScrollBar>
#include <QWheelEvent>
#include <QCoreApplication>
#include <QDir>
#include <QDebug>
#include <QStringList>
#include <QStandardItem>
#include <QMessageBox>
#include <QVariant>
#include <list>
CircuitModuleWidget::CircuitModuleWidget(QWidget *parent)
	: QWidget(parent)
	, m_circuitConfig(CircuitConfig::Instance())
	, m_iedListModel(new QStringListModel(this))
	, m_stationModel(new QStandardItemModel(this))
	, m_useSvgBytes(false)
{
	InitializeWindow();
}

CircuitModuleWidget::CircuitModuleWidget(bool useSvgBytes, QWidget* parent)
	: QWidget(parent)
	, m_circuitConfig(CircuitConfig::Instance())
	, m_iedListModel(new QStringListModel(this))
	, m_stationModel(new QStandardItemModel(this))
	, m_useSvgBytes(useSvgBytes)
{
	InitializeWindow();
}

CircuitModuleWidget::~CircuitModuleWidget()
{}

void CircuitModuleWidget::InitializeWindow()
{
	ui.setupUi(this);
	setObjectName("MainWindow");
	UpdateConnectionData();

	this->resize(1920, 1080);

	ui.logicView->viewport()->installEventFilter(this);
	ui.opticalView->viewport()->installEventFilter(this);
	ui.wholeView->viewport()->installEventFilter(this);

	ui.logicView->setFocusPolicy(Qt::WheelFocus);
	ui.opticalView->setFocusPolicy(Qt::WheelFocus);
	ui.wholeView->setFocusPolicy(Qt::WheelFocus);

	connect(ui.stationTree, SIGNAL(clicked(const QModelIndex&)), this, SLOT(onStationTreeItemClicked(const QModelIndex&)));
}

void CircuitModuleWidget::clear()
{
	ui.stationTree->setModel(NULL);
	m_stationModel->clear();
	m_svgByteCache.clear();
	ui.logicView->setScene(NULL);
	ui.opticalView->setScene(NULL);
	ui.wholeView->setScene(NULL);
}

bool CircuitModuleWidget::UpdateConnectionData()
{
	clear();
	m_iedListModel->setStringList(QStringList());
	ui.stationTree->setModel(m_stationModel);

#ifdef _BUILD_IN_EXE
	// 仅在打包成单文件时使用
	// 打包为动态库时，实时库加载由接口部分实现
	m_circuitConfig->Clear();
	if (!m_circuitConfig->LoadRTDB())
	{
		QMessageBox::warning(this, QString::fromLocal8Bit("提示"), QString::fromLocal8Bit("实时库加载失败"));
		return false;
	}
#endif

	InitTree();
	if (m_iedListModel->rowCount() == 0)
	{
		return true;
	}

	InitSvg();
	InitView();
	return true;
}

bool CircuitModuleWidget::Refresh()
{
	if (!m_circuitConfig->LoadRTDB())
	{
		QMessageBox::warning(this, QString::fromLocal8Bit("提示"), QString::fromLocal8Bit("实时库刷新失败"));
		return false;
	}
	return true;
}

void* CircuitModuleWidget::operator new(size_t size)
{
	return ::operator new(size);
}

void CircuitModuleWidget::operator delete(void* p) throw()
{
	::operator delete(p);
}

void CircuitModuleWidget::InitView()
{
	QModelIndex index = m_iedListModel->index(0);
	if (!index.isValid())
	{
		return;
	}
	QString iedName = index.data().toString();
	if (iedName.isEmpty())
	{
		return;
	}
	DisplayIed(iedName);
}

void CircuitModuleWidget::InitSvg()
{
	if (!m_useSvgBytes)
	{
		return;
	}

	m_svgByteCache.clear();

	SvgTransformer transformer;
	const QList<IED*> iedList = m_circuitConfig->GetIedList();
	foreach(IED* pIed ,iedList)
	{
		if (!pIed)
		{
			continue;
		}

		const QString iedName = pIed->name.trimmed();
		if (iedName.isEmpty())
		{
			continue;
		}

		if (iedName.contains("SW", Qt::CaseInsensitive))
		{
			continue;
		}

		if (m_svgByteCache.contains(iedName))
		{
			continue;
		}

		SvgByteData data;
		data.logic = transformer.GenerateLogicSvgBytes(iedName);
		data.optical = transformer.GenerateOpticalSvgBytes(iedName);
		data.virtualCircuit = transformer.GenerateVirtualSvgBytes(iedName);
		m_svgByteCache.insert(iedName, data);
	}
}


bool CircuitModuleWidget::eventFilter(QObject* watched, QEvent* event)
{
	if (event->type() == QEvent::Wheel) {
			return false;
		//}
	}
	return QWidget::eventFilter(watched, event);
}

void CircuitModuleWidget::InitTree()
{
	ui.stationTree->setEditTriggers(QAbstractItemView::NoEditTriggers);
	m_stationModel->clear();
	m_stationModel->setHorizontalHeaderLabels(QStringList() << QString::fromLocal8Bit("变电站"));
	ui.stationTree->setModel(m_stationModel);

	const CRtdbEleModelStation* station = m_circuitConfig->StationModel();
	if (!station)
	{
		QMessageBox::warning(this, QString::fromLocal8Bit("提示"), QString::fromLocal8Bit("未找到变电站模型"));
		return;
	}

	QString stationName = QString::fromLocal8Bit("变电站");
	if (station->m_pStationInfo && station->m_pStationInfo->stationDesc[0] != '\0')
	{
		stationName = QString::fromLocal8Bit(station->m_pStationInfo->stationDesc);
	}

	QStandardItem* stationItem = new QStandardItem(stationName);
	stationItem->setEditable(false);
	m_stationModel->appendRow(stationItem);

	QStringList iedNameList;

	for (std::list<CRtdbEleModelVoltLevel*>::const_iterator voltIt = station->m_listVoltLevel.begin();
		 voltIt != station->m_listVoltLevel.end(); ++voltIt)
	{
		const CRtdbEleModelVoltLevel* volt = *voltIt;
		if (!volt)
		{
			continue;
		}

		QString voltText = QString::fromLocal8Bit("电压等级");
		if (volt->m_pVoltInfo)
		{
			if (volt->m_pVoltInfo->levelDesc[0] != '\0')
			{
				voltText = QString::fromLocal8Bit(volt->m_pVoltInfo->levelDesc);
			}
			else if (volt->m_pVoltInfo->levelName[0] != '\0')
			{
				voltText = QString::fromLocal8Bit(volt->m_pVoltInfo->levelName);
			}
			else if (volt->m_pVoltInfo->volt != 0)
			{
				voltText = QString::number(volt->m_pVoltInfo->volt) + QString::fromLocal8Bit("kV");
			}
		}

		QStandardItem* voltItem = new QStandardItem(voltText);
		voltItem->setEditable(false);
		stationItem->appendRow(voltItem);

		for (std::list<CRtdbEleModelBay*>::const_iterator bayIt = volt->m_listBay.begin();
			 bayIt != volt->m_listBay.end(); ++bayIt)
		{
			const CRtdbEleModelBay* bay = *bayIt;
			if (!bay)
			{
				continue;
			}

			QString bayText = QString::fromLocal8Bit("间隔");
			if (bay->m_pBayInfo)
			{
				if (bay->m_pBayInfo->bayDesc[0] != '\0')
				{
					bayText = QString::fromLocal8Bit(bay->m_pBayInfo->bayDesc);
				}
				else if (bay->m_pBayInfo->bayName[0] != '\0')
				{
					bayText = QString::fromLocal8Bit(bay->m_pBayInfo->bayName);
				}
			}

			QStandardItem* bayItem = new QStandardItem(bayText);
			bayItem->setEditable(false);
			voltItem->appendRow(bayItem);

			for (std::list<CRtdbEleModelIed*>::const_iterator iedIt = bay->m_listIed.begin();
				 iedIt != bay->m_listIed.end(); ++iedIt)
			{
				const CRtdbEleModelIed* ied = *iedIt;
				if (!ied)
				{
					continue;
				}

				QString iedName;
				if (ied->m_pIedHead)
				{
					if (ied->m_pIedHead->iedName[0] != '\0')
					{
						iedName = QString::fromLocal8Bit(ied->m_pIedHead->iedName);
					}
					else if (ied->m_pIedHead->iedDesc[0] != '\0')
					{
						iedName = QString::fromLocal8Bit(ied->m_pIedHead->iedDesc);
					}
				}

				if (iedName.isEmpty())
				{
					continue;
				}

				QStandardItem* iedItem = new QStandardItem(iedName);
				iedItem->setEditable(false);
				iedItem->setData(iedName, Qt::UserRole);
				bayItem->appendRow(iedItem);
				iedNameList << iedName;
			}
		}
	}

	iedNameList.removeDuplicates();
	iedNameList.sort();
	m_iedListModel->setStringList(iedNameList);
	ui.stationTree->expandAll();
}

void CircuitModuleWidget::onStationTreeItemClicked(const QModelIndex& index)
{
	if (!index.isValid())
	{
		return;
	}

	QVariant data = index.data(Qt::UserRole);
	if (!data.isValid())
	{
		return;
	}

	const QString iedName = data.toString();
	DisplayIed(iedName);
}

void CircuitModuleWidget::DisplayIed(const QString& iedName)
{
	const QString key = iedName.trimmed();
	if (key.isEmpty())
	{
		return;
	}

	if (!m_useSvgBytes)
	{
		InitInteractiveView(ui.logicView,   logicSvgPath(key));
		InitInteractiveView(ui.opticalView, opticalSvgPath(key));
		InitInteractiveView(ui.wholeView,   virtualSvgPath(key));
		return;
	}

	SvgByteData data;
	const QHash<QString, SvgByteData>::const_iterator cacheIt = m_svgByteCache.constFind(key);
	if (cacheIt != m_svgByteCache.constEnd())
	{
		data = *cacheIt;
	}
	else
	{
		SvgTransformer transformer;
		data.logic = transformer.GenerateLogicSvgBytes(key);
		data.optical = transformer.GenerateOpticalSvgBytes(key);
		data.virtualCircuit = transformer.GenerateVirtualSvgBytes(key);
		m_svgByteCache.insert(key, data);
	}

	if (!data.logic.isEmpty())
	{
		InitInteractiveView(ui.logicView, data.logic);
	}
	else
	{
		clearView(ui.logicView);
	}

	if (!data.optical.isEmpty())
	{
		InitInteractiveView(ui.opticalView, data.optical);
	}
	else
	{
		clearView(ui.opticalView);
	}

	if (!data.virtualCircuit.isEmpty())
	{
		InitInteractiveView(ui.wholeView, data.virtualCircuit);
	}
	else
	{
		clearView(ui.wholeView);
	}
}

void CircuitModuleWidget::clearView(QGraphicsView* view)
{
	if (!view)
	{
		return;
	}
	QGraphicsScene* scene = view->scene();
	if (scene)
	{
		view->setScene(NULL);
		scene->deleteLater();
	}
}


void CircuitModuleWidget::InitInteractiveView(QGraphicsView* view, const QString& svgPath)
{
	if (!view) return;
	InteractiveSvgMapItem* item = new InteractiveSvgMapItem(svgPath);
	SetupInteractiveView(view, item);
}
void CircuitModuleWidget::InitInteractiveView(QGraphicsView* view, const QByteArray& svgBytes)
{
	if (!view) return;
	InteractiveSvgMapItem* item = new InteractiveSvgMapItem(svgBytes);
	SetupInteractiveView(view, item);
}

bool CircuitModuleWidget::InitializeCircuitData(const QString& cimeDirectory)
{
	QString targetPath = cimeDirectory.trimmed();
	if (targetPath.isEmpty())
	{
		if (!m_circuitConfig->GetIedList().isEmpty())
		{
			return true;
		}
		targetPath = m_circuitConfig->CimeDirectory();
	}
	if (targetPath.isEmpty())
	{
		targetPath = QCoreApplication::applicationDirPath();
	}
	targetPath = QDir::fromNativeSeparators(targetPath);
	m_circuitConfig->Clear();
	return m_circuitConfig->LoadCimeFile(targetPath);
}

void CircuitModuleWidget::SetupInteractiveView(QGraphicsView* view, InteractiveSvgMapItem* item)
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


