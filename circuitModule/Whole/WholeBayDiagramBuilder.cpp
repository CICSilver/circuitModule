#include "Whole/WholeBayDiagramBuilder.h"
#include "circuitconfig.h"
#include "directitems.h"
#include "SvgUtils.h"
#include <QFont>
#include <QFontMetrics>
#include <QMap>
#include <QSet>
#include <QStringList>
#include <QtAlgorithms>

enum WholeBayTreeLayoutConfig
{
	WHOLE_BAY_TREE_LEFT_MARGIN = 80,
	WHOLE_BAY_TREE_TOP_MARGIN = 40,
	WHOLE_BAY_TREE_NODE_WIDTH = RECT_DEFAULT_WIDTH * 3 / 2,
	WHOLE_BAY_TREE_NODE_GAP = 24,
	WHOLE_BAY_TREE_BLOCK_GAP = 80,
	WHOLE_BAY_TREE_MAINT_PLATE_GAP = 30,
	WHOLE_BAY_TREE_VIEW_MARGIN = 60,
	WHOLE_VALUE_ICON_SAFE_GAP = 4
};

namespace
{
	static QString read_local_text(const char* rawText, const QString& defaultText = QString())
	{
		if (!rawText || rawText[0] == '\0')
		{
			return defaultText;
		}
		return QString::fromLocal8Bit(rawText);
	}

	static QStringList collect_bay_ied_name_list(CircuitConfig* pCircuitConfig, const QString& bayName)
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
				if (!pBay || !pBay->m_pBayInfo)
				{
					continue;
				}
				QString currentBayName = read_local_text(pBay->m_pBayInfo->bayName);
				if (currentBayName.isEmpty())
				{
					currentBayName = read_local_text(pBay->m_pBayInfo->bayDesc);
				}
				if (currentBayName != bayName)
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
					QString iedName = read_local_text(pModelIed->m_pIedHead->iedName);
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

	static QString bay_tree_pair_key(const QString& firstName, const QString& secondName)
	{
		if (firstName.compare(secondName, Qt::CaseInsensitive) <= 0)
		{
			return firstName + QString::fromLatin1("|") + secondName;
		}
		return secondName + QString::fromLatin1("|") + firstName;
	}

	static void bay_tree_collect_logic_maps(CircuitConfig* pCircuitConfig,
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
				pairLogicHash[bay_tree_pair_key(srcIedName, destIedName)].append(pLogicCircuit);
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

	static void bay_tree_collect_component(const QString& startIedName,
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

	static QString bay_tree_select_center(const QStringList& componentList,
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

	static int bay_tree_node_x(int depth, int side, int columnGap)
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

	static void attach_bay_tree_logic_lines(WholeBayTreeNode* pNode)
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
			LogicCircuitLine* pLine = new LogicCircuitLine();
			pLine->pLogicCircuit = pLogicCircuit;
			pLine->pSrcIedRect = pSrcRect;
			pLine->pDestIedRect = pDestRect;
			pLine->srcArrowState = Arrow_None;
			pLine->destArrowState = Arrow_In;
			pLine->arrowState = Arrow_In;
			pNode->pRect->logic_line_list.append(pLine);
		}
	}

	static int bay_tree_prepare_subtree_height(WholeBayTreeNode* pNode)
	{
		if (!pNode || !pNode->pRect)
		{
			return 0;
		}
		int childHeight = 0;
		for (int i = 0; i < pNode->childNodeList.size(); ++i)
		{
			WholeBayTreeNode* pChildNode = pNode->childNodeList.at(i);
			bay_tree_prepare_subtree_height(pChildNode);
			childHeight += pChildNode->pRect->height + pChildNode->pRect->extend_height;
			if (i > 0)
			{
				childHeight += WHOLE_BAY_TREE_NODE_GAP;
			}
		}
		int nodeHeight = pNode->pRect->height + pNode->pRect->extend_height;
		if (childHeight > nodeHeight)
		{
			pNode->pRect->extend_height += (quint16)(childHeight - nodeHeight);
			nodeHeight = childHeight;
		}
		return nodeHeight;
	}

	static void bay_tree_place_subtree(WholeBayTreeNode* pNode, int topY)
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
			bay_tree_place_subtree(pChildNode, currentChildY);
			currentChildY += pChildNode->pRect->height + pChildNode->pRect->extend_height + WHOLE_BAY_TREE_NODE_GAP;
		}
	}

	static void adjust_bay_plate_position(QHash<QString, PlateRect>& hash, const QString& ownerKey, const QPoint& linePt,
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

	static void build_bay_tree_virtual_lines(WholeCircuitSvg& svg, IedRect* pOwnerRect)
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
				adjust_bay_plate_position(svg.plateRectHash, key, pVtLine->startPoint, pCircuit->srcSoftPlateDesc, pCircuit->srcSoftPlateRef, pCircuit->srcSoftPlateCode, pSrcRect->iedName, startPtX < endPtX);
				adjust_bay_plate_position(svg.plateRectHash, key, pVtLine->endPoint, pCircuit->destSoftPlateDesc, pCircuit->destSoftPlateRef, pCircuit->destSoftPlateCode, pDestRect->iedName, endPtX < startPtX);
				++(*pConnectIndex);
			}
		}
	}
}

WholeBayDiagramBuilder::WholeBayDiagramBuilder(CircuitConfig* pCircuitConfig)
	: m_pCircuitConfig(pCircuitConfig)
{
}

void WholeBayDiagramBuilder::Generate(const QString& bayName, WholeCircuitSvg& svg, int columnGap)
{
	QStringList bayIedNameList = collect_bay_ied_name_list(m_pCircuitConfig, bayName);
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
	bay_tree_collect_logic_maps(m_pCircuitConfig, bayIedNameList, pairLogicHash, bayAdjHash, allAdjHash);
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
		bay_tree_collect_component(startIedName, bayAdjHash, visitedSet, componentList);
		if (componentList.isEmpty())
		{
			componentList.append(startIedName);
			visitedSet.insert(startIedName);
		}
		QString centerIedName = bay_tree_select_center(componentList, allAdjHash, bayOrderHash);
		if (centerIedName.isEmpty())
		{
			centerIedName = startIedName;
		}
		QList<WholeBayTreeNode*> allNodeList;
		QList<WholeBayTreeNode*> leftRootNodeList;
		QList<WholeBayTreeNode*> rightRootNodeList;
		WholeBayTreeNode* pCenterNode = new WholeBayTreeNode();
		pCenterNode->iedName = centerIedName;
		pCenterNode->inBay = true;
		allNodeList.append(pCenterNode);
		QSet<QString> firstHopSet = allAdjHash.value(centerIedName);
		QStringList bayFirstHopNameList;
		QStringList externalFirstHopNameList;
		QSet<QString>::const_iterator firstHopIt = firstHopSet.constBegin();
		for (; firstHopIt != firstHopSet.constEnd(); ++firstHopIt)
		{
			if (bayIedNameSet.contains(*firstHopIt))
			{
				bayFirstHopNameList.append(*firstHopIt);
			}
			else
			{
				externalFirstHopNameList.append(*firstHopIt);
			}
		}
		bayFirstHopNameList.sort();
		externalFirstHopNameList.sort();
		QStringList firstHopNameList = bayFirstHopNameList + externalFirstHopNameList;
		for (int firstIndex = 0; firstIndex < firstHopNameList.size(); ++firstIndex)
		{
			QString firstHopName = firstHopNameList.at(firstIndex);
			int side = (firstIndex % 2 == 0) ? -1 : 1;
			WholeBayTreeNode* pFirstNode = new WholeBayTreeNode();
			pFirstNode->iedName = firstHopName;
			pFirstNode->inBay = bayIedNameSet.contains(firstHopName);
			pFirstNode->side = side;
			pFirstNode->depth = 1;
			pFirstNode->pParent = pCenterNode;
			pFirstNode->parentLogicList = pairLogicHash.value(bay_tree_pair_key(centerIedName, firstHopName));
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
				WholeBayTreeNode* pSecondNode = new WholeBayTreeNode();
				pSecondNode->iedName = secondHopName;
				pSecondNode->inBay = true;
				pSecondNode->side = side;
				pSecondNode->depth = 2;
				pSecondNode->pParent = pFirstNode;
				pSecondNode->parentLogicList = pairLogicHash.value(bay_tree_pair_key(firstHopName, secondHopName));
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
			IED* pIed = m_pCircuitConfig ? m_pCircuitConfig->GetIedByName(pNode->iedName) : NULL;
			QString iedDesc = pIed ? pIed->desc : QString();
			pNode->pRect = utils::GetIedRect(pNode->iedName, iedDesc,
				(quint16)bay_tree_node_x(pNode->depth, pNode->side, columnGap), (quint16)currentTopY,
				(quint16)WHOLE_BAY_TREE_NODE_WIDTH, (quint16)RECT_DEFAULT_HEIGHT,
				borderColor, utils::ColorHelper::pure_black);
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
			attach_bay_tree_logic_lines(pNode);
		}
		for (int nodeIndex = 0; nodeIndex < allNodeList.size(); ++nodeIndex)
		{
			WholeBayTreeNode* pNode = allNodeList.at(nodeIndex);
			if (!pNode || !pNode->pRect)
			{
				continue;
			}
			int circuitSize = 0;
			for (int logicIndex = 0; logicIndex < pNode->pRect->logic_line_list.size(); ++logicIndex)
			{
				LogicCircuitLine* pLine = pNode->pRect->logic_line_list.at(logicIndex);
				if (!pLine || !pLine->pLogicCircuit)
				{
					continue;
				}
				circuitSize += pLine->pLogicCircuit->circuitList.size();
			}
			pNode->pRect->extend_height = circuitSize * (CIRCUIT_VERTICAL_DISTANCE + ICON_LENGTH) + pNode->pRect->inner_gap + PLATE_HEIGHT;
			pNode->pRect->extend_height += WHOLE_BAY_TREE_MAINT_PLATE_GAP;
		}
		int leftHeight = 0;
		for (int rootIndex = 0; rootIndex < leftRootNodeList.size(); ++rootIndex)
		{
			bay_tree_prepare_subtree_height(leftRootNodeList.at(rootIndex));
			leftHeight += leftRootNodeList.at(rootIndex)->pRect->height + leftRootNodeList.at(rootIndex)->pRect->extend_height;
			if (rootIndex > 0)
			{
				leftHeight += WHOLE_BAY_TREE_NODE_GAP;
			}
		}
		int rightHeight = 0;
		for (int rootIndex = 0; rootIndex < rightRootNodeList.size(); ++rootIndex)
		{
			bay_tree_prepare_subtree_height(rightRootNodeList.at(rootIndex));
			rightHeight += rightRootNodeList.at(rootIndex)->pRect->height + rightRootNodeList.at(rootIndex)->pRect->extend_height;
			if (rootIndex > 0)
			{
				rightHeight += WHOLE_BAY_TREE_NODE_GAP;
			}
		}
		int centerHeight = pCenterNode->pRect ? pCenterNode->pRect->height + pCenterNode->pRect->extend_height : 0;
		int sideHeight = qMax(leftHeight, rightHeight);
		if (pCenterNode->pRect && sideHeight > centerHeight)
		{
			pCenterNode->pRect->extend_height += (quint16)(sideHeight - centerHeight);
			centerHeight = pCenterNode->pRect->height + pCenterNode->pRect->extend_height;
		}
		int graphHeight = qMax(centerHeight, sideHeight);
		if (graphHeight <= 0)
		{
			graphHeight = centerHeight;
		}
		if (leftHeight > 0)
		{
			int currentY = currentTopY;
			for (int rootIndex = 0; rootIndex < leftRootNodeList.size(); ++rootIndex)
			{
				WholeBayTreeNode* pRootNode = leftRootNodeList.at(rootIndex);
				bay_tree_place_subtree(pRootNode, currentY);
				currentY += pRootNode->pRect->height + pRootNode->pRect->extend_height + WHOLE_BAY_TREE_NODE_GAP;
			}
		}
		if (rightHeight > 0)
		{
			int currentY = currentTopY;
			for (int rootIndex = 0; rootIndex < rightRootNodeList.size(); ++rootIndex)
			{
				WholeBayTreeNode* pRootNode = rightRootNodeList.at(rootIndex);
				bay_tree_place_subtree(pRootNode, currentY);
				currentY += pRootNode->pRect->height + pRootNode->pRect->extend_height + WHOLE_BAY_TREE_NODE_GAP;
			}
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
			build_bay_tree_virtual_lines(svg, pNode->pRect);
		}
		for (int nodeIndex = 0; nodeIndex < allNodeList.size(); ++nodeIndex)
		{
			WholeBayTreeNode* pNode = allNodeList.at(nodeIndex);
			if (!pNode || !pNode->pRect)
			{
				continue;
			}
			maxBottom = qMax(maxBottom, (int)(pNode->pRect->y + pNode->pRect->height + pNode->pRect->extend_height));
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
		maxRightX = bay_tree_node_x(2, 1, columnGap) + WHOLE_BAY_TREE_NODE_WIDTH + WHOLE_BAY_TREE_VIEW_MARGIN;
	}
	svg.viewBoxX = 0;
	svg.viewBoxY = 0;
	svg.viewBoxWidth = (quint16)(maxRightX + WHOLE_BAY_TREE_VIEW_MARGIN);
	svg.viewBoxHeight = (quint16)(maxBottom + WHOLE_BAY_TREE_VIEW_MARGIN);
}
