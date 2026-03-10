#include "SvgTransformer.h"

using utils::ColorHelper;


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

IedRect* SvgTransformer::GetOtherIedRect(quint16 rectIndex, IedRect* mainIedRect, const IED* pIed, const quint32 border_color, const quint32 underground_color)
{
	quint8 row = rectIndex / 2;
	quint8 col = rectIndex % 2;
	quint16 x = mainIedRect->x + (col == 0 ? -IED_HORIZONTAL_DISTANCE - RECT_DEFAULT_WIDTH : IED_HORIZONTAL_DISTANCE + mainIedRect->width);
	quint16 y = mainIedRect->y - IED_VERTICAL_DISTANCE + row * (IED_VERTICAL_DISTANCE + RECT_DEFAULT_HEIGHT);
	return utils::GetIedRect(pIed->name, pIed->desc, x, y, RECT_DEFAULT_WIDTH, RECT_DEFAULT_HEIGHT - 15, border_color, underground_color);
}

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
