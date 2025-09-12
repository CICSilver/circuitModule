#include "RtdbClient.h"

#include <cstdlib>   // std::atof
#include <cstring>   // std::memset

// 初始化为未打开状态
RtdbClient::RtdbClient() : m_opened(false) {}

// 析构：当前无外部资源需释放
RtdbClient::~RtdbClient() {}

// 打开 RTDB（共享内存/文件模式由 eType 决定）
bool RtdbClient::open(RtdbOpenType eType) {
    m_opened = RTDB_Open(eType);
    return m_opened;
}

bool RtdbClient::getIedList(std::list<CRtdbEleModelIed*>& listIed) const
{
    return false;
}  

// 读取模拟量：RTDB_GetEle -> 解析 stuRtdbAnalog
bool RtdbClient::getAnalog(qulonglong code, double& outValue, bool applyScale,
                           double* scaleFactorOut, double* scaleOffsetOut, QString* unitOut) const {
    void* p = NULL;

    eCodeType et = CODE_TYPE_IED; // 初值占位
    if (!RTDB_GetEle((UINT64)code, &p, et) || et != CODE_TYPE_ANALOG) {
        return false;
    }

    const stuRtdbAnalog* a = reinterpret_cast<const stuRtdbAnalog*>(static_cast<const void*>(p));
    double v = std::atof(a->val); // Val 为字符串，如 "1.234"
    if (applyScale) {
        v = v * a->scaleFactor + a->scaleOffset; // 工程量换算
    }
    if (scaleFactorOut) *scaleFactorOut = a->scaleFactor;
    if (scaleOffsetOut) *scaleOffsetOut = a->scaleOffset;
    if (unitOut)        *unitOut        = QString::fromLatin1(a->dime);

    outValue = v;
    return true;
}

// 读取状态量：RTDB_GetEle -> 解析 stuRtdbStatus
bool RtdbClient::getStatus(qulonglong code, QString& outValue) const {
    void* p = NULL;

    eCodeType et = CODE_TYPE_IED;
    if (!RTDB_GetEle((UINT64)code, &p, et) || et != CODE_TYPE_STATUS) {
        return false;
    }

    const stuRtdbStatus* s = reinterpret_cast<const stuRtdbStatus*>(static_cast<const void*>(p));
    outValue = QString::fromLatin1(s->val);
    return true;
}

// 读取定值：RTDB_GetEle -> 解析 stuRtdbSetting
bool RtdbClient::getSetting(qulonglong code, double& outValue, int sectorIndex, bool applyScale) const {
    if (sectorIndex < 1) sectorIndex = 1;
    if (sectorIndex > MAX_SECTOR) sectorIndex = MAX_SECTOR;

    void* p = NULL;

    eCodeType et = CODE_TYPE_IED;
    if (!RTDB_GetEle((UINT64)code, &p, et) || et != CODE_TYPE_SETTING) {
        return false;
    }

    const stuRtdbSetting* st = reinterpret_cast<const stuRtdbSetting*>(static_cast<const void*>(p));
    double v = std::atof(st->val[sectorIndex - 1]);
    if (applyScale) {
        v = v * st->scaleFactor + st->scaleOffset;
    }
    outValue = v;
    return true;
}

