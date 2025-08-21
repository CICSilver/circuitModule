#include "SvgTransformer.h"
#include "qcoreapplication.h"
#include <QFile>
#include <QDebug>
#include <include/pugixml/pugixml.hpp>
#include "svgmodel.h"

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

void SvgTransformer::GenerateWholeCircuitSvg(const IED* pIed, const QString& filePath)
{
	GenerateSvg<WholeCircuitSvg>(pIed, filePath, &SvgTransformer::GenerateWholeCircuitSvgByIed, &SvgTransformer::DrawWholeSvg);
}


void SvgTransformer::GenerateLogicSvgByIed(const IED* pIed, LogicSvg& svg)
{
	// 主IED
	svg.mainIedRect = GetIedRect(
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

	//if (svg.mainIedRect) {
	//	QFont font; font.setPointSize(15);
	//	QFontMetrics fm(font);
	//	int titleWidthMain = fm.width(QString::fromLocal8Bit("检修域"));
	//	int titleWidthSide = fm.width(QString::fromLocal8Bit("影响域"));
	//	int descHeight = fm.height() * 2 + 10;
	//	const int descGap = 20;

	//	int minX = svg.mainIedRect->x - svg.mainIedRect->horizontal_margin - titleWidthMain;
	//	int maxX = svg.mainIedRect->x + svg.mainIedRect->width + svg.mainIedRect->horizontal_margin;
	//	int minY = svg.mainIedRect->y - svg.mainIedRect->vertical_margin;
	//	int maxY = svg.mainIedRect->GetExtendBottomY() + svg.mainIedRect->vertical_margin;

	//	// 左列（含标题和说明框）
	//	if (!svg.leftIedRectList.isEmpty()) {
	//		IedRect* first = svg.leftIedRectList.first();
	//		IedRect* last = svg.leftIedRectList.last();
	//		int extLeftX = first->x - first->horizontal_margin - titleWidthSide;
	//		if (extLeftX < minX) minX = extLeftX;
	//		int extTopY = first->y - first->vertical_margin;
	//		int descTopY = extTopY - (descHeight + descGap);
	//		if (descTopY < minY) minY = descTopY;
	//		int extRightX = first->x + first->width + first->horizontal_margin;
	//		if (extRightX > maxX) maxX = extRightX;
	//		int extBottomY = last->y + last->height + last->vertical_margin;
	//		if (extBottomY > maxY) maxY = extBottomY;
	//		foreach (IedRect* r, svg.leftIedRectList) {
	//			if (!r) continue;
	//			int l = r->x - r->horizontal_margin;
	//			int rgt = r->x + r->width + r->horizontal_margin;
	//			int top = r->y - r->vertical_margin;
	//			int bot = r->GetExtendBottomY() + r->vertical_margin;
	//			if (l < minX) minX = l;
	//			if (rgt > maxX) maxX = rgt;
	//			if (top < minY) minY = top;
	//			if (bot > maxY) maxY = bot;
	//		}
	//	}
	//	// 右列（含标题，不含说明框）
	//	if (!svg.rightIedRectList.isEmpty()) {
	//		IedRect* first = svg.rightIedRectList.first();
	//		IedRect* last = svg.rightIedRectList.last();
	//		int extLeftX = first->x - first->horizontal_margin - titleWidthSide;
	//		if (extLeftX < minX) minX = extLeftX;
	//		int extTopY = first->y - first->vertical_margin;
	//		if (extTopY < minY) minY = extTopY;
	//		int extRightX = first->x + first->width + first->horizontal_margin;
	//		if (extRightX > maxX) maxX = extRightX;
	//		int extBottomY = last->y + last->height + last->vertical_margin;
	//		if (extBottomY > maxY) maxY = extBottomY;
	//		foreach (IedRect* r, svg.rightIedRectList) {
	//			if (!r) continue;
	//			int l = r->x - r->horizontal_margin;
	//			int rgt = r->x + r->width + r->horizontal_margin;
	//			int top = r->y - r->vertical_margin;
	//			int bot = r->GetExtendBottomY() + r->vertical_margin;
	//			if (l < minX) minX = l;
	//			if (rgt > maxX) maxX = rgt;
	//			if (top < minY) minY = top;
	//			if (bot > maxY) maxY = bot;
	//		}
	//	}
	//	int w = maxX - minX;
	//	int h = maxY - minY;
	//	if (w > 0) svg.viewBoxWidth = w;
	//	if (h > 0) svg.viewBoxHeight = h;
	//}

	// 计算逻辑图 viewBox 尺寸（内容边界 + 边距），以保证导出后视图自适应
	//if (svg.mainIedRect) {
	//	int minX = svg.mainIedRect->x - svg.mainIedRect->horizontal_margin;
	//	int maxX = svg.mainIedRect->x + svg.mainIedRect->width + svg.mainIedRect->horizontal_margin;
	//	int minY = svg.mainIedRect->y - svg.mainIedRect->vertical_margin;
	//	int maxY = svg.mainIedRect->GetExtendBottomY() + svg.mainIedRect->vertical_margin;
	//	// 左侧 IED
	//	foreach(IedRect* r, svg.leftIedRectList) {
	//		if (!r) continue;
	//		if (r->x - r->horizontal_margin < minX) minX = r->x - r->horizontal_margin;
	//		if (r->x + r->width + r->horizontal_margin > maxX) maxX = r->x + r->width + r->horizontal_margin;
	//		if (r->y - r->vertical_margin < minY) minY = r->y - r->vertical_margin;
	//		int by = r->GetExtendBottomY() + r->vertical_margin;
	//		if (by > maxY) maxY = by;
	//	}
	//	// 右侧 IED
	//	foreach(IedRect* r, svg.rightIedRectList) {
	//		if (!r) continue;
	//		if (r->x - r->horizontal_margin < minX) minX = r->x - r->horizontal_margin;
	//		if (r->x + r->width + r->horizontal_margin > maxX) maxX = r->x + r->width + r->horizontal_margin;
	//		if (r->y - r->vertical_margin < minY) minY = r->y - r->vertical_margin;
	//		int by = r->GetExtendBottomY() + r->vertical_margin;
	//		if (by > maxY) maxY = by;
	//	}
	//	if (minX < 0) minX = 0; if (minY < 0) minY = 0;
	//	int w = maxX - minX;
	//	int h = maxY - minY;
	//	if (w > 0) svg.viewBoxWidth = w; if (h > 0) svg.viewBoxHeight = h;
	//}
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
	int iedWithSwCnt = 0;	// 经过交换机的设备数量
	QList<IedRect*> directRectList;		// 直连IED列表
	QList<IedRect*> indirectRectList;	// 经过交换机的IED列表
	QHash<IedRect*, qint8> connectPtHash;	// 记录连接点数量，交换机不记录直接绘制垂直线
	QList<QString> iedNameList;	// 已绘制的设备光纤号
	// 主IED 中心位置
	svg.mainIedRect = GetIedRect(
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
	foreach(OpticalCircuit * optCircuit, pIed->optical_list_new)
	{
		// 一端为主IED，另一端为对侧IED的光纤链路
		OpticalCircuitLine* optLine = svg.GetOpticalCircuitLineByLineCode(optCircuit->code);
		if(!optLine)
		{
			optLine = new OpticalCircuitLine();
			optLine->lineCode = optCircuit->code;
			optLine->arrowState |= optCircuit->srcIedName == pIed->name ? Arrow_Out : Arrow_In;	// 判断方向
			optLine->pOpticalCircuit = optCircuit;
			svg.opticalCircuitLineList.append(optLine);
		}else
		{
			// 已有该线路，再次出现则为反方向，更新方向
			optLine->arrowState |= Arrow_InOut;
			continue;
		}
		QString oppsiteIedName = optCircuit->srcIedName == pIed->name ? optCircuit->destIedName : optCircuit->srcIedName;
		QString oppsiteIedDesc = optCircuit->srcIedName == pIed->name ? optCircuit->destIedDesc : optCircuit->srcIedDesc;
		IedRect* pOppsiteIedRect = svg.GetIedRectByIedName(oppsiteIedName);
		if (!pOppsiteIedRect)
		{
			pOppsiteIedRect = GetIedRect(
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
			QSet<QString> iedSet = pIed->connectedIedNameSet;
			IED* pOppsiteIed = m_circuitConfig->GetIedByName(oppsiteIedName);
			if(!pOppsiteIed)
			{
				m_errStr += QString("光纤链路 %1 中的交换机设备 %2 不存在，请检查IED配置文件 \n").arg(optCircuit->code).arg(oppsiteIedName);
				continue;
			}
			iedSet.intersect(pOppsiteIed->connectedIedNameSet);
			if(iedSet.isEmpty())
			{
				m_errStr += QString("光纤链路 %1 中的交换机设备 %2 与对侧设备没有连接，请检查光纤链路配置文件 \n").arg(optCircuit->code).arg(oppsiteIedName);
				continue;
			}
			QString iedName = iedSet.values().first();
			// 交换机连接的IED
			IedRect* pIedRect = svg.GetIedRectByIedName(iedName);
			if (!pIedRect)
			{
				pIedRect = GetIedRect(
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
				m_errStr += QString("光纤链路 %1 中的交换机设备 %2 与对侧设备 %3 没有连接，请检查光纤链路配置文件 \n").arg(optCircuit->code).arg(oppsiteIedName).arg(iedName);
				continue;
			}
			OpticalCircuit* oppsiteOptCircuit = oppsiteOptList.first();	// 双向的两条光纤链路，链路编号一致，取第一条即可
			OpticalCircuitLine* oppsiteOptLine = svg.GetOpticalCircuitLineByLineCode(oppsiteOptCircuit->code);
			if(!oppsiteOptLine)
			{
				oppsiteOptLine = new OpticalCircuitLine();
				oppsiteOptLine->lineCode = oppsiteOptCircuit->code;
				oppsiteOptLine->pSrcRect = svg.GetIedRectByIedName(oppsiteOptCircuit->pSrcIed->name);
				oppsiteOptLine->pDestRect = svg.GetIedRectByIedName(oppsiteOptCircuit->pDestIed->name);
				oppsiteOptLine->pOpticalCircuit = oppsiteOptCircuit;
				svg.opticalCircuitLineList.append(oppsiteOptLine);
			}
			if (oppsiteOptList.size() > 1)
			{
				// 结果不止一条，则为双向光纤链路
				oppsiteOptLine->arrowState |= Arrow_InOut;	// 双向光纤链路
			}
			else
			{
				// 判断方向
				oppsiteOptLine->arrowState |= oppsiteOptCircuit->srcIedName == pOppsiteIed->name ? Arrow_In : Arrow_Out;
			}
			pOppsiteIedRect->y = svg.mainIedRect->y + svg.mainIedRect->height + vertical_distance + svg.switcherRectList.size() * (RECT_DEFAULT_HEIGHT + vertical_distance * 0.5);	// 调整交换机位置到主IED下方
			++connectPtHash[pIedRect];
			svg.switcherRectList.append(pOppsiteIedRect);
			iedWithSwCnt++;
		}
		else
		{
			// 直连设备
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
				quint16 startX = directRectList.last()->x + directRectList.last()->width;
				rect->x = startX + horizontal_distance + i * (horizontal_distance + rect->width);
				rect->y = directRectList.last()->y;
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
	int midPtCnt_l = 0;	// 左侧中间转折点计数
	int midPtCnt_r = 0;	// 右侧中间转折点计数
	// 连接点位置
	QHash<IedRect*, qint8> connectCntHash;
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
		int minX = directRectList.isEmpty() ?			// 直连IED在左侧，若为空则最左侧是主IED
			svg.mainIedRect->x : directRectList.first()->x;
		int minY = svg.mainIedRect->y - svg.mainIedRect->vertical_margin;	// 主IED上侧边距
		int maxX = svg.switcherRectList.isEmpty() ? // 交换机在右侧，若为空则最右侧是主IED
			svg.mainIedRect->x + svg.mainIedRect->width + svg.mainIedRect->horizontal_margin * 2 :
			svg.switcherRectList.last()->x + svg.switcherRectList.last()->width + svg.switcherRectList.last()->horizontal_margin * 2;
		int maxY = 
			directRectList.isEmpty() ?	// 下方IED在主IED下方，若为空则最下侧是主IED
				svg.switcherRectList.isEmpty() ?	
				svg.mainIedRect->GetInnerBottomY() + svg.mainIedRect->vertical_margin * 2 :	// 若交换机也为空，则下方为主IED
				svg.switcherRectList.last()->GetInnerBottomY() + svg.switcherRectList.last()->vertical_margin * 2 :	// 若直连IED为空，则下方为交换机;
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

void SvgTransformer::GenerateVirtualSvgByIed(const IED* pIed, VirtualSvg& svg)
{
	//// 生成图形
	// 矩形构成：内矩形（IED名、描述）+ 外矩形（回路、回路类型图标、压板矩形（压板连接状态）、测量值、）+
	// 添加主IED矩形，居中且位于顶部，外侧矩形高度由回路数量决定
	int svgTopMargin = 50;	// SVG视图顶部边距
	svg.mainIedRect = GetIedRect(
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
	

	//// 调整矩形位置
	// 根据单侧关联IED数量和压板高度，调整高度
	AdjustExtendRectByCircuit(svg.leftIedRectList, svg);
	AdjustExtendRectByCircuit(svg.rightIedRectList, svg);
	// 计算两侧IED的链路数量()
	size_t leftCircuitSize = svg.GetLeftCircuitSize();
	size_t rightCircuitSize = svg.GetRightCircuitSize();
	// 计算主IED高度
	size_t maxY = qMax(svg.leftIedRectList.last()->GetExtendBottomY(), svg.rightIedRectList.last()->GetExtendBottomY());
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
	//m_painter->save();
	DrawIedRect(svg.leftIedRectList);	// 左侧
	DrawIedRect(svg.rightIedRectList);	// 右侧
	//m_painter->restore();
	DrawExternalRect(svg.leftIedRectList, QString::fromLocal8Bit("影响域"), true);	// 左侧外部虚线框，上方描述矩形
	DrawExternalRect(svg.rightIedRectList, QString::fromLocal8Bit("影响域"));	// 右侧外部虚线框
	
	// 关联链路
	DrawLogicCircuitLine(svg.leftIedRectList);
	DrawLogicCircuitLine(svg.rightIedRectList);

}

void SvgTransformer::DrawOpticalSvg(OpticalSvg& svg)
{
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
	//svg.mainIedRect.pIed->
}

void SvgTransformer::DrawConnCircle(const QPoint& pt, int radius, bool isCircleUnderPt)
{
	m_painter->save();
	QBrush brush(ColorHelper::Color(ColorHelper::pure_green));
	m_painter->setBrush(brush);
	int y = isCircleUnderPt ? pt.y() + radius : pt.y() - radius;
	m_painter->drawEllipse(QPoint(pt.x(), y), radius, radius);
	m_painter->restore();
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

void SvgTransformer::drawArrowHeader(const QPoint& endPoint, double arrowAngle, QColor color, int arrowLen)
{
	m_painter->save();
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
	m_painter->restore();
}

double SvgTransformer::GetAngleByVec(const QPointF& vec) const
{
	double direction = atan2(-vec.y(), vec.x());
	double angle = direction * (180 / M_PI);
	return angle;
}

QPoint SvgTransformer::GetArrowPt(const QPoint& pt, int arrowLen, int conn_r, double angle, bool isUnderConnpt)
{
	int offset = 2;	// 箭头偏移量，向前兼容
    
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

	return GetIedRect(pIed->name, pIed->desc, x, y, RECT_DEFAULT_WIDTH, RECT_DEFAULT_HEIGHT - 15, border_color, underground_color);
}

IedRect * SvgTransformer::GetIedRect(QString iedName, QString iedDesc, const quint16 x, const quint16 y, const quint16 width, const quint16 height, const quint32 border_color, const quint32 underground_color)
{
	IedRect* iedRect = new IedRect;
	iedRect->iedName = iedName;
	iedRect->iedDesc = iedDesc;
	GetBaseRect(iedRect, x, y, width, height, border_color, underground_color);
	return iedRect;
}

void SvgTransformer::DrawDescRect(const IedRect& leftTopRect)
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
	m_painter->setPen(pen);
	m_painter->setFont(descFont);
	
	quint8 descLineWidth = 160;
	m_painter->drawRect(x, y, width, height);
	m_painter->drawText(x, y + fm.height(), "GSE");
	m_painter->drawText(x, y + fm.height() * 2, "SV");
}

void SvgTransformer::GetBaseRect(SvgRect* rect, const quint16 x, const quint16 y, const quint16 width, const quint16 height, const quint32 border_color, const quint32 underground_color)
{
	rect->width = width;
	rect->height = height;
	rect->x = x;
	rect->y = y;

	rect->border_color = border_color;
	rect->underground_color = underground_color;

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

void SvgTransformer::AdjustVirtualCircuitLinePosition(VirtualSvg& svg, QList<IedRect*>& rectList, IedRect* mainIed, bool isLeft)
{
	m_painter->save();
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
				m_painter->setFont(font);
				QFontMetrics fm = m_painter->fontMetrics();
				int valWidth = fm.width(pVtLine->valStr);

				int startOffset = ICON_LENGTH * 4 + rect->inner_gap;
				int iconOffset = ICON_LENGTH + rect->inner_gap;
				int valGap = rect->inner_gap * 2 + valWidth;
				int iedDistance = IED_HORIZONTAL_DISTANCE - rect->inner_gap * 4;
				if (isSideSource) {
					// 侧边设备到主设备
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

					pVtLine->circuitDesc = isLeft ?
						QString("%1 -> %2").arg(pCircuit->srcDesc, pCircuit->destDesc) :
						QString("%1 <- %2").arg(pCircuit->destDesc, pCircuit->srcDesc);
				}
				else {
					// 主设备到侧边设备
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

					pVtLine->circuitDesc = isLeft ?
						QString("%1 <- %2").arg(pCircuit->srcDesc, pCircuit->destDesc) :
						QString("%1 -> %2").arg(pCircuit->destDesc, pCircuit->srcDesc);
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
				// 开入软压板，图形位于回路终点
				QString key = QString("%1_%2").arg(srcRect->iedName).arg(destRect->iedName);
				AdjustVirtualCircuitPlatePosition(svg.plateRectHash, key, pVtLine->endPoint, pCircuit->destSoftPlateDesc, pCircuit->destSoftPlateRef, false, isSideSource, isLeft);
				// 开出软压板，图形位于回路起点
				AdjustVirtualCircuitPlatePosition(svg.plateRectHash, key, pVtLine->startPoint, pCircuit->srcSoftPlateDesc, pCircuit->srcSoftPlateRef, true, isSideSource, isLeft);
                // 增加连接点索引
                ++(*pConnectPtIndex);
            }
        }
    }
	m_painter->restore();
}

void SvgTransformer::AdjustVirtualCircuitPlatePosition(QHash<QString, PlateRect>& hash, const QString& iedName, const QPoint& linePt, const QString& plateDesc, const QString& plateRef, bool isSrcPlate, bool isSideSrc, bool isLeft)
{
	if( plateDesc.isEmpty() || plateRef.isEmpty())
		return;
	QString key = QString("%1_%2").arg(iedName, plateRef);
	if (!hash.contains(key))
	{
		// 当前通道压板未记录，则创建一个新的压板矩形
		PlateRect plateRect;
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
	m_painter->save();
	QPen pen;
	QBrush brush(rect->underground_color);

	if (rect->extend_height != 0) // 需要绘制额外的外部矩形框
	{
		brush.setColor(ColorHelper::Color(ColorHelper::ied_underground));
		pen.setColor(ColorHelper::Color(ColorHelper::ied_border));
		pen.setStyle(Qt::DashLine);
		int margin = 15;
		m_painter->setPen(pen);
		m_painter->setBrush(brush);
		QPoint tl(rect->x - rect->inner_gap, rect->y - rect->inner_gap);
		QPoint br(rect->x + rect->inner_gap, rect->y + rect->height + rect->extend_height + rect->inner_gap);
		QRect dashRect(rect->x - rect->inner_gap, rect->y - rect->inner_gap, 2 * rect->inner_gap + rect->width, rect->GetExtendHeight());
		m_painter->fillRect(dashRect, brush);
		m_painter->setBrush(Qt::NoBrush);
		m_painter->drawRect(dashRect);
	}
	pen.setStyle(Qt::SolidLine);
	pen.setColor(ColorHelper::Color(rect->border_color));
	m_painter->setPen(pen);
	QFont font;
	font.setFamily(rect->iedName);
	font.setPointSize(TYPE_IED);
	m_painter->setFont(font);
	if (rect->underground_color == 0) // 透明
		m_painter->drawRect(rect->x, rect->y, rect->width, rect->height);
	else
	{
		m_painter->fillRect(rect->x, rect->y, rect->width, rect->height, brush);
		//m_painter->drawRect(rect->x, rect->y, rect->width, rect->height);
	}


	m_painter->restore();
	DrawTextInRect(rect, rect->iedName, rect->iedDesc);
}

void SvgTransformer::DrawIedRect(QList<IedRect*>& rectList)
{
	if (rectList.isEmpty()) return;
	foreach(IedRect* rect, rectList)
	{
		DrawIedRect(rect);
	}
}

void SvgTransformer::DrawSwitcherRect(IedRect* rect)
{
	m_painter->save();
	QPen pen;
	pen.setColor(rect->border_color);
	m_painter->setPen(pen);
	QBrush brush(rect->underground_color);
	QFont font;
	font.setFamily(rect->iedName);
	font.setPointSize(TYPE_Switcher);
	m_painter->setFont(font);
	m_painter->fillRect(rect->x, rect->y, rect->width, rect->height, brush);
	m_painter->restore();
	DrawTextInRect(rect, rect->iedName, rect->iedDesc, 18);
}

//void SvgTransformer::DrawTextInRect(IedRect* rect)
void SvgTransformer::DrawTextInRect(SvgRect* rect, QString name, QString desc, int pointSize)
{
	// 设置字体大小
	QString text = desc;
	QPen pen;
	pen.setColor(Qt::white);
	QFont font = m_painter->font();
	font.setPointSize(pointSize);
	m_painter->setFont(font);
	m_painter->setPen(pen);

	// 计算文字位置及换行
	QFontMetrics metrics(m_painter->font());
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
		m_painter->drawText(text_x, text_y, line);
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
			m_painter->save();
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
			drawArrowHeader(GetArrowPt(line->endPoint, ARROW_LEN, 0, angle, false), angle, color);
			m_painter->restore();
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
				m_painter->save();
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
				DrawVirtualText(pVtLine->startValRect, pVtLine->valStr);
				DrawVirtualText(pVtLine->endValRect, pVtLine->valStr);
				// 绘制描述信息
				DrawVirtualText(pVtLine->circuitDescRect, pVtLine->circuitDesc);

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
					(QString)(joinFields() << pVtLine->pVirtualCircuit->remoteId << pVtLine->pVirtualCircuit->remoteSigId_A << pVtLine->pVirtualCircuit->remoteSigId_B) <<
					// virtualType 7
					(QString)(joinFields() << (pVtLine->pVirtualCircuit->type == GOOSE ? "gse" : "sv"));
				font.setFamily(vtLineInfo);
				m_painter->setFont(font);
				m_painter->drawLine(pVtLine->startPoint, pVtLine->endPoint);
				font.setPointSize(TYPE_Circuit_Arrow);
				m_painter->setFont(font);
				drawArrowHeader(pVtLine->endPoint, GetAngleByVec(pVtLine->endPoint - pVtLine->startPoint));
				
				m_painter->restore();
			}
		}
	}
}

void SvgTransformer::DrawOpticalLine(OpticalCircuitLine* optLine)
{
    QPen pen;
    pen.setColor(ColorHelper::Color(ColorHelper::pure_green));
    m_painter->save();
    m_painter->setPen(pen);
	// 记录链路信息到SVG文件
	QFont font;
	QString connStatus = optLine->pOpticalCircuit->connStatus ? "true" : "false";	// true/false
	QString optInfo = joinGroups() <<
		(QString)(joinFields() << m_element_id++) <<
		(QString)(joinFields() << optLine->pOpticalCircuit->code) <<
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
    DrawConnCircle(optLine->startPoint, CONN_R);
    DrawConnCircle(optLine->endPoint, CONN_R, false);
	quint16 underRectY = optLine->pSrcRect->y > optLine->pDestRect->y ? optLine->pSrcRect->y : optLine->pDestRect->y;

    // 绘制箭头
	font.setPointSize(TYPE_Circuit_Arrow);
	m_painter->setFont(font);
    if (optLine->arrowState & Arrow_Out)
    {
        drawArrowHeader(GetArrowPt(optLine->startPoint, ARROW_LEN, CONN_R, outAngle, true), outAngle);
        drawArrowHeader(GetArrowPt(optLine->endPoint, ARROW_LEN, CONN_R, outAngle, optLine->endPoint.y() > underRectY), outAngle);
    }
    if (optLine->arrowState & Arrow_In)
    {
        drawArrowHeader(GetArrowPt(optLine->startPoint, ARROW_LEN, CONN_R, inAngle, true), inAngle);
        drawArrowHeader(GetArrowPt(optLine->endPoint, ARROW_LEN, CONN_R, inAngle, optLine->endPoint.y() > underRectY), inAngle);
    }

    m_painter->restore();
}

void SvgTransformer::DrawPortText(OpticalCircuitLine* line, int conn_r)
{
	// 端口号绘制在矩形和连接点直接
	m_painter->save();
	int offset = conn_r * 2 + 10;	// 端口号与连接点的偏移量
	QPen pen;
	pen.setColor(ColorHelper::Color(ColorHelper::pure_white));
	QFont font;
	font.setPointSize(18);
	m_painter->setPen(pen);
	m_painter->setFont(font);
	int topPtY = line->startPoint.y() + offset;
	// 下侧设备为交换机时，端口号绘制在矩形内
	int bottomPtY = line->pSrcRect->iedName.contains("SW") || line->pDestRect->iedName.contains("SW") ? line->endPoint.y() + offset : line->endPoint.y() - offset;
	QString topPort = line->pSrcRect->y > line->pDestRect->y ? 
		line->pOpticalCircuit->destIedPort :		// 源设备位于下方时，上方端口号为目的设备端口号
		line->pOpticalCircuit->srcIedPort;			// 源设备位于上方时，上方端口号为源设备端口号
	QString bottomPort = line->pSrcRect->y > line->pDestRect->y ? 
		line->pOpticalCircuit->srcIedPort : 
		line->pOpticalCircuit->destIedPort;
	QFontMetrics fm(m_painter->font());
	int upperPortStrWidth = fm.width(topPort);
	int bottomPortStrWidth = fm.width(bottomPort);

	QPoint topPort_lt_pt(line->startPoint.x() - upperPortStrWidth / 2, topPtY - 5);
	QPoint topPort_rb_pt(line->startPoint.x() + upperPortStrWidth / 2, topPtY + fm.height());
	QPoint bottomPort_lt_pt(line->endPoint.x() - bottomPortStrWidth / 2, bottomPtY - 5);
	QPoint bottomPort_rb_pt(line->endPoint.x() + bottomPortStrWidth / 2, bottomPtY + fm.height());
	// 绘制端口号
	m_painter->drawText(QRect(topPort_lt_pt, topPort_rb_pt), topPort);
	m_painter->drawText(QRect(bottomPort_lt_pt, bottomPort_rb_pt), bottomPort);

	m_painter->restore();
}

void SvgTransformer::DrawWholeCircuitLine(CircuitLine* pCircuitLine, QPoint& startPoint)
{

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

void SvgTransformer::DrawVirtualText(const QRect& rect, QString val)
{
	m_painter->save();

	// 约定：当传入的 rect 高度为 0 时，表示这是“数值占位标签”，不直接绘制数字，
	// 仅在 SVG 中留下一个不可见的占位元素，由交互层 InteractiveSvgItem 动态绘制。
	if (rect.height() == 0) {
		// 使用 font-family 携带占位信息：id 以及矩形位置尺寸
		// group0: id，仅用流水 m_element_id
		// group1: x y w h（这里 h==0 只是标记用途，仍按数值写入）
		QString payload = joinGroups()
			<< (QString)(joinFields() << m_element_id)
			<< (QString)(joinFields() << rect.x() << rect.y() << rect.width() << rect.height());

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
		m_painter->restore();
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
	m_painter->restore();
}

void SvgTransformer::DrawPlate(const QHash<QString, PlateRect>& hash)
{
	foreach(const PlateRect& plateRect, hash)
	{
		m_painter->save();
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
			(QString)(joinFields() << plateRect.desc << plateRect.ref);
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
		m_painter->restore();
	}
}

void SvgTransformer::DrawGseIcon(const QPoint& pt, const quint32 color)
{
	m_painter->save();
	//float iconScale = 0.07;
	// 绘制边框
	QPen pen;
	pen.setColor(ColorHelper::Color(color));
	pen.setWidth(3);
	m_painter->setPen(pen);
	m_painter->drawRect(pt.x(), pt.y(), ICON_LENGTH, ICON_LENGTH);
	// 绘制内容
	qreal contentMargin = 3;
	qreal contentVerticalMargin = 7;
	qreal lineWidth = (ICON_LENGTH - contentMargin * 2) / 3;
	int firstLineWidth_bottom = lineWidth * 0.9;
	int secondLineWidth_bottom = lineWidth * 1.25;
	int thirdLineWidth_bottom = lineWidth * 0.9;
	int contentTopY = pt.y() + contentVerticalMargin;
	int contentBottomY = pt.y() + ICON_LENGTH - contentVerticalMargin;
	QPointF points[6] = {
		QPointF(pt.x() + contentMargin, contentBottomY),
		QPointF(pt.x() + contentMargin + firstLineWidth_bottom, contentBottomY),
		QPointF(pt.x() + contentMargin + firstLineWidth_bottom, contentTopY),
		QPointF(pt.x() + contentMargin + firstLineWidth_bottom + secondLineWidth_bottom, contentTopY),
		QPointF(pt.x() + contentMargin + firstLineWidth_bottom + secondLineWidth_bottom, contentBottomY),
		QPointF(pt.x() + contentMargin + firstLineWidth_bottom + secondLineWidth_bottom + thirdLineWidth_bottom + 0.5, contentBottomY)
	};
	pen.setWidth(2);
	m_painter->setPen(pen);
	m_painter->drawPolyline(points, 6);
	m_painter->restore();
}

void SvgTransformer::DrawSvIcon(const QPoint& pt, const quint32 color)
{
	m_painter->save();
	// 绘制边框
	QPen pen;
	pen.setColor(ColorHelper::Color(color));
	pen.setWidth(3);
	m_painter->setPen(pen);
	m_painter->drawRect(pt.x(), pt.y(), ICON_LENGTH, ICON_LENGTH);
	// 绘制内容
	qreal contentMargin = 2;
	qreal contentWidth = ICON_LENGTH - contentMargin * 2;
	qreal contentY = (pt.y() + static_cast<qreal>(ICON_LENGTH)) / 2;
	qreal contentTopY = pt.y() + contentMargin;
	qreal contentBottomY = pt.y() + ICON_LENGTH - contentMargin;
	QPointF startPoint(pt.x() + contentMargin, pt.y() + ICON_LENGTH / 2);
	QPointF endPoint(pt.x() + ICON_LENGTH - contentMargin, pt.y() + ICON_LENGTH / 2);
	QPointF controlPt1(pt.x() + contentWidth / 2 - 1, startPoint.y() - 5);
	QPointF controlPt2(pt.x() + contentWidth / 4 * 3 + 1, startPoint.y() + 5);
	QPainterPath path;
	path.moveTo(startPoint);
	path.cubicTo(controlPt1, controlPt1, QPointF((startPoint.x() + endPoint.x()) / 2, startPoint.y()));
	path.cubicTo(controlPt2, controlPt2, endPoint);
	pen.setWidth(2);
	m_painter->setPen(pen);
	m_painter->drawPath(path);
	m_painter->restore();
}

void SvgTransformer::DrawPlateIcon(const QPoint& centerPt, bool status, const QString& info)
{
    m_painter->save();
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
	m_painter->translate(centerPt);
	QPoint ptList[4] = {
		QPoint(0, -rectHeight / 2),
		QPoint(distance, -rectHeight / 2),
		QPoint(distance, rectHeight / 2),
		QPoint(0, rectHeight / 2),
	};
	if (!status)
	{
		// 未闭合状态矩形
		//m_painter->rotate(-45);
		//m_painter->drawPolyline(ptList, 4);
	}
	else
	{
		// 闭合状态矩形
		pen.setColor(Qt::transparent);
		m_painter->setPen(pen);
		m_painter->drawLine(ptList[0], ptList[1]);
		m_painter->drawLine(ptList[2], ptList[3]);
	}
	m_painter->restore();
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
		iedRectNode.attribute("iedname").set_value(iedName.toStdString().c_str());
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

	ReSignIedRect(doc);
	ReSignCircuitLine(doc);
	ReSignPlate(doc);
	ReSignVirtualValuePlaceholders(doc);
	ReSignSvgViewBox(doc, svg.viewBoxX, svg.viewBoxY, svg.viewBoxWidth, svg.viewBoxHeight);

	if (!doc.save_file(filename.toLocal8Bit()))
	{
		qDebug() << "save file failed";
	}
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

		lineNode.attribute("id").set_value(id.toStdString().c_str());
		lineNode.attribute("src-iedname").set_value(srcIedName.toStdString().c_str());
		lineNode.attribute("src-cbname").set_value(cbName.toStdString().c_str());
		lineNode.attribute("dest-iedname").set_value(destIedName.toStdString().c_str());
		lineNode.attribute("type").set_value("logic");
		lineNode.attribute("circuit-code").set_value(circuitId.toStdString().c_str());

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
		lineNode.append_attribute("src-ied");
		lineNode.append_attribute("src-port");
		lineNode.append_attribute("dest-ied");
		lineNode.append_attribute("dest-port");
		lineNode.append_attribute("code");
		lineNode.append_attribute("status");
		lineNode.append_attribute("remote-id");
		lineNode.append_attribute("type");
		lineNode.append_attribute("id");
		QString attrStr = lineNode.attribute("font-family").value();
		QList<QStringList> strList = splitGroupAndFields(attrStr);
		QString id = lineNode.attribute("font-weight").value();	// id
		QString code = getField(strList, 0, 0);	// code
		QString srcIedName = getField(strList, 1, 0);	// srcIedName
		QString srcPort = getField(strList, 1, 1);	// srcPort
		QString destIedName = getField(strList, 2, 0);	// destIedName
		QString destPort = getField(strList, 2, 1);	// destPort
		QString connStatus = getField(strList, 3, 0);	// connStatus
		QString remoteId = getField(strList, 4, 0);	// remoteId

		lineNode.attribute("type").set_value("optical");
		lineNode.attribute("src-ied").set_value(srcIedName.toStdString().c_str());
		lineNode.attribute("src-port").set_value(srcPort.toStdString().c_str());
		lineNode.attribute("dest-ied").set_value(destIedName.toStdString().c_str());
		lineNode.attribute("dest-port").set_value(destPort.toStdString().c_str());
		lineNode.attribute("code").set_value(code.toStdString().c_str());
		lineNode.attribute("status").set_value(connStatus.toStdString().c_str());
		lineNode.attribute("remote-id").set_value(remoteId.toStdString().c_str());
		lineNode.attribute("id").set_value(id.toStdString().c_str());
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
		connNode.attribute("id").set_value(id.toStdString().c_str());
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
		lineNode.append_attribute("id");
		lineNode.append_attribute("srcIedName");
		lineNode.append_attribute("destIedName");
		lineNode.append_attribute("srcSoftPlateDesc");
		lineNode.append_attribute("destSoftPlateDesc");
		lineNode.append_attribute("srcSoftPlateRef");
		lineNode.append_attribute("destSoftPlateRef");
		lineNode.append_attribute("remoteId");
		lineNode.append_attribute("remoteSigId_A");
		lineNode.append_attribute("remoteSigId_B");
		lineNode.append_attribute("type");
		QString attrStr = lineNode.attribute("font-family").value();
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
		QString remoteId = getField(strList, 6, 0);	// remoteId
		QString remoteSigId_A = getField(strList, 6, 1);	// remoteSigId_A
		QString remoteSigId_B = getField(strList, 6, 2);	// remoteSigId_B
		QString vType = getField(strList, 7, 0);	// vType (gse/sv)

		lineNode.attribute("srcIedName").set_value(srcIedName.toStdString().c_str());
		lineNode.attribute("destIedName").set_value(destIedName.toStdString().c_str());
		lineNode.attribute("srcSoftPlateDesc").set_value(srcSoftPlateDesc.toStdString().c_str());
		lineNode.attribute("destSoftPlateDesc").set_value(destSoftPlateDesc.toStdString().c_str());
		lineNode.attribute("srcSoftPlateRef").set_value(srcSoftPlateRef.toStdString().c_str());
		lineNode.attribute("destSoftPlateRef").set_value(destSoftPlateRef.toStdString().c_str());
		lineNode.attribute("remoteId").set_value(remoteId.toStdString().c_str());
		lineNode.attribute("remoteSigId_A").set_value(remoteSigId_A.toStdString().c_str());
		lineNode.attribute("remoteSigId_B").set_value(remoteSigId_B.toStdString().c_str());
		lineNode.attribute("type").set_value("virtual");
		lineNode.attribute("virtual-type").set_value(vType.toStdString().c_str());
		lineNode.attribute("id").set_value(id.toStdString().c_str());
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
		arrowNode.attribute("id").set_value(id.toStdString().c_str());
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
		plateNode.append_attribute("plate-desc");
		plateNode.append_attribute("plate-ref");
		plateNode.append_attribute("type");
		plateNode.append_attribute("id");
		QString plateDesc = plateNode.attribute("font-family").value();
		QList<QStringList> plateDescGrp = splitGroupAndFields(plateDesc);
		QString id = getField(plateDescGrp, 0, 0);	// id
		QString desc = getField(plateDescGrp, 1, 0);	// plate-desc
		QString ref = getField(plateDescGrp, 1, 1);	// plate-ref
		plateNode.attribute("id").set_value(id.toStdString().c_str());
		plateNode.attribute("plate-desc").set_value(desc.toStdString().c_str());
		plateNode.attribute("plate-ref").set_value(ref.toStdString().c_str());
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
			plateRectNode.attribute("id").set_value(id.toStdString().c_str());

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

		node.append_attribute("type").set_value("virtual-value");
		node.append_attribute("id").set_value(id.toStdString().c_str());
		node.append_attribute("x").set_value(x.toStdString().c_str());
		node.append_attribute("y").set_value(y.toStdString().c_str());
		node.append_attribute("w").set_value(w.toStdString().c_str());
		node.append_attribute("h").set_value(h.toStdString().c_str());

		// 重置不相关属性
		node.attribute("font-family").set_value("SimSun");
		node.attribute("font-size").set_value("15");
		node.attribute("font-weight").set_value("400");
	}
}

void SvgTransformer::ReSignSvgViewBox(pugi::xml_document& doc, int x, int y, int width, int height)
{
	pugi::xml_node svgNode = doc.child("svg");
	svgNode.attribute("viewBox").set_value(QString("%1 %2 %3 %4").arg(x).arg(y).arg(width).arg(height).toStdString().c_str());
}
