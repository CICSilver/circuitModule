#pragma once
#include "Common/DiagramBuilderBase.h"

class VirtualSvg;

class WholeDiagramBuilder : public DiagramBuilderBase
{
public:
	WholeDiagramBuilder();
	~WholeDiagramBuilder();
	WholeCircuitSvg* BuildWholeDiagramByIedName(const QString& iedName);
	WholeCircuitSvg* BuildWholeDiagramByBayName(const QString& bayName);

private:
	void GenerateWholeDiagramByIed(const IED* pIed, WholeCircuitSvg& svg);
	void GenerateWholeDiagramByBay(const QString& bayName, WholeCircuitSvg& svg);
	void TakeOverVirtualSvg(VirtualSvg& virtualSvg, WholeCircuitSvg& wholeSvg);
	void RebuildVirtualLayout(WholeCircuitSvg& svg);
	void ExpandSideDistance(WholeCircuitSvg& svg);
	void ShiftRectList(QList<IedRect*>& rectList, int offsetX) const;
	void ShiftAllIeds(WholeCircuitSvg& svg, int offsetX) const;
	qreal GetMaxRightEdge(const WholeCircuitSvg& svg) const;
	qreal GetMinLeftEdge(const WholeCircuitSvg& svg) const;
	void ResetVirtualState(WholeCircuitSvg& svg);
	void ResetConnectIndex(IedRect* pRect) const;
	void ClearVirtualLines(QList<IedRect*>& rectList);
	void RebuildVirtualLinesForRectList(WholeCircuitSvg& svg, QList<IedRect*>& rectList, IedRect* pMainIed, bool isLeft);
	void BuildVirtualLineGeometry(IedRect* pSideRect, IedRect* pMainIed, bool isLeft, bool isSideSource, bool groupedMode, int valWidth, int startOffset, VirtualCircuitLine* pVtLine, int& startValX, int& endValX, int& descRectX, int& descRectWidth) const;
	void AdjustVirtualPlatePosition(QHash<QString, PlateRect>& hash, const QString& iedName, const QPoint& linePt, const QString& plateDesc, const QString& plateRef, quint64 plateCode, bool isSrcPlate, bool isSideSrc, bool isLeft);
	bool FillOpticalSidePorts(const OpticalCircuit* pOpticalCircuit, const QString& iedName, QString& iedPort, QString& switchPort) const;
	bool FillDirectPorts(const OpticalCircuit* pOpticalCircuit, const QString& leftIedName, const QString& rightIedName, QString& leftPort, QString& rightPort) const;
	VirtualCircuit* GetFirstValidVirtualCircuit(const LogicCircuitLine* pLogicLine) const;
	WholeGroupMode GetGroupMode(const LogicCircuitLine* pLogicLine, QString& switchIedName) const;
	IedRect* CreateIedRectWithSize(const QString& iedName, int x, int y, int width, int height, quint32 borderColor) const;
	LogicCircuitLine* CreateLogicLine(LogicCircuit* pLogicCircuit, IedRect* pSrcRect, IedRect* pDestRect) const;
	void AdjustRectExtendHeight(IedRect* pRect) const;
	int GetRectOuterBottom(const IedRect* pRect) const;
	void AttachBayTreeLogicLines(struct WholeBayTreeNode* pNode);
	void BuildBayTreeVirtualLinesForRect(WholeCircuitSvg& svg, IedRect* pOwnerRect);
	void BuildGroupDecor(WholeCircuitSvg& svg);
	void BuildGroupDecorByLogicLine(WholeCircuitSvg& svg, LogicCircuitLine* pLogicLine);
	void BuildGroupPortTexts(WholeGroupDecor* pGroupDecor, const LogicCircuitLine* pLogicLine);
	void BuildGroupPortLayout(WholeGroupDecor* pGroupDecor) const;
	bool IsGroupDirectionRight(const LogicCircuitLine* pLogicLine) const;
};
