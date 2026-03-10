#pragma once
#include <QString>
#include <QStringList>
#include <QMultiHash>
#include <QDebug>
#include "pugixml/pugixml.hpp"
#include "basemodel.h"
#include "RtdbClient.h"
//#include "boost/smart_ptr/shared_ptr.hpp"
// SCL	- 
//		- Header
//		- Substation
//			- VoltageLevel(n) [name, desc]
//				- Voltage [multiplier, unit]
// 				- Bay(n) [name, desc]
//					- private<type="SF_IED"> - SFIed(n) [name, desc, version]
//		- Communication
//			- SubNetwork(n) [name, type, desc]		(DLT/860 第六部分 p58)
//				- BitRate [multiplier, unit] (optional)
//				- ConnectedAP(n) [iedName, apName, desc]	// 记录一个ied所有的goose信息和sv接入点
//				- GSE(n) [ldInst, cbName]
//					- Address
//						- P
// 
//		- IED(n) [name, desc, type, manufacturer, configVersion, virtual_terminal_conn_crc]		(DLT/860 第六部分 p42)
//			- Services	(skip)
//			- AccessPoint(n) [name, router(bool), clock(bool)] name对应ConnectedAp的apName
//				- Server
//					- LDevice(n) [inst, desc]
//						- LN0 [desc, lnType, lnClass = LLN0, inst = "", prefix] (DLT/860 第六部分 p45)
//							- DataSet(n) [name, desc]
//								- FCDA(n) [ldInst, prefix, lnClass, lnInst, doName, daName, fc]
//							- ReportControl(n) [name, datset, intgPd, rptID, confRev, buffered, bufTime]
//								- TrgOps [dchg, qchg, dupd, period]
//								- OptFields [seqNum, timeStamp, datset, reasonCode, dataRef, entryID, configRef]
// 								- RptEnabled [max]
//							- LogControl(n) [name, desc, datset, intgPd, logName, logEna, reasonCode, bufTime]
//								- TrgOps 同上
//							- DOI(n) [name, desc]
//							- Inputs 虚回路
//								- ExtRef [iedName, ldInst, prefix, lnClass, lnInst, doName, daName, intAddr] 接入位置：intAddr, 来源：ldInst/lnClass$lnInst.doName.daName
//							- GSEControl [name, datset, conRef, type, appID] datset 对应 DataSet.name，GSEControl.name在ied中唯一
//						- LN(n) [prefix, lnClass, lnType, inst, desc]
// 

// DOI [name, dexc]
//		- SDI(n) [name] (optional)	可能多层SDI嵌套
//			- DAI(n) [name, saddr, desc]
// 				- Val
//		- DAI(n) [name, saddr, desc]
//			- Val
// 虚/实回路查找：
// 入： 遍历指定IED - LDevice - LN0 - Inputs - ExtRef节点，ExtRef.iedName为src，当前指定ied为dest
//		Communication - SubNetwork - ConnectedAP - PhysConn - P.type == Port，当前指定ied的端口号。 或 在ExtRef节点的intAddress中，用/分割两个端口号 
// 压板判断 <DataSet desc="保护压板" name="dsRelayEna">
// 虚回路通断位置：XCBR.Pos.stVal@sAddr

namespace xcime
{
	class Cime;
}
class CircuitConfig
{
public:
	typedef pugi::xpath_node_set::const_iterator nodeSetConstIterator;
	static CircuitConfig* Instance()
	{
		static CircuitConfig instance;
		return &instance;
	}
	void Clear();
	//************************************
	// 函数名称:    LoadFile
	// 函数全名:	CircuitConfig::LoadFile
	// 访问权限:	public 
	// 函数说明:	从scd文件解析数据到内存，再进行关系建立。若失败，可调用GetErrorMsg获取错误信息
	// 函数参数:	QString file
	// 返回值:		bool
	//************************************
	//bool LoadScdFile(QString file);

	//************************************
	// 函数名称:	LoadCimeFile
	// 函数全名:	CircuitConfig::LoadCimeFile
	// 访问权限:	public 
	// 函数说明:	从Cime配置文件读取数据
	// 函数参数:	QString file
	// 返回值:		bool
	//************************************
	bool LoadCimeFile(QString file = QString());
	bool LoadGseCimeFile(QString file);
	bool LoadSvCimeFile(QString file);

	bool LoadIedCimeFile(QString file);
	// 加载实时库内容
	bool LoadRTDB();
	bool LoadIedFromRtdb();
	bool LoadGseFromRtdb();
	bool LoadSvFromRtdb();
	bool LoadOpticalFromRtdb();

	const CRtdbEleModelStation* StationModel() const { return m_stationModel; }
	//************************************
	// 函数名称:	LoadOpticalCimeFile
	// 函数全名:	CircuitConfig::LoadOpticalCimeFile
	// 访问权限:	public 
	// 函数说明:	解析光纤链路配置文件
	// 函数参数:	QString file
	// 返回值:		bool
	//************************************
	bool LoadOpticalCimeFile(QString file);

	const QString& CimeDirectory() const { return m_cimeDir; }
	//************************************
	// 函数名称:	SaveConfigFile
	// 函数全名:	CircuitConfig::SaveConfigFile
	// 访问权限:	public 
	// 函数说明:	创建xml格式配置文件
	// 函数参数:	QString file
	// 返回值:		bool
	//************************************
	//bool SaveConfigFile(QString &file) const;

	//************************************
	// 函数名称:	SaveConfigAsCIME
	// 函数全名:	CircuitConfig::SaveConfigAsCIME
	// 访问权限:	public 
	// 函数说明:	创建CIME格式的配置文件
	// 函数参数:	QString file
	// 返回值:		bool
	//************************************
	//bool SaveConfigAsCime(QString &file);

	//************************************
	// 函数名称:    ParseIed
	// 函数全名:	CircuitConfig::ParseIed
	// 访问权限:	public 
	// 函数说明:	解析IED相关信息，包括Communication节点中于IED相关的GSE/SMV及链路信息
	// 函数参数:	pugi::xml_node & sclNode
	// 返回值:		bool
	//************************************
	//bool ParseIed(pugi::xml_node& sclNode);

	QString GetErrorMsg() { return m_errMsg; }
	//************************************
	// 函数名称:	GetCircuitListBySrcAndDest
	// 函数全名:	CircuitConfig::GetCircuitListBySrcAndDest
	// 访问权限:	public 
	// 函数说明:	根据源IED和目的IED名称获取链路列表，不区分方向
	// 函数参数:	const QString & srcIedName
	// 函数参数:	const QString & destIedName
	// 返回值:		QList<LogicCircuit*>
	//************************************
	QList<LogicCircuit*> GetCircuitListBySrcAndDest(const QString& srcIedName, const QString& destIedName)
	{
		QList<LogicCircuit*> ret;
		//foreach(LogicCircuit* pLogicCircuit, m_inLogicCircuitHash.values(destIedName))
		//{
		//	if (pLogicCircuit->pSrcIed->name == srcIedName)
		//		ret.append(pLogicCircuit);
		//}
		foreach(LogicCircuit* pLogicCircuit, m_outLogicCircuitHash.values(srcIedName))
		{
			if (pLogicCircuit->pDestIed->name == destIedName)
				ret.append(pLogicCircuit);
		}
		return ret;
	}

	QList<LogicCircuit*> GetInLogicCircuitListByIedName(const QString& iedName)
	{
		return m_inLogicCircuitHash.values(iedName);
	}

	QList<LogicCircuit*> GetOutLogicCircuitListByIedName(const QString& iedName)
	{
		return m_outLogicCircuitHash.values(iedName);
	}

	QList<LogicCircuit*> GetAllLogicCircuitListByIEDName(const QString& iedName)
	{
		return m_inLogicCircuitHash.values(iedName) + m_outLogicCircuitHash.values(iedName);
	}

	QList<VirtualCircuit*> GetAllVirtualCircuitListByIEDName(const QString& iedName)
	{
		return m_inVirtualCircuitHash.values(iedName) + m_outVirtualCircuitHash.values(iedName);
	}	

	QList<VirtualCircuit*> GetAllVirtualCircuitListByIEDPair(const QString& srcIedName, const QString& destIedName)
	{
		QList<VirtualCircuit*> result;
		QMultiHash<QString, VirtualCircuit*>::const_iterator it = m_outVirtualCircuitHash.find(srcIedName);
		while (it != m_outVirtualCircuitHash.end() && it.key() == srcIedName) {
			VirtualCircuit* pVC = it.value();
			if (pVC && pVC->destIedName == destIedName) {
				result.append(pVC);
			}
			++it;
		}
		return result;
	}

	QList<VirtualCircuit*> GetVirtualCircuitListByOpticalCode(quint64 opticalCode) const
	{
		if (opticalCode == 0)
		{
			return QList<VirtualCircuit*>();
		}
		return m_opticalVirtualCircuitHash.values(opticalCode);
	}

	QList<OpticalCircuit*> getOpticalByIeds(const QString& ied_1, const QString& ied_2) {
		// 获取指定两端设备的所有光纤链路
		if (ied_1.isEmpty() || ied_2.isEmpty())
			return QList<OpticalCircuit*>();
		QList<OpticalCircuit*> ret;
		ret += m_opticalCircuitHash.values(qMakePair(ied_1, ied_2));
		ret += m_opticalCircuitHash.values(qMakePair(ied_2, ied_1));
		return ret;
	}

	//************************************
	// 函数名称:	getOpticalByCode
	// 函数全名:	CircuitConfig::getOpticalByCode
	// 访问权限:	public
	// 函数说明:	根据光纤链路编号获取光纤链路
	// 函数参数:	quint64 code
	// 返回值:	OpticalCircuit*
	//************************************
	OpticalCircuit* getOpticalByCode(quint64 code) const;

	// 根据IED名称和交换机名称获取经过该交换机的逻辑链路
	//QList<LogicCircuit*> getLogicCircuitListByIedNameAndSw(const QString& iedName, const QString& swName)
	//{
	//	IED* pIed = GetIedByName(iedName);
	//	foreach(QString connectedIedName, pIed->connectedIedNameSet)
	//	{
	//		if (connectedIedName == swName)
	//			continue;
	//		QList<LogicCircuit*> list = GetCircuitListBySrcAndDest(iedName, connectedIedName);
	//		if (!list.isEmpty())  
	//			return list;
	//	}
	//	return QList<LogicCircuit*>();
	//}

	IED* GetIedByName(const QString& name)
	{
		return m_iedHash.value(name, NULL);
	}

	QList<IED*> GetIedList() { return m_iedList; }
protected:
	//// 辅助函数组

	// 解析SV、GSE的CIME文件
	bool LoadVirtualCircuitFile(xcime::Cime& cime, VirtualType type);
	//void SaveIedAsCime(xcime::Cime& cime, const IED* pIed);
	
	//************************************
	// 函数名称:	ParseFcda
	// 函数全名:	CircuitConfig::ParseFcda
	// 访问权限:	protected 
	// 函数说明:	解析DataSet下的FCDA节点
	// 函数参数:	pugi::xml_node & dataesetNode
	// 函数参数:	QString iedName
	// 函数参数:	DataSet * pDataSet
	// 返回值:		void
	//************************************
	//void ParseFcda(pugi::xml_node& dataesetNode, QString iedName, DataSet* pDataSet, pugi::xpath_query& query);
	//************************************
	// 函数名称:	ParsePhysConn
	// 函数全名:	CircuitConfig::ParsePhysConn
	// 访问权限:	protected 
	// 函数说明:	解析PhysConn节点
	// 函数参数:	pugi::xml_node & sclNode
	// 函数参数:	Circuit * pCircuit
	// 返回值:		void
	//************************************
	//void ParsePhysConn(pugi::xml_node& sclNode, Circuit* pCircuit);

	//************************************
	// 函数名称:	CompleteDataDesc
	// 函数全名:	CircuitConfig::CompleteDataDesc
	// 访问权限:	protected 
	// 函数说明:	基于dataset节点，通过pData的地址查询DAI节点，补全pData结构信息
	// 函数参数:	pugi::xml_node & datasetNode
	// 函数参数:	Data * pData
	// 返回值:		void
	//************************************
	//void CompleteDataDesc(pugi::xml_node& datasetNode, Data* pData);

	inline QString attrToQStr(const pugi::xml_node& node, const char* attr)
	{
		return QString::fromUtf8(node.attribute(attr).as_string());
	}
	// ldInst/(prefix)lnClasslnInst.doName.daName
	//Data GetDataByExtref(pugi::xml_node& extrefNode)
	//{
	//	Data data;
	//	data.ldInst = extrefNode.attribute("ldInst").as_string();
	//	data.lnClass = extrefNode.attribute("lnClass").as_string();
	//	data.lnInst = extrefNode.attribute("lnInst").as_string();
	//	data.fc = extrefNode.attribute("fc").as_string();
	//	data.doName = extrefNode.attribute("doName").as_string();
	//	data.daName = extrefNode.attribute("daName").as_string();
	//	data.prefix = extrefNode.attribute("prefix").as_string();
	//	return data;
	//}

	//inline QString combineExtrefAddr(pugi::xml_node& extrefNode)
	//{
	//	QString daName = extrefNode.attribute("daName") ? QString(".%1").arg(extrefNode.attribute("daName").as_string()) : "";
	//	return QString("%1/%5%2%3.%4%6")
	//		.arg(extrefNode.attribute("ldInst").as_string())
	//		.arg(extrefNode.attribute("lnClass").as_string())
	//		.arg(extrefNode.attribute("lnInst").as_string())
	//		.arg(extrefNode.attribute("doName").as_string())
	//		.arg(extrefNode.attribute("prefix").as_string())
	//		.arg(daName);
	//}
	//inline QString combineDataPath(Data* pData)
	//{
	//	return QString("%1/%2%3$%4$%5$%6")
	//		.arg(pData->pParent->ldInst)
	//		.arg(pData->lnClass)
	//		.arg(pData->lnInst)
	//		.arg(pData->fc)
	//		.arg(pData->doName)
	//		.arg(pData->daName);
	//}
	QString GetEndDescByIntAddr(pugi::xml_node& sclNode, QString& iedName, QString& intAddr)
	{
		// intAddr: "5-A:PISV/SVINGGIO1.DelayTRtg"
		QString path = intAddr.contains(':') ? intAddr.split(':').at(1) : intAddr;
		QString ldInst = path.split('/').at(0);
		QString doName, daName, lnInst;
		QStringList list = path.split('.');
		lnInst = list.at(0).right(1);
		doName = list.at(1);
		daName = list.size() > 2 ? list.at(2) : "";
		//qDebug() << QString("ldInst = %1, lnInst = %2, doName = %3, daName = %4").arg(ldInst).arg(lnInst).arg(doName).arg(daName);
		QString query = QString("//SCL/IED[@name='%1']/AccessPoint/Server/LDevice[@inst='%2']/LN[@inst='%3']").arg(iedName).arg(ldInst).arg(lnInst);
		pugi::xpath_node_set ln_set = sclNode.select_nodes(query.toLocal8Bit());
		for (nodeSetConstIterator it = ln_set.begin(); it != ln_set.end(); ++it)
		{
			pugi::xml_node node = it->node();
			QString curLnInst = attrToQStr(node, "inst");
			QString curLnClass = attrToQStr(node, "lnClass");
			QString curPrefix = attrToQStr(node, "prefix");
			if (path.contains(curLnClass) && path.contains(curPrefix) && path.contains(curLnInst))
			{
				query = QString("./DOI[@name='%1']").arg(doName);
				pugi::xpath_node doiResult = node.select_node(query.toLocal8Bit());
				if (doiResult)
				{
					return attrToQStr(doiResult.node(), "desc");
				}
			}
		}
		return "";
	}

private:

	//************************************
	// 函数名称:	buildOpticalRelation
	// 函数全名:	CircuitConfig::buildOpticalRelation
	// 访问权限:	private
	// 函数说明:	建立虚回路与光纤链路关系
	// 函数参数:	VirtualCircuit* pVtCircuit
	// 函数参数:	const IED* pSrcIed
	// 函数参数:	const IED* pDestIed
	// 返回值:	void
	//************************************
	void buildOpticalRelation(VirtualCircuit* pVtCircuit, const IED* pSrcIed, const IED* pDestIed);
	CircuitConfig();
	CircuitConfig(const CircuitConfig&);
	CircuitConfig& operator=(const CircuitConfig&);
	~CircuitConfig()
	{
		Clear();
	}
	//************************************
	// 函数名称:	setChlDesc
	// 函数全名:	CircuitConfig::setChlDesc
	// 访问权限:	private 
	// 函数说明:	根据实时库通道元素类型，将其描述写入虚回路源/目的端描述字段。
	//				isSrc为true时写入pVC->srcDesc，否则写入pVC->destDesc；
	//				依据pChl->eType进行安全向下转型并读取各派生类型的desc字段。
	// 函数参数:	VirtualCircuit* pVC			虚回路对象
	// 函数参数:	stuRtdbEle* pChl			实时库通道元素基类指针（实际类型由eType指示）
	// 函数参数:	bool isSrc					是否为源端描述（true源/false目的）
	// 返回值:		void
	// 备注:		未知类型不处理。
	//************************************
	void setChlDesc(VirtualCircuit* pVC, stuRtdbEle* pChl, bool isSrc)
	{
		if (!pChl) return;
		QString& desc = isSrc ? pVC->srcDesc : pVC->destDesc;
		switch (pChl->eType)
		{
		case CODE_TYPE_SECTOR:
		{
			stuRtdbSector* pSector = static_cast<stuRtdbSector*>(pChl);
			desc = QString::fromLocal8Bit(pSector->desc);
			break;
		}
		case CODE_TYPE_SETTING:
		{
			stuRtdbSetting* pSetting = static_cast<stuRtdbSetting*>(pChl);
			desc = QString::fromLocal8Bit(pSetting->desc);
			break;
		}
		case CODE_TYPE_ANALOG:
		{
			stuRtdbAnalog* pAnalog = static_cast<stuRtdbAnalog*>(pChl);
			desc = QString::fromLocal8Bit(pAnalog->desc);
			break;
		}
		case CODE_TYPE_STATUS:
		{
			stuRtdbStatus* pStatus = static_cast<stuRtdbStatus*>(pChl);
			desc = QString::fromLocal8Bit(pStatus->desc);
			break;
		}
		case CODE_TYPE_DETAIL:
		{
			stuRtdbDetail* pDetail = static_cast<stuRtdbDetail*>(pChl);
			desc = QString::fromLocal8Bit(pDetail->desc);
			break;
		}
		}
	}

	//************************************
	// 函数名称:	buildCircuitRelations
	// 函数全名:	CircuitConfig::buildCircuitRelations
	// 访问权限:	private 
	// 函数说明:	将单条虚回路与对应逻辑回路建立关联；若目标逻辑回路不存在则创建。
	//				维护IED互联关系、入/出逻辑链路与虚回路的多重映射，以及全局链路列表。
	//				当源/目的IED不存在时，记录错误并释放pVtCircuit。
	// 函数参数:	VirtualCircuit* pVtCircuit
	// 函数参数:	QHash<QString, LogicCircuit*>& logicCircuitHash
	// 返回值:		void
	//************************************
	void buildCircuitRelations(VirtualCircuit* pVtCircuit, QHash<QString, LogicCircuit*>& logicCircuitHash)
	{
		QString key = QString("%1:%2:%3").arg(pVtCircuit->destIedName).arg(pVtCircuit->srcCbName).arg(pVtCircuit->srcIedName);
		IED* pSrcIed = m_iedHash.value(pVtCircuit->srcIedName, NULL);
		IED* pDestIed = m_iedHash.value(pVtCircuit->destIedName, NULL);
		if (pSrcIed && pDestIed)
		{
			// 记录跨交换机的设备
			pSrcIed->connectedIedNameSet.insert(pDestIed->name);
			pDestIed->connectedIedNameSet.insert(pSrcIed->name);
			buildOpticalRelation(pVtCircuit, pSrcIed, pDestIed);
		}
		else
		{
			m_errMsg += QString("VirtualCircuit %1:%2 not found in IED list").arg(pVtCircuit->srcIedName).arg(pVtCircuit->destIedName);
			delete pVtCircuit;
			return;
		}
		LogicCircuit* pLogicCircuit = logicCircuitHash.value(key, NULL);
		if (!pLogicCircuit)
		{
			pLogicCircuit = new LogicCircuit;
			pLogicCircuit->type = pVtCircuit->type;
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
		if (pVtCircuit->leftOpticalCode != 0)
		{
			m_opticalVirtualCircuitHash.insertMulti(pVtCircuit->leftOpticalCode, pVtCircuit);
		}
		if (pVtCircuit->rightOpticalCode != 0 &&
			pVtCircuit->rightOpticalCode != pVtCircuit->leftOpticalCode)
		{
			m_opticalVirtualCircuitHash.insertMulti(pVtCircuit->rightOpticalCode, pVtCircuit);
		}
	}
	// 用于解析物理连接节点信息 name1:port1_name2:port2
	struct CableInfo
	{
		struct IedInfo
		{
			QString name;
			QString port;
		};
		std::ptrdiff_t GetOffsetByIedName(QString iedName) const
		{
			return leftIedInfo.name == iedName ? 
				0 : rightIedInfo.name == iedName ? 
				1 : -1;
		}
		IedInfo* GetIedInfoByOffset(const std::ptrdiff_t offset)
		{
			if (offset < 0 || offset > 1) return NULL;
			return reinterpret_cast<IedInfo*>(reinterpret_cast<char*>(this) + offset * sizeof(IedInfo));
		}
		IedInfo leftIedInfo;
		IedInfo rightIedInfo;
	};
	//************************************
	// 函数名称:	GetCableListByQuery
	// 函数全名:	CircuitConfig::GetCableListByQuery
	// 访问权限:	private 
	// 函数说明:	根据xpath查询语句，获取指定的物理链路信息（已去重）
	// 函数参数:	pugi::xml_node & root
	// 函数参数:	QString & query
	// 返回值:		QList<CircuitConfig::CableInfo>
	//************************************
	QList<CableInfo> GetCableListByQuery(pugi::xml_node& root, pugi::xpath_query& query)
	{
		QList<CableInfo> res;
		pugi::xpath_node_set node_set = query.evaluate_node_set(root);
		QStringList temp;
		for (nodeSetConstIterator it = node_set.begin(); it != node_set.end(); ++it)
		{
			QString nodeText = it->node().text().as_string();
			if (temp.contains(nodeText)) continue;
			CableInfo cable = ParseCableInfo(nodeText);
			temp.append(nodeText);
			res.append(cable);
		}
		return res;
	}
	inline CableInfo ParseCableInfo(QString cableText)
	{
		CableInfo result;
		if (!cableText.contains('_') || !cableText.contains(':'))
		{
			qDebug() << __FILE__ << ":" << __LINE__ << "CableText格式错误：" << cableText;
			return result;
		}
		QStringList list = cableText.split('_');
		QStringList left = list.at(0).split(':');
		QStringList right = list.at(1).split(':');
		result.leftIedInfo.name = left.at(0);
		result.leftIedInfo.port = left.at(1);
		result.rightIedInfo.name = right.at(0);
		result.rightIedInfo.port = right.at(1);
		return result;
	}

private:
	QString m_errMsg;
	QString m_cimeDir;
	RtdbClient& m_rtdb;
	const CRtdbEleModelStation* m_stationModel;
	// =======================================================================================================================
	// 数据源，参与内存管理
	QList<IED*> m_iedList;
	QList<Data*> m_dataList;
	QList<VirtualCircuit*> m_virtualCircuitList;
	QList<OpticalCircuit*> m_opticalCircuitList;
	QList<LogicCircuit*> m_logicCircuitList;

	// =======================================================================================================================
	// 关系数据，不参与内存管理
	QHash<QString, IED*> m_iedHash;
	QMultiHash<QPair<QString, QString>, OpticalCircuit*> m_opticalCircuitHash;	// <srcIedName, destIedName> - OpticalCircuit
	//ied_ldInst_hash m_dataPathHash;			// 数据映射表，用于根据数据路径查找具体数据
	//DataSetHash m_dataSetHash;		// 数据集映射表，用于判断数据集所属控制块关系，通过ldInst和datasetName查找
	// LogicCircuit数据源由BaseCommCB管理
	QMultiHash<QString, LogicCircuit*> m_inLogicCircuitHash;		// 对于键的IedName为入链路
	QMultiHash<QString, LogicCircuit*> m_outLogicCircuitHash;		// 对于键的IedName为出链路
	QMultiHash<QString, VirtualCircuit*> m_inVirtualCircuitHash;	// 输出设备的关联链路
	QMultiHash<QString, VirtualCircuit*> m_outVirtualCircuitHash;	// 输入设备的关联链路
	QMultiHash<quint64, VirtualCircuit*> m_opticalVirtualCircuitHash;
};
