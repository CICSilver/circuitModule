#ifndef _YSD_RTDB_SHM_ELE_H_
#define _YSD_RTDB_SHM_ELE_H_

#include "YsdRtdbDefine.h"

#define RTDB_SHM_ID 8150
#define RTDB_SHM_SIZEB 209715200 //200MB

#pragma pack(push)
#pragma pack(1)

struct stuRtdbHead
{
	stuRtdbHead()
	{
		iedNum = 0;
		memset(res,0x00,256);
	}
	int iedNum;
#ifndef WIN32
	pthread_rwlock_t rwLock; //读写锁
#endif
	BYTE res[256];//备用
};


struct stuCodeAddr
{
	stuCodeAddr()
	{
		code = 0;
		addrOff = 0;
	}
	INT64 code;//标识
	UINT64 addrOff;//内存地址(相对于共享内存首地址的偏移)
};

struct stuRtdbIED
{
	stuRtdbIED()
	{
		code = 0;
		id = 0;
		memset(iedName,0x00,LEN_16);
		memset(iedDesc,0x00,LEN_64);
		memset(iedModel,0x00,LEN_32);
		memset(iedMfr,0x00,LEN_16);
		memset(iedVer,0x00,LEN_16);
		memset(iedCcdCrc,0x00,LEN_16);
		memset(iedIcdCrc,0x00,LEN_16);
		memset(ipA,0x00,LEN_32);
		memset(ipB,0x00,LEN_32);
		LengerNum = 0;
		cpuNum = 0;
		sectorNum = 0;
		groupNum = 0;
		rptNum = 0;
		ctlNum = 0;

		pLenger = NULL;
		pCpu = NULL;
		pSector = NULL;
		pGroup = NULL;
		pRpt = NULL;
		pCtl = NULL;
	}
	UINT64 code;
	int id;//IED编号
	char iedName[LEN_16];//IEDNAME
	char iedDesc[LEN_64];//IED描述
	char iedModel[LEN_32];//型号
	char iedMfr[LEN_16];//生产厂商
	char iedVer[LEN_16];//配置版本
	char iedCcdCrc[LEN_16];//CCD校验码
	char iedIcdCrc[LEN_16];//ICD校验码
	char ipA[LEN_32];
	char ipB[LEN_32];

	int LengerNum;
	stuCodeAddr *pLenger;
	int cpuNum;
	stuCodeAddr *pCpu;
	int sectorNum;
	stuCodeAddr *pSector;
	int groupNum;
	stuCodeAddr *pGroup;
	int rptNum;
	stuCodeAddr *pRpt;
	int ctlNum;
	stuCodeAddr *pCtl;
};
struct stuRtdbIEDHead
{
	stuRtdbIEDHead()
	{
		code = 0;
		id = 0;
		memset(iedName,0x00,LEN_16);
		memset(iedDesc,0x00,LEN_64);
		memset(iedModel,0x00,LEN_32);
		memset(iedMfr,0x00,LEN_16);
		memset(iedVer,0x00,LEN_16);
		memset(iedCcdCrc,0x00,LEN_16);
		memset(iedIcdCrc,0x00,LEN_16);
		memset(ipA,0x00,LEN_32);
		memset(ipB,0x00,LEN_32);
	}
	UINT64 code;
	int id;//IED编号
	char iedName[LEN_16];//IEDNAME
	char iedDesc[LEN_64];//IED描述
	char iedModel[LEN_32];//型号
	char iedMfr[LEN_16];//生产厂商
	char iedVer[LEN_16];//配置版本
	char iedCcdCrc[LEN_16];//CCD校验码
	char iedIcdCrc[LEN_16];//ICD校验码
	char ipA[LEN_32];
	char ipB[LEN_32];
};
struct stuRtdbIedBody
{
	stuRtdbIedBody()
	{
		LengerNum = 0;
		cpuNum = 0;
		sectorNum = 0;
		groupNum = 0;
		rptNum = 0;
		ctlNum = 0;

		pLenger = NULL;
		pCpu = NULL;
		pSector = NULL;
		pGroup = NULL;
		pRpt = NULL;
		pCtl = NULL;
	}
	int LengerNum;
	stuCodeAddr *pLenger;
	int cpuNum;
	stuCodeAddr *pCpu;
	int sectorNum;
	stuCodeAddr *pSector;
	int groupNum;
	stuCodeAddr *pGroup;
	int rptNum;
	stuCodeAddr *pRpt;
	int ctlNum;
	stuCodeAddr *pCtl;
};

//台账条目
struct stuRtdbLenger
{
	stuRtdbLenger()
	{
		code = 0;
		memset(desc,0x00,LEN_32);
		memset(ref,0x00,LEN_64);
	}
	UINT64 code;//编号
	char desc[LEN_32];//描述
	char ref[LEN_64];//路径
};

struct stuRtdbCpu
{
	stuRtdbCpu()
	{
		code = 0;
		cpuno = 0;
		pcpuno = 0;
		memset(ver,0x00,LEN_16);
		memset(desc,0x00,LEN_32);
	}
	UINT64 code;//编号
	BYTE cpuno;//CPU号
	BYTE pcpuno;//装置CPU号
	char ver[LEN_16];//版本
	char desc[LEN_32];//描述
};

struct stuRtdbSector
{
	stuRtdbSector()
	{
		code = 0;
		cpuno = 0;
		groupNo = 0;
		entryNo = 0;
		pentryNo = 0;
		valType = 0;
		memset(minVal,0x00,LEN_VAL);
		memset(maxVal,0x00,LEN_VAL);
		memset(stepSize,0x00,LEN_VAL);
		memset(desc,0x00,LEN_32);
		memset(ref,0x00,LEN_64);
	}
	UINT64 code;//编号
	BYTE cpuno;//cpu号
	BYTE groupNo;//组号
	BYTE entryNo;//条目号
	BYTE pentryNo;//装置条目号
	BYTE valType;//值类型
	char minVal[LEN_VAL];//最小值 以字符串方式存储值  例如浮点型1.234存储为字符串1.234'\0'形式
	char maxVal[LEN_VAL];
	char stepSize[LEN_VAL];
	char desc[LEN_32];//描述
	char ref[LEN_64];//路径
};
struct stuRtdbSetting
{
	stuRtdbSetting()
	{
		code = 0;
		cpuno = 0;
		groupNo = 0;
		entryNo = 0;
		pentryNo = 0;
		valType = 0;
		scaleFactor = 1.0f;
		scaleOffset = 0.0f;
		memset(minVal,0x00,LEN_VAL);
		memset(maxVal,0x00,LEN_VAL);
		memset(stepSize,0x00,LEN_VAL);
		precisionn = 0;
		precisionm = 0;
		memset(dime,0x00,LEN_16);
		memset(desc,0x00,LEN_32);
		memset(ref,0x00,LEN_64);
		memset(Val,0x00,MAX_SECTOR*LEN_VAL);
	}
	UINT64 code;//编号
	BYTE cpuno;//cpu号
	BYTE groupNo;//组号
	BYTE entryNo;//条目号
	BYTE pentryNo;//装置条目号
	BYTE valType;//值类型
	float scaleFactor;//系数
	float scaleOffset;//偏移
	char minVal[LEN_VAL];//最小值 以字符串方式存储值  例如浮点型1.234存储为字符串1.234'\0'形式
	char maxVal[LEN_VAL];
	char stepSize[LEN_VAL];
	BYTE precisionn;//精度N 精度(n.m) n-整数部分最大位数，m-小数部分最大位数
	BYTE precisionm;//精度M
	char dime[LEN_16];//单位
	char desc[LEN_32];//描述
	char ref[LEN_64];//路径

	char Val[MAX_SECTOR][LEN_VAL];//对应定值区(1~32)的定值
};
struct stuRtdbAnalog
{
	stuRtdbAnalog()
	{
		code = 0;
		cpuno = 0;
		groupNo = 0;
		entryNo = 0;
		pentryNo = 0;
		valType = 0;
		scaleFactor = 1.0f;
		scaleOffset = 0.0f;
		memset(minVal,0x00,LEN_VAL);
		memset(maxVal,0x00,LEN_VAL);
		memset(dime,0x00,LEN_16);
		memset(desc,0x00,LEN_32);
		memset(ref,0x00,LEN_64);
		memset(Val,0x00,LEN_VAL);
	}
	UINT64 code;//编号
	BYTE cpuno;//cpu号
	BYTE groupNo;//组号
	BYTE entryNo;//条目号
	BYTE pentryNo;//装置条目号
	BYTE valType;//值类型
	float scaleFactor;//系数
	float scaleOffset;//偏移
	char minVal[LEN_VAL];//最小值 以字符串方式存储值  例如浮点型1.234存储为字符串1.234'\0'形式
	char maxVal[LEN_VAL];
	char dime[LEN_16];//单位
	char desc[LEN_32];//描述
	char ref[LEN_64];//路径

	char Val[LEN_VAL];//模拟量值
};
struct stuRtdbStatus
{
	stuRtdbStatus()
	{
		code = 0;
		cpuno = 0;
		groupNo = 0;
		entryNo = 0;
		fun = 0;
		pfun = 0;
		inf = 0;
		pinf = 0;
		type = 0;
		memset(desc,0x00,LEN_32);
		memset(ref,0x00,LEN_64);
		memset(Val,0x00,LEN_VAL);
	}
	UINT64 code;//编号
	BYTE cpuno;//cpu号
	BYTE groupNo;//组号
	BYTE entryNo;//条目号
	BYTE fun;//功能类型
	BYTE pfun;//装置的功能类型
	BYTE inf;//信息序号
	BYTE pinf;//装置的信息序号
	BYTE type;//状态量类型 eStateType
	char desc[LEN_32];//描述
	char ref[LEN_64];//路径

	char Val[LEN_VAL];//状态值
};
struct stuRtdbGroup
{
	stuRtdbGroup()
	{
		code = 0;
		cpuno = 0;
		groupNo = 0;
		pgroupNo = 0;
		groupType = 0;
		memset(desc,0x00,LEN_32);
		memset(ref,0x00,LEN_64);
		chlNum = 0;
		pChl = NULL;
	}
	UINT64 code;//编号
	BYTE cpuno;	//CPU号
	BYTE groupNo;//组号
	BYTE pgroupNo;//装置组号
	BYTE groupType;//组类型 InfoType
	char desc[LEN_32];//描述
	char ref[LEN_64];//路径

	int chlNum;
	stuCodeAddr *pChl;
};
struct stuRtdbGroupHead
{
	stuRtdbGroupHead()
	{
		code = 0;
		cpuno = 0;
		groupNo = 0;
		pgroupNo = 0;
		groupType = 0;
		memset(desc,0x00,LEN_32);
		memset(ref,0x00,LEN_64);
	}
	UINT64 code;//编号
	BYTE cpuno;	//CPU号
	BYTE groupNo;//组号
	BYTE pgroupNo;//装置组号
	BYTE groupType;//组类型 InfoType
	char desc[LEN_32];//描述
	char ref[LEN_64];//路径
};
struct stuRtdbGroupBody
{
	stuRtdbGroupBody()
	{
		chlNum = 0;
		pChl = NULL;
	}
	int chlNum;
	stuCodeAddr *pChl;
};
struct stuRtdbRptCtrl
{
	stuRtdbRptCtrl()
	{
		code = 0;
		memset(domName,0x00,LEN_32);
		memset(dsName,0x00,LEN_32);
		memset(ref,0x00,LEN_64);
		memset(rptID,0x00,LEN_32);
		optFlds = 0;
		trpOps = 0;
		intgPd = 60;
	}
	UINT64 code;//编号
	char domName[LEN_32];//逻辑设备名称
	char dsName[LEN_32];//数据集名称
	char ref[LEN_64];//报告路径
	char rptID[LEN_32];//报告ID
	WORD optFlds;//报告可选域
	BYTE trpOps;//触发原因
	int intgPd;//周期(ms)
};
struct stuRtdbCtrlOper
{
	stuRtdbCtrlOper()
	{
		code = 0;
		memset(domName,0x00,LEN_32);
		memset(ref,0x00,LEN_64);
		ctlModel = 0;
		orCat = 0;
		memset(cdc,0x00,LEN_16);
	}
	UINT64 code;//编号
	char domName[LEN_32];//逻辑设备名称
	char ref[LEN_64];//控点路径
	UINT32 ctlModel;//控制模式
	int orCat;//标识控制源
	char cdc[LEN_16];//控点类型
};


#pragma pack()
#pragma pack(pop)

class CRtdbEleModelGroup
{
public:
	CRtdbEleModelGroup()
	{
	}
	~CRtdbEleModelGroup()
	{
		Clear();
	}

	void Clear()
	{
		m_mapChl.clear();
	}
public:
	eChItemType m_eChlType;//通道类型
	stuRtdbGroupHead *m_pGroupHead;
	std::map<UINT64,void*> m_mapChl;//数据集通道定义
};
class CRtdbEleModelIed
{
public:
	CRtdbEleModelIed()
	{
	}
	~CRtdbEleModelIed()
	{
		Clear();
	}

	void Clear();

	void* GetRtdbChl(UINT64 codeChl, eChItemType& eType);//根据标识获取数据集通道 NULL:无法找到 eType:返回的通道类型
public:
	stuRtdbIEDHead *m_pIedHead;
	std::map<UINT64,stuRtdbLenger*> m_mapLenger;
	std::map<UINT64,stuRtdbCpu*> m_mapCpu;
	std::map<UINT64,stuRtdbSector*> m_mapSector;
	std::map<UINT64,CRtdbEleModelGroup*> m_mapGroup;
	std::map<UINT64,stuRtdbRptCtrl*> m_mapRpt;
	std::map<UINT64,stuRtdbCtrlOper*> m_mapOper;
};



#endif