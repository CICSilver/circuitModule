#include "Whole/WholeDiagramBuilder.h"
#include "Virtual/VirtualDiagramBuilder.h"
#include "directitems.h"
#include <QFont>
#include <QFontMetrics>
enum wholeDiagramConfig
{
	WHOLE_GROUP_BRACE_WIDTH = 18,
	WHOLE_GROUP_ARROW_MARGIN = 16,
	WHOLE_GROUP_SWITCH_ICON_WIDTH = 86,
	WHOLE_GROUP_SWITCH_ICON_HEIGHT = 60,
	WHOLE_GROUP_SWITCH_ICON_SIDE_GAP = 4,
	WHOLE_GROUP_TOP_BOTTOM_MARGIN = 4,
	WHOLE_VALUE_ICON_SAFE_GAP = 4
};

static bool whole_fill_optical_side_ports(const OpticalCircuit* opticalCircuit, const QString& iedName, QString& iedPort, QString& switchPort)
{
	if (!opticalCircuit || iedName.isEmpty())
	{
		return false;
	}
	if (opticalCircuit->srcIedName == iedName)
	{
		iedPort = opticalCircuit->srcIedPort;
		switchPort = opticalCircuit->destIedPort;
		return !iedPort.isEmpty() && !switchPort.isEmpty();
	}
	if (opticalCircuit->destIedName == iedName)
	{
		iedPort = opticalCircuit->destIedPort;
		switchPort = opticalCircuit->srcIedPort;
		return !iedPort.isEmpty() && !switchPort.isEmpty();
	}
	return false;
}

static bool whole_fill_direct_ports(const OpticalCircuit* opticalCircuit, const QString& leftIedName, const QString& rightIedName, QString& leftPort, QString& rightPort)
{
	if (!opticalCircuit || leftIedName.isEmpty() || rightIedName.isEmpty())
	{
		return false;
	}
	if (opticalCircuit->srcIedName == leftIedName && opticalCircuit->destIedName == rightIedName)
	{
		leftPort = opticalCircuit->srcIedPort;
		rightPort = opticalCircuit->destIedPort;
		return !leftPort.isEmpty() && !rightPort.isEmpty();
	}
	if (opticalCircuit->srcIedName == rightIedName && opticalCircuit->destIedName == leftIedName)
	{
		leftPort = opticalCircuit->destIedPort;
		rightPort = opticalCircuit->srcIedPort;
		return !leftPort.isEmpty() && !rightPort.isEmpty();
	}
	return false;
}

static VirtualCircuit* whole_find_first_valid_virtual_circuit(const LogicCircuitLine* pLogicLine)
{
	if (!pLogicLine)
	{
		return NULL;
	}
	for (int i = 0; i < pLogicLine->virtual_line_list.size(); ++i)
	{
		VirtualCircuitLine* virtualLine = pLogicLine->virtual_line_list.at(i);
		if (virtualLine && virtualLine->pVirtualCircuit)
		{
			return virtualLine->pVirtualCircuit;
		}
	}
	return NULL;
}

static void whole_build_group_port_texts(CircuitConfig* circuitConfig, WholeGroupDecor* groupDecor, const LogicCircuitLine* pLogicLine)
{
	if (!circuitConfig || !groupDecor || !pLogicLine)
	{
		return;
	}
	groupDecor->leftPortText.clear();
	groupDecor->rightPortText.clear();
	const IedRect* leftIedRect = pLogicLine->pSrcIedRect;
	const IedRect* rightIedRect = pLogicLine->pDestIedRect;
	if (!leftIedRect || !rightIedRect)
	{
		return;
	}
	if (leftIedRect->x > rightIedRect->x)
	{
		const IedRect* tempRect = leftIedRect;
		leftIedRect = rightIedRect;
		rightIedRect = tempRect;
	}
	VirtualCircuit* virtualCircuit = whole_find_first_valid_virtual_circuit(pLogicLine);
	if (!virtualCircuit)
	{
		return;
	}
	if (groupDecor->groupMode == WholeGroupMode_Direct)
	{
		OpticalCircuit* directOpticalCircuit = circuitConfig->getOpticalByCode(virtualCircuit->leftOpticalCode);
		QString leftPort;
		QString rightPort;
		if (!whole_fill_direct_ports(directOpticalCircuit, leftIedRect->iedName, rightIedRect->iedName, leftPort, rightPort))
		{
			return;
		}
		groupDecor->leftPortText = leftPort;
		groupDecor->rightPortText = rightPort;
		return;
	}
	if (groupDecor->groupMode != WholeGroupMode_Switch)
	{
		return;
	}
	OpticalCircuit* firstOpticalCircuit = circuitConfig->getOpticalByCode(virtualCircuit->leftOpticalCode);
	OpticalCircuit* secondOpticalCircuit = circuitConfig->getOpticalByCode(virtualCircuit->rightOpticalCode);
	QString leftDevicePort;
	QString leftSwitchPort;
	QString rightDevicePort;
	QString rightSwitchPort;
	bool leftMatched = whole_fill_optical_side_ports(firstOpticalCircuit, leftIedRect->iedName, leftDevicePort, leftSwitchPort);
	bool rightMatched = whole_fill_optical_side_ports(secondOpticalCircuit, rightIedRect->iedName, rightDevicePort, rightSwitchPort);
	if (!leftMatched)
	{
		leftMatched = whole_fill_optical_side_ports(secondOpticalCircuit, leftIedRect->iedName, leftDevicePort, leftSwitchPort);
	}
	if (!rightMatched)
	{
		rightMatched = whole_fill_optical_side_ports(firstOpticalCircuit, rightIedRect->iedName, rightDevicePort, rightSwitchPort);
	}
	if (!leftMatched || !rightMatched)
	{
		return;
	}
	groupDecor->leftPortText = leftDevicePort + QString::fromLatin1(" / ") + leftSwitchPort;
	groupDecor->rightPortText = rightSwitchPort + QString::fromLatin1(" / ") + rightDevicePort;
}

static void whole_build_group_port_layout(WholeGroupDecor* groupDecor)
{
	if (!groupDecor)
	{
		return;
	}
	groupDecor->leftPortRect = QRectF();
	groupDecor->rightPortRect = QRectF();
	if (groupDecor->centerArrowLine.isNull())
	{
		return;
	}
	QFont font;
	font.setPointSize(WHOLE_PORT_TEXT_FONT_SIZE);
	QFontMetrics fontMetrics(font);
	qreal textHeight = fontMetrics.lineSpacing() + WHOLE_PORT_TEXT_VERTICAL_PADDING * 2;
	qreal textTop = groupDecor->centerArrowLine.p1().y() - WHOLE_PORT_TEXT_LINE_GAP - textHeight;
	qreal arrowLeftX = qMin(groupDecor->centerArrowLine.p1().x(), groupDecor->centerArrowLine.p2().x());
	qreal arrowRightX = qMax(groupDecor->centerArrowLine.p1().x(), groupDecor->centerArrowLine.p2().x());
	if (groupDecor->hasSwitchIcon && !groupDecor->switchIconRect.isNull())
	{
		qreal leftEndX = groupDecor->switchIconRect.left() - WHOLE_PORT_TEXT_SIDE_GAP;
		qreal rightStartX = groupDecor->switchIconRect.right() + WHOLE_PORT_TEXT_SIDE_GAP;
		if (!groupDecor->leftPortText.isEmpty() && leftEndX > arrowLeftX)
		{
			groupDecor->leftPortRect = QRectF(arrowLeftX, textTop, leftEndX - arrowLeftX, textHeight);
		}
		if (!groupDecor->rightPortText.isEmpty() && arrowRightX > rightStartX)
		{
			groupDecor->rightPortRect = QRectF(rightStartX, textTop, arrowRightX - rightStartX, textHeight);
		}
		return;
	}
	qreal centerX = (arrowLeftX + arrowRightX) * 0.5;
	qreal leftEndX = centerX - WHOLE_PORT_TEXT_SIDE_GAP;
	qreal rightStartX = centerX + WHOLE_PORT_TEXT_SIDE_GAP;
	if (!groupDecor->leftPortText.isEmpty() && leftEndX > arrowLeftX)
	{
		groupDecor->leftPortRect = QRectF(arrowLeftX, textTop, leftEndX - arrowLeftX, textHeight);
	}
	if (!groupDecor->rightPortText.isEmpty() && arrowRightX > rightStartX)
	{
		groupDecor->rightPortRect = QRectF(rightStartX, textTop, arrowRightX - rightStartX, textHeight);
	}
}
// #define WHOLE_GROUP_BRACE_WIDTH 18
// #define WHOLE_GROUP_ARROW_MARGIN 16
// #define WHOLE_GROUP_SWITCH_ICON_WIDTH 86
// #define WHOLE_GROUP_SWITCH_ICON_HEIGHT 60
// #define WHOLE_GROUP_SWITCH_ICON_SIDE_GAP 4
// #define WHOLE_GROUP_TOP_BOTTOM_MARGIN 4

static WholeGroupMode whole_group_mode_of_logic(const LogicCircuitLine* pLogicLine, QString& switchIedName)
{
	if (!pLogicLine || !pLogicLine->pLogicCircuit)
	{
		return WholeGroupMode_Fallback;
	}
	bool allSwitchGroup = true;
	bool allDirectGroup = true;
	switchIedName.clear();
	for (int i = 0; i < pLogicLine->pLogicCircuit->circuitList.size(); ++i)
	{
		VirtualCircuit* virtualCircuit = pLogicLine->pLogicCircuit->circuitList.at(i);
		if (!virtualCircuit)
		{
			allSwitchGroup = false;
			allDirectGroup = false;
			continue;
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
		return WholeGroupMode_Switch;
	}
	if (allDirectGroup)
	{
		return WholeGroupMode_Direct;
	}
	return WholeGroupMode_Fallback;
}

static qreal whole_center_line_min_length(bool hasSwitchIcon)
{
	qreal centerLineLength = DirectVirtualLineItem::CenterArrowMinLength();
	if (hasSwitchIcon)
	{
		qreal switchMinLength = WHOLE_GROUP_SWITCH_ICON_WIDTH + WHOLE_GROUP_SWITCH_ICON_SIDE_GAP * 2;
		if (switchMinLength > centerLineLength)
		{
			centerLineLength = switchMinLength;
		}
	}
	return centerLineLength;
}

static qreal whole_required_edge_distance(bool hasSwitchIcon)
{
	qreal textWidth = DirectVirtualLineItem::SideLabelTextWidth();
	qreal middleWidth = WHOLE_GROUP_BRACE_WIDTH * 2 + WHOLE_GROUP_ARROW_MARGIN * 2 + whole_center_line_min_length(hasSwitchIcon);
	return textWidth * 2 + middleWidth;
}

static void whole_shift_rect_list(QList<IedRect*>& rectList, int offsetX)
{
	for (int i = 0; i < rectList.size(); ++i)
	{
		IedRect* rect = rectList.at(i);
		if (!rect)
		{
			continue;
		}
		rect->x = (quint16)(rect->x + offsetX);
	}
}

static qreal whole_max_right_edge(const WholeCircuitSvg& svg)
{
	qreal maxRight = svg.mainIedRect ? (svg.mainIedRect->x + svg.mainIedRect->width) : 0;
	for (int i = 0; i < svg.leftIedRectList.size(); ++i)
	{
		IedRect* rect = svg.leftIedRectList.at(i);
		if (rect && rect->x + rect->width > maxRight)
		{
			maxRight = rect->x + rect->width;
		}
	}
	for (int i = 0; i < svg.rightIedRectList.size(); ++i)
	{
		IedRect* rect = svg.rightIedRectList.at(i);
		if (rect && rect->x + rect->width > maxRight)
		{
			maxRight = rect->x + rect->width;
		}
	}
	return maxRight;
}

static qreal whole_min_left_edge(const WholeCircuitSvg& svg)
{
	qreal minLeft = svg.mainIedRect ? svg.mainIedRect->x : 0;
	for (int i = 0; i < svg.leftIedRectList.size(); ++i)
	{
		IedRect* rect = svg.leftIedRectList.at(i);
		if (rect && rect->x < minLeft)
		{
			minLeft = rect->x;
		}
	}
	for (int i = 0; i < svg.rightIedRectList.size(); ++i)
	{
		IedRect* rect = svg.rightIedRectList.at(i);
		if (rect && rect->x < minLeft)
		{
			minLeft = rect->x;
		}
	}
	return minLeft;
}

static void whole_shift_all_ieds(WholeCircuitSvg& svg, int offsetX)
{
	if (offsetX == 0)
	{
		return;
	}
	if (svg.mainIedRect)
	{
		svg.mainIedRect->x = (quint16)(svg.mainIedRect->x + offsetX);
	}
	whole_shift_rect_list(svg.leftIedRectList, offsetX);
	whole_shift_rect_list(svg.rightIedRectList, offsetX);
}

static void whole_reset_connect_index(IedRect* rect)
{
	if (!rect)
	{
		return;
	}
	rect->left_connect_index = 0;
	rect->right_connect_index = 0;
	rect->bottom_connect_index = 0;
	rect->top_connect_index = 0;
}

static void whole_clear_virtual_lines(QList<IedRect*>& rectList)
{
	for (int i = 0; i < rectList.size(); ++i)
	{
		IedRect* rect = rectList.at(i);
		if (!rect)
		{
			continue;
		}
		whole_reset_connect_index(rect);
		for (int j = 0; j < rect->logic_line_list.size(); ++j)
		{
			LogicCircuitLine* logicLine = rect->logic_line_list.at(j);
			if (!logicLine)
			{
				continue;
			}
			qDeleteAll(logicLine->virtual_line_list);
			logicLine->virtual_line_list.clear();
		}
	}
}

static void whole_reset_virtual_state(WholeCircuitSvg& svg)
{
	whole_reset_connect_index(svg.mainIedRect);
	whole_clear_virtual_lines(svg.leftIedRectList);
	whole_clear_virtual_lines(svg.rightIedRectList);
	svg.plateRectHash.clear();
}

static void whole_adjust_virtual_plate_position(QHash<QString, PlateRect>& hash, const QString& iedName, const QPoint& linePt, const QString& plateDesc, const QString& plateRef, quint64 plateCode, bool isSrcPlate, bool isSideSrc, bool isLeft)
{
	if (plateDesc.isEmpty() || plateRef.isEmpty())
	{
		return;
	}
	QString key = QString("%1_%2").arg(iedName, plateRef);
	if (!hash.contains(key))
	{
		PlateRect plateRect;
		plateRect.code = plateCode;
		plateRect.iedName = isSrcPlate ? iedName.split("+").first() : iedName.split("+").last();
		plateRect.desc = plateDesc;
		plateRect.ref = plateRef;
		int plate_X;
		int plate_Y = linePt.y() - ICON_LENGTH / 2;
		int startOffset = (int)(ARROW_LEN * 1.5);
		int rectWidth = PLATE_WIDTH + 2 * PLATE_GAP;
		if (isSideSrc)
		{
			plate_X =
				isLeft ?
					isSrcPlate ?
						linePt.x() + startOffset :
						linePt.x() - rectWidth - startOffset :
					isSrcPlate ?
						linePt.x() - startOffset - rectWidth :
						linePt.x() + startOffset;
		}
		else
		{
			plate_X =
				isLeft ?
					isSrcPlate ?
						linePt.x() - startOffset - rectWidth :
						linePt.x() + startOffset :
					isSrcPlate ?
						linePt.x() + startOffset :
						linePt.x() - startOffset - rectWidth;
		}
		plateRect.rect = QRect(QPoint(plate_X, plate_Y), QSize(rectWidth, ICON_LENGTH));
		hash.insert(key, plateRect);
	}
	else
	{
		PlateRect& plateRect = hash[key];
		plateRect.rect.setHeight(plateRect.rect.height() + ICON_LENGTH + CIRCUIT_VERTICAL_DISTANCE);
	}
}

static void whole_build_virtual_line_geometry(const WholeCircuitSvg& svg, IedRect* sideRect, IedRect* mainIed, bool isLeft, bool isSideSource, bool groupedMode,
	int valWidth, int startOffset, VirtualCircuitLine* pVtLine, int& startValX, int& endValX, int& descRectX, int& descRectWidth)
{
	IedRect* srcRect = isSideSource ? sideRect : mainIed;
	IedRect* destRect = isSideSource ? mainIed : sideRect;
	int startPtX = 0;
	int endPtX = 0;
	if (groupedMode)
	{
		int safeDistance = (int)DirectVirtualLineItem::SideLabelSafeDistance();
		bool srcOnLeft = srcRect->x <= destRect->x;
		if (srcOnLeft)
		{
			startPtX = srcRect->x + srcRect->width - safeDistance;
			endPtX = destRect->x + safeDistance;
		}
		else
		{
			startPtX = srcRect->x + safeDistance;
			endPtX = destRect->x + destRect->width - safeDistance;
		}
	}
	else if (isSideSource)
	{
		startPtX = isLeft ? sideRect->x + sideRect->width - startOffset : sideRect->x + startOffset;
		endPtX = isLeft ? mainIed->x + startOffset : mainIed->x + mainIed->width - startOffset;
	}
	else
	{
		startPtX = isLeft ? mainIed->x + startOffset : mainIed->x + mainIed->width - startOffset;
		endPtX = isLeft ? sideRect->x + sideRect->width - startOffset : sideRect->x + startOffset;
	}
	int startIconOffset = ICON_LENGTH + srcRect->inner_gap;
	int endIconOffset = ICON_LENGTH + destRect->inner_gap;
	int valueLeftOffset = valWidth + WHOLE_VALUE_ICON_SAFE_GAP;
	int valueRightOffset = ICON_LENGTH + WHOLE_VALUE_ICON_SAFE_GAP;
	bool startOnLeft = srcRect->x <= destRect->x;
	bool endOnLeft = !startOnLeft;
	int startIconX = startOnLeft ? startPtX - startIconOffset : startPtX + srcRect->inner_gap;
	int endIconX = endOnLeft ? endPtX - endIconOffset : endPtX + destRect->inner_gap;
	startValX = startOnLeft ? startIconX - valueLeftOffset : startIconX + valueRightOffset;
	endValX = endOnLeft ? endIconX - valueLeftOffset : endIconX + valueRightOffset;
	pVtLine->startPoint.setX(startPtX);
	pVtLine->endPoint.setX(endPtX);
	pVtLine->startIconPt.setX(startIconX);
	pVtLine->endIconPt.setX(endIconX);
	IedRect* leftRect = srcRect->x <= destRect->x ? srcRect : destRect;
	IedRect* rightRect = srcRect->x <= destRect->x ? destRect : srcRect;
	descRectX = leftRect->x + leftRect->width + leftRect->inner_gap * 2;
	descRectWidth = rightRect->x - leftRect->x - leftRect->width - leftRect->inner_gap * 4;
	if (descRectWidth < 0)
	{
		descRectWidth = 0;
	}
}

static void whole_rebuild_virtual_lines_for_rect_list(WholeCircuitSvg& svg, QList<IedRect*>& rectList, IedRect* mainIed, bool isLeft)
{
	if (!mainIed)
	{
		return;
	}
	QFont font;
	font.setPointSize(18);
	QFontMetrics fm(font);
	QString valueWidthSampleText = "000.00";
	int valWidth = fm.width(valueWidthSampleText);
	for (int i = 0; i < rectList.size(); ++i)
	{
		IedRect* rect = rectList.at(i);
		if (!rect)
		{
			continue;
		}
		for (int j = 0; j < rect->logic_line_list.size(); ++j)
		{
			LogicCircuitLine* pLogicLine = rect->logic_line_list.at(j);
			if (!pLogicLine || !pLogicLine->pLogicCircuit)
			{
				continue;
			}
			QString switchIedName;
			WholeGroupMode groupMode = whole_group_mode_of_logic(pLogicLine, switchIedName);
			bool groupedMode = groupMode != WholeGroupMode_Fallback;
			for (int circuitIndex = 0; circuitIndex < pLogicLine->pLogicCircuit->circuitList.size(); ++circuitIndex)
			{
				VirtualCircuit* pCircuit = pLogicLine->pLogicCircuit->circuitList.at(circuitIndex);
				if (!pCircuit)
				{
					continue;
				}
				VirtualCircuitLine* pVtLine = new VirtualCircuitLine(pCircuit);
				bool isSideSource = pCircuit->srcIedName == rect->iedName;
				IedRect* srcRect = isSideSource ? rect : mainIed;
				IedRect* destRect = isSideSource ? mainIed : rect;
				pVtLine->pSrcIedRect = srcRect;
				pVtLine->pDestIedRect = destRect;
				quint8* pConnectPtIndex = isLeft ? &rect->right_connect_index : &rect->left_connect_index;
				int startOffset = ICON_LENGTH * 4 + rect->inner_gap;
				int startValX = 0;
				int endValX = 0;
				int descRectX = 0;
				int descRectWidth = 0;
				whole_build_virtual_line_geometry(svg, rect, mainIed, isLeft, isSideSource, groupedMode, valWidth, startOffset, pVtLine, startValX, endValX, descRectX, descRectWidth);
				if (isSideSource)
				{
					pVtLine->circuitDesc = isLeft ? QString("%1 -> %2").arg(pCircuit->srcDesc, pCircuit->destDesc) : QString("%1 <- %2").arg(pCircuit->destDesc, pCircuit->srcDesc);
				}
				else
				{
					pVtLine->circuitDesc = isLeft ? QString("%1 <- %2").arg(pCircuit->destDesc, pCircuit->srcDesc) : QString("%1 -> %2").arg(pCircuit->srcDesc, pCircuit->destDesc);
				}
				int ptY = rect->GetInnerBottomY() + CIRCUIT_VERTICAL_DISTANCE + ICON_LENGTH + (*pConnectPtIndex) * (CIRCUIT_VERTICAL_DISTANCE + ICON_LENGTH);
				int iconY = ptY - ICON_LENGTH / 2;
				pVtLine->startPoint.setY(ptY);
				pVtLine->endPoint.setY(ptY);
				pVtLine->startIconPt.setY(iconY);
				pVtLine->endIconPt.setY(iconY);
				pVtLine->startValRect = QRect(QPoint(startValX, iconY), QSize(valWidth, fm.height()));
				pVtLine->endValRect = QRect(QPoint(endValX, iconY), QSize(valWidth, fm.height()));
				if (descRectWidth > 0 && fm.width(pVtLine->circuitDesc) > descRectWidth)
				{
					pVtLine->circuitDesc = fm.elidedText(pVtLine->circuitDesc, Qt::ElideRight, descRectWidth);
				}
				pVtLine->circuitDescRect = QRect(QPoint(descRectX, (int)(ptY - fm.height() * 1.2)), QSize(descRectWidth, fm.height()));
				pLogicLine->virtual_line_list.append(pVtLine);
				QString key = QString("%1+%2").arg(srcRect->iedName).arg(destRect->iedName);
				whole_adjust_virtual_plate_position(svg.plateRectHash, key, pVtLine->endPoint, pCircuit->destSoftPlateDesc, pCircuit->destSoftPlateRef, pCircuit->destSoftPlateCode, false, isSideSource, isLeft);
				whole_adjust_virtual_plate_position(svg.plateRectHash, key, pVtLine->startPoint, pCircuit->srcSoftPlateDesc, pCircuit->srcSoftPlateRef, pCircuit->srcSoftPlateCode, true, isSideSource, isLeft);
				++(*pConnectPtIndex);
			}
		}
	}
}

static void whole_expand_side_distance(WholeCircuitSvg& svg)
{
	if (!svg.mainIedRect)
	{
		return;
	}
	int leftShift = 0;
	for (int i = 0; i < svg.leftIedRectList.size(); ++i)
	{
		IedRect* rect = svg.leftIedRectList.at(i);
		if (!rect)
		{
			continue;
		}
		for (int j = 0; j < rect->logic_line_list.size(); ++j)
		{
			LogicCircuitLine* logicLine = rect->logic_line_list.at(j);
			QString switchIedName;
			WholeGroupMode groupMode = whole_group_mode_of_logic(logicLine, switchIedName);
			if (groupMode == WholeGroupMode_Fallback)
			{
				continue;
			}
			bool hasSwitchIcon = groupMode == WholeGroupMode_Switch;
			qreal currentDistance = svg.mainIedRect->x - (rect->x + rect->width);
			qreal requiredDistance = whole_required_edge_distance(hasSwitchIcon);
			int extraDistance = (int)qMax((qreal)0, requiredDistance - currentDistance);
			if (extraDistance > leftShift)
			{
				leftShift = extraDistance;
			}
		}
	}
	int rightShift = 0;
	for (int i = 0; i < svg.rightIedRectList.size(); ++i)
	{
		IedRect* rect = svg.rightIedRectList.at(i);
		if (!rect)
		{
			continue;
		}
		for (int j = 0; j < rect->logic_line_list.size(); ++j)
		{
			LogicCircuitLine* logicLine = rect->logic_line_list.at(j);
			QString switchIedName;
			WholeGroupMode groupMode = whole_group_mode_of_logic(logicLine, switchIedName);
			if (groupMode == WholeGroupMode_Fallback)
			{
				continue;
			}
			bool hasSwitchIcon = groupMode == WholeGroupMode_Switch;
			qreal currentDistance = rect->x - (svg.mainIedRect->x + svg.mainIedRect->width);
			qreal requiredDistance = whole_required_edge_distance(hasSwitchIcon);
			int extraDistance = (int)qMax((qreal)0, requiredDistance - currentDistance);
			if (extraDistance > rightShift)
			{
				rightShift = extraDistance;
			}
		}
	}
	if (leftShift > 0)
	{
		whole_shift_rect_list(svg.leftIedRectList, -leftShift);
	}
	if (rightShift > 0)
	{
		whole_shift_rect_list(svg.rightIedRectList, rightShift);
	}
	qreal minLeft = whole_min_left_edge(svg);
	if (minLeft < 0)
	{
		whole_shift_all_ieds(svg, (int)(-minLeft));
	}
	qreal maxRight = whole_max_right_edge(svg);
	if (maxRight > svg.viewBoxWidth)
	{
		svg.viewBoxWidth = (quint16)(maxRight + 50);
	}
}

static void whole_rebuild_virtual_layout(WholeCircuitSvg& svg)
{
	whole_expand_side_distance(svg);
	whole_reset_virtual_state(svg);
	whole_rebuild_virtual_lines_for_rect_list(svg, svg.leftIedRectList, svg.mainIedRect, true);
	whole_rebuild_virtual_lines_for_rect_list(svg, svg.rightIedRectList, svg.mainIedRect, false);
}

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
	whole_rebuild_virtual_layout(*svg);
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
	QString switchIedName;
	WholeGroupMode groupMode = whole_group_mode_of_logic(pLogicLine, switchIedName);
	groupDecor->groupMode = groupMode;
	if (groupMode == WholeGroupMode_Fallback)
	{
		return;
	}
	groupDecor->hasSwitchIcon = groupMode == WholeGroupMode_Switch;
	groupDecor->switchIedName = switchIedName;
	qreal minLineY = 0;
	qreal maxLineY = 0;
	for (int i = 0; i < pLogicLine->virtual_line_list.size(); ++i)
	{
		VirtualCircuitLine* virtualLine = pLogicLine->virtual_line_list.at(i);
		if (!virtualLine)
		{
			continue;
		}
		qreal lineY = virtualLine->startPoint.y();
		if (i == 0)
		{
			minLineY = lineY;
			maxLineY = lineY;
		}
		else
		{
			if (lineY < minLineY)
			{
				minLineY = lineY;
			}
			if (lineY > maxLineY)
			{
				maxLineY = lineY;
			}
		}
	}
	const IedRect* leftIedRect = pLogicLine->pSrcIedRect;
	const IedRect* rightIedRect = pLogicLine->pDestIedRect;
	if (!leftIedRect || !rightIedRect)
	{
		groupDecor->groupMode = WholeGroupMode_Fallback;
		return;
	}
	if (leftIedRect->x > rightIedRect->x)
	{
		const IedRect* tempRect = leftIedRect;
		leftIedRect = rightIedRect;
		rightIedRect = tempRect;
	}
	qreal textWidth = DirectVirtualLineItem::SideLabelTextWidth();
	qreal leftBraceLeftX = leftIedRect->x + leftIedRect->width + textWidth;
	qreal rightBraceRightX = rightIedRect->x - textWidth;
	qreal centerLineLength = rightBraceRightX - WHOLE_GROUP_BRACE_WIDTH - WHOLE_GROUP_ARROW_MARGIN - (leftBraceLeftX + WHOLE_GROUP_BRACE_WIDTH + WHOLE_GROUP_ARROW_MARGIN);
	if (centerLineLength < whole_center_line_min_length(groupDecor->hasSwitchIcon))
	{
		groupDecor->groupMode = WholeGroupMode_Fallback;
		groupDecor->hasSwitchIcon = false;
		groupDecor->switchIedName.clear();
		return;
	}
	qreal groupTop = minLineY - ICON_LENGTH * 0.5 - WHOLE_GROUP_TOP_BOTTOM_MARGIN;
	qreal groupBottom = maxLineY + ICON_LENGTH * 0.5 + WHOLE_GROUP_TOP_BOTTOM_MARGIN;
	qreal groupHeight = groupBottom - groupTop;
	qreal centerY = (groupTop + groupBottom) * 0.5;
	groupDecor->leftBraceRect = QRectF(leftBraceLeftX, groupTop, WHOLE_GROUP_BRACE_WIDTH, groupHeight);
	groupDecor->rightBraceRect = QRectF(rightBraceRightX - WHOLE_GROUP_BRACE_WIDTH, groupTop, WHOLE_GROUP_BRACE_WIDTH, groupHeight);
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
		qreal centerX = (arrowLeftX + arrowRightX) * 0.5;
		groupDecor->switchIconRect = QRectF(centerX - WHOLE_GROUP_SWITCH_ICON_WIDTH * 0.5,
			centerY - WHOLE_GROUP_SWITCH_ICON_HEIGHT * 0.5,
			WHOLE_GROUP_SWITCH_ICON_WIDTH,
			WHOLE_GROUP_SWITCH_ICON_HEIGHT);
	}
	whole_build_group_port_texts(m_pCircuitConfig, groupDecor, pLogicLine);
	whole_build_group_port_layout(groupDecor);
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
