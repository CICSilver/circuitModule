#include "SvgTransformer.h"
#include "qcoreapplication.h"
#include <QFile>
#include <QDebug>
#include <include/pugixml/pugixml.hpp>
#include "svgmodel.h"
#include "PainterStateGuard.h"
#include "SvgUtils.h"
#include <QBuffer>
#include <sstream>
using utils::ColorHelper;

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

	struct ScopedRectRouteRange
	{
		ScopedRectRouteRange()
		{
			topMinY = 0;
			topMaxY = 0;
			bottomMinY = 0;
			bottomMaxY = 0;
			columnIndex = -1;
			orderIndex = -1;
		}
		int topMinY;
		int topMaxY;
		int bottomMinY;
		int bottomMaxY;
		int columnIndex;
		int orderIndex;
	};

	struct ScopedOpticalRoutePlan
	{
		ScopedOpticalRoutePlan()
		{
			pOpticalCircuit = NULL;
			pSrcRect = NULL;
			pDestRect = NULL;
			startSide = 0;
			endSide = 0;
			laneMinY = 0;
			laneMaxY = 0;
		}
		OpticalCircuit* pOpticalCircuit;
		IedRect* pSrcRect;
		IedRect* pDestRect;
		int startSide;
		int endSide;
		int laneMinY;
		int laneMaxY;
		QString corridorKey;
	};

	enum ScopedAnchorSide
	{
		ScopedAnchorTop = 0,
		ScopedAnchorBottom = 1
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

	static int PickScopedLaneY(int laneMinY, int laneMaxY, int routeIndex)
	{
		if (laneMinY > laneMaxY)
		{
			int tempY = laneMinY;
			laneMinY = laneMaxY;
			laneMaxY = tempY;
		}
		if (laneMinY == laneMaxY)
		{
			return laneMinY;
		}
		const int laneStep = 14;
		int laneSpan = laneMaxY - laneMinY;
		int slotCount = laneSpan / laneStep + 1;
		if (slotCount < 1)
		{
			slotCount = 1;
		}
		int actualIndex = routeIndex;
		if (actualIndex < 0)
		{
			actualIndex = 0;
		}
		actualIndex = actualIndex % slotCount;
		int usedSpan = (slotCount - 1) * laneStep;
		int startY = laneMinY + (laneSpan - usedSpan) / 2;
		return startY + actualIndex * laneStep;
	}

	static void ResolveScopedOpticalRoutePlan(const ScopedRectRouteRange& srcRange,
		const ScopedRectRouteRange& destRange,
		IedRect* pSrcRect,
		IedRect* pDestRect,
		ScopedOpticalRoutePlan& routePlan)
	{
		routePlan.pSrcRect = pSrcRect;
		routePlan.pDestRect = pDestRect;
		int srcTopY = pSrcRect ? pSrcRect->y : 0;
		int srcBottomY = GetScopedRectBottom(pSrcRect);
		int destTopY = pDestRect ? pDestRect->y : 0;
		int destBottomY = GetScopedRectBottom(pDestRect);
		int leftColumn = qMin(srcRange.columnIndex, destRange.columnIndex);
		int rightColumn = qMax(srcRange.columnIndex, destRange.columnIndex);
		const int routeGap = 18;
		if (srcBottomY + routeGap <= destTopY)
		{
			routePlan.startSide = ScopedAnchorBottom;
			routePlan.endSide = ScopedAnchorTop;
			routePlan.laneMinY = srcBottomY + routeGap;
			routePlan.laneMaxY = destTopY - routeGap;
			routePlan.corridorKey = QString("M|%1|%2|%3|%4")
				.arg(srcRange.orderIndex)
				.arg(destRange.orderIndex)
				.arg(leftColumn)
				.arg(rightColumn);
			return;
		}
		if (destBottomY + routeGap <= srcTopY)
		{
			routePlan.startSide = ScopedAnchorTop;
			routePlan.endSide = ScopedAnchorBottom;
			routePlan.laneMinY = destBottomY + routeGap;
			routePlan.laneMaxY = srcTopY - routeGap;
			routePlan.corridorKey = QString("M|%1|%2|%3|%4")
				.arg(destRange.orderIndex)
				.arg(srcRange.orderIndex)
				.arg(leftColumn)
				.arg(rightColumn);
			return;
		}

		int bottomMinY = qMax(srcRange.bottomMinY, destRange.bottomMinY);
		int bottomMaxY = qMin(srcRange.bottomMaxY, destRange.bottomMaxY);
		int topMinY = qMax(srcRange.topMinY, destRange.topMinY);
		int topMaxY = qMin(srcRange.topMaxY, destRange.topMaxY);
		bool canUseBottom = bottomMinY <= bottomMaxY;
		bool canUseTop = topMinY <= topMaxY;
		if (canUseBottom && (!canUseTop || (bottomMaxY - bottomMinY) >= (topMaxY - topMinY)))
		{
			routePlan.startSide = ScopedAnchorBottom;
			routePlan.endSide = ScopedAnchorBottom;
			routePlan.laneMinY = bottomMinY;
			routePlan.laneMaxY = bottomMaxY;
			routePlan.corridorKey = QString("B|%1|%2|%3")
				.arg(qMax(srcRange.orderIndex, destRange.orderIndex))
				.arg(leftColumn)
				.arg(rightColumn);
			return;
		}
		if (canUseTop)
		{
			routePlan.startSide = ScopedAnchorTop;
			routePlan.endSide = ScopedAnchorTop;
			routePlan.laneMinY = topMinY;
			routePlan.laneMaxY = topMaxY;
			routePlan.corridorKey = QString("T|%1|%2|%3")
				.arg(qMin(srcRange.orderIndex, destRange.orderIndex))
				.arg(leftColumn)
				.arg(rightColumn);
			return;
		}

		if ((srcRange.orderIndex + destRange.orderIndex) % 2 == 0)
		{
			routePlan.startSide = ScopedAnchorBottom;
			routePlan.endSide = ScopedAnchorBottom;
			routePlan.laneMinY = qMax(srcBottomY, destBottomY) + 20;
			routePlan.laneMaxY = routePlan.laneMinY + 40;
			routePlan.corridorKey = QString("BF|%1|%2|%3")
				.arg(qMax(srcRange.orderIndex, destRange.orderIndex))
				.arg(leftColumn)
				.arg(rightColumn);
		}
		else
		{
			routePlan.startSide = ScopedAnchorTop;
			routePlan.endSide = ScopedAnchorTop;
			routePlan.laneMaxY = qMin(srcTopY, destTopY) - 20;
			routePlan.laneMinY = routePlan.laneMaxY - 40;
			if (routePlan.laneMinY < 160)
			{
				routePlan.laneMinY = 160;
			}
			if (routePlan.laneMaxY < routePlan.laneMinY)
			{
				routePlan.laneMaxY = routePlan.laneMinY;
			}
			routePlan.corridorKey = QString("TF|%1|%2|%3")
				.arg(qMin(srcRange.orderIndex, destRange.orderIndex))
				.arg(leftColumn)
				.arg(rightColumn);
		}
	}

	static void BuildScopedOpticalLineGeometry(OpticalCircuitLine* opticalLine,
		int startSide,
		int endSide,
		int startIndex,
		int startCount,
		int endIndex,
		int endCount,
		int laneMinY,
		int laneMaxY,
		int routeIndex)
	{
		if (!opticalLine || !opticalLine->pSrcRect || !opticalLine->pDestRect)
		{
			return;
		}
		opticalLine->midPoints.clear();
		opticalLine->startPoint = BuildScopedAnchorPoint(opticalLine->pSrcRect, startSide, startIndex, startCount);
		opticalLine->endPoint = BuildScopedAnchorPoint(opticalLine->pDestRect, endSide, endIndex, endCount);
		int laneY = PickScopedLaneY(laneMinY, laneMaxY, routeIndex);
		QList<QPoint> routePointList;
		QPoint startTurnPoint(opticalLine->startPoint.x(), laneY);
		QPoint endTurnPoint(opticalLine->endPoint.x(), laneY);
		if (startTurnPoint != opticalLine->startPoint)
		{
			AppendScopedPoint(routePointList, startTurnPoint);
		}
		if (endTurnPoint != opticalLine->startPoint && endTurnPoint != opticalLine->endPoint)
		{
			AppendScopedPoint(routePointList, endTurnPoint);
		}
		for (int pointIndex = 0; pointIndex < routePointList.size(); ++pointIndex)
		{
			opticalLine->midPoints.append(routePointList.at(pointIndex));
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
		const int leftMargin = 80;
		const int topMargin = 220;
		const int columnGap = 240;
		const int rowGap = 80;
		const int routeGap = 18;
		const int tailRouteExtend = 160;
		const int firstTopLaneY = 160;
		QHash<QString, IedRect*> rectHash;
		QHash<QString, ScopedRectRouteRange> rectRouteHash;
		int maxBottom = 0;
		int totalWidth = leftMargin * 2 + RECT_DEFAULT_WIDTH;
		for (int bayIndex = 0; bayIndex < bayOrder.size(); ++bayIndex)
		{
			const QString& bayKey = bayOrder.at(bayIndex);
			QStringList visibleIedList = bayVisibleIedHash.value(bayKey);
			if (visibleIedList.isEmpty())
			{
				continue;
			}
			QStringList normalIedList;
			QStringList switchIedList;
			for (int i = 0; i < visibleIedList.size(); ++i)
			{
				if (IsSwitchIedName(visibleIedList.at(i)))
				{
					switchIedList.append(visibleIedList.at(i));
				}
				else
				{
					normalIedList.append(visibleIedList.at(i));
				}
			}
			QStringList orderedIedList = normalIedList;
			if (orderedIedList.isEmpty())
			{
				orderedIedList = switchIedList;
			}
			else
			{
				int insertIndex = orderedIedList.size() / 2;
				for (int switchIndex = 0; switchIndex < switchIedList.size(); ++switchIndex)
				{
					orderedIedList.insert(insertIndex + switchIndex, switchIedList.at(switchIndex));
				}
			}
			int columnX = leftMargin + bayIndex * (RECT_DEFAULT_WIDTH + columnGap);
			QList<IedRect*> columnRectList;
			for (int rowIndex = 0; rowIndex < orderedIedList.size(); ++rowIndex)
			{
				IED* pIed = circuitConfig->GetIedByName(orderedIedList.at(rowIndex));
				IedRect* pRect = CreateScopeRect(pIed, columnX, topMargin + rowIndex * (RECT_DEFAULT_HEIGHT + rowGap));
				if (!pRect)
				{
					continue;
				}
				rectHash.insert(pIed->name, pRect);
				if (IsSwitchIedName(pIed->name))
				{
					svg.switcherRectList.append(pRect);
				}
				else
				{
					svg.iedRectList.append(pRect);
				}
				columnRectList.append(pRect);
				maxBottom = qMax(maxBottom, GetScopedRectBottom(pRect));
			}
			for (int rectIndex = 0; rectIndex < columnRectList.size(); ++rectIndex)
			{
				IedRect* pRect = columnRectList.at(rectIndex);
				ScopedRectRouteRange routeRange;
				routeRange.columnIndex = bayIndex;
				routeRange.orderIndex = rectIndex;
				if (rectIndex > 0)
				{
					routeRange.topMinY = GetScopedRectBottom(columnRectList.at(rectIndex - 1)) + routeGap;
				}
				else
				{
					routeRange.topMinY = firstTopLaneY;
				}
				routeRange.topMaxY = pRect->y - routeGap;
				if (routeRange.topMaxY < routeRange.topMinY)
				{
					routeRange.topMaxY = routeRange.topMinY;
				}
				routeRange.bottomMinY = GetScopedRectBottom(pRect) + routeGap;
				if (rectIndex + 1 < columnRectList.size())
				{
					routeRange.bottomMaxY = columnRectList.at(rectIndex + 1)->y - routeGap;
				}
				else
				{
					routeRange.bottomMaxY = GetScopedRectBottom(pRect) + tailRouteExtend;
				}
				if (routeRange.bottomMaxY < routeRange.bottomMinY)
				{
					routeRange.bottomMaxY = routeRange.bottomMinY;
				}
				rectRouteHash.insert(pRect->iedName, routeRange);
				maxBottom = qMax(maxBottom, routeRange.bottomMaxY);
			}
			totalWidth = qMax(totalWidth, columnX + RECT_DEFAULT_WIDTH + leftMargin);
		}

		svg.mainIedRect = CreateScopeTitleRect(titleName, titleDesc, totalWidth);
		if (svg.mainIedRect)
		{
			totalWidth = qMax(totalWidth, (int)svg.mainIedRect->x + (int)svg.mainIedRect->width + leftMargin);
			maxBottom = qMax(maxBottom, (int)svg.mainIedRect->GetInnerBottomY());
		}

		QList<ScopedOpticalRoutePlan> routePlanList;
		QHash<QString, int> sideCountHash;
		for (int i = 0; i < opticalList.size(); ++i)
		{
			OpticalCircuit* pOpticalCircuit = opticalList.at(i);
			if (!pOpticalCircuit)
			{
				continue;
			}
			if (!rectHash.contains(pOpticalCircuit->srcIedName) || !rectHash.contains(pOpticalCircuit->destIedName))
			{
				continue;
			}
			if (!rectRouteHash.contains(pOpticalCircuit->srcIedName) || !rectRouteHash.contains(pOpticalCircuit->destIedName))
			{
				continue;
			}
			IedRect* pSrcRect = rectHash.value(pOpticalCircuit->srcIedName);
			IedRect* pDestRect = rectHash.value(pOpticalCircuit->destIedName);
			if (!pSrcRect || !pDestRect || pSrcRect == pDestRect)
			{
				continue;
			}
			ScopedOpticalRoutePlan routePlan;
			routePlan.pOpticalCircuit = pOpticalCircuit;
			ResolveScopedOpticalRoutePlan(
				rectRouteHash.value(pOpticalCircuit->srcIedName),
				rectRouteHash.value(pOpticalCircuit->destIedName),
				pSrcRect,
				pDestRect,
				routePlan);
			routePlanList.append(routePlan);
			QString startSideKey = BuildScopedSideKey(pOpticalCircuit->srcIedName, routePlan.startSide);
			QString endSideKey = BuildScopedSideKey(pOpticalCircuit->destIedName, routePlan.endSide);
			sideCountHash.insert(startSideKey, sideCountHash.value(startSideKey, 0) + 1);
			sideCountHash.insert(endSideKey, sideCountHash.value(endSideKey, 0) + 1);
		}

		QHash<QString, int> sideIndexHash;
		QHash<QString, int> corridorIndexHash;
		for (int routePlanIndex = 0; routePlanIndex < routePlanList.size(); ++routePlanIndex)
		{
			const ScopedOpticalRoutePlan& routePlan = routePlanList.at(routePlanIndex);
			if (!routePlan.pOpticalCircuit || !routePlan.pSrcRect || !routePlan.pDestRect)
			{
				continue;
			}
			OpticalCircuitLine* opticalLine = new OpticalCircuitLine();
			opticalLine->lineCode = routePlan.pOpticalCircuit->loopCode;
			opticalLine->arrowState = Arrow_None;
			opticalLine->pOpticalCircuit = routePlan.pOpticalCircuit;
			opticalLine->pSrcRect = routePlan.pSrcRect;
			opticalLine->pDestRect = routePlan.pDestRect;
			QString startSideKey = BuildScopedSideKey(routePlan.pOpticalCircuit->srcIedName, routePlan.startSide);
			QString endSideKey = BuildScopedSideKey(routePlan.pOpticalCircuit->destIedName, routePlan.endSide);
			int startIndex = sideIndexHash.value(startSideKey, 0);
			int endIndex = sideIndexHash.value(endSideKey, 0);
			sideIndexHash.insert(startSideKey, startIndex + 1);
			sideIndexHash.insert(endSideKey, endIndex + 1);
			int routeIndex = corridorIndexHash.value(routePlan.corridorKey, 0);
			corridorIndexHash.insert(routePlan.corridorKey, routeIndex + 1);
			BuildScopedOpticalLineGeometry(
				opticalLine,
				routePlan.startSide,
				routePlan.endSide,
				startIndex,
				sideCountHash.value(startSideKey, 1),
				endIndex,
				sideCountHash.value(endSideKey, 1),
				routePlan.laneMinY,
				routePlan.laneMaxY,
				routeIndex);
			maxBottom = qMax(maxBottom, opticalLine->startPoint.y());
			maxBottom = qMax(maxBottom, opticalLine->endPoint.y());
			for (int pointIndex = 0; pointIndex < opticalLine->midPoints.size(); ++pointIndex)
			{
				maxBottom = qMax(maxBottom, opticalLine->midPoints.at(pointIndex).y());
			}
			svg.opticalCircuitLineList.append(opticalLine);
		}

		svg.viewBoxX = 0;
		svg.viewBoxY = 0;
		svg.viewBoxWidth = qMax(totalWidth + leftMargin, SVG_VIEWBOX_WIDTH / 2);
		svg.viewBoxHeight = qMax(maxBottom + 100, topMargin + RECT_DEFAULT_HEIGHT + 100);
	}
}

SvgTransformer::SvgTransformer()
{
	m_circuitConfig = CircuitConfig::Instance();
	m_svgGenerator = new QSvgGenerator();
	m_painter = new QPainter;
	m_element_id = 0;
}

SvgTransformer::~SvgTransformer()
{
	if (m_svgGenerator)
		delete m_svgGenerator;
	if (m_painter)
		delete m_painter;

	m_svgGenerator = NULL;
	m_painter = NULL;
}

LogicSvg* SvgTransformer::BuildLogicModelByIedName(const QString& iedName)
{
	IED* pIed = m_circuitConfig->GetIedByName(iedName);
	if (!pIed) return NULL;
	LogicSvg* svg = new LogicSvg();
	GenerateLogicSvgByIed(pIed, *svg);
	return svg;
}

OpticalSvg* SvgTransformer::BuildOpticalModelByIedName(const QString& iedName)
{
	IED* pIed = m_circuitConfig->GetIedByName(iedName);
	if (!pIed) return NULL;
	OpticalSvg* svg = new OpticalSvg();
	GenerateOpticalSvgByIed(pIed, *svg);
	return svg;
}

OpticalSvg* SvgTransformer::BuildOpticalModelByStation()
{
	const CRtdbEleModelStation* station = m_circuitConfig->StationModel();
	if (!station)
	{
		return NULL;
	}

	QMap<QString, QStringList> bayIedHash;
	QMap<QString, QString> bayTextHash;
	QMap<QString, QString> iedBayHash;
	QStringList bayOrder;
	QString stationText;
	CollectBayContext(station, m_circuitConfig, bayIedHash, bayTextHash, iedBayHash, bayOrder, stationText);
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
	QSet<quint64> opticalCodeSet;
	QSet<QString>::const_iterator iedIt = visibleIedSet.constBegin();
	for (; iedIt != visibleIedSet.constEnd(); ++iedIt)
	{
		IED* pIed = m_circuitConfig->GetIedByName(*iedIt);
		if (!pIed)
		{
			continue;
		}
		for (int j = 0; j < pIed->optical_list.size(); ++j)
		{
			OpticalCircuit* opticalCircuit = pIed->optical_list.at(j);
			if (!opticalCircuit || opticalCodeSet.contains(opticalCircuit->code))
			{
				continue;
			}
			if (!visibleIedSet.contains(opticalCircuit->srcIedName) || !visibleIedSet.contains(opticalCircuit->destIedName))
			{
				continue;
			}
			opticalCodeSet.insert(opticalCircuit->code);
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
	BuildScopedOpticalSvg(m_circuitConfig, stationText, QString("Station Optical"), bayOrder, bayVisibleIedHash, opticalList, *svg);
	return svg;
}

OpticalSvg* SvgTransformer::BuildOpticalModelByBayName(const QString& bayName)
{
	const CRtdbEleModelStation* station = m_circuitConfig->StationModel();
	if (!station || bayName.isEmpty())
	{
		return NULL;
	}

	QMap<QString, QStringList> bayIedHash;
	QMap<QString, QString> bayTextHash;
	QMap<QString, QString> iedBayHash;
	QStringList bayOrder;
	QString stationText;
	CollectBayContext(station, m_circuitConfig, bayIedHash, bayTextHash, iedBayHash, bayOrder, stationText);
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
	QSet<quint64> opticalCodeSet;
	for (int i = 0; i < selectedBayIedList.size(); ++i)
	{
		IED* pIed = m_circuitConfig->GetIedByName(selectedBayIedList.at(i));
		if (!pIed)
		{
			continue;
		}
		for (int j = 0; j < pIed->optical_list.size(); ++j)
		{
			OpticalCircuit* opticalCircuit = pIed->optical_list.at(j);
			if (!opticalCircuit || opticalCodeSet.contains(opticalCircuit->code))
			{
				continue;
			}
			opticalCodeSet.insert(opticalCircuit->code);
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
	BuildScopedOpticalSvg(m_circuitConfig, bayText, QString("Bay Optical"), visibleBayOrder, bayVisibleIedHash, opticalList, *svg);
	return svg;
}

VirtualSvg* SvgTransformer::BuildVirtualModelByIedName(const QString& iedName)
{
	IED* pIed = m_circuitConfig->GetIedByName(iedName);
	if (!pIed) return NULL;
	VirtualSvg* svg = new VirtualSvg();
	GenerateVirtualSvgByIed(pIed, *svg);
	return svg;
}

WholeCircuitSvg* SvgTransformer::BuildWholeCircuitModelByIedName(const QString& iedName)
{
	IED* pIed = m_circuitConfig->GetIedByName(iedName);
	if (!pIed) return NULL;
	WholeCircuitSvg* svg = new WholeCircuitSvg();
	GenerateWholeCircuitSvgByIed(pIed, *svg);
	return svg;
}

void SvgTransformer::GenerateSvgByIedName(const QString& iedName)

{

	IED* pIed = m_circuitConfig->GetIedByName(iedName);

	if (!pIed) return;

	//// 逻辑链路图

	//QString LogicFileName = QCoreApplication::applicationDirPath() + "/" + iedName + "_logic_circuit.svg";

	//m_svgGenerator->setFileName(LogicFileName);

	//m_svgGenerator->setViewBox(QRect(0, 0, 1920, 1080));

	//m_painter->begin(m_svgGenerator);



	//// 生成逻辑链路svg描述结构

	//LogicSvg logicSvg;

	//GenerateLogicSvgByIed(pIed, logicSvg);

	//DrawLogicSvg(logicSvg);

	//m_painter->end();

	//// 解除svgGenerator对文件的占用

	//m_svgGenerator->setFileName(QString());

	//// 解析并重标识Svg中的Ied和链路属性

	//ReSignSvg(LogicFileName);



	//// 生成光纤链路svg

	//QString opticalFileName = QCoreApplication::applicationDirPath() + "/" + iedName + "_optical_circuit.svg";

	//m_svgGenerator->setFileName(opticalFileName);

	//m_svgGenerator->setViewBox(QRect(0, 0, 1920, 1080));

	//m_painter->begin(m_svgGenerator);



	//OpticalSvg opticalSvg;

	//GenerateOpticalSvgByIed(pIed, opticalSvg);

	//DrawOpticalSvg(opticalSvg);

	//m_painter->end();

	//m_svgGenerator->setFileName(QString());

	//ReSignSvg(opticalFileName);



	// 生成虚回路svg

	QString virtualFileName = QCoreApplication::applicationDirPath() + "/" + iedName + "_virtual_circuit.svg";

	m_svgGenerator->setFileName(virtualFileName);

	m_svgGenerator->setViewBox(QRect(0, 0, 2160, 1440));

	m_painter->begin(m_svgGenerator);



	VirtualSvg vtSvg;

	GenerateVirtualSvgByIed(pIed, vtSvg);

	DrawVirtualSvg(vtSvg);

	m_painter->end();

	m_svgGenerator->setFileName(QString());

	ReSignSvg(virtualFileName, vtSvg);

	if (!m_errStr.isEmpty())

	{

		qDebug() << m_errStr;

	}

}



void SvgTransformer::RenderLogicByIedName(const QString& iedName, QPaintDevice* device)

{

	if (!device) return;

	IED* pIed = m_circuitConfig->GetIedByName(iedName);

	if (!pIed) return;

	QPainter painter;

	if (!painter.begin(device)) return;

	QPainter* old = m_painter; // 备份

	m_painter = &painter;      // 替换绘图目标

	LogicSvg svg;

	GenerateLogicSvgByIed(pIed, svg);

	DrawLogicSvg(svg);

	m_painter = old;

	painter.end();

}



void SvgTransformer::RenderLogicByIedName(const QString& iedName, QPainter* activePainter)

{

	if (!activePainter) return;

	IED* pIed = m_circuitConfig->GetIedByName(iedName);

	if (!pIed) return;

	QPainter* old = m_painter;

	m_painter = activePainter;

	LogicSvg svg;

	GenerateLogicSvgByIed(pIed, svg);

	DrawLogicSvg(svg);

	m_painter = old;

}



void SvgTransformer::RenderOpticalByIedName(const QString& iedName, QPaintDevice* device)

{

	if (!device) return;

	IED* pIed = m_circuitConfig->GetIedByName(iedName);

	if (!pIed) return;

	QPainter painter;

	if (!painter.begin(device)) return;

	QPainter* old = m_painter;

	m_painter = &painter;

	OpticalSvg svg;

	GenerateOpticalSvgByIed(pIed, svg);

	DrawOpticalSvg(svg);

	m_painter = old;

	painter.end();

}



void SvgTransformer::RenderOpticalByIedName(const QString& iedName, QPainter* activePainter)

{

	if (!activePainter) return;

	IED* pIed = m_circuitConfig->GetIedByName(iedName);

	if (!pIed) return;

	QPainter* old = m_painter;

	m_painter = activePainter;

	OpticalSvg svg;

	GenerateOpticalSvgByIed(pIed, svg);

	DrawOpticalSvg(svg);

	m_painter = old;

}



void SvgTransformer::RenderVirtualByIedName(const QString& iedName, QPaintDevice* device)

{

	if (!device) return;

	IED* pIed = m_circuitConfig->GetIedByName(iedName);

	if (!pIed) return;

	QPainter painter;

	if (!painter.begin(device)) return;

	QPainter* old = m_painter;

	m_painter = &painter;

	VirtualSvg svg;

	GenerateVirtualSvgByIed(pIed, svg);

	DrawVirtualSvg(svg);

	m_painter = old;

	painter.end();

}



void SvgTransformer::RenderVirtualByIedName(const QString& iedName, QPainter* activePainter)

{

	if (!activePainter) return;

	IED* pIed = m_circuitConfig->GetIedByName(iedName);

	if (!pIed) return;

	QPainter* old = m_painter;

	m_painter = activePainter;

	VirtualSvg svg;

	GenerateVirtualSvgByIed(pIed, svg);

	DrawVirtualSvg(svg);

	m_painter = old;

}



void SvgTransformer::RenderWholeCircuitByIedName(const QString& iedName, QPaintDevice* device)

{

	if (!device) return;

	IED* pIed = m_circuitConfig->GetIedByName(iedName);

	if (!pIed) return;

	QPainter painter;

	if (!painter.begin(device)) return;

	QPainter* old = m_painter;

	m_painter = &painter;

	WholeCircuitSvg svg;

	GenerateWholeCircuitSvgByIed(pIed, svg);

	DrawWholeSvg(svg);

	m_painter = old;

	painter.end();

}



void SvgTransformer::RenderWholeCircuitByIedName(const QString& iedName, QPainter* activePainter)

{

	if (!activePainter) return;

	IED* pIed = m_circuitConfig->GetIedByName(iedName);

	if (!pIed) return;

	QPainter* old = m_painter;

	m_painter = activePainter;

	WholeCircuitSvg svg;

	GenerateWholeCircuitSvgByIed(pIed, svg);

	DrawWholeSvg(svg);

	m_painter = old;

}



QImage SvgTransformer::RenderLogicToImage(const QString& iedName, const QSize& size)

{

	IED* pIed = m_circuitConfig->GetIedByName(iedName);

	if (!pIed) return QImage();

	QSize target = size.isValid() ? size : QSize(SVG_VIEWBOX_WIDTH, SVG_VIEWBOX_HEIGHT);

	QImage img(target, QImage::Format_ARGB32_Premultiplied);

	img.fill(Qt::black);

	QPainter painter(&img);

	QPainter* old = m_painter; m_painter = &painter;

	LogicSvg svg; GenerateLogicSvgByIed(pIed, svg);

	DrawLogicSvg(svg);

	m_painter = old; painter.end();

	return img;

}



QImage SvgTransformer::RenderOpticalToImage(const QString& iedName, const QSize& size)

{

	IED* pIed = m_circuitConfig->GetIedByName(iedName);

	if (!pIed) return QImage();

	QSize target = size.isValid() ? size : QSize(SVG_VIEWBOX_WIDTH, SVG_VIEWBOX_HEIGHT);

	QImage img(target, QImage::Format_ARGB32_Premultiplied);

	img.fill(Qt::black);

	QPainter painter(&img);

	QPainter* old = m_painter; m_painter = &painter;

	OpticalSvg svg; GenerateOpticalSvgByIed(pIed, svg);

	DrawOpticalSvg(svg);

	m_painter = old; painter.end();

	return img;

}



QImage SvgTransformer::RenderVirtualToImage(const QString& iedName, const QSize& size)

{

	IED* pIed = m_circuitConfig->GetIedByName(iedName);

	if (!pIed) return QImage();

	QSize target = size.isValid() ? size : QSize(SVG_VIEWBOX_WIDTH, SVG_VIEWBOX_HEIGHT);

	QImage img(target, QImage::Format_ARGB32_Premultiplied);

	img.fill(Qt::black);

	QPainter painter(&img);

	QPainter* old = m_painter; m_painter = &painter;

	VirtualSvg svg; GenerateVirtualSvgByIed(pIed, svg);

	DrawVirtualSvg(svg);

	m_painter = old; painter.end();

	return img;

}



QImage SvgTransformer::RenderWholeCircuitToImage(const QString& iedName, const QSize& size)

{

	IED* pIed = m_circuitConfig->GetIedByName(iedName);

	if (!pIed) return QImage();

	QSize target = size.isValid() ? size : QSize(SVG_VIEWBOX_WIDTH, SVG_VIEWBOX_HEIGHT);

	QImage img(target, QImage::Format_ARGB32_Premultiplied);

	img.fill(Qt::black);

	QPainter painter(&img);

	QPainter* old = m_painter; m_painter = &painter;

	WholeCircuitSvg svg; GenerateWholeCircuitSvgByIed(pIed, svg);

	DrawWholeSvg(svg);

	m_painter = old; painter.end();

	return img;

}



void SvgTransformer::GenerateLogicSvg(const IED* pIed, const QString& filePath)

{

	GenerateSvg<LogicSvg>(pIed, filePath, &SvgTransformer::GenerateLogicSvgByIed, &SvgTransformer::DrawLogicSvg);

}



void SvgTransformer::GenerateOpticalSvg(const IED* pIed, const QString& filePath)

{

	GenerateSvg<OpticalSvg>(pIed, filePath, &SvgTransformer::GenerateOpticalSvgByIed, &SvgTransformer::DrawOpticalSvg);

}



void SvgTransformer::GenerateVirtualSvg(const IED* pIed, const QString& filePath)

{

	GenerateSvg<VirtualSvg>(pIed, filePath, &SvgTransformer::GenerateVirtualSvgByIed, &SvgTransformer::DrawVirtualSvg);

}



//void SvgTransformer::GenerateWholeCircuitSvg(const IED* pIed, const QString& filePath)

//{

//	GenerateSvg<WholeCircuitSvg>(pIed, filePath, &SvgTransformer::GenerateWholeCircuitSvgByIed, &SvgTransformer::DrawWholeSvg);

//}



QByteArray SvgTransformer::GenerateLogicSvgBytes(const QString& iedName)

{

	IED* pIed = m_circuitConfig->GetIedByName(iedName);

	if (!pIed) return QByteArray();

	QByteArray bytes;

	QBuffer buffer(&bytes);

	buffer.open(QIODevice::WriteOnly);

	QSvgGenerator generator;

	generator.setOutputDevice(&buffer);

	generator.setViewBox(QRect(0, 0, SVG_VIEWBOX_WIDTH, SVG_VIEWBOX_HEIGHT));

	QPainter painter;

	painter.begin(&generator);

	QPainter* old = m_painter;

	m_painter = &painter;

	LogicSvg svg;

	GenerateLogicSvgByIed(pIed, svg);

	DrawLogicSvg(svg);

	m_painter = old;

	painter.end();

	buffer.close();

	pugi::xml_document doc;

	doc.load_buffer(bytes.constData(), bytes.size());

	ReSignSvgDoc(doc, svg);

	std::ostringstream oss;

	doc.save(oss, "", pugi::format_raw, pugi::encoding_utf8);

	std::string s = oss.str();

	return QByteArray(s.data(), int(s.size()));

}



QByteArray SvgTransformer::GenerateOpticalSvgBytes(const QString& iedName)

{

	IED* pIed = m_circuitConfig->GetIedByName(iedName);

	if (!pIed) return QByteArray();

	QByteArray bytes;

	QBuffer buffer(&bytes);

	buffer.open(QIODevice::WriteOnly);

	QSvgGenerator generator;

	generator.setOutputDevice(&buffer);

	generator.setViewBox(QRect(0, 0, SVG_VIEWBOX_WIDTH, SVG_VIEWBOX_HEIGHT));

	QPainter painter;

	painter.begin(&generator);

	QPainter* old = m_painter;

	m_painter = &painter;

	OpticalSvg svg;

	GenerateOpticalSvgByIed(pIed, svg);

	DrawOpticalSvg(svg);

	m_painter = old;

	painter.end();

	buffer.close();

	pugi::xml_document doc;

	doc.load_buffer(bytes.constData(), bytes.size());

	ReSignSvgDoc(doc, svg);

	std::ostringstream oss; 

	doc.save(oss, "", pugi::format_raw, pugi::encoding_utf8);

	std::string s = oss.str();

	return QByteArray(s.data(), int(s.size()));

}



QByteArray SvgTransformer::GenerateVirtualSvgBytes(const QString& iedName, const QString& swName)

{

	IED* pIed = m_circuitConfig->GetIedByName(iedName);

	if (!pIed) return QByteArray();

	QByteArray bytes;

	QBuffer buffer(&bytes);

	buffer.open(QIODevice::WriteOnly);

	QSvgGenerator generator;

	generator.setOutputDevice(&buffer);

	generator.setViewBox(QRect(0, 0, SVG_VIEWBOX_WIDTH, SVG_VIEWBOX_HEIGHT));

	QPainter painter;

	painter.begin(&generator);

	QPainter* old = m_painter;

	m_painter = &painter;

	VirtualSvg svg;

	GenerateVirtualSvgByIed(pIed, svg/*, swName*/);

	DrawVirtualSvg(svg);

	m_painter = old;

	painter.end();

	buffer.close();

	pugi::xml_document doc;

	doc.load_buffer(bytes.constData(), bytes.size());

	ReSignSvgDoc(doc, svg);

	std::ostringstream oss; doc.save(oss, "", pugi::format_raw, pugi::encoding_utf8);

	std::string s = oss.str();

	return QByteArray(s.data(), int(s.size()));

}



QByteArray SvgTransformer::GenerateWholeCircuitSvgBytes(const QString& iedName)

{

	IED* pIed = m_circuitConfig->GetIedByName(iedName);

	if (!pIed) return QByteArray();

	QByteArray bytes;

	QBuffer buffer(&bytes);

	buffer.open(QIODevice::WriteOnly);

	QSvgGenerator generator;

	generator.setOutputDevice(&buffer);

	generator.setViewBox(QRect(0, 0, SVG_VIEWBOX_WIDTH, SVG_VIEWBOX_HEIGHT));

	QPainter painter;

	painter.begin(&generator);

	QPainter* old = m_painter;

	m_painter = &painter;

	WholeCircuitSvg svg;

	GenerateWholeCircuitSvgByIed(pIed, svg);

	DrawWholeSvg(svg);

	m_painter = old;

	painter.end();

	buffer.close();

	pugi::xml_document doc;

	doc.load_buffer(bytes.constData(), bytes.size());

	ReSignSvgDoc(doc, svg);

	std::ostringstream oss; doc.save(oss, "", pugi::format_raw, pugi::encoding_utf8);

	std::string s = oss.str();

	return QByteArray(s.data(), int(s.size()));

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



void SvgTransformer::ParseCircuitFromIed(LogicSvg& svg, const IED* pIed)

{

	quint8 index = 0; // 关联ied编号，用于区分位置

	QList<IedRect*>* list = &svg.leftIedRectList;

	QList<LogicCircuit*> inCircuitList = m_circuitConfig->GetInLogicCircuitListByIedName(pIed->name);

	QList<LogicCircuit*> outCircuitList = m_circuitConfig->GetOutLogicCircuitListByIedName(pIed->name);

	foreach(const QString & iedName, pIed->connectedIedNameSet)

	{

		if (iedName.contains("SW"))

		{

			// 交换机设备不绘制逻辑链路图

			continue;

		}

		IED* pConnectedIed = CircuitConfig::Instance()->GetIedByName(iedName);

		IedRect* otherIedRect = GetOtherIedRect(index, svg.mainIedRect, pConnectedIed, ColorHelper::pure_green);

		list = index % 2 == 0 ? &svg.leftIedRectList : &svg.rightIedRectList;



		//quint16 connect_point_x = col == 0 ? otherIedRect.x + otherIedRect.width : otherIedRect.x; // 左侧列连接点位于矩形右侧，右侧列连接点位于矩形左侧

		foreach(LogicCircuit * pLogicCircuit, inCircuitList)

		{

			if (pLogicCircuit->pSrcIed->name == pConnectedIed->name)

			{

				LogicCircuitLine* line = new LogicCircuitLine();

				//line->id = otherIedRect->logic_circuit_line_list.size();

				line->pSrcIedRect = otherIedRect;

				line->pDestIedRect = svg.mainIedRect;

				line->pLogicCircuit = pLogicCircuit;

				otherIedRect->logic_line_list.append(line);

			}

		}

		// 出链路

		foreach(LogicCircuit * pLogicCircuit, outCircuitList)

		{

			if (pLogicCircuit->pDestIed->name == pConnectedIed->name)

			{

				LogicCircuitLine* line = new LogicCircuitLine();

				//line->id = otherIedRect->logic_circuit_line_list.size();

				line->pSrcIedRect = svg.mainIedRect;

				line->pDestIedRect = otherIedRect;

				line->pLogicCircuit = pLogicCircuit;

				otherIedRect->logic_line_list.append(line);

			}

		}

		list->append(otherIedRect);

		++index;

	}

}



void SvgTransformer::GenerateOpticalSvgByIed(const IED* pIed, OpticalSvg& svg)

{

	if(pIed->name.contains("SW"))

	{

		// 交换机作为中心设备时，不生成光纤链路图

		return;	

	}

	const int horizontal_distance = 50;

	const int vertical_distance = 150;

	const int switcherWidth = SVG_VIEWBOX_WIDTH * 0.75;

	QList<IedRect*> directRectList;		// 直连IED列表

	QList<IedRect*> indirectRectList;	// 经过交换机的IED列表

	QHash<IedRect*, qint8> connectPtHash;	// 记录连接点数量，交换机不记录直接绘制垂直线

	QList<QString> iedNameList;	// 已绘制的设备光纤号

	// 主IED 中心位置

	svg.mainIedRect = utils::GetIedRect(

		pIed->name,

		pIed->desc,

		(SVG_VIEWBOX_WIDTH - RECT_DEFAULT_WIDTH) / 2,

		horizontal_distance,

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

				SVG_VIEWBOX_HEIGHT - RECT_DEFAULT_HEIGHT * 2 - horizontal_distance,	// y

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

			IED* pOppsiteIed = m_circuitConfig->GetIedByName(oppsiteIedName);

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

						m_circuitConfig->GetIedByName(iedName)->desc,

						0,	// x，暂不设置

						SVG_VIEWBOX_HEIGHT - RECT_DEFAULT_HEIGHT * 2 - horizontal_distance,	// y

						RECT_DEFAULT_WIDTH,

						RECT_DEFAULT_HEIGHT,

						ColorHelper::ied_border,

						ColorHelper::ied_underground);

					svg.iedRectList.append(pIedRect);

					indirectRectList.append(pIedRect);

				}

				// 获取对端设备和交换机的光纤链路

				QList<OpticalCircuit*> oppsiteOptList = m_circuitConfig->getOpticalByIeds(pOppsiteIed->name, iedName);

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

				QList<LogicCircuit*> inCircuitList = m_circuitConfig->GetInLogicCircuitListByIedName(pIed->name);

				QList<LogicCircuit*> outCircuitList = m_circuitConfig->GetOutLogicCircuitListByIedName(pIed->name);

				SetArrowStateThroughSwitch(inCircuitList, outCircuitList, iedName, iedSet.size(), oppsiteOptLine->arrowState, optLine->arrowState);

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

			pOppsiteIedRect->y = svg.mainIedRect->y + svg.mainIedRect->height + vertical_distance + svg.switcherRectList.size() * (RECT_DEFAULT_HEIGHT + vertical_distance * 0.5);	// 调整交换机位置到主IED下方

			svg.switcherRectList.append(pOppsiteIedRect);

		}

		else

		{

			// 直连设备

			//optLine->arrowState

			SetArrowStateDirect(

				m_circuitConfig->GetInLogicCircuitListByIedName(pIed->name),

				m_circuitConfig->GetOutLogicCircuitListByIedName(pIed->name),

				oppsiteIedName,

				optLine->arrowState);

			directRectList.append(pOppsiteIedRect);

			++connectPtHash[pOppsiteIedRect];

		}

		++connectPtHash[svg.mainIedRect];

		optLine->pSrcRect = optCircuit->srcIedName == pIed->name ? svg.mainIedRect : pOppsiteIedRect;

		optLine->pDestRect = optCircuit->srcIedName == pIed->name ? pOppsiteIedRect : svg.mainIedRect;

	}

	// 直连IED全部靠左排列

	int x = 0;

	

	int belowIedY = svg.switcherRectList.isEmpty() ? 3 * IED_VERTICAL_DISTANCE + vertical_distance + svg.mainIedRect->GetInnerBottomY() : svg.switcherRectList.last()->GetInnerBottomY() + vertical_distance;	// 交换机下方的IED y

	for (size_t i = 0; i < directRectList.size(); ++i)

	{

		IedRect* rect = directRectList.at(i);

			rect->x = horizontal_distance + i * (rect->width + horizontal_distance);

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

			rect->x = svg.mainIedRect->x + svg.mainIedRect->width + horizontal_distance;

			rect->y = svg.mainIedRect->y;

			totalWidth = svg.mainIedRect->width + rect->width + horizontal_distance;

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

				rect->x = startX + horizontal_distance + i * (horizontal_distance + rect->width);

				rect->y = directRectList.isEmpty() ? svg.switcherRectList.last()->y + svg.switcherRectList.last()->height + horizontal_distance * 2 : directRectList.last()->y;

				totalWidth += rect->width + horizontal_distance;

			}

			totalWidth -= horizontal_distance;

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

		int midPointDistance = 10;	// 中间转折点垂直/水平距离

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



void SvgTransformer::GenerateVirtualSvgByIed(const IED* pIed, VirtualSvg& svg/*, const QString& swName*/)

{

	//// 生成图形

	// 矩形构成：内矩形（IED名、描述）+ 外矩形（回路、回路类型图标、压板矩形（压板连接状态）、测量值、）+

	// 添加主IED矩形，居中且位于顶部，外侧矩形高度由回路数量决定

	int svgTopMargin = 50;	// SVG视图顶部边距

	svg.mainIedRect = utils::GetIedRect(

		pIed->name,

		pIed->desc,

		(SVG_VIEWBOX_WIDTH - RECT_DEFAULT_WIDTH) / 2,

		svgTopMargin,

		RECT_DEFAULT_WIDTH * 1.5,

		RECT_DEFAULT_HEIGHT,

		ColorHelper::pure_red

	);

	if(pIed->connectedIedNameSet.isEmpty())

	{

		// 主IED没有连接其他设备，直接返回

		return;

	}

	// 主设备高度 = max(侧边IED矩形高度) + margin + IED描述矩形高度;

	// 关联IED，增加矩形间隔用于显示回路信息

	ParseCircuitFromIed(svg, pIed);

	if (pIed->name == "P_B5012A")

	{

		int a = 1;

	}



	//// 调整矩形位置

	// 根据单侧关联IED数量和压板高度，调整高度

	AdjustExtendRectByCircuit(svg.leftIedRectList, svg);

	AdjustExtendRectByCircuit(svg.rightIedRectList, svg);

	// 计算两侧IED的链路数量()

	size_t leftCircuitSize = svg.GetLeftCircuitSize();

	size_t rightCircuitSize = svg.GetRightCircuitSize();

	// 计算主IED高度

	size_t maxY = qMax(

		svg.leftIedRectList.size() > 0 ? svg.leftIedRectList.last()->GetExtendBottomY() : 0, 

		svg.rightIedRectList.size() > 0 ? svg.rightIedRectList.last()->GetExtendBottomY() : 0);

	svg.mainIedRect->extend_height = maxY;

	// 根据IED位置调整SVG视图大小

	// 保证右侧边距与左侧一致，图像居中对齐

	int svgRightMargin = svg.leftIedRectList.isEmpty() ?

		svg.mainIedRect->x - svg.mainIedRect->inner_gap :	// 左侧无IED矩形时，SVG视图右侧边距为主IED矩形左侧边距

		svg.leftIedRectList.first()->x - svg.leftIedRectList.first()->inner_gap;	// 左侧有IED矩形时，SVG视图右侧边距为左侧IED矩形左侧边距

	//int svgBottomMargin = svg.mainIedRect->y - svg.mainIedRect->inner_gap;	// SVG视图底部边距为主IED矩形上侧边距

	svg.viewBoxHeight = svg.mainIedRect->GetExtendHeight() + svgTopMargin;

	svg.viewBoxWidth = svg.rightIedRectList.isEmpty() ?

		svg.mainIedRect->x + svg.mainIedRect->width + svgRightMargin :

		svg.rightIedRectList.last()->x + svg.rightIedRectList.last()->width + svgRightMargin;



	// 生成并调整回路位置、回路信息、软压板位置

	AdjustVirtualCircuitLinePosition(svg);

}



void SvgTransformer::GenerateWholeCircuitSvgByIed(const IED* pIed, WholeCircuitSvg& svg)

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



void SvgTransformer::DrawLogicSvg(LogicSvg& svg)

{

	// 主IED

	DrawIedRect(svg.mainIedRect);		// 主体

	DrawExternalRect(svg.mainIedRect, QString::fromLocal8Bit("检修域"));	// 外部虚线框



	// 关联IED

	DrawIedRect(svg.leftIedRectList);	// 左侧

	DrawIedRect(svg.rightIedRectList);	// 右侧

	DrawExternalRect(svg.leftIedRectList, QString::fromLocal8Bit("影响域"), true);	// 左侧外部虚线框，上方描述矩形

	DrawExternalRect(svg.rightIedRectList, QString::fromLocal8Bit("影响域"));	// 右侧外部虚线框

	

	// 关联链路

	DrawLogicCircuitLine(svg.leftIedRectList);

	DrawLogicCircuitLine(svg.rightIedRectList);



}



void SvgTransformer::DrawOpticalSvg(OpticalSvg& svg)

{

	if(svg.mainIedRect->iedName == "PL2205NA")

	{

		int a = 1;

	}

	// 主IED

	DrawIedRect(svg.mainIedRect);

	// 下方关联IED

	foreach(IedRect * pRect, svg.iedRectList)

	{

		DrawIedRect(pRect);

	}

	// 光纤链路

	foreach(OpticalCircuitLine* optLine, svg.opticalCircuitLineList)

	{

		DrawOpticalLine(optLine);

	}

	// 交换机

	foreach(IedRect* pRect, svg.switcherRectList)

	{

		DrawSwitcherRect(pRect);

	}

	//端口号

	foreach(OpticalCircuitLine* pLine, svg.opticalCircuitLineList)

	{

		DrawPortText(pLine, CONN_R);

	}



}



void SvgTransformer::DrawVirtualSvg(VirtualSvg& svg)

{

	// 绘制IED矩形

	DrawIedRect(svg.mainIedRect);

	DrawIedRect(svg.leftIedRectList);

	DrawIedRect(svg.rightIedRectList);



	// 绘制链路

	DrawVirtualCircuitLine(svg.leftIedRectList);

	DrawVirtualCircuitLine(svg.rightIedRectList);



	// 绘制压板

	DrawPlate(svg.plateRectHash);

}



void SvgTransformer::DrawWholeSvg(WholeCircuitSvg& svg)

{

	// 绘制IED矩形

	DrawIedRect(svg.mainIedRect);

	DrawIedRect(svg.leftIedRectList);

	DrawIedRect(svg.rightIedRectList);



	// 绘制链路

	foreach(IedRect * pIedRect, svg.leftIedRectList)

	{

		QPoint iconStartPos = QPoint(pIedRect->x + pIedRect->width - ICON_LENGTH * 2, pIedRect->y + pIedRect->height + ICON_LENGTH);

		foreach(LogicCircuitLine* pLogicCircuit, pIedRect->logic_line_list)

		{

		}

	}

}



void SvgTransformer::DrawConnCircle(const QPoint& pt, int radius, bool isCircleUnderPt)

{

	//m_painter->save();

	PAINTER_STATE_GUARD(m_painter);

	QBrush brush(ColorHelper::Color(ColorHelper::pure_green));

	m_painter->setBrush(brush);

	int y = isCircleUnderPt ? pt.y() + radius : pt.y() - radius;

	m_painter->drawEllipse(QPoint(pt.x(), y), radius, radius);

	// m_painter->restore();

}





void SvgTransformer::AdjustOtherIedRectPosition(QList<IedRect*>& rectList, const IedRect* mainIedRect)

{

	if (rectList.size() > 1)

	{

		IedRect* firstIed = rectList.first();

		IedRect* lastIed = rectList.last();

		quint16 y = mainIedRect->y - ((lastIed->y - firstIed->y + lastIed->height) - mainIedRect->height) / 2;

		for (int i = 0; i < rectList.size(); ++i)

		{

			rectList[i]->y = y + i * (IED_VERTICAL_DISTANCE + RECT_DEFAULT_HEIGHT);

		}

	}

	else if(rectList.size() > 0)

	{

		IedRect* rect = rectList.first();

		int diffHeight = (int)(mainIedRect->height) - (int)(rect->height);

		rect->y = mainIedRect->y + diffHeight / 2;

	}

}



void SvgTransformer::AdjustLogicCircuitLinePosition(LogicSvg& svg)

{

	// 调整左侧IED到主IED链路位置

	AdjustMainSideCircuitLinePosition(svg.GetLeftLogicCircuitSize(), svg.leftIedRectList, svg.mainIedRect);

	AdjustMainSideCircuitLinePosition(svg.GetRightLogicCircuitSize(), svg.rightIedRectList, svg.mainIedRect, false);

}



void SvgTransformer::AdjustVirtualCircuitLinePosition(VirtualSvg& svg)

{

	if (svg.mainIedRect->iedName == "P_B5012A")

	{

		int a = 1;

	}

	AdjustVirtualCircuitLinePosition(svg, svg.leftIedRectList, svg.mainIedRect);

	AdjustVirtualCircuitLinePosition(svg, svg.rightIedRectList, svg.mainIedRect, false);

}



void SvgTransformer::AdjustExtendRectByCircuit(IedRect& pRect)

{

	int circuitSize = 0;

	foreach(LogicCircuitLine * line, pRect.logic_line_list)

	{

		circuitSize += line->pLogicCircuit->circuitList.size();

	}

	pRect.extend_height = circuitSize * (CIRCUIT_VERTICAL_DISTANCE + ICON_LENGTH) + pRect.inner_gap + PLATE_HEIGHT;

}



void SvgTransformer::AdjustExtendRectByCircuit(QList<IedRect*>& iedList, LogicSvg& svg)

{

	quint16 lastY = svg.mainIedRect->y + svg.mainIedRect->height;

	quint8 cnt = 0;

	for (int i = 0; i < iedList.size(); ++i)

	{

		IedRect* pRect = iedList.at(i);

		pRect->y = lastY;

		AdjustExtendRectByCircuit(*pRect);

		lastY += pRect->height + pRect->extend_height + IED_VERTICAL_DISTANCE + 2 * pRect->vertical_margin;

	}

}



void SvgTransformer::SetArrowStateDirect(const QList<LogicCircuit*>& inList, const QList<LogicCircuit*>& outList, const QString& peerIedName, quint8& lineState)

{

	quint8 dummy = 0;

	SetArrowStateThroughSwitch(inList, outList, peerIedName, 1,

		dummy,

		lineState);

}



void SvgTransformer::SetArrowStateThroughSwitch(const QList<LogicCircuit*>& inList,

	const QList<LogicCircuit*>& outList,

	const QString& peerIedName,

	int peersCount,

	quint8& oppLineState,

	quint8& mainSwitchState)

{

	// 对侧 IED -> 主 IED（主IED的入链路里，源为对侧 IED）

	foreach(LogicCircuit * pLogicCircuit, inList)

	{

		if (pLogicCircuit && pLogicCircuit->pSrcIed && pLogicCircuit->pSrcIed->name == peerIedName)

		{

			// 多个 IED 经同一交换机：交换机?对侧 的方向与只有一个时相反

			oppLineState |= (peersCount > 1) ? Arrow_In : Arrow_Out;

			// 主?交换机 这段为“入”

			mainSwitchState |= Arrow_In;

			break; 

		}

	}



	// 主 IED -> 对侧 IED（主IED的出链路里，目的为对侧 IED）

	foreach(LogicCircuit * pLogicCircuit, outList)

	{

		if (pLogicCircuit && pLogicCircuit->pDestIed && pLogicCircuit->pDestIed->name == peerIedName)

		{

			// 多个 IED 经同一交换机：交换机?对侧 的方向与只有一个时相反

			oppLineState |= (peersCount > 1) ? Arrow_Out : Arrow_In;

			// 主?交换机 这段为“出”

			mainSwitchState |= Arrow_Out;

			break;

		}

	}



	// 如果两种都没命中，就不改状态（保持原值）

}





void SvgTransformer::drawArrowHeader(const QPoint& endPoint, double arrowAngle, QColor color, int arrowLen)

{

	//m_painter->save();

	PAINTER_STATE_GUARD(m_painter);

	double arrowHeadLineAngle = 150;

	QPointF leftArrowPoint = endPoint + QPointF(

		arrowLen * cos(AngleToRadians(arrowAngle + arrowHeadLineAngle)),

		-arrowLen * sin(AngleToRadians(arrowAngle + arrowHeadLineAngle)));

	QPointF rightArrowPoint = endPoint + QPointF(

		arrowLen * cos(AngleToRadians(arrowAngle - arrowHeadLineAngle)),

		-arrowLen * sin(AngleToRadians(arrowAngle - arrowHeadLineAngle)));

	int offset = 8;

	// 考虑当箭头方向不是垂直时的偏移量，offset偏移仅为箭头方向的偏移量

	// 箭头方向减少偏移，假设箭头点位置为endPoint，则偏移点位置为 

	QPointF offsetPoint = QPointF(

		endPoint.x() + offset * cos(AngleToRadians(arrowAngle)), 

		endPoint.y() - offset * sin(AngleToRadians(arrowAngle)));

	QBrush brush(color, Qt::SolidPattern);

	QPen pen(Qt::NoPen);

	m_painter->setPen(pen);

	m_painter->setBrush(brush);

	QVector<QPointF> points;

	points << leftArrowPoint << offsetPoint << rightArrowPoint << endPoint;

	m_painter->drawPolygon(points.data(), 4);

	// m_painter->restore();

}



double SvgTransformer::GetAngleByVec(const QPointF& vec) const

{

	double direction = atan2(-vec.y(), vec.x());

	double angle = direction * (180 / M_PI);

	return angle;

}



QPoint SvgTransformer::GetArrowPt(const QPoint& pt, int arrowLen, int conn_r, double angle, bool isUnderConnpt, int offset /* 箭头偏移量，向前兼容 */)

{

	//int offset = 10;	

    

    // 保留原有垂直方向的特殊处理逻辑，确保向前兼容

    if (abs(angle - 90) < 0.001 || abs(angle + 90) < 0.001) {

        // 原有垂直方向的处理，保持不变

        if (isUnderConnpt) {

            return QPoint(

                pt.x(),

                pt.y() + 2 * conn_r + (angle > 0 ? 1 : (2 * arrowLen + 5)) + offset

            );

        } else {

            return QPoint(

                pt.x(),

                pt.y() - 2 * conn_r - (angle > 0 ? (2 * arrowLen + 5) : 1) - offset

            );

        }

    }

    

    // 任意方向的箭头偏移计算

    double angleRad = AngleToRadians(angle);

    

    // 根据参数计算箭头点在连接点的哪一侧

    int direction = isUnderConnpt ? 1 : -1;

    

    // 计算总偏移距离

    int totalOffset = 2 * conn_r + arrowLen + offset;

    

    // 使用三角函数计算任意方向上的偏移点

    return QPoint(

        pt.x() + direction * totalOffset * cos(angleRad),

        pt.y() - direction * totalOffset * sin(angleRad)

    );

}



IedRect* SvgTransformer::GetOtherIedRect(quint16 rectIndex, IedRect* mainIedRect, const IED* pIed, const quint32 border_color, const quint32 underground_color)

{

	quint8 row = rectIndex / 2;

	quint8 col = rectIndex % 2;



	quint16 x = mainIedRect->x + (col == 0 ? -IED_HORIZONTAL_DISTANCE - RECT_DEFAULT_WIDTH : IED_HORIZONTAL_DISTANCE + mainIedRect->width);

	quint16 y = mainIedRect->y - IED_VERTICAL_DISTANCE + row * (IED_VERTICAL_DISTANCE + RECT_DEFAULT_HEIGHT);



	return utils::GetIedRect(pIed->name, pIed->desc, x, y, RECT_DEFAULT_WIDTH, RECT_DEFAULT_HEIGHT - 15, border_color, underground_color);

}



//IedRect * SvgTransformer::GetIedRect(QString iedName, QString iedDesc, const quint16 x, const quint16 y, const quint16 width, const quint16 height, const quint32 border_color, const quint32 underground_color)

//{

//	IedRect* iedRect = new IedRect;

//	iedRect->iedName = iedName;

//	iedRect->iedDesc = iedDesc;

//	GetBaseRect(iedRect, x, y, width, height, border_color, underground_color);

//	return iedRect;

//}



void SvgTransformer::DrawDescRect(const IedRect& leftTopRect)

{

	DrawDescRect(leftTopRect, m_painter);

}



void SvgTransformer::DrawDescRect(const IedRect& leftTopRect, QPainter* _painter)

{

	QFont descFont;

	descFont.setPointSize(15);

	QFontMetrics fm(descFont);

	quint16 width = 180;

	quint16 height = fm.height() * 2;

	quint16 x = leftTopRect.x;

	quint16 y = leftTopRect.y - height - 20;



	QPen pen;

	//pen.setColor(ColorHelper::Color(ColorHelper::line_gse));

	pen.setColor(Qt::white);

	pen.setStyle(Qt::DashLine);

	_painter->setPen(pen);

	_painter->setFont(descFont);



	quint8 descLineWidth = 160;

	_painter->drawRect(x, y, width, height);

	_painter->drawText(x, y + fm.height(), "GSE");

	_painter->drawText(x, y + fm.height() * 2, "SV");

}



//void SvgTransformer::GetBaseRect(SvgRect* rect, const quint16 x, const quint16 y, const quint16 width, const quint16 height, const quint32 border_color, const quint32 underground_color)

//{

//	rect->width = width;

//	rect->height = height;

//	rect->x = x;

//	rect->y = y;

//

//	rect->border_color = border_color;

//	rect->underground_color = underground_color;

//

//}



void SvgTransformer::AdjustMainSideCircuitLinePosition(const size_t circuitCnt, QList<IedRect*>& rectList, IedRect* mainIedRect, bool isLeft)

{

	size_t mainCircuitDiff = mainIedRect->height / (circuitCnt + 1);

	// 左侧IED列表，主IED连接点位于左侧；右侧IED列表，主IED连接点位于右侧

	quint8* mainConnectPointIndex = isLeft ? &mainIedRect->left_connect_index : &mainIedRect->right_connect_index;

	quint16 mainConnectPoint_x = isLeft ? mainIedRect->x : mainIedRect->x + mainIedRect->width;

	foreach(IedRect* rect, rectList)

	{

		size_t circuitCnt = rect->logic_line_list.size();

		size_t circuitDiff = rect->height / (circuitCnt + 1);

		// 左侧IED列表，当前IED连接点位于右侧；右侧IED列表，当前IED连接点位于左侧

		quint8* connectPointIndex = isLeft ? &rect->right_connect_index : &rect->left_connect_index;

		foreach(LogicCircuitLine* line, rect->logic_line_list)

		{

			quint16 connectPoint_x = isLeft ? rect->x + rect->width : rect->x;

			quint16 connectPoint_y = rect->y + (*connectPointIndex + 1) * circuitDiff;

			QPoint& connectPoint = rect == line->pSrcIedRect ? line->startPoint : line->endPoint;

			QPoint& mainConnectPoint = rect == line->pSrcIedRect ? line->endPoint : line->startPoint;

			connectPoint = QPoint(connectPoint_x, connectPoint_y);

			mainConnectPoint = QPoint(mainConnectPoint_x, mainIedRect->y + (*mainConnectPointIndex + 1) * mainCircuitDiff);



			quint16 midPoint_x = isLeft ?

				connectPoint_x + abs((connectPoint_x - mainConnectPoint_x) / 2) :

				mainConnectPoint_x + abs((connectPoint_x - mainConnectPoint_x) / 2);

			line->midPoint = QPoint(midPoint_x, mainConnectPoint.y());

			++(*connectPointIndex);

			++(*mainConnectPointIndex);

		}

	}

}



void SvgTransformer::AdjustVirtualCircuitLinePosition(VirtualSvg& svg, QList<IedRect*>& rectList, IedRect* mainIed, bool isLeft)

{

    foreach(IedRect* rect, rectList)

    {

        foreach(LogicCircuitLine* pLogicLine, rect->logic_line_list)

        {

            foreach(VirtualCircuit* pCircuit, pLogicLine->pLogicCircuit->circuitList)

            {

                // 设备当前逻辑链路下所属的全部通道

                VirtualCircuitLine* pVtLine = new VirtualCircuitLine(pCircuit);

                // 确定源矩形和目标矩形

                bool isSideSource = (pCircuit->srcIedName == rect->iedName);

                IedRect* srcRect = isSideSource ? rect : mainIed;

                IedRect* destRect = isSideSource ? mainIed : rect;

                pVtLine->pSrcIedRect = srcRect;

                pVtLine->pDestIedRect = destRect;

                // 确定连接点索引

                quint8* pConnectPtIndex = isLeft ? &rect->right_connect_index : &rect->left_connect_index;

                quint8* pMainConnectPtIndex = isLeft ? &mainIed->right_connect_index : &mainIed->left_connect_index;

                

                // 确定起始点X坐标和结束点X坐标

                quint16 startPt_X, endPt_X, startIconPt_X, endIconPt_X, startVal_X, endVal_X;

				QFont font;

				font.setPointSize(18);

				QFontMetrics fm(font);

				int valWidth = fm.width("000.00");



				int startOffset = ICON_LENGTH * 4 + rect->inner_gap;

				int iconOffset = ICON_LENGTH + rect->inner_gap;

				int valGap = rect->inner_gap * 2 + valWidth;

				int iedDistance = IED_HORIZONTAL_DISTANCE - rect->inner_gap * 4;

				if (isSideSource) {

					// 侧边设备是输出（源）设备，主设备是输入（目标）设备

					startPt_X = isLeft ?

						rect->x + rect->width - startOffset :

						rect->x + startOffset;



					endPt_X = isLeft ?

						mainIed->x + startOffset :

						mainIed->x + mainIed->width - startOffset;



					// 计算起始点和结束点的图标X坐标

					startIconPt_X = isLeft ?

						startPt_X - iconOffset :

						startPt_X + rect->inner_gap;



					endIconPt_X = isLeft ?

						endPt_X + rect->inner_gap :

						endPt_X - iconOffset;



					startVal_X = isLeft ?

						startIconPt_X - valGap :

						startIconPt_X + iconOffset;



					endVal_X = isLeft ?

						endIconPt_X + iconOffset :

						endIconPt_X - valGap;



					// 目标 <- 源 -> 目标

					pVtLine->circuitDesc = isLeft ?

						QString("%1 -> %2").arg(pCircuit->srcDesc, pCircuit->destDesc) :

						QString("%1 <- %2").arg(pCircuit->destDesc, pCircuit->srcDesc);

				}

				else {

					// 侧边设备是输入（目标）设备，主设备是输出（源）设备

					startPt_X = isLeft ?

						mainIed->x + startOffset :

						mainIed->x + mainIed->width - startOffset;



					endPt_X = isLeft ?

						rect->x + rect->width - startOffset :

						rect->x + startOffset;



					// 计算起始点和结束点的图标X坐标

					startIconPt_X = isLeft ?

						startPt_X + rect->inner_gap :

						startPt_X - iconOffset;



					endIconPt_X = isLeft ?

						endPt_X - iconOffset :

						endPt_X + rect->inner_gap;



					startVal_X = isLeft ?

						startIconPt_X + iconOffset :

						startIconPt_X - valGap;



					endVal_X = isLeft ?

						endIconPt_X - valGap :

						endIconPt_X + iconOffset;



					// 源 -> 目标 <- 源

					pVtLine->circuitDesc = isLeft ?

						QString("%1 <- %2").arg(pCircuit->destDesc, pCircuit->srcDesc) :

						QString("%1 -> %2").arg(pCircuit->srcDesc, pCircuit->destDesc);

				}

				// 描述信息位于设备矩形中间空白处，即左侧矩形的最右侧位置

				quint16 descRect_X = srcRect->x < destRect->x ? srcRect->x + srcRect->width + srcRect->inner_gap * 2 : destRect->x + destRect->width + destRect->inner_gap * 2;

				if (fm.width(pVtLine->circuitDesc) > iedDistance)

				{

					// 若描述信息过长，则在右侧进行省略

					pVtLine->circuitDesc = fm.elidedText(pVtLine->circuitDesc, Qt::ElideRight, iedDistance);

				}

                // 计算Y坐标，全部链路平行所以均以侧边设备位置计算

				quint16 pt_Y = rect->GetInnerBottomY() + CIRCUIT_VERTICAL_DISTANCE + ICON_LENGTH +

					*pConnectPtIndex * (CIRCUIT_VERTICAL_DISTANCE + ICON_LENGTH);

				quint16 icon_Y = pt_Y - ICON_LENGTH / 2;	// 保持中心与线段对齐



				if (svg.mainIedRect->iedName == "P_B5012A" && pCircuit->srcIedName == "MIB5012A")

				{

					int a = 1;

				}

                // 设置线路起止点

                pVtLine->startPoint = QPoint(startPt_X, pt_Y);

                pVtLine->endPoint = QPoint(endPt_X, pt_Y);

				pVtLine->startIconPt = QPoint(startIconPt_X, icon_Y);

				pVtLine->endIconPt = QPoint(endIconPt_X, icon_Y);

				pVtLine->startValRect = QRect(QPoint(startVal_X, icon_Y), QPoint(startVal_X + valWidth, icon_Y));

				pVtLine->endValRect = QRect(QPoint(endVal_X, icon_Y), QPoint(endVal_X + valWidth, icon_Y));

				pVtLine->circuitDescRect = QRect(QPoint(descRect_X, pt_Y - fm.height() * 1.2), QSize(iedDistance, fm.height()));

                pLogicLine->virtual_line_list.append(pVtLine);

				// 生成并调整当前通道的压板矩形位置

				QString key = QString("%1+%2").arg(srcRect->iedName).arg(destRect->iedName);	// 输出+输入

				if (!pCircuit->destSoftPlateDesc.isEmpty() || !pCircuit->srcSoftPlateDesc.isEmpty())

				{

					int a = 0;

				}

				// 开入软压板，图形位于回路终点

				AdjustVirtualCircuitPlatePosition(svg.plateRectHash, key, pVtLine->endPoint, pCircuit->destSoftPlateDesc, pCircuit->destSoftPlateRef, pCircuit->destSoftPlateCode, false, isSideSource, isLeft);

				// 开出软压板，图形位于回路起点

				AdjustVirtualCircuitPlatePosition(svg.plateRectHash, key, pVtLine->startPoint, pCircuit->srcSoftPlateDesc, pCircuit->srcSoftPlateRef, pCircuit->srcSoftPlateCode, true, isSideSource, isLeft);

                // 增加连接点索引

                ++(*pConnectPtIndex);

            }

        }

    }

	// m_painter->restore();

}



void SvgTransformer::AdjustVirtualCircuitPlatePosition(QHash<QString, PlateRect>& hash, const QString& iedName, const QPoint& linePt, const QString& plateDesc, const QString& plateRef, quint64 plateCode, bool isSrcPlate, bool isSideSrc, bool isLeft)

{

	if( plateDesc.isEmpty() || plateRef.isEmpty())

		return;

	QString key = QString("%1_%2").arg(iedName, plateRef);

	if (!hash.contains(key))

	{

		// 当前通道压板未记录，则创建一个新的压板矩形

		PlateRect plateRect;

		plateRect.code = plateCode;

		plateRect.iedName = isSrcPlate ? iedName.split("+").first() : iedName.split("+").last();

		plateRect.desc = plateDesc;

		plateRect.ref = plateRef;

		int plate_X, plate_Y = linePt.y() - ICON_LENGTH / 2;

		int startOffset = ARROW_LEN * 1.5;

		int rectWidth = PLATE_WIDTH + 2 * PLATE_GAP;

		if (isSideSrc)

		{

			plate_X = 

				isLeft ?

					isSrcPlate ?

						linePt.x() + startOffset :					// 左侧设备开出压板

						linePt.x() - rectWidth - startOffset :	// 左侧设备开入压板

					isSrcPlate ?

						linePt.x() - startOffset - rectWidth :	// 右侧设备开出压板

						linePt.x() + startOffset;					// 右侧设备开入压板

		}

		else

		{

			plate_X = 

				isLeft ?

					isSrcPlate ?

						linePt.x() - startOffset - rectWidth :	// 左侧设备开出压板

						linePt.x() + startOffset :					// 左侧设备开入压板

					isSrcPlate ?

						linePt.x() + startOffset :					// 右侧设备开出压板

						linePt.x() - startOffset - rectWidth;	// 右侧设备开入压板

		}

		plateRect.rect = QRect(QPoint(plate_X, plate_Y), QSize(rectWidth, ICON_LENGTH));

		hash.insert(key, plateRect);

	}

	else

	{

		// 更新压板底部位置

		PlateRect& plateRect = hash[key];

		plateRect.rect.setHeight(plateRect.rect.height() + ICON_LENGTH + CIRCUIT_VERTICAL_DISTANCE);

	}

}



void SvgTransformer::DrawIedRect(IedRect* rect)

{

	DrawIedRect(rect, m_painter);

}



void SvgTransformer::DrawIedRect(IedRect* rect, QPainter* _painter)

{

	PAINTER_STATE_GUARD(_painter);

	QPen pen;

	QBrush brush(rect->underground_color);



	if (rect->extend_height != 0) // 需要绘制额外的外部矩形框

	{

		brush.setColor(ColorHelper::Color(ColorHelper::ied_underground));

		pen.setColor(ColorHelper::Color(ColorHelper::ied_border));

		pen.setStyle(Qt::DashLine);

		int margin = 15;

		_painter->setPen(pen);

		_painter->setBrush(brush);

		QPoint tl(rect->x - rect->inner_gap, rect->y - rect->inner_gap);

		QPoint br(rect->x + rect->inner_gap, rect->y + rect->height + rect->extend_height + rect->inner_gap);

		QRect dashRect(rect->x - rect->inner_gap, rect->y - rect->inner_gap, 2 * rect->inner_gap + rect->width, rect->GetExtendHeight());

		_painter->fillRect(dashRect, brush);

		_painter->setBrush(Qt::NoBrush);

		_painter->drawRect(dashRect);

	}

	pen.setStyle(Qt::SolidLine);

	pen.setColor(ColorHelper::Color(rect->border_color));

	_painter->setPen(pen);

	QFont font;

	font.setFamily(rect->iedName);

	font.setPointSize(TYPE_IED);

	_painter->setFont(font);

	if (rect->underground_color == 0) // 透明

		_painter->drawRect(rect->x, rect->y, rect->width, rect->height);

	else

	{

		_painter->fillRect(rect->x, rect->y, rect->width, rect->height, brush);

	}





	DrawTextInRect(_painter, rect, rect->iedName, rect->iedDesc);

}



void SvgTransformer::DrawIedRect(QList<IedRect*>& rectList)

{

	DrawIedRect(rectList, m_painter);

}



void SvgTransformer::DrawIedRect(QList<IedRect*>& rectList, QPainter* _painter)

{

	if (rectList.isEmpty()) return;

	foreach(IedRect * rect, rectList)

	{

		DrawIedRect(rect, _painter);

	}

}



void SvgTransformer::DrawSwitcherRect(IedRect* rect)

{

	DrawSwitcherRect(rect, m_painter);

}



void SvgTransformer::DrawSwitcherRect(IedRect* rect, QPainter* _painter)

{

	PAINTER_STATE_GUARD(_painter);

	QPen pen;

	pen.setColor(rect->border_color);

	_painter->setPen(pen);

	QBrush brush(rect->underground_color);

	QFont font;

	font.setFamily(rect->iedName);

	font.setPointSize(TYPE_Switcher);

	_painter->setFont(font);

	_painter->fillRect(rect->x, rect->y, rect->width, rect->height, brush);

	DrawTextInRect(rect, rect->iedName, rect->iedDesc, 18);

}



//void SvgTransformer::DrawTextInRect(IedRect* rect)

void SvgTransformer::DrawTextInRect(SvgRect* rect, QString name, QString desc, int pointSize)

{

	DrawTextInRect(m_painter, rect, name, desc, pointSize);

}



void SvgTransformer::DrawTextInRect(QPainter* _painter, SvgRect* rect, QString name, QString desc, int pointSize)

{

	PAINTER_STATE_GUARD(_painter);

	// 设置字体大小

	QString text = desc;

	QPen pen;

	pen.setColor(Qt::white);

	QFont font = _painter->font();

	font.setPointSize(pointSize);

	_painter->setFont(font);

	_painter->setPen(pen);



	// 计算文字位置及换行

	QFontMetrics metrics(_painter->font());

	int maxWidth = rect->width - 2 * rect->padding;

	QStringList lines;

	QString currentLine;

	int currentWidth = 0;

	lines.append(name);

	lines.append("");

	foreach(const QChar & ch, text)

	{

		int charWidth = metrics.width(ch);

		if (currentWidth + charWidth > maxWidth)

		{

			lines.append(currentLine);

			currentLine.clear();

			currentWidth = 0;

		}

		currentLine.append(ch);

		currentWidth += charWidth;

	}

	if (!currentLine.isEmpty())

	{

		lines.append(currentLine);

	}



	int lineHeight = metrics.height();

	int totalTextHeight = lines.size() * lineHeight;

	int text_y = rect->y + (rect->height - totalTextHeight) / 2 + lineHeight - metrics.descent();



	foreach(const QString & line, lines)

	{

		int textWidth = metrics.width(line);

		int text_x = rect->x + (rect->width - textWidth) / 2;

		_painter->drawText(text_x, text_y, line);

		text_y += lineHeight;

	}

}



void SvgTransformer::DrawExternalRect(QList<IedRect*>& rectList, QString title, bool hasDescIedRect)

{

	// 绘制外部虚线框

	if (rectList.isEmpty()) return;

	IedRect* firstIed = rectList.first();

	IedRect* lastIed = rectList.last();

	QFont font;

	font.setPointSize(15);

	m_painter->setFont(font);

	QFontMetrics fm(font);

	quint16 titleWidth = fm.width(title);

	quint16 titleHeight = fm.height();

	quint16 x = firstIed->x - firstIed->horizontal_margin - titleWidth;

	quint16 y = firstIed->y - firstIed->vertical_margin;

	quint16 width = firstIed->width + 2 * firstIed->horizontal_margin + titleWidth;

	quint16 height = (lastIed->y - y + lastIed->height) + lastIed->vertical_margin;

	QRect externalRect(x, y, width, height);

	QPen pen;

	pen.setStyle(Qt::DashLine);

	pen.setColor(ColorHelper::Color(firstIed->border_color));

	m_painter->setPen(pen);

	m_painter->drawRect(externalRect);



	pen.setColor(Qt::white);

	m_painter->setPen(pen);

	m_painter->drawText(x + 10, y + titleHeight, title);



	// 描述矩形

	if (hasDescIedRect)

	{

		//IedRect* leftTopRect = rectList.first();



		quint16 line_width = 140;

		quint16 desc_width = 180;

		quint16 desc_height = fm.height() * 2 + 10;

		quint16 desc_x = x;

		quint16 desc_y = y - desc_height - 20;



		QPen pen;

		//pen.setColor(ColorHelper::Color(ColorHelper::line_gse));

		pen.setColor(Qt::white);

		pen.setStyle(Qt::DashLine);

		m_painter->setPen(pen);

		m_painter->setFont(font);



		m_painter->drawRect(desc_x, desc_y, desc_width, desc_height);

		m_painter->drawText(desc_x + line_width + 10, desc_y + fm.height(), "GSE");

		m_painter->drawText(desc_x + line_width + 10, desc_y + fm.height() * 2, "SV");



		quint16 line_x = desc_x + 5;

		quint16 line_y = desc_y + fm.height() / 2 + 5;

		pen.setStyle(Qt::SolidLine);

		pen.setColor(ColorHelper::Color(ColorHelper::line_gse));

		m_painter->setPen(pen);

		m_painter->drawLine(line_x, line_y, line_x + line_width, line_y);



		pen.setColor(ColorHelper::Color(ColorHelper::line_smv));

		m_painter->setPen(pen);

		m_painter->drawLine(line_x, line_y + fm.height(), line_x + line_width, line_y + fm.height());

	}



}



void SvgTransformer::DrawExternalRect(IedRect* rect, QString title, bool hasDescIedRect)

{

	DrawExternalRect(QList<IedRect*>() << rect, title, hasDescIedRect);

}



void SvgTransformer::DrawWholeRect(IedRect* rect)

{

	// 绘制矩形

	DrawIedRect(rect);

}



void SvgTransformer::DrawLogicCircuitLine(QList<IedRect*>& rectList)

{

	foreach(IedRect* rect, rectList)

	{

		foreach(LogicCircuitLine* line, rect->logic_line_list)

		{

			//m_painter->save();

			PAINTER_STATE_GUARD(m_painter);

			QPen pen;

			pen.setStyle(Qt::SolidLine);

			QColor color = line->pLogicCircuit->type == SV ? ColorHelper::Color(ColorHelper::line_smv) : ColorHelper::Color(ColorHelper::line_gse);

			pen.setColor(color);

			m_painter->setPen(pen);



			// 仅作链路区分，对图像显示无影响

			QFont font;

			font.setPointSize(TYPE_LogicCircuit);	// 标识该<g>节点为链路父节点

			//font.setWeight(line->id);	// 标识链路id

			QString signStr = joinGroups() <<

				(QString)(joinFields() << m_element_id++) <<

				// srcIedName cbName

				(QString)(joinFields() << line->pLogicCircuit->pSrcIed->name << line->pLogicCircuit->cbName) <<

				// destIedName

				(QString)(joinFields() << line->pLogicCircuit->pDestIed->name);

			font.setFamily(signStr);

			//font.setWeight(m_circuit_id++);	// 标识链路id

			m_painter->setFont(font);

			// 起点为子设备连接点，终点为主设备连接点

			m_painter->drawLine(line->startPoint, line->midPoint);

			m_painter->drawLine(line->midPoint, line->endPoint);

			double angle = GetAngleByVec(line->endPoint - line->midPoint);



			font.setPointSize(TYPE_Circuit_Arrow);

			m_painter->setFont(font);

			drawArrowHeader(GetArrowPt(line->endPoint, ARROW_LEN, 0, angle, false, 0), angle, color);

			// m_painter->restore();

		}

	}

}



void SvgTransformer::DrawVirtualCircuitLine(QList<IedRect*>& rectList)

{

	foreach(IedRect * rect, rectList)

	{

		foreach(LogicCircuitLine * pLogicLine, rect->logic_line_list)

		{

			foreach(VirtualCircuitLine * pVtLine, pLogicLine->virtual_line_list)

			{

				//m_painter->save();

				PAINTER_STATE_GUARD(m_painter);

				QPen pen;

				QColor green = ColorHelper::Color(ColorHelper::pure_green);

				pen.setColor(green);

				m_painter->setPen(pen);

				// 绘制图标，静态绘制时按连通状态绘制

				DrawVirtualIcon(pVtLine->startIconPt, pLogicLine->pLogicCircuit->type, ColorHelper::pure_green);

				DrawVirtualIcon(pVtLine->endIconPt, pLogicLine->pLogicCircuit->type, ColorHelper::pure_green);

				// 绘制值

				pVtLine->startValRect.setHeight(0);

				pVtLine->endValRect.setHeight(0);

				DrawVirtualText(pVtLine->startValRect, pVtLine->valStr, "out");

				DrawVirtualText(pVtLine->endValRect, pVtLine->valStr, "in");

				// 绘制描述信息

				DrawVirtualText(pVtLine->circuitDescRect, pVtLine->circuitDesc, "desc");



				// 记录信息到回路图像

				QFont font;

				font.setPointSize(TYPE_VirtualCircuit);

				//font.setWeight(m_circuit_id++);	// 标识链路id

				// 信息整合

				QString vtLineInfo = joinGroups() <<

					// id 0

					(QString)(joinGroups() << m_element_id++) <<

					// srcIedName:destIedName 1

					(QString)(joinFields() << pVtLine->pVirtualCircuit->srcIedName << pVtLine->pVirtualCircuit->destIedName) <<

					// srcSoftPlateDesc:destSoftPlateDesc 2

					(QString)(joinFields() << pVtLine->pVirtualCircuit->srcSoftPlateDesc << pVtLine->pVirtualCircuit->destSoftPlateDesc) <<

					// srcSoftPlateRef:destSoftPlateRef 3

					(QString)(joinFields() << pVtLine->pVirtualCircuit->srcSoftPlateRef << pVtLine->pVirtualCircuit->destSoftPlateRef) <<

					// srcRef:destRef 4

					(QString)(joinFields() << pVtLine->pVirtualCircuit->srcRef << pVtLine->pVirtualCircuit->destRef) <<

					// srcName:destName 5

					(QString)(joinFields() << pVtLine->pVirtualCircuit->srcName << pVtLine->pVirtualCircuit->destName) <<

					// remoteId:remoteSigId_A:remoteSigId_B 6

					(QString)(joinFields() << pVtLine->pVirtualCircuit->inRemoteCode << pVtLine->pVirtualCircuit->outRemoteCode) <<

					// virtualType 7

					(QString)(joinFields() << (pVtLine->pVirtualCircuit->type == GOOSE ? "gse" : "sv")) <<

					// softPlateCode 8

					(QString)(joinFields() << pVtLine->pVirtualCircuit->srcSoftPlateCode << pVtLine->pVirtualCircuit->destSoftPlateCode) <<

					// line Code 9

					(QString)(joinFields() << pVtLine->pVirtualCircuit->code) <<

					// circuitDesc 10

					(QString)(joinFields() << pVtLine->circuitDesc);

				font.setFamily(vtLineInfo);

				m_painter->setFont(font);

				m_painter->drawLine(pVtLine->startPoint, pVtLine->endPoint);

				font.setPointSize(TYPE_Circuit_Arrow);

				m_painter->setFont(font);

				drawArrowHeader(pVtLine->endPoint, GetAngleByVec(pVtLine->endPoint - pVtLine->startPoint));

				

				// m_painter->restore();

			}

		}

	}

}



void SvgTransformer::DrawOpticalLine(OpticalCircuitLine* optLine)

{



    QPen pen;

    pen.setColor(ColorHelper::Color(ColorHelper::pure_green));

    //m_painter->save();

	PAINTER_STATE_GUARD(m_painter);

    m_painter->setPen(pen);

	// 记录链路信息到SVG文件

	QFont font;

	QString connStatus = optLine->pOpticalCircuit->connStatus ? "true" : "false";	// true/false

	QString optInfo = joinGroups() <<

		(QString)(joinFields() << m_element_id++) <<

		(QString)(joinFields() << optLine->pOpticalCircuit->loopCode << optLine->pOpticalCircuit->code) <<

		(QString)(joinFields() << optLine->pOpticalCircuit->srcIedName << optLine->pOpticalCircuit->srcIedPort) <<

		(QString)(joinFields() << optLine->pOpticalCircuit->destIedName << optLine->pOpticalCircuit->destIedPort) <<

		(QString)(joinFields() << optLine->pOpticalCircuit->connStatus) <<

		(QString)(joinFields() << optLine->pOpticalCircuit->remoteId);

		//optLine->pOpticalCircuit->cableDesc << connStatus << optLine->pOpticalCircuit->code;

	font.setFamily(optInfo);

	font.setPointSize(TYPE_OpticalCircuit);

	//font.setWeight(m_circuit_id++);	// 标识链路id

	m_painter->setFont(font);

    double outAngle = -90;

    double inAngle = 90;

    // 绘制光纤链路折线

    QVector<QPoint> points;

    points << optLine->startPoint;

    foreach(const QPoint& pt, optLine->midPoints)

    {

        points << pt;

    }

    points << optLine->endPoint;

    m_painter->drawPolyline(points.data(), points.size());



    // 绘制连接点圆

	font.setPointSize(TYPE_Optical_ConnCircle);

	//font.setFamily(DEFAULT_FONT_FAMILY);

	m_painter->setFont(font);

	QPoint& upPoint = optLine->startPoint.y() < optLine->endPoint.y() ? optLine->startPoint : optLine->endPoint;

	QPoint& downPoint = optLine->startPoint.y() < optLine->endPoint.y() ? optLine->endPoint : optLine->startPoint;

    DrawConnCircle(upPoint, CONN_R);

    DrawConnCircle(downPoint, CONN_R, false);

	quint16 underRectY = optLine->pSrcRect->y > optLine->pDestRect->y ? optLine->pSrcRect->y : optLine->pDestRect->y;



    // 绘制箭头

	font.setPointSize(TYPE_Circuit_Arrow);

	m_painter->setFont(font);

    if (optLine->arrowState & Arrow_Out)

    {

        drawArrowHeader(GetArrowPt(upPoint, ARROW_LEN, CONN_R, outAngle, true), outAngle);

        drawArrowHeader(GetArrowPt(downPoint, ARROW_LEN, CONN_R, outAngle, downPoint.y() > underRectY), outAngle);

    }

    if (optLine->arrowState & Arrow_In)

    {

        drawArrowHeader(GetArrowPt(upPoint, ARROW_LEN, CONN_R, inAngle, true), inAngle);

        drawArrowHeader(GetArrowPt(downPoint, ARROW_LEN, CONN_R, inAngle, downPoint.y() > underRectY), inAngle);

    }



    // m_painter->restore();

}



void SvgTransformer::DrawPortText(OpticalCircuitLine* line, int conn_r)

{

	// 端口号绘制在矩形和连接点直接

	//PainterStateGuard __painter_guard(m_painter);

	//m_painter->save();

	PAINTER_STATE_GUARD(m_painter);

	int offset = conn_r * 2 + 10;	// 端口号与连接点的偏移量

	QPen pen;

	pen.setColor(ColorHelper::Color(ColorHelper::pure_white));

	QFont font;

	font.setPointSize(18);

	m_painter->setPen(pen);

	m_painter->setFont(font);

	QPoint& topPoint = line->startPoint.y() < line->endPoint.y() ? line->startPoint : line->endPoint;

	QPoint& bottomPoint = line->startPoint.y() < line->endPoint.y() ? line->endPoint : line->startPoint;

	IedRect* topRect = line->pSrcRect->y < line->pDestRect->y ? line->pSrcRect : line->pDestRect;

	IedRect* bottomRect = line->pSrcRect->y < line->pDestRect->y ? line->pDestRect : line->pSrcRect;

	int topPtY = topRect->iedName.contains("SW")? topPoint.y() - offset : topPoint.y() + offset;

	// 下侧设备为交换机时，端口号绘制在矩形内

	int bottomPtY = bottomRect->iedName.contains("SW") ? bottomPoint.y() + offset : bottomPoint.y() - offset;

	QString topPort = line->pSrcRect->y > line->pDestRect->y ? 

		line->pOpticalCircuit->destIedPort :		// 源设备位于下方时，上方端口号为目的设备端口号

		line->pOpticalCircuit->srcIedPort;			// 源设备位于上方时，上方端口号为源设备端口号

	QString bottomPort = line->pSrcRect->y > line->pDestRect->y ? 

		line->pOpticalCircuit->srcIedPort : 

		line->pOpticalCircuit->destIedPort;

	QFontMetrics fm(m_painter->font());

	int upperPortStrWidth = fm.width(topPort);

	int bottomPortStrWidth = fm.width(bottomPort);



	QPoint topPort_lt_pt(topPoint.x() - upperPortStrWidth / 2, topPtY - 5);

	QPoint topPort_rb_pt(topPoint.x() + upperPortStrWidth / 2, topPtY + fm.height());

	QPoint bottomPort_lt_pt(bottomPoint.x() - bottomPortStrWidth / 2, bottomPtY - 5);

	QPoint bottomPort_rb_pt(bottomPoint.x() + bottomPortStrWidth / 2, bottomPtY + fm.height());

	// 绘制端口号

	m_painter->drawText(QRect(topPort_lt_pt, topPort_rb_pt), topPort);

	m_painter->drawText(QRect(bottomPort_lt_pt, bottomPort_rb_pt), bottomPort);



	// m_painter->restore();

}



void SvgTransformer::DrawVirtualIcon(const QPoint& pt, VirtualType _type, const quint32 color)

{

	if(_type == GOOSE)

	{

		DrawGseIcon(pt, color);

	}

	else if(_type == SV)

	{

		DrawSvIcon(pt, color);

	}

	else

	{

		qWarning() << "SvgTransformer::DrawVirtualIcon: Unsupported virtual type";

	}

}



void SvgTransformer::DrawVirtualText(const QRect& rect, QString val, QString typeStr)

{

	//m_painter->save();

	PAINTER_STATE_GUARD(m_painter);



	if (rect.height() == 0) {

		// 使用 font-family 携带占位信息：id 以及矩形位置尺寸

		// group0: id，仅用流水 m_element_id

		// group1: x y w h（这里 h==0 只是标记用途，仍按数值写入）

		QString payload = joinGroups()

			<< (QString)(joinFields() << m_element_id)

			<< (QString)(joinFields() << rect.x() << rect.y() << rect.width() << rect.height())

			<< (QString)(joinFields() << typeStr);



		QFont font;

		font.setFamily(payload);

		font.setPointSize(TYPE_Virtual_Value); // 用特殊字号标注节点类型

		m_painter->setFont(font);



		// 画一个 1px 的透明矩形作为占位（既能落到 SVG，又不会可见）

		QPen pen(Qt::NoPen);

		QBrush brush(Qt::NoBrush);

		m_painter->setPen(pen);

		m_painter->setBrush(brush);

		m_painter->drawRect(rect);

		// m_painter->restore();

		return;

	}



	// 其他文本（如描述）仍保持原有绘制

	QPen pen;

	pen.setColor(Qt::white);

	QFont font;

	font.setPointSize(18);

	m_painter->setPen(pen);

	m_painter->setFont(font);

	m_painter->drawText(rect, val);

	// m_painter->restore();

}



void SvgTransformer::DrawPlate(const QHash<QString, PlateRect>& hash)

{

	foreach(const PlateRect& plateRect, hash)

	{

		//m_painter->save();

		PAINTER_STATE_GUARD(m_painter);

		// 绘制压板矩形

		QPen pen;

		QFont font;

		pen.setColor(ColorHelper::Color(ColorHelper::pure_white));

		//pen.setColor(Qt::transparent);

		QBrush brush(ColorHelper::Color(ColorHelper::ied_underground));

		pen.setWidth(2);

		pen.setStyle(Qt::DashDotLine);

		QVector<qreal> dashPattern;

		dashPattern << 3 << 3;

		pen.setDashPattern(dashPattern);

		m_painter->setPen(pen);

		m_painter->setBrush(brush);

		QString plateInfo = joinGroups() <<

			(QString)(joinFields() << m_element_id++) <<

			(QString)(joinFields() << plateRect.iedName << plateRect.desc << plateRect.ref << plateRect.code);

		font.setPointSize(TYPE_Plate_RECT);

		font.setFamily(plateInfo);

		m_painter->setFont(font);

		m_painter->drawRect(plateRect.rect);

		// 绘制压板图标

		QPoint centerPt(plateRect.rect.x() + plateRect.rect.width() / 2 - PLATE_WIDTH / 2 + PLATE_CIRCLE_RADIUS + PLATE_GAP, plateRect.rect.y() + plateRect.rect.height() / 2);

		DrawPlateIcon(centerPt, true, plateInfo);	// 根据压板描述判断闭合状态

		// 绘制hitbox

		m_painter->setPen(Qt::NoPen);

		m_painter->setBrush(Qt::NoBrush);

		font.setPointSize(TYPE_Plate_HITBOX);

		m_painter->setFont(font);

		m_painter->drawRect(QRect(QPoint(centerPt.x() - PLATE_CIRCLE_RADIUS, centerPt.y() - PLATE_CIRCLE_RADIUS), QSize(PLATE_WIDTH, PLATE_HEIGHT)));

		// m_painter->restore();

	}

}



void SvgTransformer::DrawGseIcon(const QPoint& pt, const quint32 color)

{

	PAINTER_STATE_GUARD(m_painter);

	// 绘制边框

	//QPen pen;

	//pen.setColor(ColorHelper::Color(color));

	//pen.setWidth(3);

	//m_painter->setPen(pen);

	//m_painter->drawRect(pt.x(), pt.y(), ICON_LENGTH, ICON_LENGTH);

	//// 绘制内容

	//qreal contentMargin = 3;

	//qreal contentVerticalMargin = 7;

	//qreal lineWidth = (ICON_LENGTH - contentMargin * 2) / 3;

	//int firstLineWidth_bottom = lineWidth * 0.9;

	//int secondLineWidth_bottom = lineWidth * 1.25;

	//int thirdLineWidth_bottom = lineWidth * 0.9;

	//int contentTopY = pt.y() + contentVerticalMargin;

	//int contentBottomY = pt.y() + ICON_LENGTH - contentVerticalMargin;

	//QPointF points[6] = {

	//	QPointF(pt.x() + contentMargin, contentBottomY),

	//	QPointF(pt.x() + contentMargin + firstLineWidth_bottom, contentBottomY),

	//	QPointF(pt.x() + contentMargin + firstLineWidth_bottom, contentTopY),

	//	QPointF(pt.x() + contentMargin + firstLineWidth_bottom + secondLineWidth_bottom, contentTopY),

	//	QPointF(pt.x() + contentMargin + firstLineWidth_bottom + secondLineWidth_bottom, contentBottomY),

	//	QPointF(pt.x() + contentMargin + firstLineWidth_bottom + secondLineWidth_bottom + thirdLineWidth_bottom + 0.5, contentBottomY)

	//};

	//pen.setWidth(2);

	//m_painter->setPen(pen);

	//m_painter->drawPolyline(points, 6);

	utils::drawGseIcon(m_painter, pt, ICON_LENGTH, ColorHelper::Color(color));

}



void SvgTransformer::DrawSvIcon(const QPoint& pt, const quint32 color)

{

	PAINTER_STATE_GUARD(m_painter);

	// 绘制边框

	//QPen pen;

	//pen.setColor(ColorHelper::Color(color));

	//pen.setWidth(3);

	//m_painter->setPen(pen);

	//m_painter->drawRect(pt.x(), pt.y(), ICON_LENGTH, ICON_LENGTH);

	//// 绘制内容

	//qreal contentMargin = 2;

	//qreal contentWidth = ICON_LENGTH - contentMargin * 2;

	//qreal contentY = (pt.y() + static_cast<qreal>(ICON_LENGTH)) / 2;

	//qreal contentTopY = pt.y() + contentMargin;

	//qreal contentBottomY = pt.y() + ICON_LENGTH - contentMargin;

	//QPointF startPoint(pt.x() + contentMargin, pt.y() + ICON_LENGTH / 2);

	//QPointF endPoint(pt.x() + ICON_LENGTH - contentMargin, pt.y() + ICON_LENGTH / 2);

	//QPointF controlPt1(pt.x() + contentWidth / 2 - 1, startPoint.y() - 5);

	//QPointF controlPt2(pt.x() + contentWidth / 4 * 3 + 1, startPoint.y() + 5);

	//QPainterPath path;

	//path.moveTo(startPoint);

	//path.cubicTo(controlPt1, controlPt1, QPointF((startPoint.x() + endPoint.x()) / 2, startPoint.y()));

	//path.cubicTo(controlPt2, controlPt2, endPoint);

	//pen.setWidth(2);

	//m_painter->setPen(pen);

	//m_painter->drawPath(path);

	utils::drawSvIcon(m_painter, pt, ICON_LENGTH, ColorHelper::Color(color));

}



void SvgTransformer::DrawPlateIcon(const QPoint& centerPt, bool status, const QString& info)

{

	PAINTER_STATE_GUARD(m_painter);

    quint32 color = status ? ColorHelper::pure_green : ColorHelper::pure_red; // 根据闭合状态使用默认颜色

    QPen pen;

    QFont font;

    pen.setWidth(2);

    pen.setColor(ColorHelper::Color(color));

    font.setPointSize(TYPE_Plate_ICON);

    font.setFamily(info);



    m_painter->setFont(font);

    m_painter->setPen(pen);

	int distance = PLATE_WIDTH - PLATE_CIRCLE_RADIUS * 4;	// 两个圆之间的距离 = 压板图形总宽度 - 两个圆的直径

	// 直径20的圆

	m_painter->drawEllipse(centerPt, PLATE_CIRCLE_RADIUS, PLATE_CIRCLE_RADIUS);

	m_painter->drawEllipse(centerPt + QPoint(distance, 0), PLATE_CIRCLE_RADIUS, PLATE_CIRCLE_RADIUS);

	// 矩形连接左侧圆

	//int rectWidth = 50;

	int rectHeight = PLATE_CIRCLE_RADIUS * 2;

	//m_painter->translate(centerPt);

	QPoint ptList[4] = {

		QPoint(0, -rectHeight / 2),

		QPoint(distance, -rectHeight / 2),

		QPoint(distance, rectHeight / 2),

		QPoint(0, rectHeight / 2),

	};

	// 闭合状态矩形

	pen.setColor(Qt::transparent);

	m_painter->setPen(pen);

	m_painter->drawLine(centerPt + ptList[0], centerPt + ptList[1]);

	m_painter->drawLine(centerPt + ptList[2], centerPt + ptList[3]);

}



void SvgTransformer::ReSignIedRect(pugi::xml_document& doc)

{

	pugi::xpath_node_set iedRectNodeSet = doc.select_nodes(QString("//g[@font-size='%1']").arg(TYPE_IED).toLocal8Bit());

	for (pugi::xpath_node_set::const_iterator it = iedRectNodeSet.begin(); it != iedRectNodeSet.end(); ++it)

	{

		pugi::xml_node iedRectNode = it->node();

		QString iedName = iedRectNode.attribute("font-family").value();

		iedRectNode.append_attribute("iedname");

		iedRectNode.append_attribute("type");

		iedRectNode.attribute("iedname").set_value(iedName.toUtf8().constData());

		iedRectNode.attribute("type").set_value("ied");



		iedRectNode.attribute("font-family").set_value("SimSun");

		iedRectNode.attribute("font-size").set_value("15");

	}

}



void SvgTransformer::ReSignSvg(const QString& filename, BaseSvg& svg)

{

	pugi::xml_document doc;

	pugi::xml_parse_result res = doc.load_file(filename.toLocal8Bit());

	if (!res)

	{

		qDebug() << __FILE__ << __LINE__ << "load file failed";

		return;

	}



	ReSignSvgDoc(doc, svg);



	if (!doc.save_file(filename.toLocal8Bit()))

	{

		qDebug() << "save file failed";

	}

}



void SvgTransformer::ReSignSvgDoc(pugi::xml_document& doc, BaseSvg& svg)

{

	ReSignIedRect(doc);

	ReSignCircuitLine(doc);

	ReSignPlate(doc);

	ReSignVirtualValuePlaceholders(doc);

	ReSignSvgViewBox(doc, svg.viewBoxX, svg.viewBoxY, svg.viewBoxWidth, svg.viewBoxHeight);

}



void SvgTransformer::ReSignCircuitLine(pugi::xml_document& doc)

{

	// 逻辑回路

	pugi::xpath_node_set circuitLineNodeSet = doc.select_nodes(QString("//g[@font-size='%1']").arg(TYPE_LogicCircuit).toLocal8Bit());

	for (nodeSetConstIterator it = circuitLineNodeSet.begin(); it != circuitLineNodeSet.end(); ++it)

	{

		pugi::xml_node lineNode = it->node();

		QString circuitDesc = lineNode.attribute("font-family").value();

		QList<QStringList> circuitDescGrp = splitGroupAndFields(circuitDesc);

		// (srcIedName, cbName) & (destIedName)

		QString id = getField(circuitDescGrp, 0, 0);

		QString srcIedName = getField(circuitDescGrp, 1, 0);

		QString cbName = getField(circuitDescGrp, 1, 1);

		QString destIedName = getField(circuitDescGrp, 2, 0);

		QString circuitId = lineNode.attribute("font-weight").value();



		lineNode.append_attribute("id");

		lineNode.append_attribute("src-iedname");

		lineNode.append_attribute("src-cbname");

		lineNode.append_attribute("dest-iedname");

		lineNode.append_attribute("type");

		lineNode.append_attribute("circuit-code");



		lineNode.attribute("id").set_value(id.toUtf8().constData());

		lineNode.attribute("src-iedname").set_value(srcIedName.toUtf8().constData());

		lineNode.attribute("src-cbname").set_value(cbName.toUtf8().constData());

		lineNode.attribute("dest-iedname").set_value(destIedName.toUtf8().constData());

		lineNode.attribute("type").set_value("logic");

		lineNode.attribute("circuit-code").set_value(circuitId.toUtf8().constData());



		// 重置不相关属性为默认值

		lineNode.attribute("font-family").set_value("SimSun");

		lineNode.attribute("font-size").set_value("15");

		lineNode.attribute("font-weight").set_value("400");

	}





	// 光纤链路

	pugi::xpath_node_set opticalLineNodeSet = doc.select_nodes(QString("//g[@font-size='%1']").arg(TYPE_OpticalCircuit).toLocal8Bit());

	for (nodeSetConstIterator it = opticalLineNodeSet.begin(); it != opticalLineNodeSet.end(); ++it)

	{

		pugi::xml_node lineNode = it->node();

		QString attrStr = lineNode.attribute("font-family").value();

		QList<QStringList> strList = splitGroupAndFields(attrStr);

		QString id = getField(strList, 0, 0);	// id

		QString loopCode = getField(strList, 1, 0);	// loopCode

		QString code = getField(strList, 1, 1);	// code

		QString srcIedName = getField(strList, 2, 0);	// srcIedName

		QString srcPort = getField(strList, 2, 1);	// srcPort

		QString destIedName = getField(strList, 3, 0);	// destIedName

		QString destPort = getField(strList, 3, 1);	// destPort

		QString connStatus = getField(strList, 4, 0);	// connStatus

		QString remoteId = getField(strList, 5, 0);	// remoteId

		lineNode.append_attribute("src-ied").set_value(srcIedName.toUtf8().constData());

		lineNode.append_attribute("src-port").set_value(srcPort.toUtf8().constData());

		lineNode.append_attribute("dest-ied").set_value(destIedName.toUtf8().constData());

		lineNode.append_attribute("dest-port").set_value(destPort.toUtf8().constData());

		lineNode.append_attribute("loopCode").set_value(loopCode.toUtf8().constData());

		lineNode.append_attribute("status").set_value(connStatus.toUtf8().constData());

		lineNode.append_attribute("remote-id").set_value(remoteId.toUtf8().constData());

		lineNode.append_attribute("code").set_value(code.toUtf8().constData());

		lineNode.append_attribute("type").set_value("optical");

		lineNode.append_attribute("id").set_value(id.toUtf8().constData());



		// 重置不相关属性为默认值

		lineNode.attribute("font-family").set_value("SimSun");

		lineNode.attribute("font-size").set_value("15");

		lineNode.attribute("font-weight").set_value("400");

	}

	pugi::xpath_node_set opticalConnNodeSet = doc.select_nodes(QString("//g[@font-size='%1']").arg(TYPE_Optical_ConnCircle).toLocal8Bit());

	for (nodeSetConstIterator it = opticalConnNodeSet.begin(); it != opticalConnNodeSet.end(); ++it)

	{

		pugi::xml_node connNode = it->node();

		// 连接点只记录id，用于配对

		connNode.append_attribute("id");

		QString id = connNode.attribute("font-weight").value();	// id

		connNode.attribute("id").set_value(id.toUtf8().constData());

		// 重置不相关属性为默认值

		connNode.attribute("font-family").set_value("SimSun");

		connNode.attribute("font-size").set_value("15");

		connNode.attribute("font-weight").set_value("400");

	}

	// 虚回路

	pugi::xpath_node_set virtualLineNodeSet = doc.select_nodes(QString("//g[@font-size='%1']").arg(TYPE_VirtualCircuit).toLocal8Bit());

	for (nodeSetConstIterator it = virtualLineNodeSet.begin(); it != virtualLineNodeSet.end(); ++it)

	{

		pugi::xml_node lineNode = it->node();

		QString attrStr = QString::fromUtf8(lineNode.attribute("font-family").value());

		QList<QStringList> strList = splitGroupAndFields(attrStr);

		QString id = getField(strList, 0, 0);	// id

		QString srcIedName = getField(strList, 1, 0);

		QString destIedName = getField(strList, 1, 1);

		QString srcSoftPlateDesc = getField(strList, 2, 0);

		QString destSoftPlateDesc = getField(strList, 2, 1);

		QString srcSoftPlateRef = getField(strList, 3, 0);

		QString destSoftPlateRef = getField(strList, 3, 1);

		QString srcRef = getField(strList, 4, 0);	// srcRef

		QString destRef = getField(strList, 4, 1);	// destRef

		QString srcName = getField(strList, 5, 0);	// srcName

		QString destName = getField(strList, 5, 1);	// destName

		QString remoteSigId_A = getField(strList, 6, 0);	// remoteSigId_A

		QString remoteSigId_B = getField(strList, 6, 1);	// remoteSigId_B

		QString vType = getField(strList, 7, 0);	// vType (gse/sv)

		QString srcSoftPlateCode = getField(strList, 8, 0);

		QString destSoftPlateCode = getField(strList, 8, 1);

		QString code = getField(strList, 9, 0);	// code

		QString circuitDesc = getField(strList, 10, 0);	// circuitDesc

		lineNode.append_attribute("id").set_value(id.toUtf8().constData());

		lineNode.append_attribute("code").set_value(code.toUtf8().constData());

		lineNode.append_attribute("srcIedName").set_value(srcIedName.toUtf8().constData());

		lineNode.append_attribute("destIedName").set_value(destIedName.toUtf8().constData());

		lineNode.append_attribute("srcSoftPlateCode").set_value(srcSoftPlateCode.toUtf8().constData());

		lineNode.append_attribute("destSoftPlateCode").set_value(destSoftPlateCode.toUtf8().constData());

		lineNode.append_attribute("srcSoftPlateDesc").set_value(srcSoftPlateDesc.toUtf8().constData());

		lineNode.append_attribute("destSoftPlateDesc").set_value(destSoftPlateDesc.toUtf8().constData());

		lineNode.append_attribute("srcSoftPlateRef").set_value(srcSoftPlateRef.toUtf8().constData());

		lineNode.append_attribute("destSoftPlateRef").set_value(destSoftPlateRef.toUtf8().constData());

		lineNode.append_attribute("circuitDesc").set_value(circuitDesc.toUtf8().constData());

		lineNode.append_attribute("remoteSigId_A").set_value(remoteSigId_A.toUtf8().constData());

		lineNode.append_attribute("remoteSigId_B").set_value(remoteSigId_B.toUtf8().constData());

		lineNode.append_attribute("type").set_value("virtual");

		lineNode.append_attribute("virtual-type").set_value(vType.toUtf8().constData());

		// 重置不相关属性

		lineNode.attribute("font-family").set_value("SimSun");

		lineNode.attribute("font-size").set_value("15");

		lineNode.attribute("font-weight").set_value("400");

	}

	// 回路箭头，绑定id

	pugi::xpath_node_set arrowNodeSet = doc.select_nodes(QString("//g[@font-size='%1']").arg(TYPE_Circuit_Arrow).toLocal8Bit());

	for (nodeSetConstIterator it = arrowNodeSet.begin(); it != arrowNodeSet.end(); ++it)

	{

		pugi::xml_node arrowNode = it->node();

		QString arrowDesc = arrowNode.attribute("font-family").value();

		QString id = getField(splitGroupAndFields(arrowDesc), 0, 0);

		arrowNode.append_attribute("id");

		arrowNode.append_attribute("type");

		arrowNode.attribute("id").set_value(id.toUtf8().constData());

		arrowNode.attribute("type").set_value("arrow");

		

		// 重置不相关属性

		arrowNode.attribute("font-family").set_value("SimSun");

		arrowNode.attribute("font-size").set_value("15");

		arrowNode.attribute("font-weight").set_value("400");

	}

}



void SvgTransformer::ReSignPlate(pugi::xml_document& doc)

{

	pugi::xpath_node_set plateNodeSet = doc.select_nodes(QString("//g[@font-size='%1']").arg(TYPE_Plate_HITBOX).toLocal8Bit());

	for (nodeSetConstIterator it = plateNodeSet.begin(); it != plateNodeSet.end(); ++it)

	{

		pugi::xml_node plateNode = it->node();

		plateNode.append_attribute("ied-name");

		plateNode.append_attribute("plate-desc");

		plateNode.append_attribute("plate-ref");

		plateNode.append_attribute("plate-code");

		plateNode.append_attribute("type");

		plateNode.append_attribute("id");

		QString plateDesc = QString::fromUtf8(plateNode.attribute("font-family").value());

		QList<QStringList> plateDescGrp = splitGroupAndFields(plateDesc);

		QString id = getField(plateDescGrp, 0, 0);	// id

		QString iedName = getField(plateDescGrp, 1, 0);	// ied-name

		QString desc = getField(plateDescGrp, 1, 1);	// plate-desc

		QString ref = getField(plateDescGrp, 1, 2);	// plate-ref

		QString code = getField(plateDescGrp, 1, 3);	// plate-code

		plateNode.attribute("id").set_value(id.toUtf8().constData());

		plateNode.attribute("ied-name").set_value(iedName.toUtf8().constData());

		plateNode.attribute("plate-desc").set_value(desc.toUtf8().constData());

		plateNode.attribute("plate-ref").set_value(ref.toUtf8().constData());

		plateNode.attribute("plate-code").set_value(code.toUtf8().constData());

		plateNode.attribute("type").set_value("plate");

		// 重置不相关属性

		plateNode.attribute("font-family").set_value("SimSun");

		plateNode.attribute("font-size").set_value("15");

	}

    pugi::xpath_node_set plateRectNodeSet = doc.select_nodes(QString("//g[@font-size='%1' or @font-size='%2']").arg(TYPE_Plate_RECT).arg(TYPE_Plate_ICON).toLocal8Bit());

    for (nodeSetConstIterator it = plateRectNodeSet.begin(); it != plateRectNodeSet.end(); ++it)

    {

            pugi::xml_node plateRectNode = it->node();

			plateRectNode.append_attribute("type");

			plateRectNode.append_attribute("id");

			QString plateDesc = plateRectNode.attribute("font-family").value();

			QList<QStringList> plateDescGrp = splitGroupAndFields(plateDesc);

			QString id = getField(plateDescGrp, 0, 0);	// id

            plateRectNode.attribute("type").set_value("plate-component");

			plateRectNode.attribute("id").set_value(id.toUtf8().constData());



            plateRectNode.attribute("font-family").set_value("SimSun");

            plateRectNode.attribute("font-size").set_value("15");

	}

}



void SvgTransformer::ReSignVirtualValuePlaceholders(pugi::xml_document& doc)

{

	// 查找所有以 TYPE_Virtual_Value 标注的占位元素，将其转换为具名属性

	pugi::xpath_node_set nodeSet = doc.select_nodes(QString("//g[@font-size='%1']").arg(TYPE_Virtual_Value).toLocal8Bit());

	for (nodeSetConstIterator it = nodeSet.begin(); it != nodeSet.end(); ++it)

	{

		pugi::xml_node node = it->node();

		QString packed = node.attribute("font-family").value();

		QList<QStringList> groups = splitGroupAndFields(packed);

		QString id = getField(groups, 0, 0);

		QString x = getField(groups, 1, 0);

		QString y = getField(groups, 1, 1);

		QString w = getField(groups, 1, 2);

		QString h = getField(groups, 1, 3);

		QString textType = getField(groups, 2, 0);



		node.append_attribute("type").set_value("virtual-value");

		node.append_attribute("id").set_value(id.toUtf8().constData());

		node.append_attribute("x").set_value(x.toUtf8().constData());

		node.append_attribute("y").set_value(y.toUtf8().constData());

		node.append_attribute("w").set_value(w.toUtf8().constData());

		node.append_attribute("h").set_value(h.toUtf8().constData());

		node.append_attribute("textType").set_value(textType.toUtf8().constData());



		// 重置不相关属性

		node.attribute("font-family").set_value("SimSun");

		node.attribute("font-size").set_value("15");

		node.attribute("font-weight").set_value("400");

	}

}



void SvgTransformer::ReSignSvgViewBox(pugi::xml_document& doc, int x, int y, int width, int height)

{

	pugi::xml_node svgNode = doc.child("svg");

	svgNode.attribute("viewBox").set_value(QString("%1 %2 %3 %4").arg(x).arg(y).arg(width).arg(height).toUtf8().constData());

}

