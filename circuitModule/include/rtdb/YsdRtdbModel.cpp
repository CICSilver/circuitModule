#include "YsdRtdbModel.h"
#include "YsdRtdbPub.h"

CRtdbModel::CRtdbModel()
{
	m_pShmBuff = NULL;
	m_pRtdbHead = NULL;

	m_iOffL1 = sizeof(stuRtdbHead);			//一级偏移起始字节
	m_iOffL2 = m_iOffL1 + 64 * 1024;		//二级偏移起始字节
	m_iOffL3 = m_iOffL2 + 10 * 1024 * 1024;	//三级偏移起始字节
	m_iOffL4 = m_iOffL3 + 30 * 1024 * 1024;	//四级偏移起始字节
}
CRtdbModel::~CRtdbModel()
{
	Clear();
}
bool CRtdbModel::Init(RtdbOpenType eType)
{
	bool bRet = OpenShm(m_pShmBuff,eType);
	bRet &= CreateEleMap();
	return bRet;
}
void CRtdbModel::Clear()
{
	m_mapIed.clear();
}
int CRtdbModel::GetIedList(std::list<CRtdbEleModelIed*> &listIed)
{
	listIed.clear();

	CRtdbEleModelIed *pIed = NULL;
	std::map<UINT64,CRtdbEleModelIed*>::iterator itIed = m_mapIed.begin();
	while(itIed != m_mapIed.end())
	{
		pIed = itIed->second;
		if(pIed != NULL)
			listIed.push_back(pIed);
		itIed++;
	}
	return listIed.size();
}
void* CRtdbModel::GetRtdbChl(UINT64 codeChl,eChItemType &eType)
{
	UINT64 codeIed = 0;
	CRtdbPub::GetIedCodeByChlCode(codeChl,codeIed);

	std::map<UINT64,CRtdbEleModelIed*>::iterator itIed = m_mapIed.find(codeIed);
	if(itIed == m_mapIed.end())
		return NULL;
	CRtdbEleModelIed *pIed = itIed->second;

	UINT64 codeGroup = 0;
	CRtdbPub::GetGroupCodeByChlCode(codeChl,codeGroup);
	std::map<UINT64,CRtdbEleModelGroup*>::iterator itGroup = pIed->m_mapGroup.find(codeGroup);
	if(itGroup == pIed->m_mapGroup.end())
		return NULL;
	CRtdbEleModelGroup *pGroup = itGroup->second;

	eType = pGroup->m_eChlType;
	std::map<UINT64,void*>::iterator itChl = pGroup->m_mapChl.find(codeChl);
	if(itChl == pGroup->m_mapChl.end())
		return NULL;

	void *pChl = itChl->second;

	return pChl;
}
bool CRtdbModel::GetRtdbEle(UINT64 code,void *pEle,eCodeType &eType)
{
	ItemCode itCode;
	itCode.u64 = code;

	bool bRet = false;
	eType = (eCodeType)itCode.stCode.type;
	switch(eType)
	{
		case CODE_TYPE_IED:
			{
				pEle = GetRtdbIed(code);
				if(pEle != NULL)
					bRet = true;
			}
			break;
		case CODE_TYPE_LEDGER:
			{
				pEle = GetRtdbLenger(code);
				if(pEle != NULL)
					bRet = true;
			}
			break;
		case CODE_TYPE_CPU:
			{
				pEle = GetRtdbCpu(code);
				if(pEle != NULL)
					bRet = true;
			}
			break;
		case CODE_TYPE_GROUP:
			{
				pEle = GetRtdbGroup(code);
				if(pEle != NULL)
					bRet = true;
			}
			break;
		case CODE_TYPE_SECTOR:
			{
				pEle = GetRtdbSector(code);
				if(pEle != NULL)
					bRet = true;
			}
			break;
		case CODE_TYPE_SETTING:
			{
				pEle = GetRtdbChlSetting(code);
				if(pEle != NULL)
					bRet = true;
			}
			break;
		case CODE_TYPE_ANALOG:
			{
				pEle = GetRtdbChlAnalog(code);
				if(pEle != NULL)
					bRet = true;
			}
			break;
		case CODE_TYPE_STATUS:
			{
				pEle = GetRtdbChlStatus(code);
				if(pEle != NULL)
					bRet = true;
			}
			break;
		case CODE_TYPE_RCB:
			{
				pEle = GetRtdbRpt(code);
				if(pEle != NULL)
					bRet = true;
			}
			break;
		case CODE_TYPE_CTL:
			{
				pEle = GetRtdbCtrl(code);
				if(pEle != NULL)
					bRet = true;
			}
			break;
		default:
			break;
	}
	return bRet;
}
CRtdbEleModelIed *CRtdbModel::GetRtdbIed(UINT64 code)
{
	UINT64 codeIed = 0;
	CRtdbPub::GetIedCodeByCode(code,codeIed);
	std::map<UINT64,CRtdbEleModelIed*>::iterator itIed = m_mapIed.find(codeIed);
	if(itIed == m_mapIed.end())
		return NULL;
	CRtdbEleModelIed *pIed = itIed->second;
	return pIed;
}
stuRtdbLenger *CRtdbModel::GetRtdbLenger(UINT64 code)
{
	UINT64 codeIed = 0;
	CRtdbPub::GetIedCodeByChlCode(code,codeIed);
	std::map<UINT64,CRtdbEleModelIed*>::iterator itIed = m_mapIed.find(codeIed);
	if(itIed == m_mapIed.end())
		return NULL;
	CRtdbEleModelIed *pIed = itIed->second;
	std::map<UINT64,stuRtdbLenger*>::iterator itLenger = pIed->m_mapLenger.find(code);
	if(itLenger == pIed->m_mapLenger.end())
		return NULL;
	stuRtdbLenger *pLenger = itLenger->second;
	return pLenger;
}
stuRtdbCpu *CRtdbModel::GetRtdbCpu(UINT64 code)
{
	UINT64 codeIed = 0;
	CRtdbPub::GetIedCodeByChlCode(code,codeIed);
	std::map<UINT64,CRtdbEleModelIed*>::iterator itIed = m_mapIed.find(codeIed);
	if(itIed == m_mapIed.end())
		return NULL;
	CRtdbEleModelIed *pIed = itIed->second;
	std::map<UINT64,stuRtdbCpu*>::iterator itCpu = pIed->m_mapCpu.find(code);
	if(itCpu == pIed->m_mapCpu.end())
		return NULL;
	stuRtdbCpu *pCpu = itCpu->second;
	return pCpu;
}
stuRtdbSector *CRtdbModel::GetRtdbSector(UINT64 code)
{
	UINT64 codeIed = 0;
	CRtdbPub::GetIedCodeByChlCode(code,codeIed);
	std::map<UINT64,CRtdbEleModelIed*>::iterator itIed = m_mapIed.find(codeIed);
	if(itIed == m_mapIed.end())
		return NULL;
	CRtdbEleModelIed *pIed = itIed->second;
	std::map<UINT64,stuRtdbSector*>::iterator itSector = pIed->m_mapSector.find(code);
	if(itSector == pIed->m_mapSector.end())
		return NULL;
	stuRtdbSector *pSector = itSector->second;
	return pSector;
}
CRtdbEleModelGroup *CRtdbModel::GetRtdbGroup(UINT64 code)
{
	UINT64 codeIed = 0;
	CRtdbPub::GetIedCodeByChlCode(code,codeIed);

	std::map<UINT64,CRtdbEleModelIed*>::iterator itIed = m_mapIed.find(codeIed);
	if(itIed == m_mapIed.end())
		return NULL;
	CRtdbEleModelIed *pIed = itIed->second;

	std::map<UINT64,CRtdbEleModelGroup*>::iterator itGroup = pIed->m_mapGroup.find(code);
	if(itGroup == pIed->m_mapGroup.end())
		return NULL;
	CRtdbEleModelGroup *pGroup = itGroup->second;
	return pGroup;
}
stuRtdbRptCtrl *CRtdbModel::GetRtdbRpt(UINT64 code)
{
	UINT64 codeIed = 0;
	CRtdbPub::GetIedCodeByChlCode(code,codeIed);

	std::map<UINT64,CRtdbEleModelIed*>::iterator itIed = m_mapIed.find(codeIed);
	if(itIed == m_mapIed.end())
		return NULL;
	CRtdbEleModelIed *pIed = itIed->second;

	std::map<UINT64,stuRtdbRptCtrl*>::iterator itRpt = pIed->m_mapRpt.find(code);
	if(itRpt == pIed->m_mapRpt.end())
		return NULL;
	stuRtdbRptCtrl *pCtrl = itRpt->second;
	return pCtrl;
}
stuRtdbCtrlOper *CRtdbModel::GetRtdbCtrl(UINT64 code)
{
	UINT64 codeIed = 0;
	CRtdbPub::GetIedCodeByChlCode(code,codeIed);

	std::map<UINT64,CRtdbEleModelIed*>::iterator itIed = m_mapIed.find(codeIed);
	if(itIed == m_mapIed.end())
		return NULL;
	CRtdbEleModelIed *pIed = itIed->second;

	std::map<UINT64,stuRtdbCtrlOper*>::iterator itOper = pIed->m_mapOper.find(code);
	if(itOper == pIed->m_mapOper.end())
		return NULL;
	stuRtdbCtrlOper *pCtrl = itOper->second;
	return pCtrl;
}
stuRtdbSetting *CRtdbModel::GetRtdbChlSetting(UINT64 code)
{
	stuRtdbSetting *pSetting = NULL;
	eChItemType eChlType = ChItemNone;
	void *p = GetRtdbChl(code,eChlType);
	if(p != NULL && eChlType == ChItemSetting)
		pSetting = (stuRtdbSetting *)p;
	return pSetting;
}
stuRtdbAnalog *CRtdbModel::GetRtdbChlAnalog(UINT64 code)
{
	stuRtdbAnalog *pAnalog = NULL;
	eChItemType eChlType = ChItemNone;
	void *p = GetRtdbChl(code,eChlType);
	if(p != NULL && eChlType == ChItemAnalog)
		pAnalog = (stuRtdbAnalog *)p;
	return pAnalog;
}
stuRtdbStatus *CRtdbModel::GetRtdbChlStatus(UINT64 code)
{
	stuRtdbStatus *pStatus = NULL;
	eChItemType eChlType = ChItemNone;
	void *p = GetRtdbChl(code,eChlType);
	if(p != NULL && eChlType == chItemStatus)
		pStatus = (stuRtdbStatus *)p;
	return pStatus;
}
bool CRtdbModel::OpenShm(BYTE *&pBuf,RtdbOpenType eType)
{
	bool bRet = m_shm.OpenShm(RTDB_SHM_ID,eType);
	if(!bRet)
		return false;
	pBuf = (BYTE*)m_shm.GetMapAddr();
	return true;
}
bool CRtdbModel::CreateEleMap()
{
	if(m_pShmBuff == NULL)
		return false;
	m_pRtdbHead = (stuRtdbHead *)m_pShmBuff;

	int iOffL1 = 0;
	int iOffL2 = 0;
	int iOffL3 = 0;
	int iOffL4 = 0;

	int i = 0;
	int j = 0;
	int iIedNum = m_pRtdbHead->iedNum;
	stuCodeAddr *pCodeAddr = NULL;
	stuRtdbIEDHead *pIedHead = NULL;
	CRtdbEleModelIed *pEleIed = NULL;
	iOffL1 += sizeof(stuRtdbHead);
	iOffL2 = m_iOffL2;

	for(i = 0; i < iIedNum; i++)
	{
		pEleIed = new CRtdbEleModelIed;//创建IED模型

		pCodeAddr = (stuCodeAddr *)(m_pShmBuff+iOffL1);//L1层的IED的CODE及地址信息
		pIedHead = (stuRtdbIEDHead *)(m_pShmBuff+pCodeAddr->addrOff);//地址信息

		pEleIed->m_pIedHead = pIedHead;

		iOffL2 += sizeof(stuRtdbIEDHead);
		ProIedEle(iOffL2,pEleIed);

		m_mapIed[pCodeAddr->code] = pEleIed;

		iOffL1 += sizeof(stuCodeAddr);
	}


	return true;
}
bool CRtdbModel::ProIedEle(int &iOffL2,CRtdbEleModelIed *pModelIed)
{
	ProIedEle_Lenger(iOffL2,pModelIed);
	ProIedEle_Cpu(iOffL2,pModelIed);
	ProIedEle_Sector(iOffL2,pModelIed);
	ProIedEle_Group(iOffL2,pModelIed);
	ProIedEle_Rpt(iOffL2,pModelIed);
	ProIedEle_Ctl(iOffL2,pModelIed);

	return true;
}
bool CRtdbModel::ProIedEle_Lenger(int &iOffL2,CRtdbEleModelIed *pModelIed)
{
	stuCodeAddr *pCodeAddr = NULL;
	int i = 0;
	int iEleNum = 0;

	memcpy(&iEleNum,(BYTE*)(m_pShmBuff+iOffL2),sizeof(int));
	iOffL2 += sizeof(int);

	if(iEleNum > 0)
	{
		for(i = 0; i < iEleNum; i++)
		{
			pCodeAddr = (stuCodeAddr *)(m_pShmBuff + iOffL2);
			pModelIed->m_mapLenger[pCodeAddr->code] = (stuRtdbLenger*)(m_pShmBuff+pCodeAddr->addrOff);

			iOffL2 += sizeof(stuCodeAddr);
		}
	}
	return true;
}
bool CRtdbModel::ProIedEle_Cpu(int &iOffL2,CRtdbEleModelIed *pModelIed)
{
	stuCodeAddr *pCodeAddr = NULL;
	int i = 0;
	int iEleNum = 0;

	memcpy(&iEleNum,(BYTE*)(m_pShmBuff+iOffL2),sizeof(int));
	iOffL2 += sizeof(int);

	if(iEleNum > 0)
	{
		for(i = 0; i < iEleNum; i++)
		{
			pCodeAddr = (stuCodeAddr *)(m_pShmBuff + iOffL2);
			pModelIed->m_mapCpu[pCodeAddr->code] = (stuRtdbCpu*)(m_pShmBuff+pCodeAddr->addrOff);

			iOffL2 += sizeof(stuCodeAddr);
		}
	}
	return true;
}
bool CRtdbModel::ProIedEle_Sector(int &iOffL2,CRtdbEleModelIed *pModelIed)
{
	stuCodeAddr *pCodeAddr = NULL;
	int i = 0;
	int iEleNum = 0;

	memcpy(&iEleNum,(BYTE*)(m_pShmBuff+iOffL2),sizeof(int));
	iOffL2 += sizeof(int);

	if(iEleNum > 0)
	{
		for(i = 0; i < iEleNum; i++)
		{
			pCodeAddr = (stuCodeAddr *)(m_pShmBuff + iOffL2);
			pModelIed->m_mapSector[pCodeAddr->code] = (stuRtdbSector*)(m_pShmBuff+pCodeAddr->addrOff);

			iOffL2 += sizeof(stuCodeAddr);
		}
	}

	return true;
}
bool CRtdbModel::ProIedEle_Group(int &iOffL2,CRtdbEleModelIed *pModelIed)
{
	stuCodeAddr *pCodeAddr = NULL;
	int i = 0;
	int iEleNum = 0;
	int iOffL3 = 0;
	stuRtdbGroupHead *pGroupHead = NULL;
	CRtdbEleModelGroup *pModelGroup = NULL;

	memcpy(&iEleNum,(BYTE*)(m_pShmBuff+iOffL2),sizeof(int));
	iOffL2 += sizeof(int);

	if(iEleNum > 0)
	{
		for(i = 0; i < iEleNum; i++)
		{
			pModelGroup = new CRtdbEleModelGroup;

			pCodeAddr = (stuCodeAddr *)(m_pShmBuff + iOffL2);

			pGroupHead = (stuRtdbGroupHead *)(m_pShmBuff+pCodeAddr->addrOff);//数据集的信息
			pModelGroup->m_pGroupHead = pGroupHead;	
			pModelGroup->m_eChlType = GetGroupChlItemType(pGroupHead->groupType);
			ProIedEle_Group_Chl(pCodeAddr->addrOff,pModelGroup);//数据集通道

			pModelIed->m_mapGroup[pCodeAddr->code] = pModelGroup;

			iOffL2 += sizeof(stuCodeAddr);
		}
	}

	return true;
}
bool CRtdbModel::ProIedEle_Rpt(int &iOffL2,CRtdbEleModelIed *pModelIed)
{
	stuCodeAddr *pCodeAddr = NULL;
	int i = 0;
	int iEleNum = 0;

	memcpy(&iEleNum,(BYTE*)(m_pShmBuff+iOffL2),sizeof(int));
	iOffL2 += sizeof(int);

	if(iEleNum > 0)
	{
		for(i = 0; i < iEleNum; i++)
		{
			pCodeAddr = (stuCodeAddr *)(m_pShmBuff + iOffL2);
			pModelIed->m_mapRpt[pCodeAddr->code] = (stuRtdbRptCtrl*)(m_pShmBuff+pCodeAddr->addrOff);

			iOffL2 += sizeof(stuCodeAddr);
		}
	}

	return true;
}
bool CRtdbModel::ProIedEle_Ctl(int &iOffL2,CRtdbEleModelIed *pModelIed)
{
	stuCodeAddr *pCodeAddr = NULL;
	int i = 0;
	int iEleNum = 0;

	memcpy(&iEleNum,(BYTE*)(m_pShmBuff+iOffL2),sizeof(int));
	iOffL2 += sizeof(int);

	if(iEleNum > 0)
	{
		for(i = 0; i < iEleNum; i++)
		{
			pCodeAddr = (stuCodeAddr *)(m_pShmBuff + iOffL2);
			pModelIed->m_mapOper[pCodeAddr->code] = (stuRtdbCtrlOper*)(m_pShmBuff+pCodeAddr->addrOff);

			iOffL2 += sizeof(stuCodeAddr);
		}
	}

	return true;
}
bool CRtdbModel::ProIedEle_Group_Chl(UINT64 &addrGroupHeadOff,CRtdbEleModelGroup *pModelGroup)
{
	UINT64 u64AddrOff = addrGroupHeadOff + sizeof(stuRtdbGroupHead);//获取数据集通道的内存地址

	stuCodeAddr *pCodeAddr = NULL;
	int i = 0;
	int iEleNum = 0;
	memcpy(&iEleNum,(BYTE*)(m_pShmBuff+u64AddrOff),sizeof(int));
	u64AddrOff += sizeof(int);

	stuRtdbSetting *pSetting = NULL;
	stuRtdbAnalog *pAnalog = NULL;
	stuRtdbStatus *pStatus = NULL;

	if(iEleNum > 0)
	{
		for(i = 0; i < iEleNum; i++)
		{
			pCodeAddr = (stuCodeAddr *)(m_pShmBuff+u64AddrOff);

			pModelGroup->m_mapChl[pCodeAddr->code] = (void*)(m_pShmBuff+pCodeAddr->addrOff);

			//test
			if(pModelGroup->m_eChlType == ChItemSetting)
			{
				pSetting = (stuRtdbSetting*)(m_pShmBuff+pCodeAddr->addrOff);
			}
			else if(pModelGroup->m_eChlType == ChItemAnalog)
			{
				pAnalog = (stuRtdbAnalog*)(m_pShmBuff+pCodeAddr->addrOff);
			}
			else if(pModelGroup->m_eChlType == chItemStatus)
			{
				pStatus = (stuRtdbStatus*)(m_pShmBuff+pCodeAddr->addrOff);
			}

			////

			u64AddrOff += sizeof(stuCodeAddr);
		}
	}

	return true;
}

ChlItemType CRtdbModel::GetGroupChlItemType(BYTE groupType)
{
	switch(groupType)
	{
	case InfoSector:	//定值区
		return ChItemSector;
	case InfoSetting:	//定值 
		return ChItemSetting;
	case InfoAnalog:	//模拟量
		return ChItemAnalog;
	case InfoSwitch:	//开关量
	case InfoRyaban:	//软压板 
	case InfoAlert:		//告警 
	case InfoFault:		//故障 
	case InfoFaultInfo:	//故障信息
		return chItemStatus;
	default:
		return ChItemNone;			 
	}
	return ChItemNone;
}