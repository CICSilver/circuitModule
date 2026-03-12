#pragma once
#include "Common/DiagramBuilderBase.h"

class VirtualSvg;

class WholeDiagramBuilder : public DiagramBuilderBase
{
public:
	WholeDiagramBuilder();
	~WholeDiagramBuilder();
	WholeCircuitSvg* BuildWholeDiagramByIedName(const QString& iedName);

private:
	void GenerateWholeDiagramByIed(const IED* pIed, WholeCircuitSvg& svg);
	void TakeOverVirtualSvg(VirtualSvg& virtualSvg, WholeCircuitSvg& wholeSvg);
	void BuildGroupDecor(WholeCircuitSvg& svg);
	void BuildGroupDecorByLogicLine(WholeCircuitSvg& svg, LogicCircuitLine* pLogicLine);
	bool IsGroupDirectionRight(const LogicCircuitLine* pLogicLine) const;
};
