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
 * - 通过 open() 打开实时库；读取时统一调用 RTDB_GetEle 拉取“结构体拷贝”，再解析字段；
 * - 支持对模拟量/定值按 value*scaleFactor + scaleOffset 应用系数/偏移的可选修正；
 * - 不依赖类成员函数 CRtdbEleModelIed::GetRtdbChl，避免链接符号缺失问题。
 *
 * =========================
 * 2. 使用说明
 * =========================
 *   RtdbClient client;
 *   if (!client.open(RTDB_OPEN_RO)) { 只读方式打开失败，处理错误 }
 *
 *   // 读取模拟量（可选缩放）
 *   double v = 0.0, k = 0.0, b = 0.0; QString unit;
 *   if (client.getAnalog(123456789ULL, v, applyScale=true, &k, &b, &unit)) {
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
#include "include/rtdb/rtdb_dll.h"      // RTDB_Open, RTDB_GetEle
#include "include/rtdb/YsdRtdbDefine.h" // eCodeType
#include "include/rtdb/YsdRtdbEle.h"    // stuRtdbAnalog, stuRtdbStatus, stuRtdbSetting

class RtdbClient {
public:
    //************************************
    // 函数名称:  RtdbClient
    // 函数全名:  RtdbClient::RtdbClient
    // 访问权限:  public
    // 函数说明:  构造函数，初始化内部状态为“未打开”。
    // 参数:      无
    // 返回值:    无
    //************************************
    RtdbClient();

    //************************************
    // 函数名称:  ~RtdbClient
    // 函数全名:  RtdbClient::~RtdbClient
    // 访问权限:  public
    // 函数说明:  析构函数；当前无外部资源需释放。
    // 参数:      无
    // 返回值:    无
    //************************************
    ~RtdbClient();

    //************************************
    // 函数名称:  open
    // 函数全名:  bool RtdbClient::open(RtdbOpenType eType)
    // 访问权限:  public
    // 函数说明:  调用 SDK RTDB_Open 打开实时库。
    // 函数参数:  RtdbOpenType eType  [in ] 打开模式（如 RTDB_OPEN_RO/RTDB_OPEN_RW）。
    // 返回值:    bool                  true 打开成功；false 打开失败。
    //************************************
    bool open(RtdbOpenType eType);

    //************************************
    // 函数名称:  isOpen
    // 函数全名:  bool RtdbClient::isOpen() const
    // 访问权限:  public
    // 函数说明:  查询内部打开标志。
    // 参数:      无
    // 返回值:    bool                  true 已打开；false 未打开。
    //************************************
    bool isOpen() const { return m_opened; }

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

    //************************************
    // 函数名称:  getCodeType
    // 函数全名:  bool RtdbClient::getCodeType(qulonglong, eCodeType&) const
    // 访问权限:  public
    // 函数说明:  仅判别给定 code 的类型（模拟量/定值/状态量等），便于调用前做分支。
    // 函数参数:  qulonglong code       [in ] 通道编码。
    // 函数参数:  eCodeType& outType    [out] 返回类型。
    // 返回值:    bool                   true 成功；false code 不存在。
    //************************************
    bool getCodeType(qulonglong code, eCodeType& outType) const;

private:
    bool m_opened;
};

#endif // RTDB_CLIENT_H
