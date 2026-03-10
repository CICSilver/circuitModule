#include "circuitconfig.h"
#include <QStringList>
#include <QSvgGenerator>
#include <QPainter>
#include <QApplication>
#include <QCoreApplication>
#include <QDir>
#include <QDebug>
#include "cime/cime.h"
#include "RtdbUtils.h"
bool debug = false;

CircuitConfig::CircuitConfig()
    : m_rtdb(RtdbClient::Instance())
    , m_stationModel(NULL)
{
    if(!m_rtdb.isOpen())
        m_rtdb.refresh();
}

bool CircuitConfig::LoadCimeFile(QString path)
{
	m_errMsg.clear();
	QString effectivePath = path.trimmed();
	if (effectivePath.isEmpty())
	{
		effectivePath = m_cimeDir;
	}
	effectivePath = QDir::fromNativeSeparators(effectivePath);
	if (effectivePath.isEmpty())
	{
		m_errMsg += "CIME directory path is empty";
		return false;
	}
	QString absoluteDirPath;
	QDir candidateDir(effectivePath);
	if (candidateDir.isAbsolute())
	{
		absoluteDirPath = QDir::cleanPath(candidateDir.absolutePath());
	}
	else
	{
		QDir base(QCoreApplication::applicationDirPath());
		absoluteDirPath = QDir::cleanPath(base.absoluteFilePath(effectivePath));
	}
	QDir absoluteDir(absoluteDirPath);
	if (!absoluteDir.exists())
	{
		m_errMsg += QString("CIME directory does not exist: %1").arg(absoluteDirPath);
		return false;
	}
	m_cimeDir = absoluteDirPath;
	Clear();
	QString iedPath = absoluteDir.filePath("ied_list.cime");
	QString opticalPath = absoluteDir.filePath("realCircuit.cime");
	QString gsePath = absoluteDir.filePath("GooseCircuit.cime");
	QString svPath = absoluteDir.filePath("SVCircuit.cime");
	// 依次加载各个配置文件
	//if (!LoadIedCimeFile(iedPath))
	//{
	//	m_errMsg += "LoadIedCimeFile failed";
	//	return false;
	//}
	if(!LoadIedCimeFile(iedPath))
	{
		m_errMsg += "LoadIedFromRtdb failed";
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

bool CircuitConfig::LoadIedFromRtdb()
{
	if (!m_rtdb.isOpen())
	{
		qDebug() << "RTDB is not open";
		return false;
	}
	const CRtdbEleModelStation* station = m_rtdb.stationModel();
	for (std::map<std::string, CRtdbEleModelBay*>::const_iterator bayIt = station->m_mapStationBay.cbegin(); bayIt != station->m_mapStationBay.cend(); ++bayIt)
	{
		foreach(CRtdbEleModelIed * pIed, bayIt->second->m_listIed)
		{
			IED* newIed = new IED;
			newIed->name = QString::fromLocal8Bit(pIed->m_pIedHead->iedName);
			newIed->id = pIed->m_pIedHead->id;
			newIed->code = pIed->m_pIedHead->code;
			newIed->desc = QString::fromLocal8Bit(pIed->m_pIedHead->iedDesc);
			newIed->ccd = QString::fromLocal8Bit(pIed->m_pIedHead->iedCcdCrc);
			newIed->icd = QString::fromLocal8Bit(pIed->m_pIedHead->iedIcdCrc);
			newIed->manufacturer = QString::fromLocal8Bit(pIed->m_pIedHead->iedMfr);
			newIed->version = pIed->m_pIedHead->iedVer;
			m_iedList.append(newIed);
			m_iedHash.insert(newIed->name, newIed);
		}
	}
	return true;
}

bool CircuitConfig::LoadGseFromRtdb()
{
    if (!m_rtdb.isOpen())
    {
        qDebug() << "RTDB is not open";
        return false;
    }
    const CRtdbEleModelCircuit* circuitModel = m_rtdb.circuitModel();
    QHash<QString, LogicCircuit*> logicHash;

    const std::list<stuRtdbGseCircuit*>& gseList = circuitModel->m_listGseCircuit;
    for (std::list<stuRtdbGseCircuit*>::const_iterator it = gseList.cbegin();
         it != gseList.cend(); ++it)
    {
        stuRtdbGseCircuit* pRtdbGseCircuit = *it;
        if (!pRtdbGseCircuit || !pRtdbGseCircuit->m_pOutIed || !pRtdbGseCircuit->m_pInIed || !pRtdbGseCircuit->m_pGseCurcuitInfo)
            continue;
        VirtualCircuit* pGse = new VirtualCircuit(GOOSE);

        pGse->code = pRtdbGseCircuit->m_pGseCurcuitInfo->code;
		pGse->linkCode = pRtdbGseCircuit->m_pGseCurcuitInfo->linkCode;
        const CRtdbEleModelIed* pRtdbOutIed = utils::asModelIed(pRtdbGseCircuit->m_pOutIed);
        const CRtdbEleModelIed* pRtdbInIed  = utils::asModelIed(pRtdbGseCircuit->m_pInIed);
        // src
        pGse->srcIedName = QString::fromLocal8Bit(pRtdbGseCircuit->m_pGseCurcuitInfo->outIedName);
        pGse->srcIedDesc = pRtdbOutIed ? QString::fromLocal8Bit(pRtdbOutIed->m_pIedHead->iedDesc) : "";
        pGse->srcRef = QString::fromLocal8Bit(pRtdbGseCircuit->m_pGseCurcuitInfo->outRef);
        setChlDesc(pGse, pRtdbGseCircuit->m_pOutChl, true);
        setChlDesc(pGse, pRtdbGseCircuit->m_pInChl, false);
        const stuRtdbStatus* pSrcPlateStatus = utils::asPlateStatus(pRtdbGseCircuit->m_pOutRyb);
		pGse->srcSoftPlateCode = pSrcPlateStatus ? pSrcPlateStatus->code : 0;
        pGse->srcSoftPlateRef  = pSrcPlateStatus ? QString::fromLocal8Bit(pSrcPlateStatus->ref)  : "";
        pGse->srcSoftPlateDesc = pSrcPlateStatus ? QString::fromLocal8Bit(pSrcPlateStatus->desc) : "";
        const CRtdbEleModelGse* pRtdbOutGocb = utils::asModelGse(pRtdbGseCircuit->m_pOutGocb);
        pGse->srcCbName = pRtdbOutGocb ? QString::fromLocal8Bit(pRtdbOutGocb->m_pGseHead->name) : "";
        // dest
        const stuRtdbStatus* pDestPlateStatus = utils::asPlateStatus(pRtdbGseCircuit->m_pInRyb);
		pGse->destSoftPlateCode = pDestPlateStatus ? pDestPlateStatus->code : 0;
        pGse->destSoftPlateRef  = pDestPlateStatus ? QString::fromLocal8Bit(pDestPlateStatus->ref)  : "";
        pGse->destSoftPlateDesc = pDestPlateStatus ? QString::fromLocal8Bit(pDestPlateStatus->desc) : "";
        pGse->destRef     = QString::fromLocal8Bit(pRtdbGseCircuit->m_pGseCurcuitInfo->inRef);
        pGse->destIedName = QString::fromLocal8Bit(pRtdbGseCircuit->m_pGseCurcuitInfo->inIedName);
        pGse->destIedDesc = pRtdbInIed ? QString::fromLocal8Bit(pRtdbInIed->m_pIedHead->iedDesc) : "";
        pGse->inRemoteCode  = pRtdbGseCircuit->m_pGseCurcuitInfo->inCode;
        pGse->outRemoteCode = pRtdbGseCircuit->m_pGseCurcuitInfo->outCode;
        pGse->pRtdbCircuit = pRtdbGseCircuit;
        buildCircuitRelations(pGse, logicHash);
    }
    return true;
}

bool CircuitConfig::LoadSvFromRtdb()
{
    if (!m_rtdb.isOpen())
    {
        qDebug() << "RTDB is not open";
        return false;
    }
    const CRtdbEleModelCircuit* circuitModel = m_rtdb.circuitModel();
    QHash<QString, LogicCircuit*> logicHash;

    const std::list<stuRtdbSvCircuit*>& svList = circuitModel->m_listSvCircuit;
    for (std::list<stuRtdbSvCircuit*>::const_iterator it = svList.cbegin();
         it != svList.cend(); ++it)
    {
        stuRtdbSvCircuit* pRtdbSvCircuit = *it;
        if (!pRtdbSvCircuit || !pRtdbSvCircuit->m_pInIed || !pRtdbSvCircuit->m_pOutIed || !pRtdbSvCircuit->m_pSvCurcuitInfo)
            continue;

        VirtualCircuit* pSv = new VirtualCircuit(SV);
        const CRtdbEleModelIed* pRtdbSrcIed  = utils::asModelIed(pRtdbSvCircuit->m_pOutIed);
        const CRtdbEleModelIed* pRtdbDestIed = utils::asModelIed(pRtdbSvCircuit->m_pInIed);
        pSv->code = pRtdbSvCircuit->m_pSvCurcuitInfo->code;
		pSv->linkCode = pRtdbSvCircuit->m_pSvCurcuitInfo->linkCode;
        // src
        pSv->srcIedName = QString::fromLocal8Bit(pRtdbSrcIed->m_pIedHead->iedName);
        pSv->srcIedDesc = QString::fromLocal8Bit(pRtdbSrcIed->m_pIedHead->iedDesc);
        const CRtdbEleModelSmv* pRtdbOutSmv = utils::asModelSmv(pRtdbSvCircuit->m_pOutChl);
        pSv->srcCbName = pRtdbOutSmv ? QString::fromLocal8Bit(pRtdbOutSmv->m_pSmvHead->name) : "";
        pSv->srcRef = QString::fromLocal8Bit(pRtdbSvCircuit->m_pSvCurcuitInfo->outRef);
        setChlDesc(pSv, pRtdbSvCircuit->m_pOutChl, true);
        // dest
        const stuRtdbStatus* pDestPlateStatus = utils::asPlateStatus(pRtdbSvCircuit->m_pInRyb);
        pSv->destIedName = QString::fromLocal8Bit(pRtdbDestIed->m_pIedHead->iedName);
        pSv->destIedDesc = QString::fromLocal8Bit(pRtdbDestIed->m_pIedHead->iedDesc);
        pSv->destSoftPlateDesc = pDestPlateStatus ? QString::fromLocal8Bit(pDestPlateStatus->desc) : "";
        pSv->destSoftPlateRef  = pDestPlateStatus ? QString::fromLocal8Bit(pDestPlateStatus->ref)  : "";
        pSv->destRef = QString::fromLocal8Bit(pRtdbSvCircuit->m_pSvCurcuitInfo->inRef);
        setChlDesc(pSv, pRtdbSvCircuit->m_pInChl, false);

        buildCircuitRelations(pSv, logicHash);
    }
    return true;
}

bool CircuitConfig::LoadOpticalFromRtdb()
{
    if (!m_rtdb.isOpen())
    {
        qDebug() << "RTDB is not open";
        return false;
    }
    const CRtdbEleModelCircuit* circuitModel = m_rtdb.circuitModel();

    const std::list<stuRtdbRealCircuit*>& realList = circuitModel->m_listRealCircuit;
    for (std::list<stuRtdbRealCircuit*>::const_iterator it = realList.cbegin();
         it != realList.cend(); ++it)
    {
        stuRtdbRealCircuit* pRtdbRealCircuit = *it;
        if (!pRtdbRealCircuit || !pRtdbRealCircuit->m_pRcvIed || !pRtdbRealCircuit->m_pOffIed || !pRtdbRealCircuit->m_pRealCurcuitInfo)
            continue;

        OpticalCircuit* pOptCircuit = new OpticalCircuit;
		pOptCircuit->code = pRtdbRealCircuit->m_pRealCurcuitInfo->code;
        pOptCircuit->loopName = QString::fromLocal8Bit(pRtdbRealCircuit->m_pRealCurcuitInfo->loopName);
        pOptCircuit->loopCode = pOptCircuit->loopName.split('#').at(1).toUInt();
        pOptCircuit->remoteId = pRtdbRealCircuit->m_pRealCurcuitInfo->linkCode;
        CRtdbEleModelIed* pRtdbRcvIed = static_cast<CRtdbEleModelIed*>(pRtdbRealCircuit->m_pRcvIed);
        CRtdbEleModelIed* pRtdbOffIed = static_cast<CRtdbEleModelIed*>(pRtdbRealCircuit->m_pOffIed);
        IED* pSrcIed  = m_iedHash.value(QString::fromLocal8Bit(pRtdbOffIed->m_pIedHead->iedName), NULL);
        IED* pDestIed = m_iedHash.value(QString::fromLocal8Bit(pRtdbRcvIed->m_pIedHead->iedName), NULL);
        if (!pSrcIed || !pDestIed)
        {
            delete pOptCircuit;
            continue;
        }
        pOptCircuit->srcIedName  = QString::fromLocal8Bit(pRtdbOffIed->m_pIedHead->iedName);
        pOptCircuit->srcIedPort  = QString::fromLocal8Bit(pRtdbRealCircuit->m_pRealCurcuitInfo->offPort.port);
        pOptCircuit->srcIedDesc  = QString::fromLocal8Bit(pRtdbOffIed->m_pIedHead->iedDesc);
        pOptCircuit->destIedName = QString::fromLocal8Bit(pRtdbRcvIed->m_pIedHead->iedName);
        pOptCircuit->destIedPort = QString::fromLocal8Bit(pRtdbRealCircuit->m_pRealCurcuitInfo->rcvPort.port);
        pOptCircuit->destIedDesc = QString::fromLocal8Bit(pRtdbRcvIed->m_pIedHead->iedDesc);
        pOptCircuit->pSrcIed = pSrcIed;
        pOptCircuit->pDestIed = pDestIed;

        pSrcIed->optical_list.append(pOptCircuit);
        pDestIed->optical_list.append(pOptCircuit);
        m_opticalCircuitList.append(pOptCircuit);
        pSrcIed->connectedIedNameSet.insert(pOptCircuit->destIedName);
        pDestIed->connectedIedNameSet.insert(pOptCircuit->srcIedName);
        m_opticalCircuitHash.insert(qMakePair(pOptCircuit->srcIedName, pOptCircuit->destIedName), pOptCircuit);
    }
    return true;
}

bool CircuitConfig::LoadRTDB()
{
	Clear();
	if (!m_rtdb.refresh())
	{
		m_stationModel = NULL;
		qDebug() << "RTDB refresh failed";
		return false;
	}

	m_stationModel = m_rtdb.stationModel();
	if (!m_stationModel)
	{
		qDebug() << "RTDB station model unavailable";
		return false;
	}

	if (!LoadIedFromRtdb())
	{
		m_errMsg += "LoadIedFromRtdb failed";
		return false;
	}
	if (!LoadOpticalFromRtdb())
	{
		m_errMsg += "LoadOpticalFromRtdb failed";
		return false;
	}
	if (!LoadGseFromRtdb())
	{
		m_errMsg += "LoadGseFromRtdb failed";
		return false;
	}
	if (!LoadSvFromRtdb())
	{
		m_errMsg += "LoadSvFromRtdb failed";
		return false;
	}
	return true;
}

bool CircuitConfig::LoadOpticalCimeFile(QString file)
{
	xcime::Cime cime;
	cime.loadFromFile(file.toStdString());
	xcime::CimeQuery query(cime["RealCircuit"]["ysd"]);
	for (xcime::CimeQuery::const_iterator it = query.cbegin(); it != query.cend(); ++it)
	{
		OpticalCircuit* pOptCircuit = new OpticalCircuit;
		xcime::RecordProxy rec = *it;
		QString loopCode = rec["LOOPCODE"];
		pOptCircuit->loopCode = loopCode.split('#').at(1).toUInt();
		//pOptCircuit->cableDesc = rec["LINECODE"];
		pOptCircuit->destIedName = rec["IEDNAME"];
		pOptCircuit->destIedPort = rec["IEDPORT"];
		pOptCircuit->destIedDesc = rec["IEDDESC"];
		pOptCircuit->srcIedName = rec["OFFSIDEIEDNAME"];
		pOptCircuit->srcIedPort = rec["OFFSIDEIEDPORT"];
		pOptCircuit->srcIedDesc = rec["OFFSIDEIEDDESC"];
		pOptCircuit->reference = rec["REFERENCE"];
		pOptCircuit->remoteId = rec["LINKXIAOXINID"].toUInt();
		IED* pSrcIed = m_iedHash.value(pOptCircuit->srcIedName, NULL);
		IED* pDestIed = m_iedHash.value(pOptCircuit->destIedName, NULL);
		if (!pSrcIed || !pDestIed)
		{
			// 不在需要展现的IED列表中，忽略该光纤链路
			delete pOptCircuit;
			continue;
		}
		pOptCircuit->pSrcIed = pSrcIed;
		pOptCircuit->pDestIed = pDestIed;
		pSrcIed->optical_list.append(pOptCircuit);
		pDestIed->optical_list.append(pOptCircuit);
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
	xcime::CimeQuery query(cime["GooseCircuit"]["ysd"]);
	QHash<QString, LogicCircuit*> logicCircuitHash;		// key为 destIedName:cbName
	for (xcime::CimeQuery::const_iterator it = query.cbegin(); it != query.cend(); ++it)
	{
		VirtualCircuit* pVtCircuit = new VirtualCircuit(type);
		xcime::RecordProxy rec = *it;
		QString srcAndDestIedName = rec["INOUTIEDNAME"];
		QString num = rec["Num"];

		QStringList iedNameList = srcAndDestIedName.split('|');
		if (iedNameList.size() != 2)
		{
			return false;
		}
		pVtCircuit->srcIedName = iedNameList.at(1);
		pVtCircuit->destIedName = iedNameList.at(0);
		if (m_iedHash.find(pVtCircuit->srcIedName) == m_iedHash.end() ||
			m_iedHash.find(pVtCircuit->destIedName) == m_iedHash.end())
		{
			// 虚回路的源或目的IED不在需要展现的IED列表中，忽略该虚回路
			delete pVtCircuit;
			continue;
		}
		pVtCircuit->srcIedDesc = rec["OUTPUTIEDDESC"];
		pVtCircuit->srcName = rec["OUTPUTNAME"];
		pVtCircuit->srcDesc = rec["OUTPUTDESC"];
		pVtCircuit->srcRef = rec["OUTPUTREFRENCE"];

		pVtCircuit->destIedDesc = rec["INPUTIEDDESC"];
		pVtCircuit->destName = rec["INPUTNAME"];
		pVtCircuit->destDesc = rec["INPUTDESC"];
		pVtCircuit->destRef = rec["INPUTREFRENCE"];
		//pVtCircuit->remoteId = rec["TerminalRemoteCommID"];
		//pVtCircuit->remoteSigId_A = rec["LinkRemoteCommID"];
		//pVtCircuit->remoteSigId_B = rec["LinkRemoteCommIDB"];

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
		buildCircuitRelations(pVtCircuit, logicCircuitHash);
		//QString key = QString("%1:%2").arg(pVtCircuit->destIedName).arg(pVtCircuit->srcCbName);
		//IED* pSrcIed = m_iedHash.value(pVtCircuit->srcIedName, NULL);
		//IED* pDestIed = m_iedHash.value(pVtCircuit->destIedName, NULL);
		//if (pSrcIed && pDestIed)
		//{
		//	// 记录跨交换机的设备
		//	pSrcIed->connectedIedNameSet.insert(pDestIed->name);
		//	pDestIed->connectedIedNameSet.insert(pSrcIed->name);
		//}
		//else
		//{
		//	m_errMsg += QString("VirtualCircuit %1:%2 not found in IED list").arg(pVtCircuit->srcIedName).arg(pVtCircuit->destIedName);
		//	delete pVtCircuit;
		//	continue;
		//}
		//LogicCircuit* pLogicCircuit = logicCircuitHash.value(key, NULL);
		//if (!pLogicCircuit)
		//{
		//	pLogicCircuit = new LogicCircuit;
		//	pLogicCircuit->type = type;
		//	pLogicCircuit->pSrcIed = m_iedHash.value(pVtCircuit->srcIedName, NULL);
		//	pLogicCircuit->pDestIed = m_iedHash.value(pVtCircuit->destIedName, NULL);
		//	pLogicCircuit->cbName = pVtCircuit->srcCbName;
		//	m_logicCircuitList.append(pLogicCircuit);
		//	logicCircuitHash.insert(key, pLogicCircuit);
		//	m_inLogicCircuitHash.insertMulti(pVtCircuit->destIedName, pLogicCircuit);
		//	m_outLogicCircuitHash.insertMulti(pVtCircuit->srcIedName, pLogicCircuit);
		//}
		//pVtCircuit->pLogicCircuit = pLogicCircuit;
		//m_inVirtualCircuitHash.insertMulti(pVtCircuit->destIedName, pVtCircuit);
		//m_outVirtualCircuitHash.insertMulti(pVtCircuit->srcIedName, pVtCircuit);
		//pLogicCircuit->circuitList.append(pVtCircuit);
		//m_virtualCircuitList.append(pVtCircuit);
	}
	return true;
}
void CircuitConfig::Clear()
{
	m_stationModel = NULL;
	qDeleteAll(m_iedList);
	m_iedList.clear();
	qDeleteAll(m_dataList);
	m_dataList.clear();
	qDeleteAll(m_virtualCircuitList);
	m_virtualCircuitList.clear();
	m_opticalVirtualCircuitHash.clear();
	qDeleteAll(m_opticalCircuitList);
	m_opticalCircuitList.clear();
	qDeleteAll(m_logicCircuitList);
	m_logicCircuitList.clear();
	// 关系容器，直接清空
	m_inLogicCircuitHash.clear();
	m_outLogicCircuitHash.clear();
	m_iedHash.clear();
}

OpticalCircuit* CircuitConfig::getOpticalByCode(quint64 code) const
{
	if (code == 0) return NULL;
	for (int i = 0; i < m_opticalCircuitList.size(); ++i) {
		OpticalCircuit* p = m_opticalCircuitList[i];
		if (p && p->code == code) return p;
	}
	return NULL;
}

void CircuitConfig::buildOpticalRelation(VirtualCircuit* pVtCircuit, const IED* pSrcIed, const IED* pDestIed)
{
	if (!pVtCircuit || !pSrcIed || !pDestIed) return;
	pVtCircuit->leftOpticalCode = 0;
	pVtCircuit->rightOpticalCode = 0;
	pVtCircuit->switchIedName.clear();
	OpticalCircuit* left = NULL;
	OpticalCircuit* right = NULL;
	QString swName;
	for (int i = 0; i < pSrcIed->optical_list.size(); ++i) {
		OpticalCircuit* oc = pSrcIed->optical_list.at(i);
		if (!oc) continue;
		QString other = (oc->srcIedName == pSrcIed->name) ? oc->destIedName : oc->srcIedName;
		if (!other.contains("SW")) continue;
		for (int j = 0; j < pDestIed->optical_list.size(); ++j) {
			OpticalCircuit* oc2 = pDestIed->optical_list.at(j);
			if (!oc2) continue;
			QString other2 = (oc2->srcIedName == pDestIed->name) ? oc2->destIedName : oc2->srcIedName;
			if (other2 == other) {
				left = oc;
				right = oc2;
				swName = other;
				break;
			}
		}
		if (left && right) break;
	}
	if (left && right) {
		pVtCircuit->leftOpticalCode = left->code;
		pVtCircuit->rightOpticalCode = right->code;
		pVtCircuit->switchIedName = swName;
		return;
	}
	QList<OpticalCircuit*> direct = getOpticalByIeds(pSrcIed->name, pDestIed->name);
	if (!direct.isEmpty()) {
		pVtCircuit->leftOpticalCode = direct.first()->code;
	}
}
