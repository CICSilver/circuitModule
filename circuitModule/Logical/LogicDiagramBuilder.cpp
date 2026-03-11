#include "Logical/LogicDiagramBuilder.h"
#include <QMap>
#include <QSet>

using utils::ColorHelper;

namespace
{
	const int BAY_LOGIC_TOP_MARGIN = 140;
	const int BAY_LOGIC_BOTTOM_MARGIN = 120;
	const int BAY_LOGIC_LEFT_MARGIN = 180;
	const int BAY_LOGIC_SIDE_DISTANCE = 520;
	const int BAY_LOGIC_ROW_GAP = 40;
	const int BAY_LOGIC_CHANNEL_START_GAP = 80;
	const int BAY_LOGIC_CHANNEL_GAP = 28;
	const int BAY_LOGIC_CHANNEL_TYPE_GAP = 80;
	const int RECT_AREA_CENTER = 0;
	const int RECT_AREA_LEFT = 1;
	const int RECT_AREA_RIGHT = 2;

	struct InternalCircuitGroup
	{
		InternalCircuitGroup()
			: type(GOOSE)
		{
		}

		QString firstIedName;
		QString secondIedName;
		VirtualType type;
		QList<LogicCircuit*> firstToSecondList;
		QList<LogicCircuit*> secondToFirstList;
	};

	struct LogicLayoutLine
	{
		LogicLayoutLine()
			: pLine(NULL)
			, routeX(0)
			, isInternal(false)
		{
		}

		LogicCircuitLine* pLine;
		int routeX;
		bool isInternal;
	};

	// 读取站模中的本地字符串
	static QString ReadLocalText(const char* rawText, const QString& defaultText = QString())
	{
		if (!rawText || rawText[0] == '\0')
		{
			return defaultText;
		}
		return QString::fromLocal8Bit(rawText);
	}

	// 获取间隔唯一标识
	static QString GetBayKey(const CRtdbEleModelBay* pBay)
	{
		if (!pBay || !pBay->m_pBayInfo)
		{
			return QString();
		}
		QString bayName = ReadLocalText(pBay->m_pBayInfo->bayName);
		if (!bayName.isEmpty())
		{
			return bayName;
		}
		return ReadLocalText(pBay->m_pBayInfo->bayDesc);
	}

	// 收集指定间隔下的IED列表
	static QStringList CollectBayIedNameList(CircuitConfig* pCircuitConfig, const QString& bayName)
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
				if (GetBayKey(pBay) != bayName)
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
					QString iedName = ReadLocalText(pModelIed->m_pIedHead->iedName);
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

	// 统计间隔内部和间隔外部的逻辑链路关系
	static void CollectBayLogicRelation(CircuitConfig* pCircuitConfig,
		const QStringList& bayIedNameList,
		QMap<QString, InternalCircuitGroup>& internalGroupHash,
		QList<LogicCircuit*>& externalLogicList,
		QStringList& peerIedNameList)
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
				if (srcInBay && destInBay)
				{
					QString firstIedName = srcIedName < destIedName ? srcIedName : destIedName;
					QString secondIedName = srcIedName < destIedName ? destIedName : srcIedName;
					QString groupKey = QString("%1|%2|%3").arg(firstIedName).arg(secondIedName).arg((int)pLogicCircuit->type);
					InternalCircuitGroup& group = internalGroupHash[groupKey];
					group.firstIedName = firstIedName;
					group.secondIedName = secondIedName;
					group.type = pLogicCircuit->type;
					if (srcIedName == firstIedName)
					{
						group.firstToSecondList.append(pLogicCircuit);
					}
					else
					{
						group.secondToFirstList.append(pLogicCircuit);
					}
				}
				else if (srcInBay || destInBay)
				{
					externalLogicList.append(pLogicCircuit);
					QString peerIedName = srcInBay ? destIedName : srcIedName;
					if (!peerIedName.isEmpty() && !peerIedNameList.contains(peerIedName))
					{
						peerIedNameList.append(peerIedName);
					}
				}
			}
		}
	}

	// 按统一行高均匀布置同一区域内设备
	static void LayoutRectList(CircuitConfig* pCircuitConfig,
		const QStringList& iedNameList,
		QList<IedRect*>& rectList,
		int rectX,
		int maxRowCount,
		quint32 borderColor)
	{
		if (iedNameList.isEmpty())
		{
			return;
		}
		int rowHeight = RECT_DEFAULT_HEIGHT + BAY_LOGIC_ROW_GAP;
		int startY = BAY_LOGIC_TOP_MARGIN + (maxRowCount - iedNameList.size()) * rowHeight / 2;
		for (int i = 0; i < iedNameList.size(); ++i)
		{
			IED* pIed = pCircuitConfig->GetIedByName(iedNameList.at(i));
			if (!pIed)
			{
				continue;
			}
			IedRect* pRect = utils::GetIedRect(
				pIed->name,
				pIed->desc,
				rectX,
				startY + i * rowHeight,
				RECT_DEFAULT_WIDTH,
				RECT_DEFAULT_HEIGHT,
				borderColor,
				ColorHelper::pure_black);
			rectList.append(pRect);
		}
	}

	// 生成指定侧连接点坐标
	static QPoint BuildConnectPoint(IedRect* pRect,
		bool useRightSide,
		QMap<IedRect*, int>& usedCountHash,
		const QMap<IedRect*, int>& totalCountHash)
	{
		int totalCount = totalCountHash.value(pRect, 1);
		int usedCount = usedCountHash.value(pRect, 0);
		int diffY = pRect->height / (totalCount + 1);
		QPoint point(useRightSide ? pRect->x + pRect->width : pRect->x,
			pRect->y + (usedCount + 1) * diffY);
		usedCountHash.insert(pRect, usedCount + 1);
		return point;
	}
}

LogicDiagramBuilder::LogicDiagramBuilder()
{
}

LogicDiagramBuilder::~LogicDiagramBuilder()
{
}

LogicSvg* LogicDiagramBuilder::BuildLogicDiagramByIedName(const QString& iedName)
{
	IED* pIed = m_pCircuitConfig->GetIedByName(iedName);
	if (!pIed) return NULL;
	LogicSvg* svg = new LogicSvg();
	GenerateLogicDiagramByIed(pIed, *svg);
	return svg;
}

LogicSvg* LogicDiagramBuilder::BuildLogicDiagramByBayName(const QString& bayName)
{
	if (bayName.isEmpty())
	{
		return NULL;
	}
	LogicSvg* pSvg = new LogicSvg();
	GenerateLogicDiagramByBay(bayName, *pSvg);
	if (pSvg->centerIedRectList.isEmpty())
	{
		delete pSvg;
		return NULL;
	}
	return pSvg;
}

void LogicDiagramBuilder::GenerateLogicDiagramByIed(const IED* pIed, LogicSvg& svg)
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
	// 相关IED
	ParseCircuitFromIed(svg, pIed);
	// 根据关联IED数量调整左右列表高度
	AdjustOtherIedRectPosition(svg.leftIedRectList, svg.mainIedRect);
	AdjustOtherIedRectPosition(svg.rightIedRectList, svg.mainIedRect);
	// 调整逻辑线位置
	AdjustLogicCircuitLinePosition(svg);
}

void LogicDiagramBuilder::GenerateLogicDiagramByBay(const QString& bayName, LogicSvg& svg)
{
	QStringList bayIedNameList = CollectBayIedNameList(m_pCircuitConfig, bayName);
	if (bayIedNameList.isEmpty())
	{
		return;
	}

	QMap<QString, InternalCircuitGroup> internalGroupHash;
	QList<LogicCircuit*> externalLogicList;
	QStringList peerIedNameList;
	CollectBayLogicRelation(m_pCircuitConfig, bayIedNameList, internalGroupHash, externalLogicList, peerIedNameList);

	QStringList leftPeerIedNameList;
	QStringList rightPeerIedNameList;
	for (int peerIndex = 0; peerIndex < peerIedNameList.size(); ++peerIndex)
	{
		if (peerIndex % 2 == 0)
		{
			leftPeerIedNameList.append(peerIedNameList.at(peerIndex));
		}
		else
		{
			rightPeerIedNameList.append(peerIedNameList.at(peerIndex));
		}
	}

	int gseLineCount = 0;
	int svLineCount = 0;
	QMap<QString, InternalCircuitGroup>::const_iterator groupIt = internalGroupHash.constBegin();
	for (; groupIt != internalGroupHash.constEnd(); ++groupIt)
	{
		int lineCount = qMax(groupIt.value().firstToSecondList.size(), groupIt.value().secondToFirstList.size());
		if (groupIt.value().type == SV)
		{
			svLineCount += lineCount;
		}
		else
		{
			gseLineCount += lineCount;
		}
	}

	int maxRowCount = qMax(bayIedNameList.size(), qMax(leftPeerIedNameList.size(), rightPeerIedNameList.size()));
	if (maxRowCount <= 0)
	{
		maxRowCount = 1;
	}

	int leftRectX = BAY_LOGIC_LEFT_MARGIN;
	int centerRectX = leftRectX + RECT_DEFAULT_WIDTH + BAY_LOGIC_SIDE_DISTANCE;
	int gseBaseX = centerRectX + RECT_DEFAULT_WIDTH + BAY_LOGIC_CHANNEL_START_GAP;
	int svBaseX = gseBaseX;
	if (gseLineCount > 0)
	{
		svBaseX += gseLineCount * BAY_LOGIC_CHANNEL_GAP;
	}
	if (gseLineCount > 0 && svLineCount > 0)
	{
		svBaseX += BAY_LOGIC_CHANNEL_TYPE_GAP;
	}
	int lastChannelX = centerRectX + RECT_DEFAULT_WIDTH;
	if (gseLineCount > 0)
	{
		lastChannelX = qMax(lastChannelX, gseBaseX + (gseLineCount - 1) * BAY_LOGIC_CHANNEL_GAP);
	}
	if (svLineCount > 0)
	{
		lastChannelX = qMax(lastChannelX, svBaseX + (svLineCount - 1) * BAY_LOGIC_CHANNEL_GAP);
	}
	int rightRectX = qMax(centerRectX + RECT_DEFAULT_WIDTH + BAY_LOGIC_SIDE_DISTANCE, lastChannelX + BAY_LOGIC_CHANNEL_START_GAP);

	LayoutRectList(m_pCircuitConfig, bayIedNameList, svg.centerIedRectList, centerRectX, maxRowCount, ColorHelper::pure_red);
	LayoutRectList(m_pCircuitConfig, leftPeerIedNameList, svg.leftIedRectList, leftRectX, maxRowCount, ColorHelper::pure_green);
	LayoutRectList(m_pCircuitConfig, rightPeerIedNameList, svg.rightIedRectList, rightRectX, maxRowCount, ColorHelper::pure_green);

	QMap<QString, IedRect*> rectHash;
	QMap<IedRect*, int> rectAreaHash;
	for (int rectIndex = 0; rectIndex < svg.centerIedRectList.size(); ++rectIndex)
	{
		rectHash.insert(svg.centerIedRectList.at(rectIndex)->iedName, svg.centerIedRectList.at(rectIndex));
		rectAreaHash.insert(svg.centerIedRectList.at(rectIndex), RECT_AREA_CENTER);
	}
	for (int rectIndex = 0; rectIndex < svg.leftIedRectList.size(); ++rectIndex)
	{
		rectHash.insert(svg.leftIedRectList.at(rectIndex)->iedName, svg.leftIedRectList.at(rectIndex));
		rectAreaHash.insert(svg.leftIedRectList.at(rectIndex), RECT_AREA_LEFT);
	}
	for (int rectIndex = 0; rectIndex < svg.rightIedRectList.size(); ++rectIndex)
	{
		rectHash.insert(svg.rightIedRectList.at(rectIndex)->iedName, svg.rightIedRectList.at(rectIndex));
		rectAreaHash.insert(svg.rightIedRectList.at(rectIndex), RECT_AREA_RIGHT);
	}

	QList<LogicLayoutLine> layoutLineList;
	QMap<LogicCircuitLine*, int> internalRouteXHash;
	QMap<IedRect*, int> leftTotalCountHash;
	QMap<IedRect*, int> rightTotalCountHash;
	int nextGseX = gseBaseX;
	int nextSvX = svBaseX;

	groupIt = internalGroupHash.constBegin();
	for (; groupIt != internalGroupHash.constEnd(); ++groupIt)
	{
		const InternalCircuitGroup& group = groupIt.value();
		IedRect* pFirstRect = rectHash.value(group.firstIedName, NULL);
		IedRect* pSecondRect = rectHash.value(group.secondIedName, NULL);
		if (!pFirstRect || !pSecondRect)
		{
			continue;
		}
		IedRect* pStartRect = pFirstRect;
		IedRect* pEndRect = pSecondRect;
		QString startIedName = group.firstIedName;
		QString endIedName = group.secondIedName;
		if (pFirstRect->y > pSecondRect->y)
		{
			pStartRect = pSecondRect;
			pEndRect = pFirstRect;
			startIedName = group.secondIedName;
			endIedName = group.firstIedName;
		}
		int lineCount = qMax(group.firstToSecondList.size(), group.secondToFirstList.size());
		for (int lineIndex = 0; lineIndex < lineCount; ++lineIndex)
		{
			LogicCircuit* pPreferCircuit = lineIndex < group.firstToSecondList.size() ?
				group.firstToSecondList.at(lineIndex) : group.secondToFirstList.at(lineIndex);
			if (!pPreferCircuit)
			{
				continue;
			}
			LogicCircuitLine* pLine = new LogicCircuitLine();
			pLine->pLogicCircuit = pPreferCircuit;
			pLine->pSrcIedRect = pStartRect;
			pLine->pDestIedRect = pEndRect;
			bool hasForward = lineIndex < group.firstToSecondList.size();
			bool hasBackward = lineIndex < group.secondToFirstList.size();
			if (hasForward && hasBackward)
			{
				pLine->srcArrowState = Arrow_In;
				pLine->destArrowState = Arrow_In;
			}
			else
			{
				QString actualSrcName = pPreferCircuit->pSrcIed ? pPreferCircuit->pSrcIed->name : QString();
				if (actualSrcName == startIedName)
				{
					pLine->srcArrowState = Arrow_None;
					pLine->destArrowState = Arrow_In;
				}
				else
				{
					pLine->srcArrowState = Arrow_In;
					pLine->destArrowState = Arrow_None;
				}
			}
			pLine->arrowState = pLine->srcArrowState | pLine->destArrowState;
			pStartRect->logic_line_list.append(pLine);
			LogicLayoutLine layoutLine;
			layoutLine.pLine = pLine;
			layoutLine.isInternal = true;
			layoutLine.routeX = pPreferCircuit->type == SV ? nextSvX : nextGseX;
			layoutLineList.append(layoutLine);
			internalRouteXHash.insert(pLine, layoutLine.routeX);
			rightTotalCountHash.insert(pStartRect, rightTotalCountHash.value(pStartRect, 0) + 1);
			rightTotalCountHash.insert(pEndRect, rightTotalCountHash.value(pEndRect, 0) + 1);
			if (pPreferCircuit->type == SV)
			{
				nextSvX += BAY_LOGIC_CHANNEL_GAP;
			}
			else
			{
				nextGseX += BAY_LOGIC_CHANNEL_GAP;
			}
		}
	}

	for (int logicIndex = 0; logicIndex < externalLogicList.size(); ++logicIndex)
	{
		LogicCircuit* pLogicCircuit = externalLogicList.at(logicIndex);
		if (!pLogicCircuit || !pLogicCircuit->pSrcIed || !pLogicCircuit->pDestIed)
		{
			continue;
		}
		IedRect* pSrcRect = rectHash.value(pLogicCircuit->pSrcIed->name, NULL);
		IedRect* pDestRect = rectHash.value(pLogicCircuit->pDestIed->name, NULL);
		if (!pSrcRect || !pDestRect)
		{
			continue;
		}
		LogicCircuitLine* pLine = new LogicCircuitLine();
		pLine->pLogicCircuit = pLogicCircuit;
		pLine->pSrcIedRect = pSrcRect;
		pLine->pDestIedRect = pDestRect;
		pLine->srcArrowState = Arrow_None;
		pLine->destArrowState = Arrow_In;
		pLine->arrowState = Arrow_In;
		pSrcRect->logic_line_list.append(pLine);
		LogicLayoutLine layoutLine;
		layoutLine.pLine = pLine;
		layoutLine.isInternal = false;
		layoutLineList.append(layoutLine);

		int srcArea = rectAreaHash.value(pSrcRect, RECT_AREA_CENTER);
		int destArea = rectAreaHash.value(pDestRect, RECT_AREA_CENTER);
		bool srcUseRightSide = srcArea == RECT_AREA_LEFT || (srcArea == RECT_AREA_CENTER && destArea == RECT_AREA_RIGHT);
		bool destUseRightSide = destArea == RECT_AREA_LEFT || (destArea == RECT_AREA_CENTER && srcArea == RECT_AREA_RIGHT);
		QMap<IedRect*, int>& srcTotalCountHash = srcUseRightSide ? rightTotalCountHash : leftTotalCountHash;
		QMap<IedRect*, int>& destTotalCountHash = destUseRightSide ? rightTotalCountHash : leftTotalCountHash;
		srcTotalCountHash.insert(pSrcRect, srcTotalCountHash.value(pSrcRect, 0) + 1);
		destTotalCountHash.insert(pDestRect, destTotalCountHash.value(pDestRect, 0) + 1);
	}

	QMap<IedRect*, int> leftUsedCountHash;
	QMap<IedRect*, int> rightUsedCountHash;
	for (int layoutIndex = 0; layoutIndex < layoutLineList.size(); ++layoutIndex)
	{
		LogicCircuitLine* pLine = layoutLineList.at(layoutIndex).pLine;
		if (!pLine || !pLine->pSrcIedRect || !pLine->pDestIedRect)
		{
			continue;
		}
		int srcArea = rectAreaHash.value(pLine->pSrcIedRect, RECT_AREA_CENTER);
		int destArea = rectAreaHash.value(pLine->pDestIedRect, RECT_AREA_CENTER);
		if (layoutLineList.at(layoutIndex).isInternal)
		{
			pLine->startPoint = BuildConnectPoint(pLine->pSrcIedRect, true, rightUsedCountHash, rightTotalCountHash);
			pLine->endPoint = BuildConnectPoint(pLine->pDestIedRect, true, rightUsedCountHash, rightTotalCountHash);
			int routeX = internalRouteXHash.value(pLine, pLine->startPoint.x());
			pLine->turnPointList.clear();
			pLine->turnPointList.append(QPoint(routeX, pLine->startPoint.y()));
			pLine->turnPointList.append(QPoint(routeX, pLine->endPoint.y()));
			pLine->midPoint = pLine->turnPointList.first();
		}
		else
		{
			bool srcUseRightSide = srcArea == RECT_AREA_LEFT || (srcArea == RECT_AREA_CENTER && destArea == RECT_AREA_RIGHT);
			bool destUseRightSide = destArea == RECT_AREA_LEFT || (destArea == RECT_AREA_CENTER && srcArea == RECT_AREA_RIGHT);
			pLine->startPoint = BuildConnectPoint(pLine->pSrcIedRect,
				srcUseRightSide,
				srcUseRightSide ? rightUsedCountHash : leftUsedCountHash,
				srcUseRightSide ? rightTotalCountHash : leftTotalCountHash);
			pLine->endPoint = BuildConnectPoint(pLine->pDestIedRect,
				destUseRightSide,
				destUseRightSide ? rightUsedCountHash : leftUsedCountHash,
				destUseRightSide ? rightTotalCountHash : leftTotalCountHash);
			QPoint centerPoint = srcArea == RECT_AREA_CENTER ? pLine->startPoint : pLine->endPoint;
			int midPointX = qMin(pLine->startPoint.x(), pLine->endPoint.x()) + qAbs(pLine->startPoint.x() - pLine->endPoint.x()) / 2;
			pLine->turnPointList.clear();
			pLine->midPoint = QPoint(midPointX, centerPoint.y());
		}
	}

	int maxBottom = BAY_LOGIC_TOP_MARGIN + RECT_DEFAULT_HEIGHT;
	QList<IedRect*> allRectList = svg.centerIedRectList + svg.leftIedRectList + svg.rightIedRectList;
	for (int rectIndex = 0; rectIndex < allRectList.size(); ++rectIndex)
	{
		maxBottom = qMax(maxBottom, (int)(allRectList.at(rectIndex)->y + allRectList.at(rectIndex)->height));
	}
	int viewRight = centerRectX + RECT_DEFAULT_WIDTH;
	if (!svg.rightIedRectList.isEmpty())
	{
		viewRight = qMax(viewRight, rightRectX + RECT_DEFAULT_WIDTH);
	}
	else
	{
		viewRight = qMax(viewRight, lastChannelX + BAY_LOGIC_CHANNEL_START_GAP);
	}
	svg.viewBoxX = 0;
	svg.viewBoxY = 0;
	svg.viewBoxWidth = viewRight + BAY_LOGIC_LEFT_MARGIN;
	svg.viewBoxHeight = maxBottom + BAY_LOGIC_BOTTOM_MARGIN;
}

void LogicDiagramBuilder::AdjustLogicCircuitLinePosition(LogicSvg& svg)
{
	// 调整左右IED列表与主IED的逻辑链路位置
	AdjustMainSideCircuitLinePosition(svg.GetLeftLogicCircuitSize(), svg.leftIedRectList, svg.mainIedRect);
	AdjustMainSideCircuitLinePosition(svg.GetRightLogicCircuitSize(), svg.rightIedRectList, svg.mainIedRect, false);
}
