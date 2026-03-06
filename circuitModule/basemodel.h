#pragma once
#include <QString>
#include <QStringList>
#include <QList>
#include <QMap>
#include <QDebug>
#include "../../rtdb_dll/YsdRtdbEle.h"
struct IED;
struct Data;
//struct DataSet;
struct GSEControl;
struct SMVControl;
struct LogicCircuit;
struct BaseCommCB;
struct BaseControl;

// 虚回路类型
enum VirtualType
{
	GOOSE = 0,
	SV
};

// 连接状态类型
enum OptLinkStatusType
{
	LS_NORMAL = 1,		// 正常值
	LS_INVERTED,		// 取反值
	LS_VTDERIVED		// 虚回路衍生
};

// 光纤实回路
struct OpticalCircuit
{
	quint64 code;					// 光纤链路ID	从1开始
	quint64 loopCode;				// 光纤环路编号，loopName的#后部分
	QString loopName;			// 光纤名称，如：光纤线#环路编号
	QString srcIedName;			// 源设备别名
	QString destIedName;		// 目标设备别名
	QString srcIedPort;			// 源设备端口
	QString destIedPort;		// 目标设备端口
	QString srcIedDesc;			// 源设备描述
	QString destIedDesc;		// 目标设备描述
	//QString cableDesc;			// 线缆描述，如 PL2205LA:5-C_IL2202ALM:1-A
	QString reference;			// 路径，如 IL2202ALM/G1/1-A
	OptLinkStatusType linkStatusType;	// 光纤链路状态类型
	quint64 remoteId;
	bool connStatus;
	IED* pSrcIed;				// 源设备
	IED* pDestIed;				// 目标设备
	//QMap<IED*, QList<VirtualCircuit*>> vtCircuitMap;	// 若为直连则直接存储对侧设备的链路；若经过交换机，则存储所有通过该交换机连接的设备的链路，映射到设备下
};

// GSE/SV虚回路通道
struct VirtualCircuit
{
	explicit VirtualCircuit(VirtualType _type)
		: type(_type)
	{
		srcName = "";
		outRemoteCode = -1;
		inRemoteCode = -1;
		val = _type == GOOSE ? 0.00 : 57.73;
		connStatus = false;
		leftOpticalCode = 0;
		rightOpticalCode = 0;
	}
	VirtualType type;				// 链路类型
	quint64 code;					// 虚回路编号
	quint64 linkCode;				// 链路状态ID
	quint64 leftOpticalCode;		// 左侧光纤链路ID
	quint64 rightOpticalCode;		// 右侧光纤链路ID
	QString switchIedName;		// 交换机名称
	QString srcIedName;				// 源设备别名
	QString destIedName;			// 目标设备别名
	QString srcIedDesc;				// 源设备描述
	QString destIedDesc;			// 目标设备描述
	QString srcName;				// 输出名称
	QString destName;				// 输入名称
	QString srcDesc;				// 输出别名
	QString destDesc;				// 输入别名
	QString srcRef;					// 输出路径，如PL2205LAPIGO/B1PTRC1.ST.StrBF.PhsA
	QString destRef;				// 输入路径
	quint64 srcSoftPlateCode;		// 开出软压板ID
	QString srcSoftPlateDesc;		// 开出软压板描述，当type为SV时不使用
	QString srcSoftPlateRef;		// 开出软压板路径
	quint64 destSoftPlateCode;		// 开入软压板ID
	QString destSoftPlateDesc;		// 开入软压板描述
	QString destSoftPlateRef;		// 开入软压板路径
	QString srcCbName;				// 输出控制块名称
	quint64 outRemoteCode;			// 输出遥信
	quint64 inRemoteCode;			// 输入遥信
	//QString remoteCode;				// 当type为Goose时，代表Goose端子遥信ID；
									// 当type为SV时，代表SV端子遥测ID
	LogicCircuit* pLogicCircuit;	// 所属逻辑链路
	stuRtdbGseCircuit* pRtdbCircuit;// 实时库对应的GSE链路指针
	float val;						// 原始值
	bool connStatus;				// 通断状态
	
};

struct IED
{
	quint64 code;
	int id;								// 设备ID
	QString name;						// 设备名称
	QString type;						// 设备型号
	QString desc;						// 设备描述
	QString configVersion;				// 配置版本
	QString manufacturer;				// 厂商
	QString version;
	QString ccd;						// CCD校验码
	QString icd;						// ICD校验码
	QSet<QString> connectedIedNameSet;	// 链路关联IED设备列表
	QList<OpticalCircuit*> optical_list;
};

struct LogicCircuit
{
	IED* pSrcIed;
	IED* pDestIed;
	VirtualType type;					
	QString cbName;						// 输出控制块名称
	QList<VirtualCircuit*> circuitList;	// 通道列表
};

// 通信控制块基类
struct BaseCommCB
{
	virtual ~BaseCommCB()
	{
		qDeleteAll(logicCircuitList);
		logicCircuitList.clear();
	}
	inline QString key() const { return QString("%1:%2:%3").arg(cbName).arg(ldInst).arg(apName); }
	QString apName;
	QString subnetworkName;
	QString ldInst;
	QString cbName;
	// Address
	QString macAddr;
	quint16 appid;	// GSE网络应用ID，如: 0x0050
	quint16 vlanid;
	quint16 vlanPriority;
	VirtualType cbType;
	QList<LogicCircuit*> logicCircuitList;
	IED* pIed;	// 从属IED
};
struct SMV : public BaseCommCB
{
	SMVControl* pSvcb;
};
struct GSE : public BaseCommCB
{
	quint16 minTime;
	quint16 maxTime;
	GSEControl* pGocb;
};