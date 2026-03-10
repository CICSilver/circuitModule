#include "SvgTransformer.h"

using utils::ColorHelper;


LogicSvg* SvgTransformer::BuildLogicModelByIedName(const QString& iedName)
{
	IED* pIed = m_circuitConfig->GetIedByName(iedName);
	if (!pIed) return NULL;
	LogicSvg* svg = new LogicSvg();
	GenerateLogicSvgByIed(pIed, *svg);
	return svg;
}

void SvgTransformer::GenerateLogicSvgByIed(const IED* pIed, LogicSvg& svg)
{
	// 主IED
	svg.mainIedRect = utils::GetIedRect(
		pIed->name,
		pIed->desc,
		(SVG_VIEWBOX_WIDTH - RECT_DEFAULT_WIDTH) / 2,
		(SVG_VIEWBOX_HEIGHT - RECT_DEFAULT_HEIGHT) / 2,
		RECT_DEFAULT_WIDTH,
		RECT_DEFAULT_HEIGHT,
		ColorHelper::pure_red,
		ColorHelper::pure_black
	);
	// 关联IED
	ParseCircuitFromIed(svg, pIed);
	// 根据单侧关联IED数量，调整高度
    AdjustOtherIedRectPosition(svg.leftIedRectList, svg.mainIedRect);
	AdjustOtherIedRectPosition(svg.rightIedRectList, svg.mainIedRect);
	// 调整链路位置
	AdjustLogicCircuitLinePosition(svg);
}

void SvgTransformer::AdjustLogicCircuitLinePosition(LogicSvg& svg)
{
	// 调整左侧IED到主IED链路位置
	AdjustMainSideCircuitLinePosition(svg.GetLeftLogicCircuitSize(), svg.leftIedRectList, svg.mainIedRect);
	AdjustMainSideCircuitLinePosition(svg.GetRightLogicCircuitSize(), svg.rightIedRectList, svg.mainIedRect, false);
}
