#include "ControlBlockListWindow.h"
#include "circuitconfig.h"
#include <QAbstractItemView>
#include <QHeaderView>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVBoxLayout>
#include <QtAlgorithms>

static QString build_control_block_type_text(VirtualType type)
{
	return type == GOOSE ? QString::fromLatin1("GOOSE") : QString::fromLatin1("SV");
}

static QString build_control_block_party_text(ControlBlockPartyType partyType)
{
	return partyType == CB_PUBLISHER ? QString::fromLocal8Bit("发布") : QString::fromLocal8Bit("订阅");
}

static QString build_control_block_appid_text(int appid)
{
	if (appid < 0)
	{
		return QString();
	}
	return QString::fromLatin1("0x%1").arg(appid, 4, 16, QChar('0')).toUpper();
}

static bool control_block_info_less(const OpticalControlBlockInfo& left, const OpticalControlBlockInfo& right)
{
	if (left.type != right.type)
	{
		return left.type < right.type;
	}
	if (left.partyType != right.partyType)
	{
		return left.partyType < right.partyType;
	}
	int nameCompare = QString::compare(left.controlBlockName, right.controlBlockName, Qt::CaseInsensitive);
	if (nameCompare != 0)
	{
		return nameCompare < 0;
	}
	if (left.controlBlockCode != right.controlBlockCode)
	{
		return left.controlBlockCode < right.controlBlockCode;
	}
	return QString::compare(left.endpointRef, right.endpointRef, Qt::CaseInsensitive) < 0;
}

ControlBlockListWindow::ControlBlockListWindow(QWidget* parent)
	: QWidget(parent)
	, m_tableWidget(new QTableWidget(this))
{
	setAttribute(Qt::WA_DeleteOnClose, true);
	QVBoxLayout* layout = new QVBoxLayout(this);
	layout->setContentsMargins(0, 0, 0, 0);
	layout->addWidget(m_tableWidget);
	QStringList headerList;
	headerList << QString::fromLocal8Bit("类型")
		<< QString::fromLocal8Bit("角色")
		<< QString::fromLocal8Bit("控制块名")
		<< QString::fromLocal8Bit("控制块ID")
		<< QString::fromLocal8Bit("控制块Ref")
		<< QString::fromLocal8Bit("数据集")
		<< QString::fromLocal8Bit("标识")
		<< QString::fromLatin1("APPID")
		<< QString::fromLatin1("MAC")
		<< QString::fromLocal8Bit("对端IED")
		<< QString::fromLocal8Bit("端点通道Ref");
	m_tableWidget->setColumnCount(headerList.size());
	m_tableWidget->setHorizontalHeaderLabels(headerList);
	m_tableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
	m_tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
	m_tableWidget->setSelectionMode(QAbstractItemView::SingleSelection);
	m_tableWidget->setAlternatingRowColors(true);
	m_tableWidget->verticalHeader()->setVisible(false);
	m_tableWidget->horizontalHeader()->setStretchLastSection(true);
	resize(1100, 420);
}

bool ControlBlockListWindow::LoadByOpticalConnPoint(quint64 opticalCode, const QString& iedName)
{
	if (opticalCode == 0 || iedName.isEmpty() || iedName.contains("SW", Qt::CaseInsensitive))
	{
		return false;
	}
	CircuitConfig* circuitConfig = CircuitConfig::Instance();
	if (!circuitConfig)
	{
		return false;
	}
	QList<OpticalControlBlockInfo> sourceList = circuitConfig->GetControlBlockInfoListByOpticalCode(opticalCode);
	QList<OpticalControlBlockInfo> displayList;
	for (int index = 0; index < sourceList.size(); ++index)
	{
		const OpticalControlBlockInfo& controlBlockInfo = sourceList.at(index);
		if (controlBlockInfo.iedName == iedName)
		{
			displayList.append(controlBlockInfo);
		}
	}
	if (displayList.isEmpty())
	{
		return false;
	}
	qSort(displayList.begin(), displayList.end(), control_block_info_less);
	m_tableWidget->clearContents();
	m_tableWidget->setRowCount(displayList.size());
	for (int row = 0; row < displayList.size(); ++row)
	{
		const OpticalControlBlockInfo& controlBlockInfo = displayList.at(row);
		m_tableWidget->setItem(row, 0, new QTableWidgetItem(build_control_block_type_text(controlBlockInfo.type)));
		m_tableWidget->setItem(row, 1, new QTableWidgetItem(build_control_block_party_text(controlBlockInfo.partyType)));
		m_tableWidget->setItem(row, 2, new QTableWidgetItem(controlBlockInfo.controlBlockName));
		m_tableWidget->setItem(row, 3, new QTableWidgetItem(controlBlockInfo.controlBlockCode == 0 ? QString() : QString::number((qulonglong)controlBlockInfo.controlBlockCode)));
		m_tableWidget->setItem(row, 4, new QTableWidgetItem(controlBlockInfo.controlBlockRef));
		m_tableWidget->setItem(row, 5, new QTableWidgetItem(controlBlockInfo.dataSetName));
		m_tableWidget->setItem(row, 6, new QTableWidgetItem(controlBlockInfo.identityName));
		m_tableWidget->setItem(row, 7, new QTableWidgetItem(build_control_block_appid_text(controlBlockInfo.appid)));
		m_tableWidget->setItem(row, 8, new QTableWidgetItem(controlBlockInfo.macAddr));
		m_tableWidget->setItem(row, 9, new QTableWidgetItem(controlBlockInfo.peerIedName));
		m_tableWidget->setItem(row, 10, new QTableWidgetItem(controlBlockInfo.endpointRef));
	}
	m_tableWidget->resizeColumnsToContents();
	setWindowTitle(QString::fromLatin1("Control Blocks - %1 - Optical %2").arg(iedName).arg((qulonglong)opticalCode));
	return true;
}
