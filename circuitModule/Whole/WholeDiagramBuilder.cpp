#include "Whole/WholeDiagramBuilder.h"
#include "Virtual/VirtualDiagramBuilder.h"
#include "directitems.h"
#include <QFont>
#include <QFontMetrics>
#include <QMap>
#include <QSet>
#include <QStringList>
enum WholeDiagramConfig
{
	WHOLE_GROUP_BRACE_WIDTH = 18,		// 虚实回路分组大括号宽度
	WHOLE_GROUP_ARROW_MARGIN = 16,		// 大括号与中间实线之间的留白
	WHOLE_GROUP_SWITCH_ICON_WIDTH = 86,	// 虚实回路交换机图标宽度
	WHOLE_GROUP_SWITCH_ICON_HEIGHT = 60,	// 虚实回路交换机图标高度
	WHOLE_GROUP_SWITCH_ICON_SIDE_GAP = 4,	// 交换机图标与实线之间的留白
	WHOLE_GROUP_TOP_BOTTOM_MARGIN = 4,	// 分组区域上下额外边距
	WHOLE_VALUE_ICON_SAFE_GAP = 4		// 值文本与图标之间的安全间距
};

bool WholeDiagramBuilder::FillOpticalSidePorts(const OpticalCircuit* opticalCircuit, const QString& iedName, QString& iedPort, QString& switchPort) const
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

bool WholeDiagramBuilder::FillDirectPorts(const OpticalCircuit* opticalCircuit, const QString& leftIedName, const QString& rightIedName, QString& leftPort, QString& rightPort) const
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

VirtualCircuit* WholeDiagramBuilder::GetFirstValidVirtualCircuit(const LogicCircuitLine* pLogicLine) const
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

void WholeDiagramBuilder::BuildGroupPortTexts(WholeGroupDecor* groupDecor, const LogicCircuitLine* pLogicLine)
{
	if (!m_pCircuitConfig || !groupDecor || !pLogicLine)
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
	VirtualCircuit* virtualCircuit = GetFirstValidVirtualCircuit(pLogicLine);
	if (!virtualCircuit)
	{
		return;
	}
	if (groupDecor->groupMode == WholeGroupMode_Direct)
	{
		OpticalCircuit* directOpticalCircuit = m_pCircuitConfig->getOpticalByCode(virtualCircuit->leftOpticalCode);
		QString leftPort;
		QString rightPort;
		if (!FillDirectPorts(directOpticalCircuit, leftIedRect->iedName, rightIedRect->iedName, leftPort, rightPort))
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
	OpticalCircuit* firstOpticalCircuit = m_pCircuitConfig->getOpticalByCode(virtualCircuit->leftOpticalCode);
	OpticalCircuit* secondOpticalCircuit = m_pCircuitConfig->getOpticalByCode(virtualCircuit->rightOpticalCode);
	QString leftDevicePort;
	QString leftSwitchPort;
	QString rightDevicePort;
	QString rightSwitchPort;
	bool leftMatched = FillOpticalSidePorts(firstOpticalCircuit, leftIedRect->iedName, leftDevicePort, leftSwitchPort);
	bool rightMatched = FillOpticalSidePorts(secondOpticalCircuit, rightIedRect->iedName, rightDevicePort, rightSwitchPort);
	if (!leftMatched)
	{
		leftMatched = FillOpticalSidePorts(secondOpticalCircuit, leftIedRect->iedName, leftDevicePort, leftSwitchPort);
	}
	if (!rightMatched)
	{
		rightMatched = FillOpticalSidePorts(firstOpticalCircuit, rightIedRect->iedName, rightDevicePort, rightSwitchPort);
	}
	if (!leftMatched || !rightMatched)
	{
		return;
	}
	groupDecor->leftPortText = leftDevicePort + QString::fromLatin1(" / ") + leftSwitchPort;
	groupDecor->rightPortText = rightSwitchPort + QString::fromLatin1(" / ") + rightDevicePort;
}

void WholeDiagramBuilder::BuildGroupPortLayout(WholeGroupDecor* groupDecor) const
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

WholeGroupMode WholeDiagramBuilder::GetGroupMode(const LogicCircuitLine* pLogicLine, QString& switchIedName) const
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

void WholeDiagramBuilder::ShiftRectList(QList<IedRect*>& rectList, int offsetX) const
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

qreal WholeDiagramBuilder::GetMaxRightEdge(const WholeCircuitSvg& svg) const
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

qreal WholeDiagramBuilder::GetMinLeftEdge(const WholeCircuitSvg& svg) const
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

void WholeDiagramBuilder::ShiftAllIeds(WholeCircuitSvg& svg, int offsetX) const
{
	if (offsetX == 0)
	{
		return;
	}
	if (svg.mainIedRect)
	{
		svg.mainIedRect->x = (quint16)(svg.mainIedRect->x + offsetX);
	}
	ShiftRectList(svg.leftIedRectList, offsetX);
	ShiftRectList(svg.rightIedRectList, offsetX);
}

void WholeDiagramBuilder::ResetConnectIndex(IedRect* rect) const
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

void WholeDiagramBuilder::ClearVirtualLines(QList<IedRect*>& rectList)
{
	for (int i = 0; i < rectList.size(); ++i)
	{
		IedRect* rect = rectList.at(i);
		if (!rect)
		{
			continue;
		}
		ResetConnectIndex(rect);
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

void WholeDiagramBuilder::ResetVirtualState(WholeCircuitSvg& svg)
{
	ResetConnectIndex(svg.mainIedRect);
	ClearVirtualLines(svg.leftIedRectList);
	ClearVirtualLines(svg.rightIedRectList);
	svg.plateRectHash.clear();
}

void WholeDiagramBuilder::AdjustVirtualPlatePosition(QHash<QString, PlateRect>& hash, const QString& iedName, const QPoint& linePt, const QString& plateDesc, const QString& plateRef, quint64 plateCode, bool isSrcPlate, bool isSideSrc, bool isLeft)
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

void WholeDiagramBuilder::BuildVirtualLineGeometry(IedRect* sideRect, IedRect* mainIed, bool isLeft, bool isSideSource, bool groupedMode,
	int valWidth, int startOffset, VirtualCircuitLine* pVtLine, int& startValX, int& endValX, int& descRectX, int& descRectWidth) const
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

void WholeDiagramBuilder::RebuildVirtualLinesForRectList(WholeCircuitSvg& svg, QList<IedRect*>& rectList, IedRect* mainIed, bool isLeft)
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
			WholeGroupMode groupMode = GetGroupMode(pLogicLine, switchIedName);
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
				BuildVirtualLineGeometry(rect, mainIed, isLeft, isSideSource, groupedMode, valWidth, startOffset, pVtLine, startValX, endValX, descRectX, descRectWidth);
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
				AdjustVirtualPlatePosition(svg.plateRectHash, key, pVtLine->endPoint, pCircuit->destSoftPlateDesc, pCircuit->destSoftPlateRef, pCircuit->destSoftPlateCode, false, isSideSource, isLeft);
				AdjustVirtualPlatePosition(svg.plateRectHash, key, pVtLine->startPoint, pCircuit->srcSoftPlateDesc, pCircuit->srcSoftPlateRef, pCircuit->srcSoftPlateCode, true, isSideSource, isLeft);
				++(*pConnectPtIndex);
			}
		}
	}
}

void WholeDiagramBuilder::ExpandSideDistance(WholeCircuitSvg& svg)
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
			WholeGroupMode groupMode = GetGroupMode(logicLine, switchIedName);
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
			WholeGroupMode groupMode = GetGroupMode(logicLine, switchIedName);
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
		ShiftRectList(svg.leftIedRectList, -leftShift);
	}
	if (rightShift > 0)
	{
		ShiftRectList(svg.rightIedRectList, rightShift);
	}
	qreal minLeft = GetMinLeftEdge(svg);
	if (minLeft < 0)
	{
		ShiftAllIeds(svg, (int)(-minLeft));
	}
	qreal maxRight = GetMaxRightEdge(svg);
	if (maxRight > svg.viewBoxWidth)
	{
		svg.viewBoxWidth = (quint16)(maxRight + 50);
	}
}

void WholeDiagramBuilder::RebuildVirtualLayout(WholeCircuitSvg& svg)
{
	ExpandSideDistance(svg);
	ResetVirtualState(svg);
	RebuildVirtualLinesForRectList(svg, svg.leftIedRectList, svg.mainIedRect, true);
	RebuildVirtualLinesForRectList(svg, svg.rightIedRectList, svg.mainIedRect, false);
}



static QString whole_read_local_text(const char* rawText, const QString& defaultText = QString())
{
	if (!rawText || rawText[0] == '\0')
	{
		return defaultText;
	}
	return QString::fromLocal8Bit(rawText);
}

static QString whole_get_bay_key(const CRtdbEleModelBay* pBay)
{
	if (!pBay || !pBay->m_pBayInfo)
	{
		return QString();
	}
	QString bayName = whole_read_local_text(pBay->m_pBayInfo->bayName);
	if (!bayName.isEmpty())
	{
		return bayName;
	}
	return whole_read_local_text(pBay->m_pBayInfo->bayDesc);
}

static QStringList whole_collect_bay_ied_name_list(CircuitConfig* pCircuitConfig, const QString& bayName)
{
	QStringList bayIedNameList;
	const CRtdbEleModelStation* pStation = pCircuitConfig ? pCircuitConfig->StationModel() : NULL;
	if (!pStation || bayName.isEmpty())
	{
		return bayIedNameList;
	}
	for (std::list<CRtdbEleModelVoltLevel*>::const_iterator voltIt = pStation->m_listVoltLevel.begin();
		voltIt != pStation->m_listVoltLevel.end(); ++voltIt)
	{
		const CRtdbEleModelVoltLevel* pVolt = *voltIt;
		if (!pVolt)
		{
			continue;
		}
		for (std::list<CRtdbEleModelBay*>::const_iterator bayIt = pVolt->m_listBay.begin();
			bayIt != pVolt->m_listBay.end(); ++bayIt)
		{
			const CRtdbEleModelBay* pBay = *bayIt;
			if (whole_get_bay_key(pBay) != bayName)
			{
				continue;
			}
			for (std::list<CRtdbEleModelIed*>::const_iterator iedIt = pBay->m_listIed.begin();
				iedIt != pBay->m_listIed.end(); ++iedIt)
			{
				const CRtdbEleModelIed* pModelIed = *iedIt;
				if (!pModelIed || !pModelIed->m_pIedHead)
				{
					continue;
				}
				QString iedName = whole_read_local_text(pModelIed->m_pIedHead->iedName);
				if (iedName.isEmpty() || !pCircuitConfig->GetIedByName(iedName) || bayIedNameList.contains(iedName))
				{
					continue;
				}
				bayIedNameList.append(iedName);
			}
			return bayIedNameList;
		}
	}
	return bayIedNameList;
}

IedRect* WholeDiagramBuilder::CreateIedRectWithSize(const QString& iedName, int x, int y, int width, int height, quint32 borderColor) const
{
	IED* pIed = m_pCircuitConfig ? m_pCircuitConfig->GetIedByName(iedName) : NULL;
	QString iedDesc = pIed ? pIed->desc : QString();
	return utils::GetIedRect(iedName, iedDesc, (quint16)x, (quint16)y, (quint16)width, (quint16)height, borderColor, utils::ColorHelper::pure_black);
}

LogicCircuitLine* WholeDiagramBuilder::CreateLogicLine(LogicCircuit* pLogicCircuit, IedRect* pSrcRect, IedRect* pDestRect) const
{
	LogicCircuitLine* pLine = new LogicCircuitLine();
	pLine->pLogicCircuit = pLogicCircuit;
	pLine->pSrcIedRect = pSrcRect;
	pLine->pDestIedRect = pDestRect;
	pLine->srcArrowState = Arrow_None;
	pLine->destArrowState = Arrow_In;
	pLine->arrowState = Arrow_In;
	return pLine;
}

void WholeDiagramBuilder::AdjustRectExtendHeight(IedRect* pRect) const
{
	if (!pRect)
	{
		return;
	}
	int circuitSize = 0;
	for (int i = 0; i < pRect->logic_line_list.size(); ++i)
	{
		LogicCircuitLine* pLine = pRect->logic_line_list.at(i);
		if (!pLine || !pLine->pLogicCircuit)
		{
			continue;
		}
		circuitSize += pLine->pLogicCircuit->circuitList.size();
	}
	pRect->extend_height = circuitSize * (CIRCUIT_VERTICAL_DISTANCE + ICON_LENGTH) + pRect->inner_gap + PLATE_HEIGHT;
}

int WholeDiagramBuilder::GetRectOuterBottom(const IedRect* pRect) const
{
	if (!pRect)
	{
		return 0;
	}
	return pRect->y + pRect->height + pRect->extend_height;
}

static void whole_adjust_bay_plate_position(QHash<QString, PlateRect>& hash, const QString& ownerKey, const QPoint& linePt,
	const QString& plateDesc, const QString& plateRef, quint64 plateCode, const QString& iedName, bool placeRight)
{
	if (plateDesc.isEmpty() || plateRef.isEmpty())
	{
		return;
	}
	QString key = QString("%1_%2").arg(ownerKey, plateRef);
	if (!hash.contains(key))
	{
		PlateRect plateRect;
		plateRect.code = plateCode;
		plateRect.iedName = iedName;
		plateRect.desc = plateDesc;
		plateRect.ref = plateRef;
		int rectWidth = PLATE_WIDTH + 2 * PLATE_GAP;
		int plateY = linePt.y() - ICON_LENGTH / 2;
		int startOffset = (int)(ARROW_LEN * 1.5);
		int plateX = placeRight ? linePt.x() + startOffset : linePt.x() - rectWidth - startOffset;
		plateRect.rect = QRect(QPoint(plateX, plateY), QSize(rectWidth, ICON_LENGTH));
		hash.insert(key, plateRect);
	}
	else
	{
		PlateRect& plateRect = hash[key];
		plateRect.rect.setHeight(plateRect.rect.height() + ICON_LENGTH + CIRCUIT_VERTICAL_DISTANCE);
	}
}



enum WholeBayTreeConfig
{
	WHOLE_BAY_TREE_LEFT_MARGIN = 80,
	WHOLE_BAY_TREE_TOP_MARGIN = 40,
	WHOLE_BAY_TREE_NODE_WIDTH = RECT_DEFAULT_WIDTH * 3 / 2,
	WHOLE_BAY_TREE_COLUMN_GAP = 650,	// 间隔虚实回路图的设备列之间距离，若小于610则会退化为虚回路图
	WHOLE_BAY_TREE_NODE_GAP = 24,
	WHOLE_BAY_TREE_BLOCK_GAP = 80,
	WHOLE_BAY_TREE_MAINT_PLATE_GAP = 30,
	WHOLE_BAY_TREE_VIEW_MARGIN = 60
};

struct WholeBayTreeNode
{
	WholeBayTreeNode()
		: inBay(false)
		, side(0)
		, depth(0)
		, pParent(NULL)
		, pRect(NULL)
	{
	}

	QString iedName;
	bool inBay;
	int side;
	int depth;
	WholeBayTreeNode* pParent;
	IedRect* pRect;
	QList<WholeBayTreeNode*> childNodeList;
	QList<LogicCircuit*> parentLogicList;
};

static QString whole_bay_tree_pair_key(const QString& firstName, const QString& secondName)
{
	if (firstName.compare(secondName, Qt::CaseInsensitive) <= 0)
	{
		return firstName + QString::fromLatin1("|") + secondName;
	}
	return secondName + QString::fromLatin1("|") + firstName;
}

static void whole_bay_tree_collect_logic_maps(CircuitConfig* pCircuitConfig,
	const QStringList& bayIedNameList,
	QMap<QString, QList<LogicCircuit*> >& pairLogicHash,
	QMap<QString, QSet<QString> >& bayAdjHash,
	QMap<QString, QSet<QString> >& allAdjHash)
{
	QSet<QString> bayIedNameSet;
	QSet<QString> processedCircuitKeySet;
	for (int i = 0; i < bayIedNameList.size(); ++i)
	{
		bayIedNameSet.insert(bayIedNameList.at(i));
	}
	for (int bayIndex = 0; bayIndex < bayIedNameList.size(); ++bayIndex)
	{
		QList<LogicCircuit*> logicCircuitList = pCircuitConfig->GetAllLogicCircuitListByIEDName(bayIedNameList.at(bayIndex));
		for (int logicIndex = 0; logicIndex < logicCircuitList.size(); ++logicIndex)
		{
			LogicCircuit* pLogicCircuit = logicCircuitList.at(logicIndex);
			if (!pLogicCircuit || !pLogicCircuit->pSrcIed || !pLogicCircuit->pDestIed)
			{
				continue;
			}
			QString circuitKey = QString("%1|%2|%3|%4")
				.arg(pLogicCircuit->pSrcIed->name)
				.arg(pLogicCircuit->pDestIed->name)
				.arg((int)pLogicCircuit->type)
				.arg(pLogicCircuit->cbName);
			if (processedCircuitKeySet.contains(circuitKey))
			{
				continue;
			}
			processedCircuitKeySet.insert(circuitKey);
			QString srcIedName = pLogicCircuit->pSrcIed->name;
			QString destIedName = pLogicCircuit->pDestIed->name;
			bool srcInBay = bayIedNameSet.contains(srcIedName);
			bool destInBay = bayIedNameSet.contains(destIedName);
			if (!srcInBay && !destInBay)
			{
				continue;
			}
			pairLogicHash[whole_bay_tree_pair_key(srcIedName, destIedName)].append(pLogicCircuit);
			allAdjHash[srcIedName].insert(destIedName);
			allAdjHash[destIedName].insert(srcIedName);
			if (srcInBay && destInBay)
			{
				bayAdjHash[srcIedName].insert(destIedName);
				bayAdjHash[destIedName].insert(srcIedName);
			}
		}
	}
}

static void whole_bay_tree_collect_component(const QString& startIedName,
	const QMap<QString, QSet<QString> >& bayAdjHash,
	QSet<QString>& visitedSet,
	QStringList& componentList)
{
	QStringList pendingList;
	pendingList.append(startIedName);
	visitedSet.insert(startIedName);
	while (!pendingList.isEmpty())
	{
		QString currentIedName = pendingList.takeFirst();
		componentList.append(currentIedName);
		QSet<QString> neighborSet = bayAdjHash.value(currentIedName);
		QSet<QString>::const_iterator it = neighborSet.constBegin();
		for (; it != neighborSet.constEnd(); ++it)
		{
			if (visitedSet.contains(*it))
			{
				continue;
			}
			visitedSet.insert(*it);
			pendingList.append(*it);
		}
	}
}

static QStringList whole_bay_tree_sort_names(const QSet<QString>& nameSet, const QSet<QString>& bayIedNameSet)
{
	QStringList bayNameList;
	QStringList externalNameList;
	QSet<QString>::const_iterator it = nameSet.constBegin();
	for (; it != nameSet.constEnd(); ++it)
	{
		if (bayIedNameSet.contains(*it))
		{
			bayNameList.append(*it);
		}
		else
		{
			externalNameList.append(*it);
		}
	}
	bayNameList.sort();
	externalNameList.sort();
	return bayNameList + externalNameList;
}

static QString whole_bay_tree_select_center(const QStringList& componentList,
	const QMap<QString, QSet<QString> >& allAdjHash,
	const QMap<QString, int>& bayOrderHash)
{
	QString bestIedName;
	int bestDegree = -1;
	int bestOrder = 0x7fffffff;
	for (int i = 0; i < componentList.size(); ++i)
	{
		QString iedName = componentList.at(i);
		int degree = allAdjHash.value(iedName).size();
		int order = bayOrderHash.value(iedName, 0x7fffffff);
		if (degree > bestDegree)
		{
			bestDegree = degree;
			bestOrder = order;
			bestIedName = iedName;
			continue;
		}
		if (degree == bestDegree)
		{
			if (order < bestOrder)
			{
				bestOrder = order;
				bestIedName = iedName;
				continue;
			}
			if (order == bestOrder && (bestIedName.isEmpty() || QString::compare(iedName, bestIedName, Qt::CaseInsensitive) < 0))
			{
				bestIedName = iedName;
			}
		}
	}
	return bestIedName;
}

static WholeBayTreeNode* whole_bay_tree_create_node(const QString& iedName, bool inBay, int side, int depth, WholeBayTreeNode* pParent)
{
	WholeBayTreeNode* pNode = new WholeBayTreeNode();
	pNode->iedName = iedName;
	pNode->inBay = inBay;
	pNode->side = side;
	pNode->depth = depth;
	pNode->pParent = pParent;
	return pNode;
}

static int whole_bay_tree_node_x(int depth, int side, int columnGap)
{
	int leftSecondX = WHOLE_BAY_TREE_LEFT_MARGIN;
	int leftFirstX = leftSecondX + WHOLE_BAY_TREE_NODE_WIDTH + columnGap;
	int centerX = leftFirstX + WHOLE_BAY_TREE_NODE_WIDTH + columnGap;
	int rightFirstX = centerX + WHOLE_BAY_TREE_NODE_WIDTH + columnGap;
	int rightSecondX = rightFirstX + WHOLE_BAY_TREE_NODE_WIDTH + columnGap;
	if (depth == 0)
	{
		return centerX;
	}
	if (depth == 1)
	{
		return side < 0 ? leftFirstX : rightFirstX;
	}
	return side < 0 ? leftSecondX : rightSecondX;
}

void WholeDiagramBuilder::AttachBayTreeLogicLines(WholeBayTreeNode* pNode)
{
	if (!pNode || !pNode->pParent || !pNode->pRect || !pNode->pParent->pRect)
	{
		return;
	}
	for (int logicIndex = 0; logicIndex < pNode->parentLogicList.size(); ++logicIndex)
	{
		LogicCircuit* pLogicCircuit = pNode->parentLogicList.at(logicIndex);
		if (!pLogicCircuit || !pLogicCircuit->pSrcIed || !pLogicCircuit->pDestIed)
		{
			continue;
		}
		IedRect* pSrcRect = NULL;
		IedRect* pDestRect = NULL;
		if (pLogicCircuit->pSrcIed->name == pNode->iedName)
		{
			pSrcRect = pNode->pRect;
		}
		else if (pLogicCircuit->pSrcIed->name == pNode->pParent->iedName)
		{
			pSrcRect = pNode->pParent->pRect;
		}
		if (pLogicCircuit->pDestIed->name == pNode->iedName)
		{
			pDestRect = pNode->pRect;
		}
		else if (pLogicCircuit->pDestIed->name == pNode->pParent->iedName)
		{
			pDestRect = pNode->pParent->pRect;
		}
		if (!pSrcRect || !pDestRect)
		{
			continue;
		}
		LogicCircuitLine* pLine = CreateLogicLine(pLogicCircuit, pSrcRect, pDestRect);
		pNode->pRect->logic_line_list.append(pLine);
	}
}

static int whole_bay_tree_rect_outer_height(const IedRect* pRect)
{
	if (!pRect)
	{
		return 0;
	}
	return pRect->height + pRect->extend_height;
}

static int whole_bay_tree_child_total_height(WholeBayTreeNode* pNode)
{
	if (!pNode)
	{
		return 0;
	}
	int childHeight = 0;
	for (int i = 0; i < pNode->childNodeList.size(); ++i)
	{
		childHeight += whole_bay_tree_rect_outer_height(pNode->childNodeList.at(i)->pRect);
		if (i > 0)
		{
			childHeight += WHOLE_BAY_TREE_NODE_GAP;
		}
	}
	return childHeight;
}

static int whole_bay_tree_prepare_subtree_height(WholeBayTreeNode* pNode)
{
	if (!pNode || !pNode->pRect)
	{
		return 0;
	}
	for (int i = 0; i < pNode->childNodeList.size(); ++i)
	{
		whole_bay_tree_prepare_subtree_height(pNode->childNodeList.at(i));
	}
	int nodeHeight = whole_bay_tree_rect_outer_height(pNode->pRect);
	int childHeight = whole_bay_tree_child_total_height(pNode);
	if (childHeight > nodeHeight)
	{
		pNode->pRect->extend_height += (quint16)(childHeight - nodeHeight);
		nodeHeight = childHeight;
	}
	return nodeHeight;
}

static int whole_bay_tree_subtree_height(WholeBayTreeNode* pNode)
{
	if (!pNode || !pNode->pRect)
	{
		return 0;
	}
	return whole_bay_tree_rect_outer_height(pNode->pRect);
}

static void whole_bay_tree_place_subtree(WholeBayTreeNode* pNode, int topY)
{
	if (!pNode || !pNode->pRect)
	{
		return;
	}
	pNode->pRect->y = (quint16)topY;
	if (pNode->childNodeList.isEmpty())
	{
		return;
	}
	int currentChildY = topY;
	for (int i = 0; i < pNode->childNodeList.size(); ++i)
	{
		WholeBayTreeNode* pChildNode = pNode->childNodeList.at(i);
		whole_bay_tree_place_subtree(pChildNode, currentChildY);
		currentChildY += whole_bay_tree_subtree_height(pChildNode) + WHOLE_BAY_TREE_NODE_GAP;
	}
}

static int whole_bay_tree_list_height(const QList<WholeBayTreeNode*>& nodeList)
{
	int totalHeight = 0;
	for (int i = 0; i < nodeList.size(); ++i)
	{
		totalHeight += whole_bay_tree_subtree_height(nodeList.at(i));
		if (i > 0)
		{
			totalHeight += WHOLE_BAY_TREE_NODE_GAP;
		}
	}
	return totalHeight;
}

static void whole_bay_tree_place_list(const QList<WholeBayTreeNode*>& nodeList, int topY)
{
	int currentY = topY;
	for (int i = 0; i < nodeList.size(); ++i)
	{
		WholeBayTreeNode* pNode = nodeList.at(i);
		whole_bay_tree_place_subtree(pNode, currentY);
		currentY += whole_bay_tree_subtree_height(pNode) + WHOLE_BAY_TREE_NODE_GAP;
	}
}

void WholeDiagramBuilder::BuildBayTreeVirtualLinesForRect(WholeCircuitSvg& svg, IedRect* pOwnerRect)
{
	if (!pOwnerRect)
	{
		return;
	}
	QFont font;
	font.setPointSize(18);
	QFontMetrics fm(font);
	QString valueWidthSampleText = "000.00";
	int valWidth = fm.width(valueWidthSampleText);
	for (int logicIndex = 0; logicIndex < pOwnerRect->logic_line_list.size(); ++logicIndex)
	{
		LogicCircuitLine* pLogicLine = pOwnerRect->logic_line_list.at(logicIndex);
		if (!pLogicLine || !pLogicLine->pLogicCircuit || !pLogicLine->pSrcIedRect || !pLogicLine->pDestIedRect)
		{
			continue;
		}
		IedRect* pSrcRect = pLogicLine->pSrcIedRect;
		IedRect* pDestRect = pLogicLine->pDestIedRect;
		IedRect* pPeerRect = pOwnerRect == pSrcRect ? pDestRect : pSrcRect;
		bool ownerOnLeft = pOwnerRect->x <= pPeerRect->x;
		quint8* pConnectIndex = ownerOnLeft ? &pOwnerRect->right_connect_index : &pOwnerRect->left_connect_index;
		IedRect* pLeftRect = pSrcRect->x <= pDestRect->x ? pSrcRect : pDestRect;
		IedRect* pRightRect = pSrcRect->x <= pDestRect->x ? pDestRect : pSrcRect;
		int safeDistance = (int)DirectVirtualLineItem::SideLabelSafeDistance();
		int descRectX = pLeftRect->x + pLeftRect->width + pLeftRect->inner_gap * 2;
		int descRectWidth = pRightRect->x - pLeftRect->x - pLeftRect->width - pLeftRect->inner_gap * 4;
		if (descRectWidth < 0)
		{
			descRectWidth = 0;
		}
		for (int circuitIndex = 0; circuitIndex < pLogicLine->pLogicCircuit->circuitList.size(); ++circuitIndex)
		{
			VirtualCircuit* pCircuit = pLogicLine->pLogicCircuit->circuitList.at(circuitIndex);
			if (!pCircuit)
			{
				continue;
			}
			VirtualCircuitLine* pVtLine = new VirtualCircuitLine(pCircuit);
			pVtLine->pSrcIedRect = pSrcRect;
			pVtLine->pDestIedRect = pDestRect;
			bool srcOnLeft = pSrcRect->x <= pDestRect->x;
			int startPtX = srcOnLeft ? pSrcRect->x + pSrcRect->width - safeDistance : pSrcRect->x + safeDistance;
			int endPtX = srcOnLeft ? pDestRect->x + safeDistance : pDestRect->x + pDestRect->width - safeDistance;
			int startIconOffset = ICON_LENGTH + pSrcRect->inner_gap;
			int endIconOffset = ICON_LENGTH + pDestRect->inner_gap;
			int valueLeftOffset = valWidth + WHOLE_VALUE_ICON_SAFE_GAP;
			int valueRightOffset = ICON_LENGTH + WHOLE_VALUE_ICON_SAFE_GAP;
			int startIconX = srcOnLeft ? startPtX - startIconOffset : startPtX + pSrcRect->inner_gap;
			int endIconX = srcOnLeft ? endPtX + pDestRect->inner_gap : endPtX - endIconOffset;
			int startValX = srcOnLeft ? startIconX - valueLeftOffset : startIconX + valueRightOffset;
			int endValX = srcOnLeft ? endIconX + valueRightOffset : endIconX - valueLeftOffset;
			int ptY = pOwnerRect->GetInnerBottomY() + CIRCUIT_VERTICAL_DISTANCE + ICON_LENGTH + (*pConnectIndex) * (CIRCUIT_VERTICAL_DISTANCE + ICON_LENGTH);
			int iconY = ptY - ICON_LENGTH / 2;
			pVtLine->startPoint = QPoint(startPtX, ptY);
			pVtLine->endPoint = QPoint(endPtX, ptY);
			pVtLine->startIconPt = QPoint(startIconX, iconY);
			pVtLine->endIconPt = QPoint(endIconX, iconY);
			pVtLine->startValRect = QRect(QPoint(startValX, iconY), QSize(valWidth, fm.height()));
			pVtLine->endValRect = QRect(QPoint(endValX, iconY), QSize(valWidth, fm.height()));
			pVtLine->circuitDesc = QString("%1 -> %2").arg(pCircuit->srcDesc, pCircuit->destDesc);
			if (descRectWidth > 0 && fm.width(pVtLine->circuitDesc) > descRectWidth)
			{
				pVtLine->circuitDesc = fm.elidedText(pVtLine->circuitDesc, Qt::ElideRight, descRectWidth);
			}
			pVtLine->circuitDescRect = QRect(QPoint(descRectX, (int)(ptY - fm.height() * 1.2)), QSize(descRectWidth, fm.height()));
			pLogicLine->virtual_line_list.append(pVtLine);
			QString key = QString("%1+%2").arg(pSrcRect->iedName).arg(pDestRect->iedName);
			whole_adjust_bay_plate_position(svg.plateRectHash, key, pVtLine->startPoint, pCircuit->srcSoftPlateDesc, pCircuit->srcSoftPlateRef, pCircuit->srcSoftPlateCode, pSrcRect->iedName, startPtX < endPtX);
			whole_adjust_bay_plate_position(svg.plateRectHash, key, pVtLine->endPoint, pCircuit->destSoftPlateDesc, pCircuit->destSoftPlateRef, pCircuit->destSoftPlateCode, pDestRect->iedName, endPtX < startPtX);
			++(*pConnectIndex);
		}
	}
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
	RebuildVirtualLayout(*svg);
	BuildGroupDecor(*svg);
	return svg;
}

WholeCircuitSvg* WholeDiagramBuilder::BuildWholeDiagramByBayName(const QString& bayName)
{
	return BuildWholeDiagramByBayName(bayName, WHOLE_BAY_TREE_COLUMN_GAP);
}

WholeCircuitSvg* WholeDiagramBuilder::BuildWholeDiagramByBayName(const QString& bayName, int columnGap)
{
	WholeCircuitSvg* svg = new WholeCircuitSvg();
	GenerateWholeDiagramByBay(bayName, *svg, columnGap);
	if (svg->centerIedRectList.isEmpty())
	{
		delete svg;
		return NULL;
	}
	BuildGroupDecor(*svg);
	return svg;
}

void WholeDiagramBuilder::GenerateWholeDiagramByIed(const IED* pIed, WholeCircuitSvg& svg)
{
	Q_UNUSED(pIed);
	Q_UNUSED(svg);
}


void WholeDiagramBuilder::GenerateWholeDiagramByBay(const QString& bayName, WholeCircuitSvg& svg)
{
	GenerateWholeDiagramByBay(bayName, svg, WHOLE_BAY_TREE_COLUMN_GAP);
}

void WholeDiagramBuilder::GenerateWholeDiagramByBay(const QString& bayName, WholeCircuitSvg& svg, int columnGap)
{
	QStringList bayIedNameList = whole_collect_bay_ied_name_list(m_pCircuitConfig, bayName);
	if (bayIedNameList.isEmpty())
	{
		return;
	}
	QSet<QString> bayIedNameSet;
	QMap<QString, int> bayOrderHash;
	for (int i = 0; i < bayIedNameList.size(); ++i)
	{
		bayIedNameSet.insert(bayIedNameList.at(i));
		bayOrderHash.insert(bayIedNameList.at(i), i);
	}
	QMap<QString, QList<LogicCircuit*> > pairLogicHash;
	QMap<QString, QSet<QString> > bayAdjHash;
	QMap<QString, QSet<QString> > allAdjHash;
	whole_bay_tree_collect_logic_maps(m_pCircuitConfig, bayIedNameList, pairLogicHash, bayAdjHash, allAdjHash);
	QSet<QString> visitedSet;
	int currentTopY = WHOLE_BAY_TREE_TOP_MARGIN;
	int maxBottom = 0;
	int maxRightX = 0;
	for (int bayIndex = 0; bayIndex < bayIedNameList.size(); ++bayIndex)
	{
		QString startIedName = bayIedNameList.at(bayIndex);
		if (visitedSet.contains(startIedName))
		{
			continue;
		}
		QStringList componentList;
		whole_bay_tree_collect_component(startIedName, bayAdjHash, visitedSet, componentList);
		if (componentList.isEmpty())
		{
			componentList.append(startIedName);
			visitedSet.insert(startIedName);
		}
		QString centerIedName = whole_bay_tree_select_center(componentList, allAdjHash, bayOrderHash);
		if (centerIedName.isEmpty())
		{
			centerIedName = startIedName;
		}
		QList<WholeBayTreeNode*> allNodeList;
		QList<WholeBayTreeNode*> leftRootNodeList;
		QList<WholeBayTreeNode*> rightRootNodeList;
		WholeBayTreeNode* pCenterNode = whole_bay_tree_create_node(centerIedName, true, 0, 0, NULL);
		allNodeList.append(pCenterNode);
		QSet<QString> firstHopSet = allAdjHash.value(centerIedName);
		QStringList firstHopNameList = whole_bay_tree_sort_names(firstHopSet, bayIedNameSet);
		for (int firstIndex = 0; firstIndex < firstHopNameList.size(); ++firstIndex)
		{
			QString firstHopName = firstHopNameList.at(firstIndex);
			int side = (firstIndex % 2 == 0) ? -1 : 1;
			WholeBayTreeNode* pFirstNode = whole_bay_tree_create_node(firstHopName, bayIedNameSet.contains(firstHopName), side, 1, pCenterNode);
			pFirstNode->parentLogicList = pairLogicHash.value(whole_bay_tree_pair_key(centerIedName, firstHopName));
			allNodeList.append(pFirstNode);
			if (side < 0)
			{
				leftRootNodeList.append(pFirstNode);
			}
			else
			{
				rightRootNodeList.append(pFirstNode);
			}
			QSet<QString> secondHopSet = allAdjHash.value(firstHopName);
			QStringList secondHopNameList;
			QSet<QString>::const_iterator secondIt = secondHopSet.constBegin();
			for (; secondIt != secondHopSet.constEnd(); ++secondIt)
			{
				if (!bayIedNameSet.contains(*secondIt))
				{
					continue;
				}
				if (*secondIt == centerIedName)
				{
					continue;
				}
				if (firstHopSet.contains(*secondIt))
				{
					continue;
				}
				secondHopNameList.append(*secondIt);
			}
			secondHopNameList.sort();
			for (int secondIndex = 0; secondIndex < secondHopNameList.size(); ++secondIndex)
			{
				QString secondHopName = secondHopNameList.at(secondIndex);
				WholeBayTreeNode* pSecondNode = whole_bay_tree_create_node(secondHopName, true, side, 2, pFirstNode);
				pSecondNode->parentLogicList = pairLogicHash.value(whole_bay_tree_pair_key(firstHopName, secondHopName));
				pFirstNode->childNodeList.append(pSecondNode);
				allNodeList.append(pSecondNode);
			}
		}
		for (int nodeIndex = 0; nodeIndex < allNodeList.size(); ++nodeIndex)
		{
			WholeBayTreeNode* pNode = allNodeList.at(nodeIndex);
			if (!pNode)
			{
				continue;
			}
			quint32 borderColor = pNode->depth == 0 ? utils::ColorHelper::pure_red : utils::ColorHelper::pure_green;
			pNode->pRect = CreateIedRectWithSize(pNode->iedName, whole_bay_tree_node_x(pNode->depth, pNode->side, columnGap), currentTopY, WHOLE_BAY_TREE_NODE_WIDTH, RECT_DEFAULT_HEIGHT, borderColor);
			if (!pNode->pRect)
			{
				continue;
			}
			if (pNode->depth == 0)
			{
				svg.centerIedRectList.append(pNode->pRect);
			}
			else if (pNode->side < 0)
			{
				svg.leftIedRectList.append(pNode->pRect);
			}
			else
			{
				svg.rightIedRectList.append(pNode->pRect);
			}
		}
		for (int nodeIndex = 0; nodeIndex < allNodeList.size(); ++nodeIndex)
		{
			WholeBayTreeNode* pNode = allNodeList.at(nodeIndex);
			if (!pNode || pNode->depth == 0)
			{
				continue;
			}
			AttachBayTreeLogicLines(pNode);
		}
		for (int nodeIndex = 0; nodeIndex < allNodeList.size(); ++nodeIndex)
		{
			WholeBayTreeNode* pNode = allNodeList.at(nodeIndex);
			if (!pNode || !pNode->pRect)
			{
				continue;
			}
			AdjustRectExtendHeight(pNode->pRect);
			pNode->pRect->extend_height += WHOLE_BAY_TREE_MAINT_PLATE_GAP;
		}
		for (int rootIndex = 0; rootIndex < leftRootNodeList.size(); ++rootIndex)
		{
			whole_bay_tree_prepare_subtree_height(leftRootNodeList.at(rootIndex));
		}
		for (int rootIndex = 0; rootIndex < rightRootNodeList.size(); ++rootIndex)
		{
			whole_bay_tree_prepare_subtree_height(rightRootNodeList.at(rootIndex));
		}
		int centerHeight = whole_bay_tree_rect_outer_height(pCenterNode->pRect);
		int leftHeight = whole_bay_tree_list_height(leftRootNodeList);
		int rightHeight = whole_bay_tree_list_height(rightRootNodeList);
		int sideHeight = qMax(leftHeight, rightHeight);
		if (pCenterNode->pRect && sideHeight > centerHeight)
		{
			pCenterNode->pRect->extend_height += (quint16)(sideHeight - centerHeight);
			centerHeight = whole_bay_tree_rect_outer_height(pCenterNode->pRect);
		}
		int graphHeight = qMax(centerHeight, sideHeight);
		if (graphHeight <= 0)
		{
			graphHeight = centerHeight;
		}
		if (leftHeight > 0)
		{
			whole_bay_tree_place_list(leftRootNodeList, currentTopY);
		}
		if (rightHeight > 0)
		{
			whole_bay_tree_place_list(rightRootNodeList, currentTopY);
		}
		if (pCenterNode->pRect)
		{
			pCenterNode->pRect->y = (quint16)currentTopY;
		}
		for (int nodeIndex = 0; nodeIndex < allNodeList.size(); ++nodeIndex)
		{
			WholeBayTreeNode* pNode = allNodeList.at(nodeIndex);
			if (!pNode || pNode->depth == 0 || !pNode->pRect)
			{
				continue;
			}
			BuildBayTreeVirtualLinesForRect(svg, pNode->pRect);
		}
		for (int nodeIndex = 0; nodeIndex < allNodeList.size(); ++nodeIndex)
		{
			WholeBayTreeNode* pNode = allNodeList.at(nodeIndex);
			if (!pNode || !pNode->pRect)
			{
				continue;
			}
			maxBottom = qMax(maxBottom, GetRectOuterBottom(pNode->pRect));
			maxRightX = qMax(maxRightX, pNode->pRect->x + pNode->pRect->width);
		}
		currentTopY += graphHeight + WHOLE_BAY_TREE_BLOCK_GAP;
		qDeleteAll(allNodeList);
	}
	QHash<QString, PlateRect>::const_iterator plateIt = svg.plateRectHash.constBegin();
	for (; plateIt != svg.plateRectHash.constEnd(); ++plateIt)
	{
		const QRect& rect = plateIt.value().rect;
		maxBottom = qMax(maxBottom, rect.bottom());
		maxRightX = qMax(maxRightX, rect.right());
	}
	if (maxRightX <= 0)
	{
		maxRightX = whole_bay_tree_node_x(2, 1, columnGap) + WHOLE_BAY_TREE_NODE_WIDTH + WHOLE_BAY_TREE_VIEW_MARGIN;
	}
	svg.viewBoxX = 0;
	svg.viewBoxY = 0;
	svg.viewBoxWidth = (quint16)(maxRightX + WHOLE_BAY_TREE_VIEW_MARGIN);
	svg.viewBoxHeight = (quint16)(maxBottom + WHOLE_BAY_TREE_VIEW_MARGIN);
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
	if (svg.mainIedRect)
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
		return;
	}
	QList<IedRect*> rectList;
	rectList += svg.centerIedRectList;
	rectList += svg.leftIedRectList;
	rectList += svg.rightIedRectList;
	for (int i = 0; i < rectList.size(); ++i)
	{
		IedRect* rect = rectList.at(i);
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
	WholeGroupMode groupMode = GetGroupMode(pLogicLine, switchIedName);
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
	BuildGroupPortTexts(groupDecor, pLogicLine);
	BuildGroupPortLayout(groupDecor);
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
