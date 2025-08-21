#ifndef _YSD_RTDB_DEFINE_H_
#define _YSD_RTDB_DEFINE_H_

#include "YsdRtdbInclude.h"

typedef enum eCodeType //编码类型
{
	CODE_TYPE_IED		=	0,//IED
	CODE_TYPE_LEDGER	=	1,//台帐
	CODE_TYPE_CPU		=	2,//CPU号
	CODE_TYPE_GROUP		=	3,//数据集
	CODE_TYPE_SECTOR	=	4,//定值区
	CODE_TYPE_SETTING	=	5,//定值
	CODE_TYPE_ANALOG	=	6,//模拟量
	CODE_TYPE_STATUS	=	7,//状态量
	CODE_TYPE_RCB		=	8,//报告块
	CODE_TYPE_CTL		=	9,//控制块
}CodeType;


typedef enum eInfoType
{
	InfoNone = 0,
	InfoSector = 1,		//定值区号 
	InfoSetting = 2,	//定值 
	InfoAnalog = 3,		//模拟量 
	InfoSwitch = 4,		//开关量 
	InfoRyaban = 5,		//软压板 
	InfoAlert = 6,		//告警 
	InfoFault = 7,		//故障 
	InfoFaultInfo = 8,	//故障信息
}InfoType;

typedef enum eChItemType
{
	ChItemNone	= 0,//初始化
	ChItemSector = 1,//定值区
	ChItemSetting = 2,//定值通道
	ChItemAnalog = 3,//模拟量通道
	chItemStatus = 4,//状态量通道
}ChlItemType;

typedef enum eValueType
{
	VALTYP_NONE			= 0,
	VALTYP_STRING		= 1,
	VALTYP_BITSTRING	= 2,//BS1成组8位串（一般2字节16位）
	VALTYP_UINT			= 3,
	VALTYP_INT			= 4,
	VALTYP_FLOAT		= 7,
}ValueType;


/**
 * 状态量类型主要涉及信息显示时所采用字符串，类型定义：
 *	0: 开关量；
 *	1: 保护动作信号；
 *  2: 软压板
 *	3: 装置自检告警
*/
typedef enum eStateType
{//状态量类型
	StateSwitch = 0,//开关量
	StateEvent = 1,//保护动作信号
	StateRyaban = 2,//软压板
	StateAlert = 3,//装置自检告警
}StateType;


#endif