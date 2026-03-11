#include "Optical/OpticalDiagramBuilder.h"
#include <QSet>

using utils::ColorHelper;

OpticalDiagramBuilder::OpticalDiagramBuilder()
{
}

OpticalDiagramBuilder::~OpticalDiagramBuilder()
{
}

namespace
{
	static bool IsSwitchIedName(const QString& iedName)
	{
		return iedName.contains("SW", Qt::CaseInsensitive);
	}
	static QString ReadLocalText(const char* rawText, const QString& defaultText = QString())
	{
		if (!rawText || rawText[0] == '\0')
		{
			return defaultText;
		}
		return QString::fromLocal8Bit(rawText);
	}
	static QString GetStationDisplayName(const CRtdbEleModelStation* station)
	{
		if (!station || !station->m_pStationInfo)
		{
			return QString("Station");
		}
		QString stationText = ReadLocalText(station->m_pStationInfo->stationDesc);
		if (!stationText.isEmpty())
		{
			return stationText;
		}
		return QString("Station");
	}
	static QString GetBayKey(const CRtdbEleModelBay* bay)
	{
		if (!bay || !bay->m_pBayInfo)
		{
			return QString();
		}
		QString bayName = ReadLocalText(bay->m_pBayInfo->bayName);
		if (!bayName.isEmpty())
		{
			return bayName;
		}
		return ReadLocalText(bay->m_pBayInfo->bayDesc);
	}
	static QString GetBayDisplayName(const CRtdbEleModelBay* bay)
	{
		if (!bay || !bay->m_pBayInfo)
		{
			return QString("Bay");
		}
		QString bayText = ReadLocalText(bay->m_pBayInfo->bayDesc);
		if (!bayText.isEmpty())
		{
			return bayText;
		}
		bayText = ReadLocalText(bay->m_pBayInfo->bayName);
		if (!bayText.isEmpty())
		{
			return bayText;
		}
		return QString("Bay");
	}
	static void CollectBayContext(const CRtdbEleModelStation* station,
		CircuitConfig* circuitConfig,
		QMap<QString, QStringList>& bayIedHash,
		QMap<QString, QString>& bayTextHash,
		QMap<QString, QString>& iedBayHash,
		QStringList& bayOrder,
		QString& stationText)
	{
		stationText = GetStationDisplayName(station);
		if (!station)
		{
			return;
		}
		for (std::list<CRtdbEleModelVoltLevel*>::const_iterator voltIt = station->m_listVoltLevel.begin();
			voltIt != station->m_listVoltLevel.end(); ++voltIt)
		{
			const CRtdbEleModelVoltLevel* volt = *voltIt;
			if (!volt)
			{
				continue;
			}
			for (std::list<CRtdbEleModelBay*>::const_iterator bayIt = volt->m_listBay.begin();
				bayIt != volt->m_listBay.end(); ++bayIt)
			{
				const CRtdbEleModelBay* bay = *bayIt;
				QString bayKey = GetBayKey(bay);
				if (bayKey.isEmpty())
				{
					continue;
				}
				if (!bayTextHash.contains(bayKey))
				{
					bayOrder.append(bayKey);
					bayTextHash.insert(bayKey, GetBayDisplayName(bay));
				}
				QStringList& bayIedList = bayIedHash[bayKey];
				for (std::list<CRtdbEleModelIed*>::const_iterator iedIt = bay->m_listIed.begin();
					iedIt != bay->m_listIed.end(); ++iedIt)
				{
					const CRtdbEleModelIed* modelIed = *iedIt;
					if (!modelIed || !modelIed->m_pIedHead)
					{
						continue;
					}
					QString iedName = ReadLocalText(modelIed->m_pIedHead->iedName);
					if (iedName.isEmpty() || !circuitConfig->GetIedByName(iedName))
					{
						continue;
					}
					if (!bayIedList.contains(iedName))
					{
						bayIedList.append(iedName);
					}
					iedBayHash.insert(iedName, bayKey);
				}
			}
		}
	}
	static QStringList BuildVisibleBayOrder(const QStringList& stationBayOrder,
		const QSet<QString>& visibleBaySet,
		const QString& firstBay)
	{
		QStringList result;
		if (!firstBay.isEmpty() && visibleBaySet.contains(firstBay))
		{
			result.append(firstBay);
		}
		for (int i = 0; i < stationBayOrder.size(); ++i)
		{
			const QString& bayKey = stationBayOrder.at(i);
			if (visibleBaySet.contains(bayKey) && !result.contains(bayKey))
			{
				result.append(bayKey);
			}
		}
		QSet<QString>::const_iterator bayIt = visibleBaySet.constBegin();
		for (; bayIt != visibleBaySet.constEnd(); ++bayIt)
		{
			if (!result.contains(*bayIt))
			{
				result.append(*bayIt);
			}
		}
		return result;
	}
	static QStringList FilterVisibleIedList(const QStringList& sourceIedList,
		const QSet<QString>& visibleIedSet,
		bool keepAll)
	{
		QStringList result;
		for (int i = 0; i < sourceIedList.size(); ++i)
		{
			const QString& iedName = sourceIedList.at(i);
			if (keepAll || visibleIedSet.contains(iedName))
			{
				result.append(iedName);
			}
		}
		return result;
	}
	static IedRect* CreateScopeRect(const IED* pIed, int x, int y)
	{
		if (!pIed)
		{
			return NULL;
		}
		return utils::GetIedRect(
			pIed->name,
			pIed->desc,
			(quint16)x,
			(quint16)y,
			RECT_DEFAULT_WIDTH,
			RECT_DEFAULT_HEIGHT,
			ColorHelper::ied_border,
			ColorHelper::ied_underground);
	}
	static IedRect* CreateScopeTitleRect(const QString& titleName, const QString& titleDesc, int totalWidth)
	{
		int titleWidth = RECT_DEFAULT_WIDTH + 140;
		if (totalWidth > titleWidth + 120)
		{
			titleWidth = qMin(totalWidth - 120, 620);
		}
		int titleX = totalWidth > titleWidth ? (totalWidth - titleWidth) / 2 : 40;
		if (titleX < 40)
		{
			titleX = 40;
		}
		return utils::GetIedRect(
			titleName,
			titleDesc,
			(quint16)titleX,
			40,
			(quint16)titleWidth,
			RECT_DEFAULT_HEIGHT,
			ColorHelper::pure_red,
			ColorHelper::pure_black);
	}
	static int GetScopedRectBottom(const IedRect* pRect)
	{
		if (!pRect)
		{
			return 0;
		}
		return pRect->y + pRect->height;
	}
		enum ScopedAnchorSide
	{
		ScopedAnchorTop = 0,
		ScopedAnchorBottom = 1
	};
	enum ScopedLayerType
	{
		ScopedLayerTop = 0,
		ScopedLayerSwitch = 1,
		ScopedLayerBottom = 2
	};
	struct ScopedRectRouteInfo
	{
		ScopedRectRouteInfo()
		{
			pRect = NULL;
			layerType = ScopedLayerTop;
			orderIndex = -1;
		}
		IedRect* pRect;
		int layerType;
		int orderIndex;
	};
	struct ScopedOpticalRoutePlan
	{
		ScopedOpticalRoutePlan()
		{
			pOpticalCircuit = NULL;
			pSrcRect = NULL;
			pDestRect = NULL;
			srcLayerType = ScopedLayerTop;
			destLayerType = ScopedLayerBottom;
			startSide = ScopedAnchorBottom;
			endSide = ScopedAnchorTop;
			startIndex = 0;
			startCount = 1;
			endIndex = 0;
			endCount = 1;
			horizontalMinX = 0;
			horizontalMaxX = 0;
			laneIndex = 0;
			trunkY = 0;
			startVerticalLaneIndex = 0;
			startVerticalLaneCount = 1;
			endVerticalLaneIndex = 0;
			endVerticalLaneCount = 1;
			startVerticalMinY = 0;
			startVerticalMaxY = 0;
			endVerticalMinY = 0;
			endVerticalMaxY = 0;
			startVerticalX = 0;
			endVerticalX = 0;
		}
		OpticalCircuit* pOpticalCircuit;
		IedRect* pSrcRect;
		IedRect* pDestRect;
		int srcLayerType;
		int destLayerType;
		int startSide;
		int endSide;
		int startIndex;
		int startCount;
		int endIndex;
		int endCount;
		int horizontalMinX;
		int horizontalMaxX;
		int laneIndex;
		int trunkY;
		int startVerticalLaneIndex;
		int startVerticalLaneCount;
		int endVerticalLaneIndex;
		int endVerticalLaneCount;
		int startVerticalMinY;
		int startVerticalMaxY;
		int endVerticalMinY;
		int endVerticalMaxY;
		int startVerticalX;
		int endVerticalX;
		QString startSideKey;
		QString endSideKey;
		QString corridorKey;
		QString overlapGroupKey;
		QString startVerticalGroupKey;
		QString endVerticalGroupKey;
	};
	struct ScopedVerticalSegmentRef
	{
		ScopedVerticalSegmentRef()
		{
			routePlanIndex = -1;
			isStartSegment = true;
			minY = 0;
			maxY = 0;
			laneIndex = 0;
		}
		int routePlanIndex;
		bool isStartSegment;
		int minY;
		int maxY;
		int laneIndex;
	};
	static QString BuildScopedSideKey(const QString& iedName, int anchorSide)
	{
		return QString("%1|%2").arg(iedName).arg(anchorSide);
	}
	static QPoint BuildScopedAnchorPoint(const IedRect* pRect, int anchorSide, int anchorIndex, int anchorCount)
	{
		if (!pRect)
		{
			return QPoint();
		}
		int actualCount = qMax(anchorCount, 1);
		int actualIndex = anchorIndex;
		if (actualIndex < 0)
		{
			actualIndex = 0;
		}
		if (actualIndex >= actualCount)
		{
			actualIndex = actualCount - 1;
		}
		int innerMargin = 24;
		int usableWidth = (int)pRect->width - innerMargin * 2;
		if (usableWidth < 40)
		{
			innerMargin = 10;
			usableWidth = (int)pRect->width - innerMargin * 2;
		}
		int anchorX = 0;
		if (usableWidth > 0)
		{
			anchorX = pRect->x + innerMargin + usableWidth * (actualIndex + 1) / (actualCount + 1);
		}
		else
		{
			anchorX = pRect->x + (int)pRect->width * (actualIndex + 1) / (actualCount + 1);
		}
		int anchorY = anchorSide == ScopedAnchorTop ? pRect->y : GetScopedRectBottom(pRect);
		return QPoint(anchorX, anchorY);
	}
	static void AppendScopedPoint(QList<QPoint>& pointList, const QPoint& point)
	{
		if (pointList.isEmpty() || pointList.last() != point)
		{
			pointList.append(point);
		}
	}
	static int BuildScopedSpreadOffset(int routeIndex, int routeCount, int step)
	{
		int actualCount = routeCount;
		if (actualCount < 1)
		{
			actualCount = 1;
		}
		int actualIndex = routeIndex;
		if (actualIndex < 0)
		{
			actualIndex = 0;
		}
		if (actualIndex >= actualCount)
		{
			actualIndex = actualCount - 1;
		}
		return (actualIndex * 2 - (actualCount - 1)) * step / 2;
	}
	static int GetScopedMaxAbsSpread(int routeCount, int step)
	{
		if (routeCount <= 1)
		{
			return 0;
		}
		return qAbs(BuildScopedSpreadOffset(routeCount - 1, routeCount, step));
	}
	static int EstimateScopedLayerGap(int maxSideCount, int maxCorridorCount, int baseLayerGap)
	{
		int maxSideSpread = GetScopedMaxAbsSpread(maxSideCount, SCOPED_BRANCH_STEP);
		int maxCorridorSpread = GetScopedMaxAbsSpread(maxCorridorCount, SCOPED_LANE_STEP);
		int requiredGap = SAFE_DISTANCE + SCOPED_BRANCH_BASE + SCOPED_TRUNK_BASE + maxSideSpread + maxCorridorSpread + 40;
		if (requiredGap < baseLayerGap)
		{
			requiredGap = baseLayerGap;
		}
		return requiredGap;
	}
	static bool IsScopedHorizontalOverlap(int minX_1, int maxX_1, int minX_2, int maxX_2)
	{
		return !(maxX_1 < minX_2 || maxX_2 < minX_1);
	}
	static void SortScopedRouteIndicesByHorizontalSpan(QList<int>& routeIndexList, const QList<ScopedOpticalRoutePlan>& routePlanList)
	{
		for (int leftIndex = 0; leftIndex < routeIndexList.size(); ++leftIndex)
		{
			for (int rightIndex = leftIndex + 1; rightIndex < routeIndexList.size(); ++rightIndex)
			{
				const ScopedOpticalRoutePlan& leftRoutePlan = routePlanList.at(routeIndexList.at(leftIndex));
				const ScopedOpticalRoutePlan& rightRoutePlan = routePlanList.at(routeIndexList.at(rightIndex));
				bool shouldSwap = false;
				if (rightRoutePlan.horizontalMinX < leftRoutePlan.horizontalMinX)
				{
					shouldSwap = true;
				}
				else if (rightRoutePlan.horizontalMinX == leftRoutePlan.horizontalMinX &&
					rightRoutePlan.horizontalMaxX < leftRoutePlan.horizontalMaxX)
				{
					shouldSwap = true;
				}
				if (shouldSwap)
				{
					int tempRouteIndex = routeIndexList.at(leftIndex);
					routeIndexList[leftIndex] = routeIndexList.at(rightIndex);
					routeIndexList[rightIndex] = tempRouteIndex;
				}
			}
		}
	}
	static QString BuildScopedHorizontalGroupKey(int startSide, int endSide)
	{
		if (startSide == ScopedAnchorBottom && endSide == ScopedAnchorBottom)
		{
			return "bottom";
		}
		if (startSide == ScopedAnchorTop && endSide == ScopedAnchorTop)
		{
			return "top";
		}
		return "middle";
	}
	static int AssignScopedOverlapLaneIndices(QList<ScopedOpticalRoutePlan>& routePlanList)
	{
		QHash<QString, QList<int> > overlapGroupRouteHash;
		for (int routePlanIndex = 0; routePlanIndex < routePlanList.size(); ++routePlanIndex)
		{
			const ScopedOpticalRoutePlan& routePlan = routePlanList.at(routePlanIndex);
			overlapGroupRouteHash[routePlan.overlapGroupKey].append(routePlanIndex);
		}
		int maxLaneCount = 1;
		QHash<QString, QList<int> >::iterator groupIt = overlapGroupRouteHash.begin();
		for (; groupIt != overlapGroupRouteHash.end(); ++groupIt)
		{
			QList<int> routeIndexList = groupIt.value();
			SortScopedRouteIndicesByHorizontalSpan(routeIndexList, routePlanList);
			QList<int> laneRightXList;
			for (int routeIndex = 0; routeIndex < routeIndexList.size(); ++routeIndex)
			{
				ScopedOpticalRoutePlan& routePlan = routePlanList[routeIndexList.at(routeIndex)];
				int assignedLane = -1;
				for (int laneIndex = 0; laneIndex < laneRightXList.size(); ++laneIndex)
				{
					int laneRightX = laneRightXList.at(laneIndex);
					if (!IsScopedHorizontalOverlap(routePlan.horizontalMinX, routePlan.horizontalMaxX, 0, laneRightX))
					{
						assignedLane = laneIndex;
						laneRightXList[laneIndex] = routePlan.horizontalMaxX;
						break;
					}
				}
				if (assignedLane < 0)
				{
					assignedLane = laneRightXList.size();
					laneRightXList.append(routePlan.horizontalMaxX);
				}
				routePlan.laneIndex = assignedLane;
			}
			int laneCount = laneRightXList.size();
			for (int routeIndex = 0; routeIndex < routeIndexList.size(); ++routeIndex)
			{
				ScopedOpticalRoutePlan& routePlan = routePlanList[routeIndexList.at(routeIndex)];
				routePlan.laneIndex = laneCount - 1 - routePlan.laneIndex;
			}
			maxLaneCount = qMax(maxLaneCount, laneCount);
		}
		return maxLaneCount;
	}
	static QPoint MoveScopedPointBySide(const QPoint& point, int anchorSide, int distance)
	{
		if (anchorSide == ScopedAnchorTop)
		{
			return QPoint(point.x(), point.y() - distance);
		}
		return QPoint(point.x(), point.y() + distance);
	}
	static int ComputeScopedTrunkY(const ScopedOpticalRoutePlan& routePlan)
	{
		QPoint startPoint = BuildScopedAnchorPoint(routePlan.pSrcRect, routePlan.startSide, routePlan.startIndex, routePlan.startCount);
		QPoint endPoint = BuildScopedAnchorPoint(routePlan.pDestRect, routePlan.endSide, routePlan.endIndex, routePlan.endCount);
		QPoint startSafePoint = MoveScopedPointBySide(startPoint, routePlan.startSide, SAFE_DISTANCE);
		QPoint endSafePoint = MoveScopedPointBySide(endPoint, routePlan.endSide, SAFE_DISTANCE);
		int startBranchOffset = SCOPED_BRANCH_BASE + qAbs(BuildScopedSpreadOffset(routePlan.startIndex, routePlan.startCount, SCOPED_BRANCH_STEP));
		QPoint startBranchPoint = MoveScopedPointBySide(startSafePoint, routePlan.startSide, startBranchOffset);
		int trunkY = startBranchPoint.y();
		if (routePlan.startSide != routePlan.endSide)
		{
			int upperY = qMin(startSafePoint.y(), endSafePoint.y());
			trunkY = upperY + SCOPED_TRUNK_BASE + routePlan.laneIndex * SCOPED_LANE_STEP;
		}
		else if (routePlan.startSide == ScopedAnchorBottom && routePlan.endSide == ScopedAnchorBottom)
		{
			int baseY = qMax(startSafePoint.y(), endSafePoint.y());
			trunkY = baseY + SCOPED_TRUNK_BASE + routePlan.laneIndex * SCOPED_LANE_STEP;
		}
		else
		{
			int baseY = qMin(startSafePoint.y(), endSafePoint.y());
			trunkY = baseY - SCOPED_TRUNK_BASE - routePlan.laneIndex * SCOPED_LANE_STEP;
		}
		return trunkY;
	}
	static void SortScopedVerticalSegments(QList<ScopedVerticalSegmentRef>& segmentList)
	{
		for (int leftIndex = 0; leftIndex < segmentList.size(); ++leftIndex)
		{
			for (int rightIndex = leftIndex + 1; rightIndex < segmentList.size(); ++rightIndex)
			{
				const ScopedVerticalSegmentRef& leftSegment = segmentList.at(leftIndex);
				const ScopedVerticalSegmentRef& rightSegment = segmentList.at(rightIndex);
				bool shouldSwap = false;
				if (rightSegment.minY < leftSegment.minY)
				{
					shouldSwap = true;
				}
				else if (rightSegment.minY == leftSegment.minY && rightSegment.maxY < leftSegment.maxY)
				{
					shouldSwap = true;
				}
				if (shouldSwap)
				{
					ScopedVerticalSegmentRef tempSegment = segmentList.at(leftIndex);
					segmentList[leftIndex] = segmentList.at(rightIndex);
					segmentList[rightIndex] = tempSegment;
				}
			}
		}
	}
	static int ComputeScopedVerticalLaneX(int safePointX, int laneIndex, int laneCount)
	{
				if (laneCount <= 1)
		{
			return safePointX;
		}
		return safePointX + BuildScopedSpreadOffset(laneIndex, laneCount, SCOPED_LANE_STEP);
	}
	static void RefreshScopedHorizontalSpan(ScopedOpticalRoutePlan& routePlan)
	{
		routePlan.horizontalMinX = qMin(routePlan.startVerticalX, routePlan.endVerticalX);
		routePlan.horizontalMaxX = qMax(routePlan.startVerticalX, routePlan.endVerticalX);
	}
	static int AssignScopedVerticalLaneIndices(QList<ScopedOpticalRoutePlan>& routePlanList)
	{
		QHash<QString, QList<ScopedVerticalSegmentRef> > verticalGroupHash;
		for (int routePlanIndex = 0; routePlanIndex < routePlanList.size(); ++routePlanIndex)
		{
			ScopedOpticalRoutePlan& routePlan = routePlanList[routePlanIndex];
			routePlan.trunkY = ComputeScopedTrunkY(routePlan);
			routePlan.startVerticalLaneIndex = 0;
			routePlan.startVerticalLaneCount = 1;
			routePlan.endVerticalLaneIndex = 0;
			routePlan.endVerticalLaneCount = 1;
			routePlan.startVerticalX = 0;
			routePlan.endVerticalX = 0;
			QPoint startPoint = BuildScopedAnchorPoint(routePlan.pSrcRect, routePlan.startSide, routePlan.startIndex, routePlan.startCount);
			QPoint endPoint = BuildScopedAnchorPoint(routePlan.pDestRect, routePlan.endSide, routePlan.endIndex, routePlan.endCount);
			QPoint startSafePoint = MoveScopedPointBySide(startPoint, routePlan.startSide, SAFE_DISTANCE);
			QPoint endSafePoint = MoveScopedPointBySide(endPoint, routePlan.endSide, SAFE_DISTANCE);
			routePlan.startVerticalGroupKey = QString::number(startSafePoint.x());
			routePlan.endVerticalGroupKey = QString::number(endSafePoint.x());
			routePlan.startVerticalX = startSafePoint.x();
			routePlan.endVerticalX = endSafePoint.x();
			routePlan.startVerticalMinY = qMin(startSafePoint.y(), routePlan.trunkY);
			routePlan.startVerticalMaxY = qMax(startSafePoint.y(), routePlan.trunkY);
			routePlan.endVerticalMinY = qMin(endSafePoint.y(), routePlan.trunkY);
			routePlan.endVerticalMaxY = qMax(endSafePoint.y(), routePlan.trunkY);
			if (startSafePoint.y() != routePlan.trunkY)
			{
				ScopedVerticalSegmentRef startSegment;
				startSegment.routePlanIndex = routePlanIndex;
				startSegment.isStartSegment = true;
				startSegment.minY = routePlan.startVerticalMinY;
				startSegment.maxY = routePlan.startVerticalMaxY;
				verticalGroupHash[routePlan.startVerticalGroupKey].append(startSegment);
			}
			if (endSafePoint.y() != routePlan.trunkY)
			{
				ScopedVerticalSegmentRef endSegment;
				endSegment.routePlanIndex = routePlanIndex;
				endSegment.isStartSegment = false;
				endSegment.minY = routePlan.endVerticalMinY;
				endSegment.maxY = routePlan.endVerticalMaxY;
				verticalGroupHash[routePlan.endVerticalGroupKey].append(endSegment);
			}
		}
		int maxLaneCount = 1;
		QHash<QString, QList<ScopedVerticalSegmentRef> >::iterator groupIt = verticalGroupHash.begin();
		for (; groupIt != verticalGroupHash.end(); ++groupIt)
		{
			QList<ScopedVerticalSegmentRef> segmentList = groupIt.value();
			SortScopedVerticalSegments(segmentList);
			QList<int> laneBottomYList;
			for (int segmentIndex = 0; segmentIndex < segmentList.size(); ++segmentIndex)
			{
				ScopedVerticalSegmentRef& segmentRef = segmentList[segmentIndex];
				int assignedLane = -1;
				for (int laneIndex = 0; laneIndex < laneBottomYList.size(); ++laneIndex)
				{
					if (segmentRef.minY >= laneBottomYList.at(laneIndex))
					{
						assignedLane = laneIndex;
						laneBottomYList[laneIndex] = segmentRef.maxY;
						break;
					}
				}
				if (assignedLane < 0)
				{
					assignedLane = laneBottomYList.size();
					laneBottomYList.append(segmentRef.maxY);
				}
				segmentRef.laneIndex = assignedLane;
				ScopedOpticalRoutePlan& routePlan = routePlanList[segmentRef.routePlanIndex];
				if (segmentRef.isStartSegment)
				{
					routePlan.startVerticalLaneIndex = assignedLane;
				}
				else
				{
					routePlan.endVerticalLaneIndex = assignedLane;
				}
			}
			for (int segmentIndex = 0; segmentIndex < segmentList.size(); ++segmentIndex)
			{
				const ScopedVerticalSegmentRef& segmentRef = segmentList.at(segmentIndex);
				ScopedOpticalRoutePlan& routePlan = routePlanList[segmentRef.routePlanIndex];
				if (segmentRef.isStartSegment)
				{
					routePlan.startVerticalLaneCount = qMax(1, laneBottomYList.size());
				}
				else
				{
					routePlan.endVerticalLaneCount = qMax(1, laneBottomYList.size());
				}
			}
			maxLaneCount = qMax(maxLaneCount, laneBottomYList.size());
		}
		for (int routePlanIndex = 0; routePlanIndex < routePlanList.size(); ++routePlanIndex)
		{
			ScopedOpticalRoutePlan& routePlan = routePlanList[routePlanIndex];
			QPoint startPoint = BuildScopedAnchorPoint(routePlan.pSrcRect, routePlan.startSide, routePlan.startIndex, routePlan.startCount);
			QPoint endPoint = BuildScopedAnchorPoint(routePlan.pDestRect, routePlan.endSide, routePlan.endIndex, routePlan.endCount);
			QPoint startSafePoint = MoveScopedPointBySide(startPoint, routePlan.startSide, SAFE_DISTANCE);
			QPoint endSafePoint = MoveScopedPointBySide(endPoint, routePlan.endSide, SAFE_DISTANCE);
			if (startSafePoint.y() != routePlan.trunkY)
			{
				routePlan.startVerticalX = ComputeScopedVerticalLaneX(startSafePoint.x(), routePlan.startVerticalLaneIndex, routePlan.startVerticalLaneCount);
			}
			else
			{
				routePlan.startVerticalX = startSafePoint.x();
			}
			if (endSafePoint.y() != routePlan.trunkY)
			{
				routePlan.endVerticalX = ComputeScopedVerticalLaneX(endSafePoint.x(), routePlan.endVerticalLaneIndex, routePlan.endVerticalLaneCount);
			}
			else
			{
				routePlan.endVerticalX = endSafePoint.x();
			}
		}
		return maxLaneCount;
	}
	static int RefineScopedRouteLayout(QList<ScopedOpticalRoutePlan>& routePlanList)
	{
		int maxLaneCount = 1;
		for (int refineIndex = 0; refineIndex < SCOPED_LAYOUT_REFINE_PASSES; ++refineIndex)
		{
			QList<ScopedOpticalRoutePlan> previousRoutePlanList = routePlanList;
			for (int routePlanIndex = 0; routePlanIndex < routePlanList.size(); ++routePlanIndex)
			{
				ScopedOpticalRoutePlan& routePlan = routePlanList[routePlanIndex];
				RefreshScopedHorizontalSpan(routePlan);
			}
			int laneCount = AssignScopedOverlapLaneIndices(routePlanList);
			if (laneCount > maxLaneCount)
			{
				maxLaneCount = laneCount;
			}
			AssignScopedVerticalLaneIndices(routePlanList);
			bool layoutChanged = false;
			for (int routePlanIndex = 0; routePlanIndex < routePlanList.size(); ++routePlanIndex)
			{
				const ScopedOpticalRoutePlan& previousRoutePlan = previousRoutePlanList.at(routePlanIndex);
				const ScopedOpticalRoutePlan& currentRoutePlan = routePlanList.at(routePlanIndex);
				if (previousRoutePlan.laneIndex != currentRoutePlan.laneIndex ||
					previousRoutePlan.trunkY != currentRoutePlan.trunkY ||
					previousRoutePlan.startVerticalLaneIndex != currentRoutePlan.startVerticalLaneIndex ||
					previousRoutePlan.startVerticalLaneCount != currentRoutePlan.startVerticalLaneCount ||
					previousRoutePlan.endVerticalLaneIndex != currentRoutePlan.endVerticalLaneIndex ||
					previousRoutePlan.endVerticalLaneCount != currentRoutePlan.endVerticalLaneCount ||
					previousRoutePlan.startVerticalX != currentRoutePlan.startVerticalX ||
					previousRoutePlan.endVerticalX != currentRoutePlan.endVerticalX)
				{
					layoutChanged = true;
					break;
				}
			}
			if (!layoutChanged)
			{
				break;
			}
		}
		return maxLaneCount;
	}
	static bool ShouldPlaceScopedIedAtBottom(const IED* pIed)
	{
		if (!pIed)
		{
			return false;
		}
		QString upperName = pIed->name.toUpper();
		if (upperName.startsWith("IL"))
		{
			return true;
		}
		if (upperName.contains("TERM") || upperName.contains("IO"))
		{
			return true;
		}
		return false;
	}
	static void RebalanceScopedLayerList(QStringList& topIedNameList, QStringList& bottomIedNameList)
	{
		if (topIedNameList.isEmpty() && bottomIedNameList.size() > 1)
		{
			int moveCount = bottomIedNameList.size() / 2;
			if (moveCount < 1)
			{
				moveCount = 1;
			}
			for (int moveIndex = 0; moveIndex < moveCount; ++moveIndex)
			{
				topIedNameList.append(bottomIedNameList.takeFirst());
			}
		}
		if (bottomIedNameList.isEmpty() && topIedNameList.size() > 1)
		{
			int moveCount = topIedNameList.size() / 2;
			if (moveCount < 1)
			{
				moveCount = 1;
			}
			for (int moveIndex = 0; moveIndex < moveCount; ++moveIndex)
			{
				bottomIedNameList.prepend(topIedNameList.takeLast());
			}
		}
	}
	static int GetScopedRowWidth(int itemCount, int rectWidth, int rectGap)
	{
		if (itemCount <= 0)
		{
			return 0;
		}
		return itemCount * rectWidth + (itemCount - 1) * rectGap;
	}
	static int GetScopedRowStartX(int totalWidth, int itemCount, int rectWidth, int rectGap)
	{
		int rowWidth = GetScopedRowWidth(itemCount, rectWidth, rectGap);
		if (rowWidth <= 0)
		{
			return (totalWidth - rectWidth) / 2;
		}
		int startX = (totalWidth - rowWidth) / 2;
		if (startX < 40)
		{
			startX = 40;
		}
		return startX;
	}
	static QString BuildScopedCorridorKey(const OpticalCircuit* pOpticalCircuit)
	{
		if (!pOpticalCircuit)
		{
			return QString();
		}
		QString firstName = pOpticalCircuit->srcIedName;
		QString secondName = pOpticalCircuit->destIedName;
		if (QString::compare(firstName, secondName, Qt::CaseInsensitive) > 0)
		{
			QString tempName = firstName;
			firstName = secondName;
			secondName = tempName;
		}
		return QString("%1|%2").arg(firstName).arg(secondName);
	}
	static void ResolveScopedAnchorSide(int srcLayerType,
		int destLayerType,
		const IedRect* pSrcRect,
		const IedRect* pDestRect,
		int& startSide,
		int& endSide)
	{
		if (srcLayerType == destLayerType)
		{
			if (srcLayerType == ScopedLayerTop)
			{
				startSide = ScopedAnchorBottom;
				endSide = ScopedAnchorBottom;
				return;
			}
			if (srcLayerType == ScopedLayerBottom)
			{
				startSide = ScopedAnchorTop;
				endSide = ScopedAnchorTop;
				return;
			}
			int srcY = pSrcRect ? pSrcRect->y : 0;
			int destY = pDestRect ? pDestRect->y : 0;
			if (srcY <= destY)
			{
				startSide = ScopedAnchorBottom;
				endSide = ScopedAnchorTop;
			}
			else
			{
				startSide = ScopedAnchorTop;
				endSide = ScopedAnchorBottom;
			}
			return;
		}
		int srcY = pSrcRect ? pSrcRect->y : 0;
		int destY = pDestRect ? pDestRect->y : 0;
		if (srcY <= destY)
		{
			startSide = ScopedAnchorBottom;
			endSide = ScopedAnchorTop;
		}
		else
		{
			startSide = ScopedAnchorTop;
			endSide = ScopedAnchorBottom;
		}
	}
	static void AppendArrowDirection(quint8& fromArrowState, quint8& toArrowState)
	{
		fromArrowState |= Arrow_Out;
		toArrowState |= Arrow_In;
	}
	static void ResolveOpticalArrowState(CircuitConfig* circuitConfig,
		const OpticalCircuit* pOpticalCircuit,
		quint8& srcArrowState,
		quint8& destArrowState)
	{
		srcArrowState = Arrow_None;
		destArrowState = Arrow_None;
		if (!circuitConfig || !pOpticalCircuit)
		{
			return;
		}
		QList<VirtualCircuit*> virtualCircuitList = circuitConfig->GetVirtualCircuitListByOpticalCode(pOpticalCircuit->code);
		if (virtualCircuitList.isEmpty())
		{
			return;
		}
		bool srcIsSwitch = IsSwitchIedName(pOpticalCircuit->srcIedName);
		bool destIsSwitch = IsSwitchIedName(pOpticalCircuit->destIedName);
		QString leafIedName;
		bool leafIsSrc = false;
		if (srcIsSwitch != destIsSwitch)
		{
			leafIedName = srcIsSwitch ? pOpticalCircuit->destIedName : pOpticalCircuit->srcIedName;
			leafIsSrc = leafIedName.compare(pOpticalCircuit->srcIedName, Qt::CaseInsensitive) == 0;
		}
		for (int virtualIndex = 0; virtualIndex < virtualCircuitList.size(); ++virtualIndex)
		{
			VirtualCircuit* pVirtualCircuit = virtualCircuitList.at(virtualIndex);
			if (!pVirtualCircuit)
			{
				continue;
			}
			if (srcIsSwitch != destIsSwitch)
			{
				if (pVirtualCircuit->srcIedName == leafIedName)
				{
					if (leafIsSrc)
					{
						AppendArrowDirection(srcArrowState, destArrowState);
					}
					else
					{
						AppendArrowDirection(destArrowState, srcArrowState);
					}
				}
				else if (pVirtualCircuit->destIedName == leafIedName)
				{
					if (leafIsSrc)
					{
						AppendArrowDirection(destArrowState, srcArrowState);
					}
					else
					{
						AppendArrowDirection(srcArrowState, destArrowState);
					}
				}
				continue;
			}
			if (pVirtualCircuit->srcIedName == pOpticalCircuit->srcIedName &&
				pVirtualCircuit->destIedName == pOpticalCircuit->destIedName)
			{
				AppendArrowDirection(srcArrowState, destArrowState);
			}
			else if (pVirtualCircuit->srcIedName == pOpticalCircuit->destIedName &&
				pVirtualCircuit->destIedName == pOpticalCircuit->srcIedName)
			{
				AppendArrowDirection(destArrowState, srcArrowState);
			}
		}
	}
	static void ResolveScopedArrowState(CircuitConfig* circuitConfig,
		const OpticalCircuit* pOpticalCircuit,
		quint8& srcArrowState,
		quint8& destArrowState)
	{
		ResolveOpticalArrowState(circuitConfig, pOpticalCircuit, srcArrowState, destArrowState);
	}
	static void AppendLineArrowState(OpticalCircuitLine* pLine, const QString& iedName, quint8 arrowState)
	{
		if (!pLine || !pLine->pOpticalCircuit)
		{
			return;
		}
		if (pLine->pOpticalCircuit->srcIedName.compare(iedName, Qt::CaseInsensitive) == 0)
		{
			pLine->srcArrowState |= arrowState;
		}
		else if (pLine->pOpticalCircuit->destIedName.compare(iedName, Qt::CaseInsensitive) == 0)
		{
			pLine->destArrowState |= arrowState;
		}
		pLine->arrowState = pLine->srcArrowState | pLine->destArrowState;
	}
	static void AppendLineArrowDirection(OpticalCircuitLine* pLine, const QString& fromIedName, const QString& toIedName)
	{
		AppendLineArrowState(pLine, fromIedName, Arrow_Out);
		AppendLineArrowState(pLine, toIedName, Arrow_In);
	}
	static QString ResolveLinePeerIedName(OpticalCircuitLine* pLine, const QString& knownIedName)
	{
		if (!pLine || !pLine->pOpticalCircuit)
		{
			return QString();
		}
		if (pLine->pOpticalCircuit->srcIedName.compare(knownIedName, Qt::CaseInsensitive) == 0)
		{
			return pLine->pOpticalCircuit->destIedName;
		}
		if (pLine->pOpticalCircuit->destIedName.compare(knownIedName, Qt::CaseInsensitive) == 0)
		{
			return pLine->pOpticalCircuit->srcIedName;
		}
		return QString();
	}
	static void BuildScopedOpticalLineGeometry(OpticalCircuitLine* opticalLine,
		int startSide,
		int endSide,
		int startIndex,
		int startCount,
		int endIndex,
		int endCount,
		int laneIndex,
		int trunkY,
		int startVerticalX,
		int endVerticalX)
	{
				if (!opticalLine || !opticalLine->pSrcRect || !opticalLine->pDestRect)
		{
			return;
		}
		opticalLine->midPoints.clear();
		opticalLine->startPoint = BuildScopedAnchorPoint(opticalLine->pSrcRect, startSide, startIndex, startCount);
		opticalLine->endPoint = BuildScopedAnchorPoint(opticalLine->pDestRect, endSide, endIndex, endCount);
		QPoint startSafePoint = MoveScopedPointBySide(opticalLine->startPoint, startSide, SAFE_DISTANCE);
		QPoint endSafePoint = MoveScopedPointBySide(opticalLine->endPoint, endSide, SAFE_DISTANCE);
		int startTurnY = startSafePoint.y();
		int endTurnY = endSafePoint.y();
		if (startSafePoint.y() != trunkY)
		{
			int startAvailableOffset = qAbs(trunkY - startSafePoint.y()) - 4;
			if (startAvailableOffset < 0)
			{
				startAvailableOffset = 0;
			}
			int startDoglegOffset = qMin(startAvailableOffset, laneIndex * SCOPED_DOGLEG_STEP);
			if (startDoglegOffset > 0)
			{
				startTurnY = startSafePoint.y() + (trunkY > startSafePoint.y() ? startDoglegOffset : -startDoglegOffset);
			}
		}
		if (endSafePoint.y() != trunkY)
		{
			int endAvailableOffset = qAbs(trunkY - endSafePoint.y()) - 4;
			if (endAvailableOffset < 0)
			{
				endAvailableOffset = 0;
			}
			int endDoglegOffset = qMin(endAvailableOffset, laneIndex * SCOPED_DOGLEG_STEP);
			if (endDoglegOffset > 0)
			{
				endTurnY = endSafePoint.y() + (trunkY > endSafePoint.y() ? endDoglegOffset : -endDoglegOffset);
			}
		}
		QList<QPoint> pointList;
		AppendScopedPoint(pointList, startSafePoint);
		if (startTurnY != startSafePoint.y())
		{
			AppendScopedPoint(pointList, QPoint(startSafePoint.x(), startTurnY));
		}
		if (startVerticalX != startSafePoint.x())
		{
			AppendScopedPoint(pointList, QPoint(startVerticalX, startTurnY));
		}
		if (startTurnY != trunkY)
		{
			AppendScopedPoint(pointList, QPoint(startVerticalX, trunkY));
		}
		if (endVerticalX != startVerticalX)
		{
			AppendScopedPoint(pointList, QPoint(endVerticalX, trunkY));
		}
		if (endTurnY != trunkY)
		{
			AppendScopedPoint(pointList, QPoint(endVerticalX, endTurnY));
		}
		if (endVerticalX != endSafePoint.x())
		{
			AppendScopedPoint(pointList, QPoint(endSafePoint.x(), endTurnY));
		}
		if (endTurnY != endSafePoint.y())
		{
			AppendScopedPoint(pointList, QPoint(endSafePoint.x(), endSafePoint.y()));
		}
		for (int pointIndex = 0; pointIndex < pointList.size(); ++pointIndex)
		{
			opticalLine->midPoints.append(pointList.at(pointIndex));
		}
	}
	static void BuildScopedOpticalSvg(CircuitConfig* circuitConfig,
		const QString& titleName,
		const QString& titleDesc,
		const QStringList& bayOrder,
		const QMap<QString, QStringList>& bayVisibleIedHash,
		const QList<OpticalCircuit*>& opticalList,
		OpticalSvg& svg)
	{
		QStringList topIedNameList;
		QStringList bottomIedNameList;
		QStringList switchIedNameList;
		QHash<QString, IedRect*> rectHash;
		QHash<QString, ScopedRectRouteInfo> rectInfoHash;
		for (int bayIndex = 0; bayIndex < bayOrder.size(); ++bayIndex)
		{
			QStringList visibleIedList = bayVisibleIedHash.value(bayOrder.at(bayIndex));
			for (int iedIndex = 0; iedIndex < visibleIedList.size(); ++iedIndex)
			{
				const QString& iedName = visibleIedList.at(iedIndex);
				if (IsSwitchIedName(iedName))
				{
					if (!switchIedNameList.contains(iedName))
					{
						switchIedNameList.append(iedName);
					}
					continue;
				}
				IED* pIed = circuitConfig ? circuitConfig->GetIedByName(iedName) : NULL;
				if (ShouldPlaceScopedIedAtBottom(pIed))
				{
					if (!bottomIedNameList.contains(iedName))
					{
						bottomIedNameList.append(iedName);
					}
				}
				else if (!topIedNameList.contains(iedName))
				{
					topIedNameList.append(iedName);
				}
			}
		}
		RebalanceScopedLayerList(topIedNameList, bottomIedNameList);
		QHash<QString, ScopedRectRouteInfo> previewRectInfoHash;
		for (int topIndex = 0; topIndex < topIedNameList.size(); ++topIndex)
		{
			ScopedRectRouteInfo rectInfo;
			rectInfo.layerType = ScopedLayerTop;
			rectInfo.orderIndex = topIndex;
			previewRectInfoHash.insert(topIedNameList.at(topIndex), rectInfo);
		}
		for (int switchIndex = 0; switchIndex < switchIedNameList.size(); ++switchIndex)
		{
			ScopedRectRouteInfo rectInfo;
			rectInfo.layerType = ScopedLayerSwitch;
			rectInfo.orderIndex = switchIndex;
			previewRectInfoHash.insert(switchIedNameList.at(switchIndex), rectInfo);
		}
		for (int bottomIndex = 0; bottomIndex < bottomIedNameList.size(); ++bottomIndex)
		{
			ScopedRectRouteInfo rectInfo;
			rectInfo.layerType = ScopedLayerBottom;
			rectInfo.orderIndex = bottomIndex;
			previewRectInfoHash.insert(bottomIedNameList.at(bottomIndex), rectInfo);
		}
		QHash<QString, int> previewSideCountHash;
		int maxSideCount = 1;
		for (int opticalIndex = 0; opticalIndex < opticalList.size(); ++opticalIndex)
		{
			OpticalCircuit* pOpticalCircuit = opticalList.at(opticalIndex);
			if (!pOpticalCircuit)
			{
				continue;
			}
			if (!previewRectInfoHash.contains(pOpticalCircuit->srcIedName) ||
				!previewRectInfoHash.contains(pOpticalCircuit->destIedName))
			{
				continue;
			}
			ScopedRectRouteInfo srcInfo = previewRectInfoHash.value(pOpticalCircuit->srcIedName);
			ScopedRectRouteInfo destInfo = previewRectInfoHash.value(pOpticalCircuit->destIedName);
			int startSide = ScopedAnchorBottom;
			int endSide = ScopedAnchorTop;
			ResolveScopedAnchorSide(srcInfo.layerType, destInfo.layerType, NULL, NULL, startSide, endSide);
			QString startSideKey = BuildScopedSideKey(pOpticalCircuit->srcIedName, startSide);
			QString endSideKey = BuildScopedSideKey(pOpticalCircuit->destIedName, endSide);
			int startCount = previewSideCountHash.value(startSideKey, 0) + 1;
			int endCount = previewSideCountHash.value(endSideKey, 0) + 1;
			previewSideCountHash.insert(startSideKey, startCount);
			previewSideCountHash.insert(endSideKey, endCount);
			maxSideCount = qMax(maxSideCount, qMax(startCount, endCount));
		}
		int dynamicLayerGapY = EstimateScopedLayerGap(maxSideCount, qMax(maxSideCount, 1), SCOPED_BASE_LAYER_GAP_Y);
		int maxRowCount = qMax(topIedNameList.size(), bottomIedNameList.size());
		if (maxRowCount < 1)
		{
			maxRowCount = 1;
		}
		int totalWidth = qMax(SVG_VIEWBOX_WIDTH / 2,
			SCOPED_SIDE_MARGIN * 2 + GetScopedRowWidth(maxRowCount, RECT_DEFAULT_WIDTH, SCOPED_ROW_GAP));
		int switchWidth = totalWidth - SCOPED_SIDE_MARGIN * 2;
		if (switchWidth < RECT_DEFAULT_WIDTH + 220)
		{
			switchWidth = RECT_DEFAULT_WIDTH + 220;
			totalWidth = switchWidth + SCOPED_SIDE_MARGIN * 2;
		}
		svg.mainIedRect = CreateScopeTitleRect(titleName, titleDesc, totalWidth);
		int topRowStartX = GetScopedRowStartX(totalWidth, topIedNameList.size(), RECT_DEFAULT_WIDTH, SCOPED_ROW_GAP);
		for (int topIndex = 0; topIndex < topIedNameList.size(); ++topIndex)
		{
			IED* pIed = circuitConfig ? circuitConfig->GetIedByName(topIedNameList.at(topIndex)) : NULL;
			IedRect* pRect = CreateScopeRect(pIed,
				topRowStartX + topIndex * (RECT_DEFAULT_WIDTH + SCOPED_ROW_GAP),
				SCOPED_TOP_LAYER_Y);
			if (!pRect || !pIed)
			{
				continue;
			}
			svg.iedRectList.append(pRect);
			rectHash.insert(pIed->name, pRect);
			ScopedRectRouteInfo rectInfo;
			rectInfo.pRect = pRect;
			rectInfo.layerType = ScopedLayerTop;
			rectInfo.orderIndex = topIndex;
			rectInfoHash.insert(pIed->name, rectInfo);
		}
		int switchY = SCOPED_TOP_LAYER_Y + RECT_DEFAULT_HEIGHT + dynamicLayerGapY;
		int switchStartX = (totalWidth - switchWidth) / 2;
		for (int switchIndex = 0; switchIndex < switchIedNameList.size(); ++switchIndex)
		{
			IED* pSwitchIed = circuitConfig ? circuitConfig->GetIedByName(switchIedNameList.at(switchIndex)) : NULL;
			if (!pSwitchIed)
			{
				continue;
			}
			IedRect* pRect = utils::GetIedRect(
				pSwitchIed->name,
				pSwitchIed->desc,
				(quint16)switchStartX,
				(quint16)(switchY + switchIndex * (RECT_DEFAULT_HEIGHT + SCOPED_SWITCH_GAP_Y)),
				(quint16)switchWidth,
				RECT_DEFAULT_HEIGHT,
				ColorHelper::ied_border,
				ColorHelper::ied_underground);
			svg.switcherRectList.append(pRect);
			rectHash.insert(pSwitchIed->name, pRect);
			ScopedRectRouteInfo rectInfo;
			rectInfo.pRect = pRect;
			rectInfo.layerType = ScopedLayerSwitch;
			rectInfo.orderIndex = switchIndex;
			rectInfoHash.insert(pSwitchIed->name, rectInfo);
		}
		int bottomLayerY = switchY + switchIedNameList.size() * RECT_DEFAULT_HEIGHT;
		if (!switchIedNameList.isEmpty())
		{
			bottomLayerY += qMax(0, switchIedNameList.size() - 1) * SCOPED_SWITCH_GAP_Y;
		}
		bottomLayerY += dynamicLayerGapY;
		if (switchIedNameList.isEmpty())
		{
			bottomLayerY = SCOPED_TOP_LAYER_Y + RECT_DEFAULT_HEIGHT + dynamicLayerGapY * 2;
		}
		int bottomRowStartX = GetScopedRowStartX(totalWidth, bottomIedNameList.size(), RECT_DEFAULT_WIDTH, SCOPED_ROW_GAP);
		for (int bottomIndex = 0; bottomIndex < bottomIedNameList.size(); ++bottomIndex)
		{
			IED* pIed = circuitConfig ? circuitConfig->GetIedByName(bottomIedNameList.at(bottomIndex)) : NULL;
			IedRect* pRect = CreateScopeRect(pIed,
				bottomRowStartX + bottomIndex * (RECT_DEFAULT_WIDTH + SCOPED_ROW_GAP),
				bottomLayerY);
			if (!pRect || !pIed)
			{
				continue;
			}
			svg.iedRectList.append(pRect);
			rectHash.insert(pIed->name, pRect);
			ScopedRectRouteInfo rectInfo;
			rectInfo.pRect = pRect;
			rectInfo.layerType = ScopedLayerBottom;
			rectInfo.orderIndex = bottomIndex;
			rectInfoHash.insert(pIed->name, rectInfo);
		}
		QList<ScopedOpticalRoutePlan> routePlanList;
		QHash<QString, int> sideCountHash;
		QHash<QString, int> corridorCountHash;
		for (int opticalIndex = 0; opticalIndex < opticalList.size(); ++opticalIndex)
		{
			OpticalCircuit* pOpticalCircuit = opticalList.at(opticalIndex);
			if (!pOpticalCircuit)
			{
				continue;
			}
			if (!rectHash.contains(pOpticalCircuit->srcIedName) || !rectHash.contains(pOpticalCircuit->destIedName))
			{
				continue;
			}
			ScopedRectRouteInfo srcInfo = rectInfoHash.value(pOpticalCircuit->srcIedName);
			ScopedRectRouteInfo destInfo = rectInfoHash.value(pOpticalCircuit->destIedName);
			if (!srcInfo.pRect || !destInfo.pRect)
			{
				continue;
			}
			ScopedOpticalRoutePlan routePlan;
			routePlan.pOpticalCircuit = pOpticalCircuit;
			routePlan.pSrcRect = srcInfo.pRect;
			routePlan.pDestRect = destInfo.pRect;
			routePlan.srcLayerType = srcInfo.layerType;
			routePlan.destLayerType = destInfo.layerType;
			ResolveScopedAnchorSide(srcInfo.layerType, destInfo.layerType,
				srcInfo.pRect, destInfo.pRect, routePlan.startSide, routePlan.endSide);
			routePlan.startSideKey = BuildScopedSideKey(pOpticalCircuit->srcIedName, routePlan.startSide);
			routePlan.endSideKey = BuildScopedSideKey(pOpticalCircuit->destIedName, routePlan.endSide);
			routePlan.corridorKey = BuildScopedCorridorKey(pOpticalCircuit);
			routePlanList.append(routePlan);
			sideCountHash.insert(routePlan.startSideKey, sideCountHash.value(routePlan.startSideKey, 0) + 1);
			sideCountHash.insert(routePlan.endSideKey, sideCountHash.value(routePlan.endSideKey, 0) + 1);
			corridorCountHash.insert(routePlan.corridorKey, corridorCountHash.value(routePlan.corridorKey, 0) + 1);
		}
		QHash<QString, int> sideIndexHash;
		for (int routePlanIndex = 0; routePlanIndex < routePlanList.size(); ++routePlanIndex)
		{
			ScopedOpticalRoutePlan& routePlan = routePlanList[routePlanIndex];
			routePlan.startIndex = sideIndexHash.value(routePlan.startSideKey, 0);
			routePlan.endIndex = sideIndexHash.value(routePlan.endSideKey, 0);
			routePlan.startCount = sideCountHash.value(routePlan.startSideKey, 1);
			routePlan.endCount = sideCountHash.value(routePlan.endSideKey, 1);
			sideIndexHash.insert(routePlan.startSideKey, routePlan.startIndex + 1);
			sideIndexHash.insert(routePlan.endSideKey, routePlan.endIndex + 1);
			QPoint startPoint = BuildScopedAnchorPoint(routePlan.pSrcRect, routePlan.startSide, routePlan.startIndex, routePlan.startCount);
			QPoint endPoint = BuildScopedAnchorPoint(routePlan.pDestRect, routePlan.endSide, routePlan.endIndex, routePlan.endCount);
			routePlan.startVerticalX = startPoint.x();
			routePlan.endVerticalX = endPoint.x();
			RefreshScopedHorizontalSpan(routePlan);
			routePlan.overlapGroupKey = BuildScopedHorizontalGroupKey(routePlan.startSide, routePlan.endSide);
		}
		int maxCorridorCount = RefineScopedRouteLayout(routePlanList);
		if (maxCorridorCount > 1)
		{
			int expandedLayerGapY = EstimateScopedLayerGap(maxSideCount, maxCorridorCount, SCOPED_BASE_LAYER_GAP_Y);
			if (expandedLayerGapY > dynamicLayerGapY)
			{
				dynamicLayerGapY = expandedLayerGapY;
			}
		}
		switchY = SCOPED_TOP_LAYER_Y + RECT_DEFAULT_HEIGHT + dynamicLayerGapY;
		if (!switchIedNameList.isEmpty())
		{
			for (int switchIndex = 0; switchIndex < svg.switcherRectList.size(); ++switchIndex)
			{
				IedRect* pRect = svg.switcherRectList.at(switchIndex);
				if (pRect)
				{
					pRect->y = switchY + switchIndex * (RECT_DEFAULT_HEIGHT + SCOPED_SWITCH_GAP_Y);
				}
			}
		}
		if (!bottomIedNameList.isEmpty())
		{
			bottomLayerY = switchY + switchIedNameList.size() * RECT_DEFAULT_HEIGHT;
			if (!switchIedNameList.isEmpty())
			{
				bottomLayerY += qMax(0, switchIedNameList.size() - 1) * SCOPED_SWITCH_GAP_Y;
			}
			bottomLayerY += dynamicLayerGapY;
			if (switchIedNameList.isEmpty())
			{
				bottomLayerY = SCOPED_TOP_LAYER_Y + RECT_DEFAULT_HEIGHT + dynamicLayerGapY * 2;
			}
			for (int bottomIndex = 0; bottomIndex < bottomIedNameList.size() && bottomIndex < svg.iedRectList.size(); ++bottomIndex)
			{
				IedRect* pRect = rectHash.value(bottomIedNameList.at(bottomIndex), NULL);
				if (pRect)
				{
					pRect->y = bottomLayerY;
				}
			}
		}
		RefineScopedRouteLayout(routePlanList);
		int maxBottom = bottomLayerY + RECT_DEFAULT_HEIGHT;
		for (int routePlanIndex = 0; routePlanIndex < routePlanList.size(); ++routePlanIndex)
		{
			const ScopedOpticalRoutePlan& routePlan = routePlanList.at(routePlanIndex);
			if (!routePlan.pOpticalCircuit || !routePlan.pSrcRect || !routePlan.pDestRect)
			{
				continue;
			}
			OpticalCircuitLine* pOpticalLine = new OpticalCircuitLine();
			pOpticalLine->lineCode = routePlan.pOpticalCircuit->loopCode;
			pOpticalLine->pOpticalCircuit = routePlan.pOpticalCircuit;
			pOpticalLine->pSrcRect = routePlan.pSrcRect;
			pOpticalLine->pDestRect = routePlan.pDestRect;
			ResolveScopedArrowState(circuitConfig,
				routePlan.pOpticalCircuit,
				pOpticalLine->srcArrowState,
				pOpticalLine->destArrowState);
			pOpticalLine->arrowState = pOpticalLine->srcArrowState | pOpticalLine->destArrowState;
			BuildScopedOpticalLineGeometry(
				pOpticalLine,
				routePlan.startSide,
				routePlan.endSide,
				routePlan.startIndex,
				routePlan.startCount,
				routePlan.endIndex,
				routePlan.endCount,
				routePlan.laneIndex,
				routePlan.trunkY,
				routePlan.startVerticalX,
				routePlan.endVerticalX);
			maxBottom = qMax(maxBottom, pOpticalLine->startPoint.y());
			maxBottom = qMax(maxBottom, pOpticalLine->endPoint.y());
			for (int pointIndex = 0; pointIndex < pOpticalLine->midPoints.size(); ++pointIndex)
			{
				maxBottom = qMax(maxBottom, pOpticalLine->midPoints.at(pointIndex).y());
			}
			svg.opticalCircuitLineList.append(pOpticalLine);
		}
		svg.viewBoxX = 0;
		svg.viewBoxY = 0;
		svg.viewBoxWidth = qMax(totalWidth + SCOPED_SIDE_MARGIN, SVG_VIEWBOX_WIDTH / 2);
		svg.viewBoxHeight = qMax(maxBottom + 120, bottomLayerY + RECT_DEFAULT_HEIGHT + 120);
	}
}

OpticalSvg::~OpticalSvg()
{
	QSet<IedRect*> deletedRectSet;
	foreach(IedRect* pRect, iedRectList)
	{
		if (!pRect || deletedRectSet.contains(pRect))
		{
			continue;
		}
		deletedRectSet.insert(pRect);
		delete pRect;
	}
	foreach(IedRect* pRect, switcherRectList)
	{
		if (!pRect || deletedRectSet.contains(pRect))
		{
			continue;
		}
		deletedRectSet.insert(pRect);
		delete pRect;
	}
	iedRectList.clear();
	switcherRectList.clear();
	QSet<OpticalCircuitLine*> deletedLineSet;
	foreach(OpticalCircuitLine* pLine, opticalCircuitLineList)
	{
		if (!pLine || deletedLineSet.contains(pLine))
		{
			continue;
		}
		deletedLineSet.insert(pLine);
		delete pLine;
	}
	opticalCircuitLineList.clear();
}

OpticalSvg* OpticalDiagramBuilder::BuildOpticalDiagramByIedName(const QString& iedName)
{
	IED* pIed = m_pCircuitConfig->GetIedByName(iedName);
	if (!pIed) return NULL;
	OpticalSvg* svg = new OpticalSvg();
	GenerateOpticalDiagramByIed(pIed, *svg);
	return svg;
}

OpticalSvg* OpticalDiagramBuilder::BuildOpticalDiagramByStation()
{
	const CRtdbEleModelStation* station = m_pCircuitConfig->StationModel();
	if (!station)
	{
		return NULL;
	}
	QMap<QString, QStringList> bayIedHash;
	QMap<QString, QString> bayTextHash;
	QMap<QString, QString> iedBayHash;
	QStringList bayOrder;
	QString stationText;
	CollectBayContext(station, m_pCircuitConfig, bayIedHash, bayTextHash, iedBayHash, bayOrder, stationText);
	if (bayOrder.isEmpty())
	{
		return NULL;
	}
	QSet<QString> visibleIedSet;
	for (int i = 0; i < bayOrder.size(); ++i)
	{
		const QStringList& bayIedList = bayIedHash.value(bayOrder.at(i));
		for (int j = 0; j < bayIedList.size(); ++j)
		{
			visibleIedSet.insert(bayIedList.at(j));
		}
	}
	QList<OpticalCircuit*> opticalList;
	QSet<quint64> opticalLoopCodeSet;
	QSet<QString>::const_iterator iedIt = visibleIedSet.constBegin();
	for (; iedIt != visibleIedSet.constEnd(); ++iedIt)
	{
		IED* pIed = m_pCircuitConfig->GetIedByName(*iedIt);
		if (!pIed)
		{
			continue;
		}
		for (int j = 0; j < pIed->optical_list.size(); ++j)
		{
			OpticalCircuit* opticalCircuit = pIed->optical_list.at(j);
			if (!opticalCircuit)
			{
				continue;
			}
			quint64 opticalUniqueKey = opticalCircuit->loopCode != 0 ? opticalCircuit->loopCode : opticalCircuit->code;
			if (opticalLoopCodeSet.contains(opticalUniqueKey))
			{
				continue;
			}
			if (!visibleIedSet.contains(opticalCircuit->srcIedName) || !visibleIedSet.contains(opticalCircuit->destIedName))
			{
				continue;
			}
			opticalLoopCodeSet.insert(opticalUniqueKey);
			opticalList.append(opticalCircuit);
		}
	}
	QMap<QString, QStringList> bayVisibleIedHash;
	for (int i = 0; i < bayOrder.size(); ++i)
	{
		const QString& bayKey = bayOrder.at(i);
		bayVisibleIedHash.insert(bayKey, FilterVisibleIedList(bayIedHash.value(bayKey), visibleIedSet, true));
	}
	OpticalSvg* svg = new OpticalSvg();
	BuildScopedOpticalSvg(m_pCircuitConfig, stationText, QString("Station Optical"), bayOrder, bayVisibleIedHash, opticalList, *svg);
	return svg;
}

OpticalSvg* OpticalDiagramBuilder::BuildOpticalDiagramByBayName(const QString& bayName)
{
	const CRtdbEleModelStation* station = m_pCircuitConfig->StationModel();
	if (!station || bayName.isEmpty())
	{
		return NULL;
	}
	QMap<QString, QStringList> bayIedHash;
	QMap<QString, QString> bayTextHash;
	QMap<QString, QString> iedBayHash;
	QStringList bayOrder;
	QString stationText;
	CollectBayContext(station, m_pCircuitConfig, bayIedHash, bayTextHash, iedBayHash, bayOrder, stationText);
	QStringList selectedBayIedList = bayIedHash.value(bayName);
	if (selectedBayIedList.isEmpty())
	{
		return NULL;
	}
	QSet<QString> visibleIedSet;
	for (int i = 0; i < selectedBayIedList.size(); ++i)
	{
		visibleIedSet.insert(selectedBayIedList.at(i));
	}
	QList<OpticalCircuit*> opticalList;
	QSet<quint64> opticalLoopCodeSet;
	for (int i = 0; i < selectedBayIedList.size(); ++i)
	{
		IED* pIed = m_pCircuitConfig->GetIedByName(selectedBayIedList.at(i));
		if (!pIed)
		{
			continue;
		}
		for (int j = 0; j < pIed->optical_list.size(); ++j)
		{
			OpticalCircuit* opticalCircuit = pIed->optical_list.at(j);
			if (!opticalCircuit)
			{
				continue;
			}
			quint64 opticalUniqueKey = opticalCircuit->loopCode != 0 ? opticalCircuit->loopCode : opticalCircuit->code;
			if (opticalLoopCodeSet.contains(opticalUniqueKey))
			{
				continue;
			}
			opticalLoopCodeSet.insert(opticalUniqueKey);
			opticalList.append(opticalCircuit);
			visibleIedSet.insert(opticalCircuit->srcIedName);
			visibleIedSet.insert(opticalCircuit->destIedName);
		}
	}
	QSet<QString> visibleBaySet;
	visibleBaySet.insert(bayName);
	QSet<QString>::const_iterator visibleIedIt = visibleIedSet.constBegin();
	for (; visibleIedIt != visibleIedSet.constEnd(); ++visibleIedIt)
	{
		QString peerBay = iedBayHash.value(*visibleIedIt, *visibleIedIt);
		if (!peerBay.isEmpty())
		{
			visibleBaySet.insert(peerBay);
		}
	}
	QStringList visibleBayOrder = BuildVisibleBayOrder(bayOrder, visibleBaySet, bayName);
	QMap<QString, QStringList> bayVisibleIedHash;
	for (int i = 0; i < visibleBayOrder.size(); ++i)
	{
		const QString& visibleBay = visibleBayOrder.at(i);
		if (visibleBay == bayName)
		{
			bayVisibleIedHash.insert(visibleBay, FilterVisibleIedList(bayIedHash.value(visibleBay), visibleIedSet, true));
		}
		else if (bayIedHash.contains(visibleBay))
		{
			bayVisibleIedHash.insert(visibleBay, FilterVisibleIedList(bayIedHash.value(visibleBay), visibleIedSet, false));
		}
		else
		{
			QStringList fallbackIedList;
			QSet<QString>::const_iterator fallbackIt = visibleIedSet.constBegin();
			for (; fallbackIt != visibleIedSet.constEnd(); ++fallbackIt)
			{
				if (iedBayHash.value(*fallbackIt, *fallbackIt) == visibleBay)
				{
					fallbackIedList.append(*fallbackIt);
				}
			}
			bayVisibleIedHash.insert(visibleBay, fallbackIedList);
		}
	}
	QString bayText = bayTextHash.value(bayName, bayName);
	OpticalSvg* svg = new OpticalSvg();
	BuildScopedOpticalSvg(m_pCircuitConfig, bayText, QString("Bay Optical"), visibleBayOrder, bayVisibleIedHash, opticalList, *svg);
	return svg;
}

void OpticalDiagramBuilder::GenerateOpticalDiagramByIed(const IED* pIed, OpticalSvg& svg)
{
	if(pIed->name.contains("SW"))
	{
		// 交换机作为中心设备时，不生成光纤链路图
		return;	
	}
	const int switcherWidth = SVG_VIEWBOX_WIDTH * IED_OPTICAL_SWITCHER_WIDTH_RATIO;
	QList<IedRect*> directRectList;		// 直连IED列表
	QList<IedRect*> indirectRectList;	// 经过交换机的IED列表
	QHash<IedRect*, qint8> connectPtHash;	// 记录连接点数量，交换机不记录直接绘制垂直线
	QList<QString> iedNameList;	// 已绘制的设备光纤号
	// 主IED 中心位置
	svg.mainIedRect = utils::GetIedRect(
		pIed->name,
		pIed->desc,
		(SVG_VIEWBOX_WIDTH - RECT_DEFAULT_WIDTH) / 2,
		IED_OPTICAL_HORIZONTAL_DISTANCE,
		RECT_DEFAULT_WIDTH,
		RECT_DEFAULT_HEIGHT,
		ColorHelper::ied_border,
		ColorHelper::ied_underground);
	if(pIed->connectedIedNameSet.isEmpty())
	{
		// 主IED没有连接其他设备，直接返回
		return;
	}
	foreach(OpticalCircuit * optCircuit, pIed->optical_list)
	{
		// 一端为主IED，另一端为对侧IED的光纤链路
		OpticalCircuitLine* optLine = svg.GetOpticalCircuitLineByLineCode(optCircuit->loopCode);
		if(!optLine)
		{
			optLine = new OpticalCircuitLine();
			optLine->lineCode = optCircuit->loopCode;
			optLine->pOpticalCircuit = optCircuit;
			svg.opticalCircuitLineList.append(optLine);
		}
		else
		{
			continue;
		}
		QString oppsiteIedName = optCircuit->srcIedName == pIed->name ? optCircuit->destIedName : optCircuit->srcIedName;
		QString oppsiteIedDesc = optCircuit->srcIedName == pIed->name ? optCircuit->destIedDesc : optCircuit->srcIedDesc;
		IedRect* pOppsiteIedRect = svg.GetIedRectByIedName(oppsiteIedName);
		if (!pOppsiteIedRect)
		{
			pOppsiteIedRect = utils::GetIedRect(
				oppsiteIedName,
				oppsiteIedDesc,
				0,															// x，暂不设置
				SVG_VIEWBOX_HEIGHT - RECT_DEFAULT_HEIGHT * 2 - IED_OPTICAL_HORIZONTAL_DISTANCE,	// y
				RECT_DEFAULT_WIDTH,
				RECT_DEFAULT_HEIGHT,
				ColorHelper::ied_border,
				ColorHelper::ied_underground);
			svg.iedRectList.append(pOppsiteIedRect);
		}
		if(oppsiteIedName.contains("SW"))
		{
			// 光纤一端是交换机，获取另一端光纤
			// pIed的connectedIedNameList和交换机的connectedIedNameList的交集即为对侧IED
			optLine->arrowState = Arrow_None; // 经过交换机的光纤链路，方向不明确，初始化
			QSet<QString> iedSet = pIed->connectedIedNameSet;
			IED* pOppsiteIed = m_pCircuitConfig->GetIedByName(oppsiteIedName);
			if(!pOppsiteIed)
			{
				m_errStr += QString("光纤链路 %1 中的交换机设备 %2 不存在，请检查IED配置文件 \n").arg(optCircuit->loopCode).arg(oppsiteIedName);
				continue;
			}
			iedSet.intersect(pOppsiteIed->connectedIedNameSet);
			if(iedSet.isEmpty())
			{
				m_errStr += QString("光纤链路 %1 中的交换机设备 %2 与对侧设备没有连接，请检查光纤链路配置文件 \n").arg(optCircuit->loopCode).arg(oppsiteIedName);
				continue;
			}
			foreach(QString iedName, iedSet)
			{
				// 真正的对端IED名称
				//QString iedName = iedSet.values().first();
				// 交换机连接的IED
				IedRect* pIedRect = svg.GetIedRectByIedName(iedName);
				if (!pIedRect)
				{
					pIedRect = utils::GetIedRect(
						iedName,
						m_pCircuitConfig->GetIedByName(iedName)->desc,
						0,	// x，暂不设置
						SVG_VIEWBOX_HEIGHT - RECT_DEFAULT_HEIGHT * 2 - IED_OPTICAL_HORIZONTAL_DISTANCE,	// y
						RECT_DEFAULT_WIDTH,
						RECT_DEFAULT_HEIGHT,
						ColorHelper::ied_border,
						ColorHelper::ied_underground);
					svg.iedRectList.append(pIedRect);
					indirectRectList.append(pIedRect);
				}
				// 获取对端设备和交换机的光纤链路
				QList<OpticalCircuit*> oppsiteOptList = m_pCircuitConfig->getOpticalByIeds(pOppsiteIed->name, iedName);
				if(oppsiteOptList.isEmpty())
				{
					m_errStr += QString("光纤链路 %1 中的交换机设备 %2 与对侧设备 %3 没有连接，请检查光纤链路配置文件 \n").arg(optCircuit->loopCode).arg(oppsiteIedName).arg(iedName);
					continue;
				}
				OpticalCircuit* oppsiteOptCircuit = oppsiteOptList.first();	// 双向的两条光纤链路，链路编号一致，取第一条即可
				OpticalCircuitLine* oppsiteOptLine = svg.GetOpticalCircuitLineByLineCode(oppsiteOptCircuit->loopCode);
				if(!oppsiteOptLine)
				{
					oppsiteOptLine = new OpticalCircuitLine();
					oppsiteOptLine->lineCode = oppsiteOptCircuit->loopCode;
					oppsiteOptLine->pSrcRect = svg.GetIedRectByIedName(oppsiteOptCircuit->pSrcIed->name);
					oppsiteOptLine->pDestRect = svg.GetIedRectByIedName(oppsiteOptCircuit->pDestIed->name);
					oppsiteOptLine->pOpticalCircuit = oppsiteOptCircuit;
					svg.opticalCircuitLineList.append(oppsiteOptLine);
				}
				QList<LogicCircuit*> inCircuitList = m_pCircuitConfig->GetInLogicCircuitListByIedName(pIed->name);
				QList<LogicCircuit*> outCircuitList = m_pCircuitConfig->GetOutLogicCircuitListByIedName(pIed->name);
				SetArrowStateThroughSwitch(inCircuitList, outCircuitList, pIed->name, iedName, oppsiteOptLine, optLine);
				//foreach(LogicCircuit * pLogicCircuit, inCircuitList)
				//{

				//	if (pLogicCircuit->pSrcIed->name == iedName)
				//	{

				//		// 当只有一个经过交换机的IED时，该IED与主IED同在上方，方向相反；多个交换机时，方向相同
				//		oppsiteOptLine->arrowState |= iedSet.size() > 1 ? Arrow_In : Arrow_Out;
				//		optLine->arrowState |= Arrow_In;
				//		break;
				//	}
				//}
				//foreach(LogicCircuit * pLogicCircuit, outCircuitList)
				//{

				//	if (pLogicCircuit->pDestIed->name == iedName)
				//	{

				//		// 方向为出
				//		oppsiteOptLine->arrowState |= iedSet.size() > 1 ? Arrow_Out : Arrow_In;
				//		optLine->arrowState |= Arrow_Out;
				//		break;
				//	}
				//}
				++connectPtHash[pIedRect];
			}
			pOppsiteIedRect->y = svg.mainIedRect->y + svg.mainIedRect->height + IED_OPTICAL_VERTICAL_DISTANCE + svg.switcherRectList.size() * (RECT_DEFAULT_HEIGHT + IED_OPTICAL_VERTICAL_DISTANCE * 0.5);	// 调整交换机位置到主IED下方
			svg.switcherRectList.append(pOppsiteIedRect);
		}
		else
		{
			// 直连设备
			SetArrowStateDirect(
				m_pCircuitConfig->GetInLogicCircuitListByIedName(pIed->name),
				m_pCircuitConfig->GetOutLogicCircuitListByIedName(pIed->name),
				pIed->name,
				oppsiteIedName,
				optLine);
			directRectList.append(pOppsiteIedRect);
			++connectPtHash[pOppsiteIedRect];
		}
		++connectPtHash[svg.mainIedRect];
		optLine->pSrcRect = optCircuit->srcIedName == pIed->name ? svg.mainIedRect : pOppsiteIedRect;
		optLine->pDestRect = optCircuit->srcIedName == pIed->name ? pOppsiteIedRect : svg.mainIedRect;
	}
	// 直连IED全部靠左排列
	int x = 0;
	int belowIedY = svg.switcherRectList.isEmpty() ? 3 * IED_VERTICAL_DISTANCE + IED_OPTICAL_VERTICAL_DISTANCE + svg.mainIedRect->GetInnerBottomY() : svg.switcherRectList.last()->GetInnerBottomY() + IED_OPTICAL_VERTICAL_DISTANCE;	// 交换机下方的IED y
	for (size_t i = 0; i < directRectList.size(); ++i)
	{
		IedRect* rect = directRectList.at(i);
			rect->x = IED_OPTICAL_HORIZONTAL_DISTANCE + i * (rect->width + IED_OPTICAL_HORIZONTAL_DISTANCE);
			rect->y = belowIedY;
	}
	// 有直连情况则主IED与直连IED对齐，否则主IED居中
	svg.mainIedRect->x = directRectList.isEmpty() ? (SVG_VIEWBOX_WIDTH - RECT_DEFAULT_WIDTH) / 2 : directRectList.last()->x + directRectList.last()->width;	
	int switcherX = svg.mainIedRect->x;		// 交换机起始X，默认为与主IED对齐
	int totalWidth = 0;
	if (!indirectRectList.isEmpty())
	{
		if(indirectRectList.size() == 1)
		{
			// 只有一个过交换机的IED，放在主IED右侧，交换机与主IED对齐
			// 位置关系为: 
			//			主IED	对侧IED
			//			交    换    机
			// 直连IED
			IedRect* rect = indirectRectList.first();
			rect->x = svg.mainIedRect->x + svg.mainIedRect->width + IED_OPTICAL_HORIZONTAL_DISTANCE;
			rect->y = svg.mainIedRect->y;
			totalWidth = svg.mainIedRect->width + rect->width + IED_OPTICAL_HORIZONTAL_DISTANCE;
		}else
		{
			// 不止一个过交换机的IED，位置关系为：
			// 位置关系为: 
			//			主IED	
			//			交换机
			// 直连IED  对侧IED
			for (size_t i = 0; i < indirectRectList.size(); ++i)
			{
				IedRect* rect = indirectRectList.at(i);
				quint16 startX = directRectList.isEmpty() ? svg.switcherRectList.first()->x : directRectList.last()->x + directRectList.last()->width;
				rect->x = startX + IED_OPTICAL_HORIZONTAL_DISTANCE + i * (IED_OPTICAL_HORIZONTAL_DISTANCE + rect->width);
				rect->y = directRectList.isEmpty() ? svg.switcherRectList.last()->y + svg.switcherRectList.last()->height + IED_OPTICAL_HORIZONTAL_DISTANCE * 2 : directRectList.last()->y;
				totalWidth += rect->width + IED_OPTICAL_HORIZONTAL_DISTANCE;
			}
			totalWidth -= IED_OPTICAL_HORIZONTAL_DISTANCE;
			switcherX = indirectRectList.first()->x;		// 多个交换机IED时，交换机与第一个经过交换机的IED对齐
		}
	}
	// 交换机位置
	foreach(IedRect* switcher, svg.switcherRectList)
	{
		switcher->x = switcherX;
		switcher->width = totalWidth;
	}
	if (directRectList.isEmpty())
	{
		// 没有直连IED时，主IED与交换机居中
		IedRect* swRect = svg.switcherRectList.first();
		svg.mainIedRect->x = swRect->x + (swRect->width - svg.mainIedRect->width) / 2;
	}
	int midPtCnt_l = 0;	// 左侧中间转折点计数
	int midPtCnt_r = 0;	// 右侧中间转折点计数
	// 连接点位置
	QHash<IedRect*, qint8> connectCntHash;
	if(pIed->name == "PL2205NA")
	{
		int a = 1;
	}
	for(size_t i = 0;i < svg.opticalCircuitLineList.size(); ++i)
	{
		OpticalCircuitLine* optLine = svg.opticalCircuitLineList.at(i);
		int midPointDistance = IED_OPTICAL_MIDPOINT_DISTANCE;	// 中间转折点垂直/水平距离
		// 分两种光纤回路，直连和间接
		if(svg.switcherRectList.contains(optLine->pDestRect) ||
			svg.switcherRectList.contains(optLine->pSrcRect))
		{
			// 一侧是交换机的间接回路，
			// 则连接点以另一侧的设备连接点位置垂直连接到交换机
			IedRect* pSwRect = svg.switcherRectList.contains(optLine->pDestRect) ?
				optLine->pDestRect : optLine->pSrcRect;	// 交换机矩形
			IedRect* pIedRect = svg.switcherRectList.contains(optLine->pDestRect) ?
				optLine->pSrcRect : optLine->pDestRect;	// 另一侧IED矩形
			int startPt_X = pIedRect->x + pIedRect->width / (connectPtHash[pIedRect] + 1) * (connectCntHash[pIedRect]++ + 1);	// 连接点X = rect.width / (连接点数量 + 1)
			int startPt_Y = pIedRect->y > pSwRect->y ?
				pIedRect->y : 					// IED在交换机下方，则起始点位于IED上方
				pIedRect->y + pIedRect->height;	// IED在交换机上方，则起始点位于IED下方
			int endPt_Y = pIedRect->y > pSwRect->y ?
				pSwRect->y + pSwRect->height :	// IED在交换机下方，则连接点位于交换机下方
				pSwRect->y;						// IED在交换机上方，则连接点位于交换机上方
			optLine->startPoint = QPoint(startPt_X, startPt_Y);
			optLine->endPoint = QPoint(startPt_X, endPt_Y);
		}
		else
		{
			// 直连情况，两侧设备有一侧是主设备
			IedRect* pUpRect = optLine->pSrcRect == svg.mainIedRect ? optLine->pSrcRect : optLine->pDestRect;	// 上层设备
			IedRect* pDownRect = optLine->pSrcRect == svg.mainIedRect ? optLine->pDestRect : optLine->pSrcRect;	// 下层设备
			int startPt_X = pUpRect->x + pUpRect->width / (connectPtHash[pUpRect] + 1) * (connectCntHash[pUpRect]++ + 1);
			int startPt_Y = pUpRect->y + pUpRect->height;	// 连接点位于上方设备的底部
			int connSize = connectPtHash[pDownRect];
			int endPt_X = pDownRect->x + pDownRect->width / (connectPtHash[pDownRect] + 1);	// 同起始位置连接点
			int endPt_Y = pDownRect->y > pUpRect->y ?
				pDownRect->y :											// 当对侧IED在主IED下方时，连接点位于对侧IED上方
				pDownRect->y + pDownRect->height;
			optLine->startPoint = QPoint(startPt_X, startPt_Y);
			optLine->endPoint = QPoint(endPt_X, endPt_Y);
			//添加中间转折点
			// 转折点X = 起始点X
			// 转折点Y = 
			// (交换机Y - 主IED底部Y) / 2 + 主IED底部Y + midPointDistance * midPtCnt，有交换机时，即主IED与交换机间隔的中部
			// (底部IED Y - 主IED底部Y) / 2 + 主IED底部Y + midPointDistance * midPtCnt，无交换机时，即上下IED间隔的中部
			int mainIedBottomY = svg.mainIedRect->y + svg.mainIedRect->height;
			int* midPtCnt = startPt_X > endPt_X ? &midPtCnt_l : &midPtCnt_r;
			int midPt_Y = svg.switcherRectList.isEmpty() ?
				(svg.iedRectList.first()->y - mainIedBottomY) / 2 + mainIedBottomY + midPointDistance * (*midPtCnt) :
				(svg.switcherRectList.first()->y - mainIedBottomY) / 2 + mainIedBottomY + midPointDistance * (*midPtCnt);
			optLine->midPoints.append(QPoint(startPt_X, midPt_Y));
			optLine->midPoints.append(QPoint(endPt_X, midPt_Y));
			optLine->endPoint = QPoint(endPt_X, endPt_Y);
			(*midPtCnt)++;
		}
	}
	// 重新计算光纤图 viewBox：考虑主 IED、下方 IED 及交换机
	if (svg.mainIedRect) {
		int minX = directRectList.isEmpty() ?			// 直连IED在左侧，若为空则最左侧是主IED/交换机
			qMin(svg.mainIedRect->x, svg.switcherRectList.first()->x) : directRectList.first()->x;
		int minY = svg.mainIedRect->y - svg.mainIedRect->vertical_margin;	// 主IED上侧边距
		int maxX = svg.switcherRectList.isEmpty() ? // 交换机在右侧，若为空则最右侧是主IED
			svg.mainIedRect->x + svg.mainIedRect->width + svg.mainIedRect->horizontal_margin * 2 :
			svg.switcherRectList.last()->x + svg.switcherRectList.last()->width + svg.switcherRectList.last()->horizontal_margin * 2;
		int maxY = 
			directRectList.isEmpty() ?	// 下方IED在主IED下方，若为空则最下侧是主IED
				svg.switcherRectList.isEmpty() ?	
				svg.mainIedRect->GetInnerBottomY() + svg.mainIedRect->vertical_margin * 2 :	// 若交换机也为空，则下方为主IED
			indirectRectList.size() > 1 ?
				indirectRectList.last()->GetInnerBottomY() + indirectRectList.last()->vertical_margin * 2 :	// 若有多个经过交换机的IED，则下方为最后一个IED
				svg.switcherRectList.last()->GetInnerBottomY() + svg.switcherRectList.last()->vertical_margin * 2 :	// 若直连IED为空，则下方为;
			directRectList.last()->GetInnerBottomY() + svg.mainIedRect->vertical_margin * 2;
		if (minX < 0) minX = 0; if (minY < 0) minY = 0;
		int w = maxX - minX;
		int h = maxY - minY;
		svg.viewBoxX = minX - svg.mainIedRect->horizontal_margin;
		svg.viewBoxY = minY - svg.mainIedRect->vertical_margin;
		if (w > 0) svg.viewBoxWidth = w;
		if (h > 0) svg.viewBoxHeight = h;
	}
}

void OpticalDiagramBuilder::SetArrowStateDirect(const QList<LogicCircuit*>& inList, const QList<LogicCircuit*>& outList, const QString& mainIedName, const QString& peerIedName, OpticalCircuitLine* pLine)
{
	foreach(LogicCircuit * pLogicCircuit, inList)
	{
		if (pLogicCircuit && pLogicCircuit->pSrcIed && pLogicCircuit->pSrcIed->name == peerIedName)
		{
			AppendLineArrowDirection(pLine, peerIedName, mainIedName);
			break;
		}
	}
	foreach(LogicCircuit * pLogicCircuit, outList)
	{
		if (pLogicCircuit && pLogicCircuit->pDestIed && pLogicCircuit->pDestIed->name == peerIedName)
		{
			AppendLineArrowDirection(pLine, mainIedName, peerIedName);
			break;
		}
	}
}

void OpticalDiagramBuilder::SetArrowStateThroughSwitch(const QList<LogicCircuit*>& inList,
	const QList<LogicCircuit*>& outList,
	const QString& mainIedName,
	const QString& peerIedName,
	OpticalCircuitLine* pOppLine,
	OpticalCircuitLine* pMainSwitchLine)
{
	QString switchIedName = ResolveLinePeerIedName(pMainSwitchLine, mainIedName);
	if (switchIedName.isEmpty())
	{
		return;
	}
	foreach(LogicCircuit * pLogicCircuit, inList)
	{
		if (pLogicCircuit && pLogicCircuit->pSrcIed && pLogicCircuit->pSrcIed->name == peerIedName)
		{
			AppendLineArrowDirection(pOppLine, peerIedName, switchIedName);
			AppendLineArrowDirection(pMainSwitchLine, switchIedName, mainIedName);
			break;
		}
	}
	foreach(LogicCircuit * pLogicCircuit, outList)
	{
		if (pLogicCircuit && pLogicCircuit->pDestIed && pLogicCircuit->pDestIed->name == peerIedName)
		{
			AppendLineArrowDirection(pMainSwitchLine, mainIedName, switchIedName);
			AppendLineArrowDirection(pOppLine, switchIedName, peerIedName);
			break;
		}
	}
}
