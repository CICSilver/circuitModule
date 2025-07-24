#include "SvgTransformer.h"
#include "qcoreapplication.h"
#include <QFile>
#include <QDebug>
#include <include/pugixml/pugixml.hpp>
#include "svgmodel.h"
#define GET_RADIANS(angle) (angle * M_PI / 180)
SvgTransformer::SvgTransformer()
{
	m_circuitConfig = CircuitConfig::Instance();
	m_svgGenerator = new QSvgGenerator();
	m_painter = new QPainter();
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
	m_painter->drawPoint(QPoint(100, 100));

	m_svgGenerator->setFileName(QString());
	VirtualSvg vtSvg;
	GenerateVirtualSvgByIed(pIed, vtSvg);
	DrawVirtualSvg(vtSvg);
	m_painter->end();
	//ReSignSvg(virtualFileName);
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
	AdjustCircuitLinePosition(svg);
}

void SvgTransformer::ParseCircuitFromIed(LogicSvg& svg, const IED* pIed)
{
	CircuitConfig* pConfig = CircuitConfig::Instance();
	quint8 index = 0; // 关联ied编号，用于区分位置
	QList<IedRect*>* list = &svg.leftIedRectList;
	QList<LogicCircuit*> inCircuitList = pConfig->GetInLogicCircuitListByIedName(pIed->name);
	QList<LogicCircuit*> outCircuitList = pConfig->GetOutLogicCircuitListByIedName(pIed->name);
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
				otherIedRect->logic_circuit_line_list.append(line);
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
				otherIedRect->logic_circuit_line_list.append(line);
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
	CircuitConfig* pConfig = CircuitConfig::Instance();
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
			IED* pOppsiteIed = pConfig->GetIedByName(oppsiteIedName);
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
					pConfig->GetIedByName(iedName)->desc,
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
			QList<OpticalCircuit*> oppsiteOptList = pConfig->getOpticalByIeds(pOppsiteIed->name, iedName);
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
	int belowIedY = svg.switcherRectList.last()->y + svg.switcherRectList.last()->height + vertical_distance;	// 交换机下方的IED y
	for (size_t i = 0; i < directRectList.size(); ++i)
	{
		IedRect* rect = directRectList.at(i);
			rect->x = horizontal_distance + i * (rect->width + horizontal_distance);
			rect->y = belowIedY;
	}
	svg.mainIedRect->x = directRectList.last()->x + directRectList.last()->width;	// 主IED移动到直连
	int switcherX = svg.mainIedRect->x;		// 交换机起始X，默认为与主IED对齐
	int totalWidth = 0;
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
}

void SvgTransformer::GenerateVirtualSvgByIed(const IED* pIed, VirtualSvg& svg)
{
	CircuitConfig* pConfig = CircuitConfig::Instance();
	//// 生成图形
	// 矩形构成：内矩形（IED名、描述）+ 外矩形（回路、回路类型图标、压板矩形（压板连接状态）、测量值、）+
	// 添加主IED矩形，居中且位于顶部，外侧矩形高度由回路数量决定
	svg.mainIedRect = GetIedRect(
		pIed->name,
		pIed->desc,
		(SVG_VIEWBOX_WIDTH - RECT_DEFAULT_WIDTH) / 2,
		20,
		RECT_DEFAULT_WIDTH,
		RECT_DEFAULT_HEIGHT,
		ColorHelper::pure_red,
		ColorHelper::ied_underground
	);
	// 主设备高度 = max(侧边IED矩形高度) + margin + IED描述矩形高度;
	// 关联IED
	ParseCircuitFromIed(svg, pIed);


	//// 调整矩形位置
	// 根据单侧关联IED数量，调整高度
	AdjustExtendRectByCircuit(svg.leftIedRectList, svg);
	AdjustExtendRectByCircuit(svg.rightIedRectList, svg);
	// 计算两侧IED的链路数量()
	size_t leftCircuitSize = svg.GetLeftCircuitSize();
	size_t rightCircuitSize = svg.GetRightCircuitSize();
	// 计算主IED高度
	size_t mainCircuitSize = leftCircuitSize > rightCircuitSize ? leftCircuitSize : rightCircuitSize;
	size_t margin = ICON_LENGTH * 2;
	svg.mainIedRect->extend_height = ICON_LENGTH * (mainCircuitSize + CIRCUIT_VERTICAL_DISTANCE) + margin;// 总数据引用数量+间距+margin
}

void SvgTransformer::GenerateWholeCircuitSvgByIed(const IED* pIed, WholeCircuitSvg& svg)
{
	CircuitConfig* pConfig = CircuitConfig::Instance();
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
	DrawCircuitLine(svg.leftIedRectList);
	DrawCircuitLine(svg.rightIedRectList);

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
	DrawIedRect(svg.mainIedRect);
	DrawIedRect(svg.leftIedRectList);
	DrawIedRect(svg.rightIedRectList);
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
		foreach(LogicCircuitLine* pLogicCircuit, pIedRect->logic_circuit_line_list)
		{
		}
	}
	//svg.mainIedRect.pIed->
}

void SvgTransformer::DrawArrowLine(const QPoint& start, const QPoint& end, const QPoint& arrowPt)
{
	double angle = GetAngleByVec(end - start);
	//m_painter->drawLine(start, end);
	drawArrowHeader(arrowPt, angle);
}


void SvgTransformer::DrawConnCircle(const QPoint& pt, int radius, bool isCircleUnderPt)
{
	QBrush brush(ColorHelper::Color(ColorHelper::pure_green));
	m_painter->setBrush(brush);
	int y = isCircleUnderPt ? pt.y() + radius : pt.y() - radius;
	m_painter->drawEllipse(QPoint(pt.x(), y), radius, radius);
	m_painter->setBrush(Qt::NoBrush);
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

void SvgTransformer::AdjustCircuitLinePosition(LogicSvg& svg)
{
	// 调整左侧IED到主IED链路位置
	AdjustMainSideCircuitLinePosition(svg.GetLeftLogicCircuitSize(), svg.leftIedRectList, svg.mainIedRect);
	AdjustMainSideCircuitLinePosition(svg.GetRightLogicCircuitSize(), svg.rightIedRectList, svg.mainIedRect, false);
}

void SvgTransformer::AdjustExtendRectByCircuit(IedRect& pRect)
{
	int circuitSize = 0;
	foreach(LogicCircuitLine * line, pRect.logic_circuit_line_list)
	{
		circuitSize += line->pLogicCircuit->circuitList.size();
	}
	pRect.extend_height = circuitSize * (CIRCUIT_VERTICAL_DISTANCE + ICON_LENGTH) + ICON_LENGTH;
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
	m_painter->setBrush(brush);
	QVector<QPointF> points;
	points << leftArrowPoint << offsetPoint << rightArrowPoint << endPoint;
	//m_painter->drawLine(endPoint, leftArrowPoint);
	//m_painter->drawLine(endPoint, rightArrowPoint);
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

SwitcherRect* SvgTransformer::GetSwitcherRect(const QString& switcherName, const quint16 x, const quint16 y, const quint16 width, const quint16 height, const quint32 border_color, const quint32 underground_color)
{
	SwitcherRect* pRect = new SwitcherRect;
	pRect->switcher_name = switcherName;
	GetBaseRect(pRect, x, y, width, height, border_color, underground_color);
	return pRect;
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
		size_t circuitCnt = rect->logic_circuit_line_list.size();
		size_t circuitDiff = rect->height / (circuitCnt + 1);
		// 左侧IED列表，当前IED连接点位于右侧；右侧IED列表，当前IED连接点位于左侧
		quint8* connectPointIndex = isLeft ? &rect->right_connect_index : &rect->left_connect_index;
		foreach(LogicCircuitLine* line, rect->logic_circuit_line_list)
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
		QPoint tl(rect->x - margin, rect->y - margin);
		QPoint br(rect->x + margin, rect->y + rect->height + rect->extend_height + margin);
		QRect dashRect(rect->x - margin, rect->y - margin, 2 * margin + rect->width, 2 * margin + rect->height + rect->extend_height);
		m_painter->fillRect(dashRect, brush);
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
		m_painter->fillRect(rect->x, rect->y, rect->width, rect->height, brush);


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
	CircuitConfig* pConfig = CircuitConfig::Instance();
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

void SvgTransformer::DrawCircuitLine(QList<IedRect*>& rectList)
{
	foreach(IedRect* rect, rectList)
	{
		foreach(LogicCircuitLine* line, rect->logic_circuit_line_list)
		{
			QPen pen;
			pen.setStyle(Qt::SolidLine);
			QColor color = line->pLogicCircuit->type == SV ? ColorHelper::Color(ColorHelper::line_smv) : ColorHelper::Color(ColorHelper::line_gse);
			pen.setColor(color);
			m_painter->setPen(pen);

			// 仅作链路区分，对图像显示无影响
			QFont font;
			font.setPointSize(TYPE_Circuit);	// 标识该<g>节点为链路父节点
			//font.setWeight(line->id);	// 标识链路id
			QString signStr = QString("%1:%2-%3")
				.arg(line->pLogicCircuit->pSrcIed->name)
				.arg(line->pLogicCircuit->cbName)
				.arg(line->pLogicCircuit->pDestIed->name);	// 标识链路的 srcIed - destIed 信息
			font.setFamily(signStr);
			m_painter->setFont(font);
			// 起点为子设备连接点，终点为主设备连接点
			m_painter->drawLine(line->startPoint, line->midPoint);
			m_painter->drawLine(line->midPoint, line->endPoint);
			double angle = GetAngleByVec(line->endPoint - line->midPoint);
			drawArrowHeader(GetArrowPt(line->endPoint, ARROW_LEN, 0, angle, false), angle, color);
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
	QString optInfo = QString("%1_%2_%3").arg(optLine->pOpticalCircuit->cableDesc).arg(connStatus).arg(optLine->pOpticalCircuit->code);	// PL2205LA:5-C_IL2202ALM:1-A_connstatus_lineCode
	font.setFamily(optInfo);
	font.setPointSize(TYPE_OpticalCircuit);
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
    DrawConnCircle(optLine->startPoint, CONN_R);
    DrawConnCircle(optLine->endPoint, CONN_R, false);
	quint16 underRectY = optLine->pSrcRect->y > optLine->pDestRect->y ? optLine->pSrcRect->y : optLine->pDestRect->y;
    // 绘制箭头
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
	//QPen pen;
	//bool isSwitcherOnTop = line->endPoint.y() < line->startPoint.y();	// 交换机位于IED矩形上方
	//int scale = isSwitcherOnTop ? 1 : -1;
	//pen.setColor(ColorHelper::Color(ColorHelper::pure_white));

	//// 绘制交换机端口号 endPoint
	//QFont font;
	//font.setPointSize(18);
	//m_painter->setPen(pen);
	//QFontMetrics fm(m_painter->font());
	//int portStrWidth = fm.width(line->switcherPort);
	//int portStrPoint_x = line->endPoint.x() - portStrWidth / 2;
	//int portStrPoint_rt_y = isSwitcherOnTop ? line->endPoint.y() - fm.height() : line->endPoint.y();
	//int portStrPoint_lb_y = isSwitcherOnTop ? line->endPoint.y() : line->endPoint.y() + fm.height();
	//m_painter->drawText(QRect(QPoint(portStrPoint_x, portStrPoint_rt_y), QPoint(portStrPoint_x + portStrWidth, portStrPoint_lb_y)), line->switcherPort);

	//// 绘制IED端口号 startPoint
	//portStrWidth = fm.width(line->iedPort);
	//portStrPoint_x = line->endPoint.x() - portStrWidth / 2;
	//portStrPoint_rt_y = isSwitcherOnTop ? line->startPoint.y() : line->startPoint.y() - fm.height();
	//portStrPoint_lb_y = isSwitcherOnTop ? line->startPoint.y() + fm.height() : line->startPoint.y();
	//m_painter->drawText(QRect(QPoint(portStrPoint_x, portStrPoint_rt_y), QPoint(portStrPoint_x + portStrWidth, portStrPoint_lb_y)), line->iedPort);
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
	//QPointF contentStartPoint(pt.x() + contentMargin, pt.y() + contentMargin);
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

void SvgTransformer::DrawPlateIcon(const QPoint& centerPt, bool isClosed, const quint32 defaultColor)
{
	m_painter->save();
	quint32 color = defaultColor != 0 ? 
		defaultColor : isClosed ? ColorHelper::pure_green : ColorHelper::pure_red; // 有指定颜色时，使用指定颜色，否则根据闭合状态使用默认颜色
	QPen pen;
	pen.setWidth(2);
	pen.setColor(ColorHelper::Color(color));
	m_painter->setPen(pen);
	int distance = 40;	// 两个圆之间的距离
	int radius = 5;
	// 直径20的圆
	m_painter->drawEllipse(centerPt, radius, radius);
	m_painter->drawEllipse(centerPt + QPoint(distance, 0), radius, radius);
	// 矩形连接左侧圆
	int rectWidth = 40;
	int rectHeight = radius * 2;
	m_painter->translate(centerPt);
	QPoint ptList[4] = {
		QPoint(0, -rectHeight / 2),
		QPoint(rectWidth, -rectHeight / 2),
		QPoint(rectWidth, rectHeight / 2),
		QPoint(0, rectHeight / 2),
	};
	if (!isClosed)
	{
		// 未闭合状态
		m_painter->rotate(-45);
		m_painter->drawPolyline(ptList, 4);
	}
	else
	{
		// 闭合状态
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

void SvgTransformer::DrawPlateIconWithRect(const QPoint& pt, const quint32 color)
{
}

void SvgTransformer::ReSignSvg(const QString& filename)
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

	if (!doc.save_file(filename.toLocal8Bit()))
	{
		qDebug() << "save file failed";
	}
}

void SvgTransformer::ReSignCircuitLine(pugi::xml_document& doc)
{
	pugi::xpath_node_set circuitLineNodeSet = doc.select_nodes(QString("//g[@font-size='%1']").arg(TYPE_Circuit).toLocal8Bit());
	for (nodeSetConstIterator it = circuitLineNodeSet.begin(); it != circuitLineNodeSet.end(); ++it)
	{
		pugi::xml_node circuitLineNode = it->node();
		QString circuitDesc = circuitLineNode.attribute("font-family").value();
		QString srcIedName = circuitDesc.split("-").first().split(":").first();
		QString cbName = circuitDesc.split("-").first().split(":").last();
		QString destIedName = circuitDesc.split("-").last();
		QString id = circuitLineNode.attribute("font-weight").value();

		//circuitLineNode.append_attribute("id");
		circuitLineNode.append_attribute("src-iedname");
		circuitLineNode.append_attribute("src-cbname");
		circuitLineNode.append_attribute("dest-iedname");
		circuitLineNode.append_attribute("type");
		circuitLineNode.append_attribute("line-code");

		//circuitLineNode.attribute("id").set_value(id.toStdString().c_str());
		circuitLineNode.attribute("src-iedname").set_value(srcIedName.toStdString().c_str());
		circuitLineNode.attribute("src-cbname").set_value(cbName.toStdString().c_str());
		circuitLineNode.attribute("dest-iedname").set_value(destIedName.toStdString().c_str());
		circuitLineNode.attribute("type").set_value("circuit");

		// 重置不相关属性为默认值
		circuitLineNode.attribute("font-family").set_value("SimSun");
		circuitLineNode.attribute("font-size").set_value("15");
		circuitLineNode.attribute("font-weight").set_value("400");
	}


	// 光纤链路
	pugi::xpath_node_set opticalLineNodeSet = doc.select_nodes(QString("//g[@font-size='%1']").arg(TYPE_OpticalCircuit).toLocal8Bit());
	for (nodeSetConstIterator it = opticalLineNodeSet.begin(); it != opticalLineNodeSet.end(); ++it)
	{
		pugi::xml_node lineNode = it->node();
		lineNode.append_attribute("srcIed");
		lineNode.append_attribute("srcPort");
		lineNode.append_attribute("destIed");
		lineNode.append_attribute("destPort");
		lineNode.append_attribute("lineCode");
		lineNode.append_attribute("connStatus");
		lineNode.append_attribute("type");

		QString attrStr = lineNode.attribute("font-family").value();
		QStringList strList = attrStr.split("_");
		QStringList srcInfoList = strList.at(0).split(":");	// PL2205LA:5-C
		QStringList destInfoList = strList.at(1).split(":");	// IL2202ALM:1-A
		QString connStatus = strList.at(2);	// true/false
		QString lineCode = strList.at(3);	// code
		lineNode.attribute("type").set_value("optical");
		lineNode.attribute("srcIed").set_value(srcInfoList.at(0).toStdString().c_str());
		lineNode.attribute("srcPort").set_value(srcInfoList.at(1).toStdString().c_str());
		lineNode.attribute("destIed").set_value(destInfoList.at(0).toStdString().c_str());
		lineNode.attribute("destPort").set_value(destInfoList.at(1).toStdString().c_str());
		lineNode.attribute("lineCode").set_value(lineCode.toStdString().c_str());
		lineNode.attribute("connStatus").set_value(connStatus.toStdString().c_str());
		// 重置不相关属性为默认值
		lineNode.attribute("font-family").set_value("SimSun");
		lineNode.attribute("font-size").set_value("15");
		lineNode.attribute("font-weight").set_value("400");
	}
}

void SvgTransformer::GetSwitcherFromLogicCircuits(QList<LogicCircuit*> logicCircuitList, OpticalSvg& svg)
{

}
