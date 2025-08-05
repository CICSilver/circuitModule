#include "circuitconfig.h"
#include <QStringList>
#include <QSvgGenerator>
#include <QPainter>
#include <QApplication>
#include "cime/cime.h"
bool debug = false;

bool CircuitConfig::LoadCimeFile(/*QString file*/)
{
	Clear();
	QString opticalPath = QCoreApplication::applicationDirPath() + "/realCircuit.cime";
	QString iedPath = QCoreApplication::applicationDirPath() + "/ied_list.cime";
	QString gsePath = QCoreApplication::applicationDirPath() + "/GooseCircuit.cime";
	QString svPath = QCoreApplication::applicationDirPath() + "/SVCircuit.cime";
	if (!LoadIedCimeFile(iedPath))
	{
		m_errMsg += "LoadIedCimeFile failed";
		return false;
	}
	if (!LoadOpticalCimeFile(opticalPath))
	{
		m_errMsg += "LoadOpticalCimeFile failed";
		return false;
	}
	if (!LoadGseCimeFile(gsePath))
	{
		m_errMsg += "LoadGseCimeFile failed";
		return false;
	}
	if (!LoadSvCimeFile(svPath))
	{
		m_errMsg += "LoadSvCimeFile failed";
		return false;
	}
	return true;
}

bool CircuitConfig::LoadGseCimeFile(QString file)
{
	xcime::Cime cime;
	cime.loadFromFile(file.toStdString());
	LoadVirtualCircuitFile(cime, GOOSE);
	return true;
}

bool CircuitConfig::LoadSvCimeFile(QString file)
{
	xcime::Cime cime;
	cime.loadFromFile(file.toStdString());
	LoadVirtualCircuitFile(cime, SV);
	return true;
}

bool CircuitConfig::LoadIedCimeFile(QString file)
{
	xcime::Cime cime;
	cime.loadFromFile(file.toStdString());
	xcime::CimeQuery query(cime["IED"]["ysd"]);
	for (xcime::CimeQuery::const_iterator it = query.cbegin(); it != query.cend(); ++it)
	{
		IED* pIed = new IED;
		xcime::RecordProxy rec = *it;
		pIed->name = rec["IEDNAME"];
		pIed->desc = rec["IEDDESC"];
		pIed->type = rec["IEDMODEL"];
		pIed->manufacturer = rec["MANU"];
		pIed->version = rec["VERSION"];
		pIed->ccd = rec["CCDCRC"];
		pIed->icd = rec["ICDCRC"];
		m_iedList.append(pIed);
		m_iedHash.insert(pIed->name, pIed);
	}
	return true;
}

bool CircuitConfig::LoadOpticalCimeFile(QString file)
{
	xcime::Cime cime;
	cime.loadFromFile(file.toStdString());
	xcime::CimeQuery query(cime["IED"]["ysd"]);
	for (xcime::CimeQuery::const_iterator it = query.cbegin(); it != query.cend(); ++it)
	{
		OpticalCircuit* pOptCircuit = new OpticalCircuit;
		xcime::RecordProxy rec = *it;
		QString loopCode = rec["LOOPCODE"];
		pOptCircuit->code = loopCode.split('#').at(1).toUInt();
		pOptCircuit->cableDesc = rec["LINECODE"];
		pOptCircuit->destIedName = rec["IEDNAME"];
		pOptCircuit->destIedPort = rec["IEDPORT"];
		pOptCircuit->destIedDesc = rec["IEDDESC"];
		pOptCircuit->srcIedName = rec["OFFSIDEIEDNAME"];
		pOptCircuit->srcIedPort = rec["OFFSIDEIEDPORT"];
		pOptCircuit->srcIedDesc = rec["OFFSIDEIEDDESC"];
		pOptCircuit->reference = rec["REFERENCE"];
		IED* pSrcIed = m_iedHash[pOptCircuit->srcIedName];
		IED *pDestIed = m_iedHash[pOptCircuit->destIedName];
		pOptCircuit->pSrcIed = pSrcIed;
		pOptCircuit->pDestIed = pDestIed;
		pSrcIed->optical_list_new.append(pOptCircuit);
		pDestIed->optical_list_new.append(pOptCircuit);
		m_opticalCircuitList.append(pOptCircuit);
		// 记录光纤直连设备及交换机设备
		pSrcIed->connectedIedNameSet.insert(pOptCircuit->destIedName);
		pDestIed->connectedIedNameSet.insert(pOptCircuit->srcIedName);
		// 建立光纤链路与两侧设备的关联关系
		m_opticalCircuitHash.insert(qMakePair(pOptCircuit->srcIedName, pOptCircuit->destIedName), pOptCircuit);
	}
	return true;
}

bool CircuitConfig::LoadVirtualCircuitFile(xcime::Cime& cime, VirtualType type)
{
	xcime::CimeQuery query(cime["IED"]["ysd"]);
	QHash<QString, LogicCircuit*> logicCircuitHash;		// key为 destIedName:cbName
	for (xcime::CimeQuery::const_iterator it = query.cbegin(); it != query.cend(); ++it)
	{
		VirtualCircuit* pVtCircuit = new VirtualCircuit(type);
		xcime::RecordProxy rec = *it;
		QString srcAndDestIedName = rec["INOUTIEDNAME"];
		QStringList iedNameList = srcAndDestIedName.split('|');
		if (iedNameList.size() != 2)
		{
			return false;
		}
		pVtCircuit->srcIedName = iedNameList.at(1);
		pVtCircuit->destIedName = iedNameList.at(0);
		pVtCircuit->srcIedDesc = rec["OUTPUTIEDDESC"];
		pVtCircuit->srcName = rec["OUTPUTNAME"];
		pVtCircuit->srcDesc = rec["OUTPUTDESC"];
		pVtCircuit->srcRef = rec["OUTPUTREFRENCE"];

		pVtCircuit->destIedDesc = rec["INPUTIEDDESC"];
		pVtCircuit->destName = rec["INPUTNAME"];
		pVtCircuit->destDesc = rec["INPUTDESC"];
		pVtCircuit->destRef = rec["INPUTREFRENCE"];
		if (type == SV)
		{
			pVtCircuit->destSoftPlateRef = rec["INPUTENAREF"];
			pVtCircuit->destSoftPlateDesc = rec["INPUTENADOC"];
			pVtCircuit->srcCbName = rec["OUTPUTSVCB"];
		}
		else if (type == GOOSE)
		{
			pVtCircuit->srcSoftPlateRef = rec["OUTPUTENAREF"];
			pVtCircuit->srcSoftPlateDesc = rec["OUTPUTENADOC"];
			pVtCircuit->srcCbName = rec["OUTPUTGOCB"];
		}
		
		QString key = QString("%1:%2").arg(pVtCircuit->destIedName).arg(pVtCircuit->srcCbName);
		IED* pSrcIed = m_iedHash.value(pVtCircuit->srcIedName, NULL);
		IED* pDestIed = m_iedHash.value(pVtCircuit->destIedName, NULL);
		if (pSrcIed && pDestIed)
		{
			// 记录跨交换机的设备
			pSrcIed->connectedIedNameSet.insert(pDestIed->name);
			pDestIed->connectedIedNameSet.insert(pSrcIed->name);
		}
		else
		{
			m_errMsg += QString("VirtualCircuit %1:%2 not found in IED list").arg(pVtCircuit->srcIedName).arg(pVtCircuit->destIedName);
			delete pVtCircuit;
			continue;
		}
		LogicCircuit* pLogicCircuit = logicCircuitHash.value(key, NULL);
		if (!pLogicCircuit)
		{
			pLogicCircuit = new LogicCircuit;
			pLogicCircuit->type = type;
			pLogicCircuit->pSrcIed = m_iedHash.value(pVtCircuit->srcIedName, NULL);
			pLogicCircuit->pDestIed = m_iedHash.value(pVtCircuit->destIedName, NULL);
			pLogicCircuit->cbName = pVtCircuit->srcCbName;
			m_logicCircuitList.append(pLogicCircuit);
			logicCircuitHash.insert(key, pLogicCircuit);
			m_inLogicCircuitHash.insertMulti(pVtCircuit->destIedName, pLogicCircuit);
			m_outLogicCircuitHash.insertMulti(pVtCircuit->srcIedName, pLogicCircuit);
		}
		pVtCircuit->pLogicCircuit = pLogicCircuit;
		m_inVirtualCircuitHash.insertMulti(pVtCircuit->destIedName, pVtCircuit);
		m_outVirtualCircuitHash.insertMulti(pVtCircuit->srcIedName, pVtCircuit);
		pLogicCircuit->circuitList.append(pVtCircuit);
		m_virtualCircuitList.append(pVtCircuit);
	}
	return true;
}
void CircuitConfig::Clear()
{
	qDeleteAll(m_iedList);
	m_iedList.clear();
	qDeleteAll(m_dataList);
	m_dataList.clear();
	qDeleteAll(m_virtualCircuitList);
	m_virtualCircuitList.clear();
	qDeleteAll(m_opticalCircuitList);
	m_opticalCircuitList.clear();
	qDeleteAll(m_logicCircuitList);
	m_logicCircuitList.clear();
	qDeleteAll(m_virtualCircuitList);
	m_virtualCircuitList.clear();
	// 关系容器，直接清空
	m_inLogicCircuitHash.clear();
	m_outLogicCircuitHash.clear();
	m_iedHash.clear();
	m_dataPathHash.clear();
	m_dataSetHash.clear();
}
