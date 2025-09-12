#ifndef _RTDB_DLL_H_
#define _RTDB_DLL_H_

#include "rtdb_dll_def.h"
#include "../../ysd_rtdb_include/YsdRtdbEle.h"

//导出的全局函数接口
ExpFunc bool RTDB_Open(RtdbOpenType eType);//打开RTDB
ExpFunc bool RTDB_GetAllIed(std::list<CRtdbEleModelIed*> &listIed);
ExpFunc bool RTDB_GetEle(UINT64 code,void **p,eCodeType &eType);
ExpFunc bool RTDB_SaveAllData();
ExpFunc bool RTDB_SaveIedData(UINT64 code);


#endif