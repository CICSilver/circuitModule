#include "RtdbUtils.h"

namespace utils
{
	const CRtdbEleModelGse* asModelGse(stuRtdbEle* base)
	{
		return base && base->eType == CODE_TYPE_GSE ?
			static_cast<CRtdbEleModelGse*>(base) : NULL;
	}
	const CRtdbEleModelSmv* asModelSmv(stuRtdbEle* base)
	{
		return base && base->eType == CODE_TYPE_SMV ?
			static_cast<CRtdbEleModelSmv*>(base) : NULL;
	}
	const CRtdbEleModelIed* asModelIed(stuRtdbEle* base)
	{
		return base && base->eType == CODE_TYPE_IED ?
			static_cast<CRtdbEleModelIed*>(base) : NULL;
	}

	const stuRtdbStatus* asPlateStatus(stuRtdbEle* base)
	{
		stuRtdbStatus* status = base && base->eType == CODE_TYPE_STATUS ?
			static_cast<stuRtdbStatus*>(base) : NULL;
		return status && status->type == StateRyaban ?
			status : NULL;
	}

};