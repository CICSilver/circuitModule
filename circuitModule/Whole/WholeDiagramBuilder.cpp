#include "Whole/WholeDiagramBuilder.h"

using utils::ColorHelper;

WholeDiagramBuilder::WholeDiagramBuilder()
{
}

WholeDiagramBuilder::~WholeDiagramBuilder()
{
}

WholeCircuitSvg* WholeDiagramBuilder::BuildWholeDiagramByIedName(const QString& iedName)
{
	IED* pIed = m_pCircuitConfig->GetIedByName(iedName);
	if (!pIed) return NULL;
	WholeCircuitSvg* svg = new WholeCircuitSvg();
	GenerateWholeDiagramByIed(pIed, *svg);
	return svg;
}

void WholeDiagramBuilder::GenerateWholeDiagramByIed(const IED* pIed, WholeCircuitSvg& svg)
{
	//svg.mainIedRect = GetMainIedRect(pIed, ColorHelper::pure_red);
	svg.mainIedRect->y = 50;
	// 解析链路并生成两侧矩形（位置未矫正）
	ParseCircuitFromIed(svg, pIed);
	size_t horizontalDistance = 300;
	// 根据链路数量调整外部矩形高度
	AdjustExtendRectByCircuit(svg.leftIedRectList, svg);
	AdjustExtendRectByCircuit(svg.rightIedRectList, svg);
	// 根据两侧IED外部矩形高度，调整中心矩形高度
	//svg.mainIedRect->extend_height = svg.GetSideLargestHeight() - svg.mainIedRect->y;
	// 计算两侧IED的链路数量()
	size_t leftCircuitSize = svg.GetLeftCircuitSize();
	size_t rightCircuitSize = svg.GetRightCircuitSize();
	// 计算主IED高度
	size_t mainCircuitSize = leftCircuitSize > rightCircuitSize ? leftCircuitSize : rightCircuitSize;
	size_t margin = ICON_LENGTH * 2;
	svg.mainIedRect->extend_height = ICON_LENGTH * (mainCircuitSize + CIRCUIT_VERTICAL_DISTANCE) + margin;// 总数据引用数量+间距+margin
}
