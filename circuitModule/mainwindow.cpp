#include "mainwindow.h"
#include "CircuitDiagramProxy.h"
#include "directwidget.h"
#include <QCoreApplication>
#include <QDir>
#include <QDebug>
#include <QStringList>
#include <QStandardItem>
#include <QMessageBox>
#include <QVariant>
#include <QLayout>

namespace
{
	enum TreeNodeType
	{
		TreeNodeType_Unknown = 0,
		TreeNodeType_Station,
		TreeNodeType_Bay,
		TreeNodeType_Ied
	};

	const int TREE_NODE_TYPE_ROLE = Qt::UserRole + 1;
}

static DirectWidget* CreateDirectWidget(QWidget* containerWidget, QWidget* placeholderWidget, const char* objectName)
{
	if (!containerWidget)
	{
		return NULL;
	}

	QLayout* layout = containerWidget->layout();
	DirectWidget* directWidget = new DirectWidget(containerWidget);
	directWidget->setObjectName(objectName);

	if (placeholderWidget)
	{
		if (layout)
		{
			layout->removeWidget(placeholderWidget);
		}
		placeholderWidget->hide();
	}

	if (layout)
	{
		layout->addWidget(directWidget);
	}
	else
	{
		directWidget->setGeometry(containerWidget->rect());
	}
	return directWidget;
}

static void ReleaseDirectWidget(QWidget* containerWidget, DirectWidget*& directWidget)
{
	if (!directWidget)
	{
		return;
	}

	QLayout* layout = containerWidget ? containerWidget->layout() : NULL;
	if (layout)
	{
		layout->removeWidget(directWidget);
	}
	delete directWidget;
	directWidget = NULL;
}

CircuitModuleWidget::CircuitModuleWidget(QWidget *parent)
	: QWidget(parent)
	, m_circuitConfig(CircuitConfig::Instance())
	, m_iedListModel(new QStringListModel(this))
	, m_stationModel(new QStandardItemModel(this))
	, m_useSvgBytes(false)
	, m_directLogicWidget(NULL)
	, m_directOpticalWidget(NULL)
	, m_directVirtualWidget(NULL)
{
	InitializeWindow();
}

CircuitModuleWidget::CircuitModuleWidget(bool useSvgBytes, QWidget* parent)
	: QWidget(parent)
	, m_circuitConfig(CircuitConfig::Instance())
	, m_iedListModel(new QStringListModel(this))
	, m_stationModel(new QStandardItemModel(this))
	, m_useSvgBytes(useSvgBytes)
	, m_directLogicWidget(NULL)
	, m_directOpticalWidget(NULL)
	, m_directVirtualWidget(NULL)
{
	InitializeWindow();
}

CircuitModuleWidget::~CircuitModuleWidget()
{}

void CircuitModuleWidget::InitializeWindow()
{
	ui.setupUi(this);
	setObjectName("MainWindow");
	InitializeDirectWidgets();
	UpdateConnectionData();

	this->resize(1920, 1080);

	connect(ui.stationTree, SIGNAL(clicked(const QModelIndex&)), this, SLOT(onStationTreeItemClicked(const QModelIndex&)));
}

void CircuitModuleWidget::clear()
{
	ui.stationTree->setModel(NULL);
	m_stationModel->clear();
	ResetDirectWidgets();
}

bool CircuitModuleWidget::UpdateConnectionData()
{
	clear();
	m_iedListModel->setStringList(QStringList());
	ui.stationTree->setModel(m_stationModel);

#ifdef _BUILD_IN_EXE
	m_circuitConfig->Clear();
	if (!m_circuitConfig->LoadRTDB())
	{
		QMessageBox::warning(this, QString("Warning"), QString("RTDB load failed"));
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
		QMessageBox::warning(this, QString("Warning"), QString("RTDB refresh failed"));
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
}

bool CircuitModuleWidget::eventFilter(QObject* watched, QEvent* event)
{
	Q_UNUSED(watched);
	if (event->type() == QEvent::Wheel)
	{
		return false;
	}
	return QWidget::eventFilter(watched, event);
}

void CircuitModuleWidget::InitTree()
{
	ui.stationTree->setEditTriggers(QAbstractItemView::NoEditTriggers);
	m_stationModel->clear();
	m_stationModel->setHorizontalHeaderLabels(QStringList() << QString("Station"));
	ui.stationTree->setModel(m_stationModel);

	const CRtdbEleModelStation* station = m_circuitConfig->StationModel();
	if (!station)
	{
		QMessageBox::warning(this, QString("Warning"), QString("Station model not found"));
		return;
	}

	QString stationName = QString("Station");
	if (station->m_pStationInfo && station->m_pStationInfo->stationDesc[0] != '\0')
	{
		stationName = QString::fromLocal8Bit(station->m_pStationInfo->stationDesc);
	}

	QStandardItem* stationItem = new QStandardItem(stationName);
	stationItem->setEditable(false);
	stationItem->setData(stationName, Qt::UserRole);
	stationItem->setData(TreeNodeType_Station, TREE_NODE_TYPE_ROLE);
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

		QString voltText = QString("Level");
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
				voltText = QString::number(volt->m_pVoltInfo->volt) + QString("kV");
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

			QString bayText = QString("Bay");
			QString bayName = bayText;
			if (bay->m_pBayInfo)
			{
				if (bay->m_pBayInfo->bayName[0] != '\0')
				{
					bayName = QString::fromLocal8Bit(bay->m_pBayInfo->bayName);
				}
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
			bayItem->setData(bayName, Qt::UserRole);
			bayItem->setData(TreeNodeType_Bay, TREE_NODE_TYPE_ROLE);
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
				iedItem->setData(TreeNodeType_Ied, TREE_NODE_TYPE_ROLE);
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

	const int nodeType = index.data(TREE_NODE_TYPE_ROLE).toInt();
	const QString nodeKey = data.toString();
	if (nodeType == TreeNodeType_Station)
	{
		DisplayStationOptical();
		ReleaseDirectWidget(ui.tab, m_directLogicWidget);
		m_directLogicWidget = CreateDirectWidget(ui.tab, ui.logicView, "directLogicWidget");
		ReleaseDirectWidget(ui.tab_3, m_directVirtualWidget);
		m_directVirtualWidget = CreateDirectWidget(ui.tab_3, ui.wholeView, "directVirtualWidget");
		ui.tabWidget->setCurrentWidget(ui.tab_2);
		return;
	}
	if (nodeType == TreeNodeType_Bay)
	{
		DisplayBayLogic(nodeKey);
		DisplayBayOptical(nodeKey);
		ReleaseDirectWidget(ui.tab_3, m_directVirtualWidget);
		m_directVirtualWidget = CreateDirectWidget(ui.tab_3, ui.wholeView, "directVirtualWidget");
		ui.tabWidget->setCurrentWidget(ui.tab_2);
		return;
	}
	DisplayIed(nodeKey);
}

void CircuitModuleWidget::DisplayIed(const QString& iedName)
{
	const QString key = iedName.trimmed();
	if (key.isEmpty())
	{
		return;
	}

	DisplayLogicIed(key);
	DisplayOpticalIed(key);
	DisplayVirtualIed(key);
}

void CircuitModuleWidget::InitializeDirectWidgets()
{
	if (!m_directLogicWidget)
	{
		m_directLogicWidget = CreateDirectWidget(ui.tab, ui.logicView, "directLogicWidget");
	}
	if (!m_directOpticalWidget)
	{
		m_directOpticalWidget = CreateDirectWidget(ui.tab_2, ui.opticalView, "directOpticalWidget");
	}
	if (!m_directVirtualWidget)
	{
		m_directVirtualWidget = CreateDirectWidget(ui.tab_3, ui.wholeView, "directVirtualWidget");
	}
}

void CircuitModuleWidget::ResetDirectWidgets()
{
	ReleaseDirectWidget(ui.tab, m_directLogicWidget);
	ReleaseDirectWidget(ui.tab_2, m_directOpticalWidget);
	ReleaseDirectWidget(ui.tab_3, m_directVirtualWidget);
	InitializeDirectWidgets();
}

void CircuitModuleWidget::DisplayLogicIed(const QString& iedName)
{
	if (!m_directLogicWidget)
	{
		InitializeDirectWidgets();
	}

	CircuitDiagramProxy diagramProxy;
	LogicDiagramModel* logicDiagram = diagramProxy.BuildLogicDiagramByIedName(iedName);
	if (!logicDiagram)
	{
		ReleaseDirectWidget(ui.tab, m_directLogicWidget);
		m_directLogicWidget = CreateDirectWidget(ui.tab, ui.logicView, "directLogicWidget");
		return;
	}

	m_directLogicWidget->ParseFromLogicSvg(*logicDiagram);
	delete logicDiagram;
}

void CircuitModuleWidget::DisplayBayLogic(const QString& bayName)
{
	if (!m_directLogicWidget)
	{
		InitializeDirectWidgets();
	}

	CircuitDiagramProxy diagramProxy;
	LogicDiagramModel* logicDiagram = diagramProxy.BuildLogicDiagramByBayName(bayName);
	if (!logicDiagram)
	{
		ReleaseDirectWidget(ui.tab, m_directLogicWidget);
		m_directLogicWidget = CreateDirectWidget(ui.tab, ui.logicView, "directLogicWidget");
		return;
	}

	m_directLogicWidget->ParseFromLogicSvg(*logicDiagram);
	delete logicDiagram;
}

void CircuitModuleWidget::DisplayOpticalIed(const QString& iedName)
{
	if (!m_directOpticalWidget)
	{
		InitializeDirectWidgets();
	}

	CircuitDiagramProxy diagramProxy;
	OpticalDiagramModel* opticalDiagram = diagramProxy.BuildOpticalDiagramByIedName(iedName);
	if (!opticalDiagram)
	{
		ReleaseDirectWidget(ui.tab_2, m_directOpticalWidget);
		m_directOpticalWidget = CreateDirectWidget(ui.tab_2, ui.opticalView, "directOpticalWidget");
		return;
	}

	m_directOpticalWidget->ParseFromOpticalSvg(*opticalDiagram);
	delete opticalDiagram;
}

void CircuitModuleWidget::DisplayVirtualIed(const QString& iedName)
{
	if (!m_directVirtualWidget)
	{
		InitializeDirectWidgets();
	}

	CircuitDiagramProxy diagramProxy;
	VirtualDiagramModel* virtualDiagram = diagramProxy.BuildVirtualDiagramByIedName(iedName);
	if (!virtualDiagram)
	{
		ReleaseDirectWidget(ui.tab_3, m_directVirtualWidget);
		m_directVirtualWidget = CreateDirectWidget(ui.tab_3, ui.wholeView, "directVirtualWidget");
		return;
	}

	m_directVirtualWidget->ParseFromVirtualSvg(*virtualDiagram);
	delete virtualDiagram;
}

void CircuitModuleWidget::DisplayStationOptical()
{
	if (!m_directOpticalWidget)
	{
		InitializeDirectWidgets();
	}

	CircuitDiagramProxy diagramProxy;
	OpticalDiagramModel* opticalDiagram = diagramProxy.BuildOpticalDiagramByStation();
	if (!opticalDiagram)
	{
		ReleaseDirectWidget(ui.tab_2, m_directOpticalWidget);
		m_directOpticalWidget = CreateDirectWidget(ui.tab_2, ui.opticalView, "directOpticalWidget");
		return;
	}

	m_directOpticalWidget->ParseFromOpticalSvg(*opticalDiagram);
	delete opticalDiagram;
}

void CircuitModuleWidget::DisplayBayOptical(const QString& bayName)
{
	if (!m_directOpticalWidget)
	{
		InitializeDirectWidgets();
	}

	CircuitDiagramProxy diagramProxy;
	OpticalDiagramModel* opticalDiagram = diagramProxy.BuildOpticalDiagramByBayName(bayName);
	if (!opticalDiagram)
	{
		ReleaseDirectWidget(ui.tab_2, m_directOpticalWidget);
		m_directOpticalWidget = CreateDirectWidget(ui.tab_2, ui.opticalView, "directOpticalWidget");
		return;
	}

	m_directOpticalWidget->ParseFromOpticalSvg(*opticalDiagram);
	delete opticalDiagram;
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
