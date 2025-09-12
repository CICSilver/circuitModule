#include "rtdb_dll.h"
#include "../../ysd_rtdb_include/YsdRtdbAccess.h"


bool RTDB_Open(RtdbOpenType eType)//´ò¿ªRTDB
{
	return CRtdbAccess::GetRtdbAccess()->LoadRtdb(eType);
}
bool RTDB_GetAllIed(std::list<CRtdbEleModelIed*> &listIed)
{
	int iRet = CRtdbAccess::GetRtdbAccess()->GetIedList(listIed);
	if(iRet < 0)
		return false;
	return true;
}
bool RTDB_GetEle(UINT64 code,void **p,eCodeType &eType)
{
	bool bRet = CRtdbAccess::GetRtdbAccess()->GetRtdbEle(code,p,eType);
	return bRet;
}
bool RTDB_SaveAllData()
{
	bool bRet = CRtdbAccess::GetRtdbAccess()->SaveData();
	return bRet;
}
bool RTDB_SaveIedData(UINT64 code)
{
	bool bRet = CRtdbAccess::GetRtdbAccess()->SaveIedData(code);
	return bRet;
}
