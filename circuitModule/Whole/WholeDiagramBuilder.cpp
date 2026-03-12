#include "Whole/WholeDiagramBuilder.h"
#include "Virtual/VirtualDiagramBuilder.h"

#define WHOLE_GROUP_BRACE_WIDTH 18
#define WHOLE_GROUP_HALF_GAP 85
#define WHOLE_GROUP_ARROW_MARGIN 16
#define WHOLE_GROUP_SWITCH_ICON_WIDTH 86
#define WHOLE_GROUP_SWITCH_ICON_HEIGHT 60
#define WHOLE_GROUP_TOP_BOTTOM_MARGIN 4

WholeDiagramBuilder::WholeDiagramBuilder()
{
}

WholeDiagramBuilder::~WholeDiagramBuilder()
{
}

WholeCircuitSvg* WholeDiagramBuilder::BuildWholeDiagramByIedName(const QString& iedName)
{
	VirtualDiagramBuilder virtualBuilder;
	VirtualSvg* virtualSvg = virtualBuilder.BuildVirtualDiagramByIedName(iedName);
	m_errStr = virtualBuilder.GetErrorString();
	if (!virtualSvg)
	{
		return NULL;
	}
	WholeCircuitSvg* svg = new WholeCircuitSvg();
	TakeOverVirtualSvg(*virtualSvg, *svg);
	delete virtualSvg;
	BuildGroupDecor(*svg);
	return svg;
}

void WholeDiagramBuilder::GenerateWholeDiagramByIed(const IED* pIed, WholeCircuitSvg& svg)
{
	Q_UNUSED(pIed);
	Q_UNUSED(svg);
}

void WholeDiagramBuilder::TakeOverVirtualSvg(VirtualSvg& virtualSvg, WholeCircuitSvg& wholeSvg)
{
	wholeSvg.mainIedRect = virtualSvg.mainIedRect;
	virtualSvg.mainIedRect = NULL;
	wholeSvg.viewBoxX = virtualSvg.viewBoxX;
	wholeSvg.viewBoxY = virtualSvg.viewBoxY;
	wholeSvg.viewBoxWidth = virtualSvg.viewBoxWidth;
	wholeSvg.viewBoxHeight = virtualSvg.viewBoxHeight;
	wholeSvg.centerIedRectList = virtualSvg.centerIedRectList;
	wholeSvg.leftIedRectList = virtualSvg.leftIedRectList;
	wholeSvg.rightIedRectList = virtualSvg.rightIedRectList;
	wholeSvg.descRectList = virtualSvg.descRectList;
	wholeSvg.plateRectHash = virtualSvg.plateRectHash;
	virtualSvg.centerIedRectList.clear();
	virtualSvg.leftIedRectList.clear();
	virtualSvg.rightIedRectList.clear();
	virtualSvg.descRectList.clear();
}

void WholeDiagramBuilder::BuildGroupDecor(WholeCircuitSvg& svg)
{
	for (int i = 0; i < svg.leftIedRectList.size(); ++i)
	{
		IedRect* rect = svg.leftIedRectList.at(i);
		if (!rect)
		{
			continue;
		}
		for (int j = 0; j < rect->logic_line_list.size(); ++j)
		{
			BuildGroupDecorByLogicLine(svg, rect->logic_line_list.at(j));
		}
	}
	for (int i = 0; i < svg.rightIedRectList.size(); ++i)
	{
		IedRect* rect = svg.rightIedRectList.at(i);
		if (!rect)
		{
			continue;
		}
		for (int j = 0; j < rect->logic_line_list.size(); ++j)
		{
			BuildGroupDecorByLogicLine(svg, rect->logic_line_list.at(j));
		}
	}
}

void WholeDiagramBuilder::BuildGroupDecorByLogicLine(WholeCircuitSvg& svg, LogicCircuitLine* pLogicLine)
{
	if (!pLogicLine)
	{
		return;
	}
	WholeGroupDecor* groupDecor = new WholeGroupDecor();
	groupDecor->pLogicLine = pLogicLine;
	svg.groupDecorList.append(groupDecor);
	if (pLogicLine->virtual_line_list.isEmpty())
	{
		return;
	}
	bool allSwitchGroup = true;
	bool allDirectGroup = true;
	QString switchIedName;
	qreal minLineX = 0;
	qreal maxLineX = 0;
	qreal minLineY = 0;
	qreal maxLineY = 0;
	for (int i = 0; i < pLogicLine->virtual_line_list.size(); ++i)
	{
		VirtualCircuitLine* virtualLine = pLogicLine->virtual_line_list.at(i);
		if (!virtualLine || !virtualLine->pVirtualCircuit)
		{
			allSwitchGroup = false;
			allDirectGroup = false;
			continue;
		}
		VirtualCircuit* virtualCircuit = virtualLine->pVirtualCircuit;
		qreal lineLeftX = qMin((qreal)virtualLine->startPoint.x(), (qreal)virtualLine->endPoint.x());
		qreal lineRightX = qMax((qreal)virtualLine->startPoint.x(), (qreal)virtualLine->endPoint.x());
		qreal lineY = virtualLine->startPoint.y();
		if (i == 0)
		{
			minLineX = lineLeftX;
			maxLineX = lineRightX;
			minLineY = lineY;
			maxLineY = lineY;
		}
		else
		{
			if (lineLeftX < minLineX)
			{
				minLineX = lineLeftX;
			}
			if (lineRightX > maxLineX)
			{
				maxLineX = lineRightX;
			}
			if (lineY < minLineY)
			{
				minLineY = lineY;
			}
			if (lineY > maxLineY)
			{
				maxLineY = lineY;
			}
		}
		if (virtualCircuit->leftOpticalCode == 0)
		{
			allSwitchGroup = false;
			allDirectGroup = false;
			continue;
		}
		if (virtualCircuit->rightOpticalCode != 0)
		{
			allDirectGroup = false;
			if (virtualCircuit->switchIedName.isEmpty())
			{
				allSwitchGroup = false;
				continue;
			}
			if (switchIedName.isEmpty())
			{
				switchIedName = virtualCircuit->switchIedName;
			}
			else if (switchIedName != virtualCircuit->switchIedName)
			{
				allSwitchGroup = false;
			}
		}
		else
		{
			allSwitchGroup = false;
		}
	}
	if (allSwitchGroup)
	{
		groupDecor->groupMode = WholeGroupMode_Switch;
		groupDecor->hasSwitchIcon = true;
		groupDecor->switchIedName = switchIedName;
	}
	else if (allDirectGroup)
	{
		groupDecor->groupMode = WholeGroupMode_Direct;
	}
	else
	{
		groupDecor->groupMode = WholeGroupMode_Fallback;
		return;
	}
	qreal centerX = (minLineX + maxLineX) * 0.5;
	qreal groupTop = minLineY - ICON_LENGTH * 0.5 - WHOLE_GROUP_TOP_BOTTOM_MARGIN;
	qreal groupBottom = maxLineY + ICON_LENGTH * 0.5 + WHOLE_GROUP_TOP_BOTTOM_MARGIN;
	qreal groupHeight = groupBottom - groupTop;
	qreal centerY = (groupTop + groupBottom) * 0.5;
	groupDecor->leftBraceRect = QRectF(centerX - WHOLE_GROUP_HALF_GAP - WHOLE_GROUP_BRACE_WIDTH, groupTop, WHOLE_GROUP_BRACE_WIDTH, groupHeight);
	groupDecor->rightBraceRect = QRectF(centerX + WHOLE_GROUP_HALF_GAP, groupTop, WHOLE_GROUP_BRACE_WIDTH, groupHeight);
	groupDecor->gapStartX = groupDecor->leftBraceRect.left();
	groupDecor->gapEndX = groupDecor->rightBraceRect.right();
	qreal arrowLeftX = groupDecor->leftBraceRect.right() + WHOLE_GROUP_ARROW_MARGIN;
	qreal arrowRightX = groupDecor->rightBraceRect.left() - WHOLE_GROUP_ARROW_MARGIN;
	if (IsGroupDirectionRight(pLogicLine))
	{
		groupDecor->centerArrowLine = QLineF(QPointF(arrowLeftX, centerY), QPointF(arrowRightX, centerY));
	}
	else
	{
		groupDecor->centerArrowLine = QLineF(QPointF(arrowRightX, centerY), QPointF(arrowLeftX, centerY));
	}
	if (groupDecor->hasSwitchIcon)
	{
		groupDecor->switchIconRect = QRectF(centerX - WHOLE_GROUP_SWITCH_ICON_WIDTH * 0.5,
			centerY - WHOLE_GROUP_SWITCH_ICON_HEIGHT * 0.5,
			WHOLE_GROUP_SWITCH_ICON_WIDTH,
			WHOLE_GROUP_SWITCH_ICON_HEIGHT);
	}
}

bool WholeDiagramBuilder::IsGroupDirectionRight(const LogicCircuitLine* pLogicLine) const
{
	if (!pLogicLine)
	{
		return true;
	}
	if (!pLogicLine->virtual_line_list.isEmpty())
	{
		const VirtualCircuitLine* virtualLine = pLogicLine->virtual_line_list.first();
		if (virtualLine)
		{
			return virtualLine->endPoint.x() >= virtualLine->startPoint.x();
		}
	}
	return pLogicLine->endPoint.x() >= pLogicLine->startPoint.x();
}
