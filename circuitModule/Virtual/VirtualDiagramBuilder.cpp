#include "Virtual/VirtualDiagramBuilder.h"
#include <QFont>
#include <QFontMetrics>

#define VIRTUAL_MAINT_PLATE_OUTER_GAP 30	// 虚回路区与检修压板整体的间距

using utils::ColorHelper;

VirtualDiagramBuilder::VirtualDiagramBuilder()
{
}

VirtualDiagramBuilder::~VirtualDiagramBuilder()
{
}

VirtualSvg* VirtualDiagramBuilder::BuildVirtualDiagramByIedName(const QString& iedName)
{
	IED* pIed = m_pCircuitConfig->GetIedByName(iedName);
	if (!pIed) return NULL;
	VirtualSvg* svg = new VirtualSvg();
	GenerateVirtualDiagramByIed(pIed, *svg);
	return svg;
}

VirtualSvg* VirtualDiagramBuilder::BuildVirtualDiagramByIedPair(const QString& mainIedName, const QString& peerIedName)
{
	if (mainIedName.isEmpty() || peerIedName.isEmpty() || mainIedName == peerIedName)
	{
		return NULL;
	}
	IED* pMainIed = m_pCircuitConfig->GetIedByName(mainIedName);
	IED* pPeerIed = m_pCircuitConfig->GetIedByName(peerIedName);
	if (!pMainIed || !pPeerIed)
	{
		return NULL;
	}
	VirtualSvg* svg = new VirtualSvg();
	int svgTopMargin = 50;
	svg->mainIedRect = utils::GetIedRect(
		pMainIed->name,
		pMainIed->desc,
		(SVG_VIEWBOX_WIDTH - RECT_DEFAULT_WIDTH) / 2,
		svgTopMargin,
		RECT_DEFAULT_WIDTH * 1.5,
		RECT_DEFAULT_HEIGHT,
		ColorHelper::pure_red
	);
	IedRect* peerIedRect = GetOtherIedRect(0, svg->mainIedRect, pPeerIed, ColorHelper::pure_green);
	QList<LogicCircuit*> inCircuitList = m_pCircuitConfig->GetInLogicCircuitListByIedName(pMainIed->name);
	QList<LogicCircuit*> outCircuitList = m_pCircuitConfig->GetOutLogicCircuitListByIedName(pMainIed->name);
	for (int i = 0; i < inCircuitList.size(); ++i)
	{
		LogicCircuit* logicCircuit = inCircuitList.at(i);
		if (!logicCircuit || !logicCircuit->pSrcIed || logicCircuit->pSrcIed->name != pPeerIed->name)
		{
			continue;
		}
		LogicCircuitLine* line = new LogicCircuitLine();
		line->pSrcIedRect = peerIedRect;
		line->pDestIedRect = svg->mainIedRect;
		line->pLogicCircuit = logicCircuit;
		peerIedRect->logic_line_list.append(line);
	}
	for (int i = 0; i < outCircuitList.size(); ++i)
	{
		LogicCircuit* logicCircuit = outCircuitList.at(i);
		if (!logicCircuit || !logicCircuit->pDestIed || logicCircuit->pDestIed->name != pPeerIed->name)
		{
			continue;
		}
		LogicCircuitLine* line = new LogicCircuitLine();
		line->pSrcIedRect = svg->mainIedRect;
		line->pDestIedRect = peerIedRect;
		line->pLogicCircuit = logicCircuit;
		peerIedRect->logic_line_list.append(line);
	}
	if (peerIedRect->logic_line_list.isEmpty())
	{
		delete peerIedRect;
		delete svg;
		return NULL;
	}
	svg->leftIedRectList.append(peerIedRect);
	AdjustExtendRectByCircuit(svg->leftIedRectList, *svg);
	for (int i = 0; i < svg->leftIedRectList.size(); ++i)
	{
		IedRect* rect = svg->leftIedRectList.at(i);
		if (!rect)
		{
			continue;
		}
		rect->extend_height += VIRTUAL_MAINT_PLATE_OUTER_GAP;
	}
	size_t maxY = svg->leftIedRectList.last()->GetExtendBottomY();
	svg->mainIedRect->extend_height = maxY;
	int svgRightMargin = svg->leftIedRectList.first()->x - svg->leftIedRectList.first()->inner_gap;
	svg->viewBoxHeight = svg->mainIedRect->GetExtendHeight() + svgTopMargin;
	svg->viewBoxWidth = svg->mainIedRect->x + svg->mainIedRect->width + svgRightMargin;
	AdjustVirtualCircuitLinePosition(*svg);
	return svg;
}

void VirtualDiagramBuilder::GenerateVirtualDiagramByIed(const IED* pIed, VirtualSvg& svg/*, const QString& swName*/)
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
	//// 调整矩形位置
	// 根据单侧关联IED数量和压板高度，调整高度
	AdjustExtendRectByCircuit(svg.leftIedRectList, svg);
	AdjustExtendRectByCircuit(svg.rightIedRectList, svg);
	for (int i = 0; i < svg.leftIedRectList.size(); ++i)
	{
		IedRect* rect = svg.leftIedRectList.at(i);
		if (!rect)
		{
			continue;
		}
		rect->extend_height += VIRTUAL_MAINT_PLATE_OUTER_GAP;
	}
	for (int i = 0; i < svg.rightIedRectList.size(); ++i)
	{
		IedRect* rect = svg.rightIedRectList.at(i);
		if (!rect)
		{
			continue;
		}
		rect->extend_height += VIRTUAL_MAINT_PLATE_OUTER_GAP;
	}
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

void VirtualDiagramBuilder::AdjustVirtualCircuitLinePosition(VirtualSvg& svg)
{
	AdjustVirtualCircuitLinePosition(svg, svg.leftIedRectList, svg.mainIedRect);
	AdjustVirtualCircuitLinePosition(svg, svg.rightIedRectList, svg.mainIedRect, false);
}

void VirtualDiagramBuilder::AdjustVirtualCircuitLinePosition(VirtualSvg& svg, QList<IedRect*>& rectList, IedRect* mainIed, bool isLeft)
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
				if (isSideSource)
				{
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
				else
				{
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

void VirtualDiagramBuilder::AdjustVirtualCircuitPlatePosition(QHash<QString, PlateRect>& hash, const QString& iedName, const QPoint& linePt, const QString& plateDesc, const QString& plateRef, quint64 plateCode, bool isSrcPlate, bool isSideSrc, bool isLeft)
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
