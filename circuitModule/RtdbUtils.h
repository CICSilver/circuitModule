#pragma once
#include "../../rtdb_dll/YsdRtdbEle.h"
namespace utils
{
	const CRtdbEleModelGse* asModelGse(stuRtdbEle* base);
	const CRtdbEleModelSmv* asModelSmv(stuRtdbEle* base);
	const CRtdbEleModelIed* asModelIed(stuRtdbEle* base);

	const stuRtdbStatus* asPlateStatus(stuRtdbEle* base);
	//static const CRtdbEleModel
};