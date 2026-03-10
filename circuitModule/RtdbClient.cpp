#include "RtdbClient.h"

#include <cstdlib>   // std::atof
#include <cstring>   // std::memset
namespace
{
    struct RtdbFieldOffsets
    {
        size_t desc_off;
        size_t val_off;
        RtdbFieldOffsets()
            : desc_off(0)
            , val_off(0)
        {
        }

        RtdbFieldOffsets(size_t d, size_t v)
            : desc_off(d)
            , val_off(v)
        {
        }
    };
    static RtdbFieldOffsets g_rtdbFieldOffsets[CODE_TYPE_UNKNOWN + 1];
    struct RtdbOffsetInitializer
    {
        RtdbOffsetInitializer()
        {
            g_rtdbFieldOffsets[CODE_TYPE_ANALOG] =
                RtdbFieldOffsets(
                    offsetof(stuRtdbAnalog, desc),
                    offsetof(stuRtdbAnalog, val)
                );

            g_rtdbFieldOffsets[CODE_TYPE_STATUS] =
                RtdbFieldOffsets(
                    offsetof(stuRtdbStatus, desc),
                    offsetof(stuRtdbStatus, val)
                );
        }
    };
    RtdbOffsetInitializer g_rtdbOffsetInitializer;
};

// 初始化为未打开状态
RtdbClient& RtdbClient::Instance()
{
	static RtdbClient instance;
	return instance;
}

RtdbClient::RtdbClient() : m_opened(false), m_stationModelLoaded(false), m_circuitModelLoaded(false), m_stationModel(NULL)
{
    m_lastOpenType = RTDB_OPEN_RW;
}

// 析构：当前无外部资源需释放
RtdbClient::~RtdbClient() {}

bool RtdbClient::refresh(RtdbOpenType eType)
{
	if (m_opened && m_lastOpenType == eType)
    {
        return true;
    }
    m_lastOpenType = eType;
    m_stationModel = NULL;
    m_stationModelLoaded = false;
    m_circuitModelLoaded = false;
    m_opened = RTDB_Open(m_lastOpenType);
    return m_opened;
}

bool RtdbClient::getAnalog(qulonglong code, double& outValue, bool applyScale,
                           double* scaleFactorOut, double* scaleOffsetOut, QString* unitOut) const {
    void* p = NULL;

    eCodeType et = CODE_TYPE_UNKNOWN; // 初值占位
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

stuRtdbAnalog* RtdbClient::getAnalog(qulonglong code) const
{
    void* p = NULL;

    eCodeType et = CODE_TYPE_UNKNOWN;
    if (!RTDB_GetEle((UINT64)code, &p, et) || et != CODE_TYPE_ANALOG) {
        return NULL;
    }

    stuRtdbAnalog* analog = reinterpret_cast<stuRtdbAnalog*>(static_cast<void*>(p));
    return analog;
}

// 读取状态量：RTDB_GetEle -> 解析 stuRtdbStatus
bool RtdbClient::getStatus(qulonglong code, QString& outValue) const {
    void* p = NULL;

    eCodeType et = CODE_TYPE_UNKNOWN;
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

    eCodeType et = CODE_TYPE_UNKNOWN;
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

stuRtdbStatus* RtdbClient::getRyb(qulonglong code) const
{
	void* p = NULL;
	eCodeType et = CODE_TYPE_UNKNOWN;
    if (!RTDB_GetEle((UINT64)code, &p, et) || et != CODE_TYPE_STATUS || !p) {
        return NULL;
    }
    return static_cast<stuRtdbStatus*>(p);
}

stuRtdbSvCircuit* RtdbClient::getSvCircuit(qulonglong code) const
{
	const CRtdbEleModelCircuit* circuitModel = const_cast<CRtdbEleModelCircuit*>(this->circuitModel());
    if (circuitModel)
    {
        for (std::list<stuRtdbSvCircuit*>::const_iterator it = circuitModel->m_listSvCircuit.cbegin(); it != circuitModel->m_listSvCircuit.cend(); ++it)
        {
            stuRtdbSvCircuit* pCircuit = *it;
            if (pCircuit && pCircuit->m_pSvCurcuitInfo && pCircuit->m_pSvCurcuitInfo->code == code)
            {
                return pCircuit;
            }
		}
    }
    return NULL;
}

stuRtdbGseCircuit* RtdbClient::getGseCircuit(qulonglong code) const
{
    const CRtdbEleModelCircuit* circuitModel = const_cast<CRtdbEleModelCircuit*>(this->circuitModel());
    if (circuitModel)
    {
        for (std::list<stuRtdbGseCircuit*>::const_iterator it = circuitModel->m_listGseCircuit.cbegin(); it != circuitModel->m_listGseCircuit.cend(); ++it)
        {
            stuRtdbGseCircuit* pCircuit = *it;
            if (pCircuit && pCircuit->m_pGseCurcuitInfo && pCircuit->m_pGseCurcuitInfo->code == code)
            {
                return pCircuit;
            }
        }
    }
	return NULL;
}

stuRtdbRealCircuit* RtdbClient::getRealCircuit(qulonglong code) const
{
    const CRtdbEleModelCircuit* circuitModel = const_cast<CRtdbEleModelCircuit*>(this->circuitModel());
    if (circuitModel)
    {
		for (std::list<stuRtdbRealCircuit*>::const_iterator it = circuitModel->m_listRealCircuit.cbegin(); it != circuitModel->m_listRealCircuit.cend(); ++it)
        {
            stuRtdbRealCircuit* pCircuit = *it;
            if (pCircuit && pCircuit->m_pRealCurcuitInfo && pCircuit->m_pRealCurcuitInfo->code == code)
            {
                return pCircuit;
            }
        }
    }
    return NULL;
}

const char* RtdbClient::getDesc(stuRtdbEle* ele) const
{
    if (!ele) return NULL;
    switch (ele->eType)
    {
        case CODE_TYPE_ANALOG:
			return getAnalogDesc(ele);
        case CODE_TYPE_STATUS:
            return getStatusDesc(ele);
    }
    return NULL;
}

const char* RtdbClient::getVal(stuRtdbEle* ele) const
{
	if (!ele) return NULL;
    switch (ele->eType)
    {
    case CODE_TYPE_ANALOG:
		return getAnalogVal(ele);
    case CODE_TYPE_STATUS:
		return getStatusVal(ele);
    }
    return NULL;
}

const char* RtdbClient::getAnalogDesc(stuRtdbEle* ele) const
{
    if (!ele || ele->eType != CODE_TYPE_ANALOG) return NULL;
	RtdbFieldOffsets off = g_rtdbFieldOffsets[ele->eType];
    return reinterpret_cast<const char*>(ele) + off.desc_off;
}

const char* RtdbClient::getAnalogVal(stuRtdbEle* ele) const
{
	if (!ele || ele->eType != CODE_TYPE_ANALOG) return NULL;
	RtdbFieldOffsets off = g_rtdbFieldOffsets[ele->eType];
	return reinterpret_cast<const char*>(ele) + off.val_off;
}

const char* RtdbClient::getStatusDesc(stuRtdbEle* ele) const
{
	if (!ele || ele->eType != CODE_TYPE_STATUS) return NULL;
	RtdbFieldOffsets off = g_rtdbFieldOffsets[ele->eType];
    return reinterpret_cast<const char*>(ele) + off.desc_off;
}

const char* RtdbClient::getStatusVal(stuRtdbEle* ele) const
{
	if (!ele || ele->eType != CODE_TYPE_STATUS) return NULL;
	RtdbFieldOffsets off = g_rtdbFieldOffsets[ele->eType];
    return reinterpret_cast<const char*>(ele) + off.val_off;
}

const CRtdbEleModelStation* RtdbClient::stationModel() const
{
    if (!ensureStationModelLoaded())
    {
        return NULL;
    }
    return m_stationModel;
}

const CRtdbEleModelCircuit* RtdbClient::circuitModel() const
{
    if (!ensureCircuitModelLoaded())
    {
        return NULL;
    }
    return m_circuitModel;
}

bool RtdbClient::ensureStationModelLoaded() const
{
    if (!m_opened)
    {
        return false;
    }

    if (m_stationModelLoaded)
    {
        return true;
    }

    m_stationModel = NULL;
    if (!RTDB_GetModelStation(&m_stationModel))
    {
        return false;
    }

    m_stationModelLoaded = true;
    return true;
}

bool RtdbClient::ensureCircuitModelLoaded() const
{
    if (!m_opened)
    {
        return false;
    }
    if (m_circuitModel)
    {
		return true;
    }

    m_circuitModel = NULL;
    if (!RTDB_GetModelCircuit(&m_circuitModel))
    {
        return false;
    }
    m_circuitModelLoaded = true;
    return true;
}
