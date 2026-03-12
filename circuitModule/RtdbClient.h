#ifndef RTDB_CLIENT_H
#define RTDB_CLIENT_H

/**
 * @file RtdbClient.h
 * @brief RTDB 实时库读数轻量封装（仅使用 RTDB_GetEle，不依赖 CRtdbEleModelIed）
 *
 * =========================
 * 1. 简介
 * =========================
 * - 本类封装 RTDB SDK 的常用读数流程，提供“按通道编码(code)读取”模拟量/状态量/定值的便捷接口；
 * - 通过 refresh() 在需要时重新获取 RTDB 数据，首次调用自动执行初始化；
 * - 支持对模拟量/定值按 value*scaleFactor + scaleOffset 应用系数/偏移的可选修正；
 * - 不依赖类成员函数 CRtdbEleModelIed::GetRtdbChl，避免链接符号缺失问题。
 *
 * =========================
 * 2. 使用说明
 * =========================
 *   RtdbClient& client = RtdbClient::Instance();
 *   if (!client.refresh()) { // 刷新失败的处理 }
 *
 *   // 读取模拟量（可选缩放）
 *   double v = 0.0, k = 0.0, b = 0.0; QString unit;
 *   if (client.getAnalog(123456789ULL, v, true, &k, &b, &unit)) {
 *       // v = 原值*scaleFactor(k) + scaleOffset(b)
 *   }
 *
 *   // 读取状态量
 *   QString s;
 *   client.getStatus(223456789ULL, s);
 *
 *   // 读取定值（按定值区 1..32）
 *   double z = 0.0;
 *   client.getSetting(323456789ULL, z, sectorIndex=1, applyScale=true);
 *
 * =========================
 * 3. 依赖与环境
 * =========================
 * - 依赖 SDK 头文件：include/rtdb/rtdb_dll.h, YsdRtdbDefine.h, YsdRtdbEle.h；
 * - 依赖 Qt 基础类型：QString；
 * - Windows/VS 项目中需正确链接 RTDB SDK 对应的库与运行环境（.lib/.dll 位数需与工程一致）。
 *
 * =========================
 * 4. 注意事项
 * =========================
 * - 本封装的读取均为“拷贝读取”，不会返回共享内存原始指针；线程安全、可重入；
 * - scaleFactor/scaleOffset 含义：工程量换算，工程值 = 原始值*scaleFactor + scaleOffset；
 * - code 必须是 RTDB 内已存在的通道编码；若无命中或类型不匹配，函数返回 false。
 */

#include <QString>
#include "../../rtdb_dll/rtdb_dll.h"      // RTDB_Open, RTDB_GetEle
#include "../../rtdb_dll/YsdRtdbEle.h"    // stuRtdbAnalog, stuRtdbStatus, stuRtdbSetting
#include "../../ysd_rtdb_include/YsdRtdbDefine.h" // eCodeType

class RtdbClient
{
public:
	static RtdbClient& Instance();

	//************************************
	// 函数名称:  refresh
	// 函数全名:  bool RtdbClient::refresh(RtdbOpenType eType)
	// 访问权限:  public
	// 函数说明:  尝试从 RTDB 重新获取数据，首次调用亦执行初始化。
	// 参数说明:  RtdbOpenType eType  [in ] 刷新模式，默认 RTDB_OPEN_RW。
	// 返回值:    bool                  true 刷新/初始化成功；false 连接失败。
	//************************************
	bool refresh(RtdbOpenType eType = RTDB_OPEN_RW);
	//************************************
	// 函数名称:  isOpen
	// 函数全名:  bool RtdbClient::isOpen() const
	// 访问权限:  public
	// 函数说明:  查询内部打开标志。
	// 参数:      无
	// 返回值:    bool                  true 已打开；false 未打开。
	//************************************
	bool isOpen() const { return m_opened; }
	const CRtdbEleModelStation* stationModel() const;
	const CRtdbEleModelCircuit* circuitModel() const;

	//************************************
	// 函数名称:  getAnalog
	// 函数全名:  bool RtdbClient::getAnalog(qulonglong, double&, bool, double*, double*, QString*) const
	// 访问权限:  public
	// 函数说明:  按 code 读取模拟量。Val 为字符串，转换为 double；可选应用系数/偏移。
	// 函数参数:  qulonglong code       [in ] 通道编码（UINT64）。
	// 函数参数:  double& outValue      [out] 数值（可已应用修正）。
	// 函数参数:  bool applyScale       [in ] 是否按 v = v*k + b 修正，默认 false。
	// 函数参数:  double* scaleFactorOut[out] 可选，返回系数 k。
	// 函数参数:  double* scaleOffsetOut[out] 可选，返回偏移 b。
	// 函数参数:  QString* unitOut      [out] 可选，返回单位字符串。
	// 返回值:    bool                   true 成功；false code 不存在或类型不匹配。
	//************************************
	bool getAnalog(qulonglong code, double& outValue, bool applyScale = false,
		double* scaleFactorOut = 0, double* scaleOffsetOut = 0, QString* unitOut = 0) const;
	stuRtdbAnalog* getAnalog(qulonglong code) const;

	//************************************
	// 函数名称:  getStatus
	// 函数全名:  bool RtdbClient::getStatus(qulonglong, QString&) const
	// 访问权限:  public
	// 函数说明:  按 code 读取状态量，直接返回字符串 Val。
	// 函数参数:  qulonglong code       [in ] 通道编码。
	// 函数参数:  QString& outValue     [out] 状态字符串。
	// 返回值:    bool                   true 成功；false code 不存在或类型不匹配。
	//************************************
	bool getStatus(qulonglong code, QString& outValue) const;

	//************************************
	// 函数名称:  getSetting
	// 函数全名:  bool RtdbClient::getSetting(qulonglong, double&, int, bool) const
	// 访问权限:  public
	// 函数说明:  按 code 读取定值。定值按区(1..32)存储，Val[sectorIndex-1] 为字符串；可选应用系数/偏移。
	// 函数参数:  qulonglong code       [in ] 通道编码。
	// 函数参数:  double& outValue      [out] 数值。
	// 函数参数:  int sectorIndex       [in ] 定值区索引(1..32)，默认 1。
	// 函数参数:  bool applyScale       [in ] 是否按 v = v*k + b 修正，默认 false。
	// 返回值:    bool                   true 成功；false code 不存在或类型不匹配。
	//************************************
	bool getSetting(qulonglong code, double& outValue, int sectorIndex = 1, bool applyScale = false) const;

	stuRtdbStatus* getRyb(qulonglong code) const;
	stuRtdbSvCircuit* getSvCircuit(qulonglong code) const;
	stuRtdbGseCircuit* getGseCircuit(qulonglong code) const;
	stuRtdbRealCircuit* getRealCircuit(qulonglong code) const;

	const char* getDesc(stuRtdbEle* ele) const;
	const char* getVal(stuRtdbEle* ele) const;

	// ************************************
	// code 读写操作
	// code 的二进制布局（小端，对应 SDK 的 stuItemCode/ItemCode）：
	// bits  0..15  : chl  (unsigned short)
	// bits 16..23  : ds   (unsigned char)
	// bits 24..31  : type (unsigned char) -> eCodeType
	// bits 32..47  : ied  (unsigned short)
	// bits 48..63  : res  (unsigned short)
	// 提供纯位运算解码/编码，避免依赖内存布局和对齐差异。
	struct itemCode
	{
		unsigned short chl;   // 通道编码
		unsigned char  ds;    // 数据集编码
		unsigned char  type;  // 类型（eCodeType）
		unsigned short ied;   // IED 编码
		unsigned short res;   // 预留/项类型
		itemCode() : chl(0), ds(0), type(0), ied(0), res(0)
		{
		}

		// 从 64 位整型解析
		static itemCode fromU64(qulonglong c)
		{
			itemCode p;
			p.chl  = static_cast<unsigned short>( c        & 0xFFFFULL);
			p.ds   = static_cast<unsigned char >( (c >> 16) & 0xFFULL );
			p.type = static_cast<unsigned char >( (c >> 24) & 0xFFULL );
			p.ied  = static_cast<unsigned short>( (c >> 32) & 0xFFFFULL );
			p.res  = static_cast<unsigned short>( (c >> 48) & 0xFFFFULL );
			return p;
		}
		// 打包为 64 位整型
		qulonglong toU64() const
		{
			qulonglong c = 0;
			c |= static_cast<qulonglong>(chl  & 0xFFFFu);
			c |= static_cast<qulonglong>(ds   & 0xFFu)   << 16;
			c |= static_cast<qulonglong>(type & 0xFFu)   << 24;
			c |= static_cast<qulonglong>(ied  & 0xFFFFu) << 32;
			c |= static_cast<qulonglong>(res  & 0xFFFFu) << 48;
			return c;
		}

	// code.ied=..; code.ds=..; code.chl=..; code.type=..; code.res=..

		// 调试输出
		QString toString() const
		{
			const char* typeName = "";
			switch (static_cast<eCodeType>(type))
			{
			case CODE_TYPE_IED:     typeName = "IED"; break;
			case CODE_TYPE_LEDGER:  typeName = "LEDGER"; break;
			case CODE_TYPE_CPU:     typeName = "CPU"; break;
			case CODE_TYPE_GROUP:   typeName = "GROUP"; break;
			case CODE_TYPE_SECTOR:  typeName = "SECTOR"; break;
			case CODE_TYPE_SETTING: typeName = "SETTING"; break;
			case CODE_TYPE_ANALOG:  typeName = "ANALOG"; break;
			case CODE_TYPE_STATUS:  typeName = "STATUS"; break;
			case CODE_TYPE_RCB:     typeName = "RCB"; break;
			case CODE_TYPE_CTL:     typeName = "CTL"; break;
			default:                typeName = "UNKNOWN"; break;
			}
			return QString("ied=%1 ds=%2 type=%3(%4) chl=%5 res=%6")
				.arg(ied).arg(ds).arg(type).arg(typeName).arg(chl).arg(res);
		}
	};
private:
	RtdbClient();
	~RtdbClient();

	RtdbClient(const RtdbClient&);
	RtdbClient& operator=(const RtdbClient&);

	bool ensureStationModelLoaded() const;
	bool ensureCircuitModelLoaded() const;
	const char* getAnalogDesc(stuRtdbEle* ele) const;
	const char* getAnalogVal(stuRtdbEle* ele) const;
	const char* getStatusDesc(stuRtdbEle* ele) const;
	const char* getStatusVal(stuRtdbEle* ele) const;

	bool m_opened;
	//mutable std::list<CRtdbEleModelIed*> m_iedList; // 缓存 IED 列表，避免重复查询
	mutable CRtdbEleModelCircuit* m_circuitModel;
	mutable CRtdbEleModelStation* m_stationModel;
	mutable bool m_stationModelLoaded;
	mutable bool m_circuitModelLoaded;

	RtdbOpenType m_lastOpenType;
};

#endif // RTDB_CLIENT_H
