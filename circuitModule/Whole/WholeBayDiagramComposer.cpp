#include "Whole/WholeBayDiagramComposer.h"
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

	static LogicCircuitLine* whole_create_logic_line(LogicCircuit* pLogicCircuit, IedRect* pSrcRect, IedRect* pDestRect)
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

	static IedRect* whole_create_ied_rect_with_size(CircuitConfig* pCircuitConfig, const QString& iedName, int x, int y, int width, int height, quint32 borderColor)
	{
		IED* pIed = pCircuitConfig ? pCircuitConfig->GetIedByName(iedName) : NULL;
		QString iedDesc = pIed ? pIed->desc : QString();
		return utils::GetIedRect(iedName, iedDesc, (quint16)x, (quint16)y, (quint16)width, (quint16)height, borderColor, utils::ColorHelper::pure_black);
	}

	static void whole_adjust_rect_extend_height(IedRect* pRect)
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

	static int whole_get_rect_outer_bottom(const IedRect* pRect)
	{
		if (!pRect)
		{
			return 0;
		}
		return pRect->y + pRect->height + pRect->extend_height;
	}

	static void whole_attach_bay_tree_logic_lines(WholeBayTreeNode* pNode)
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
			LogicCircuitLine* pLine = whole_create_logic_line(pLogicCircuit, pSrcRect, pDestRect);
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

	static void whole_build_bay_tree_virtual_lines(WholeCircuitSvg& svg, IedRect* pOwnerRect)
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
}

WholeBayDiagramComposer::WholeBayDiagramComposer(CircuitConfig* pCircuitConfig)
	: m_pCircuitConfig(pCircuitConfig)
{
}

void WholeBayDiagramComposer::Generate(const QString& bayName, WholeCircuitSvg& svg, int columnGap)
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
			pNode->pRect = whole_create_ied_rect_with_size(m_pCircuitConfig, pNode->iedName, whole_bay_tree_node_x(pNode->depth, pNode->side, columnGap), currentTopY, WHOLE_BAY_TREE_NODE_WIDTH, RECT_DEFAULT_HEIGHT, borderColor);
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
			whole_attach_bay_tree_logic_lines(pNode);
		}
		for (int nodeIndex = 0; nodeIndex < allNodeList.size(); ++nodeIndex)
		{
			WholeBayTreeNode* pNode = allNodeList.at(nodeIndex);
			if (!pNode || !pNode->pRect)
			{
				continue;
			}
			whole_adjust_rect_extend_height(pNode->pRect);
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
			whole_build_bay_tree_virtual_lines(svg, pNode->pRect);
		}
		for (int nodeIndex = 0; nodeIndex < allNodeList.size(); ++nodeIndex)
		{
			WholeBayTreeNode* pNode = allNodeList.at(nodeIndex);
			if (!pNode || !pNode->pRect)
			{
				continue;
			}
			maxBottom = qMax(maxBottom, whole_get_rect_outer_bottom(pNode->pRect));
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
