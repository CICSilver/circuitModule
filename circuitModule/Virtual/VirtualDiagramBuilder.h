#pragma once
#include "Common/DiagramBuilderBase.h"

class VirtualDiagramBuilder : public DiagramBuilderBase
{
public:
	VirtualDiagramBuilder();
	~VirtualDiagramBuilder();
	//************************************
	// 函数名称:	BuildVirtualDiagramByIedName
	// 函数全名:	VirtualDiagramBuilder::BuildVirtualDiagramByIedName
	// 函数权限:	public
	// 函数说明:	按IED名称生成虚回路图模型
	// 输入参数:	const QString& iedName	IED名称
	// 输出参数:	无
	// 返回值:	VirtualSvg*
	//************************************
	VirtualSvg* BuildVirtualDiagramByIedName(const QString& iedName);
	//************************************
	// 函数名称:	BuildVirtualDiagramByBayName
	// 函数全名:	VirtualDiagramBuilder::BuildVirtualDiagramByBayName
	// 函数权限:	public
	// 函数说明:	按间隔名称生成虚回路图模型
	// 输入参数:	const QString& bayName	间隔名称
	// 输出参数:	无
	// 返回值:	VirtualSvg*
	//************************************
	VirtualSvg* BuildVirtualDiagramByBayName(const QString& bayName);
	//************************************
	// 函数名称:	BuildVirtualDiagramByIedPair
	// 函数全名:	VirtualDiagramBuilder::BuildVirtualDiagramByIedPair
	// 函数权限:	public
	// 函数说明:	按IED对生成虚回路图模型
	// 输入参数:	const QString& mainIedName	主IED名称
	// 输入参数:	const QString& peerIedName	对端IED名称
	// 输出参数:	无
	// 返回值:	VirtualSvg*
	//************************************
	VirtualSvg* BuildVirtualDiagramByIedPair(const QString& mainIedName, const QString& peerIedName);

private:
	//************************************
	// 函数名称:	GenerateVirtualDiagramByIed
	// 函数全名:	VirtualDiagramBuilder::GenerateVirtualDiagramByIed
	// 函数权限:	private
	// 函数说明:	根据IED对象生成虚回路图模型
	// 输入参数:	const IED* pIed	当前IED对象
	// 输出参数:	VirtualSvg& svg	生成的虚回路图模型
	// 返回值:	void
	//************************************
	void GenerateVirtualDiagramByIed(const IED* pIed, VirtualSvg& svg);
	//************************************
	// 函数名称:	TakeOverWholeSvg
	// 函数全名:	VirtualDiagramBuilder::TakeOverWholeSvg
	// 函数权限:	private
	// 函数说明:	接管全图模型中的虚回路图元
	// 输入参数:	WholeCircuitSvg& wholeSvg	源全图模型
	// 输出参数:	VirtualSvg& virtualSvg	接管后的虚回路图模型
	// 返回值:	void
	//************************************
	void TakeOverWholeSvg(WholeCircuitSvg& wholeSvg, VirtualSvg& virtualSvg);
	//************************************
	// 函数名称:	AppendPeerLogicLines
	// 函数全名:	VirtualDiagramBuilder::AppendPeerLogicLines
	// 函数权限:	private
	// 函数说明:	补充主IED与对端IED之间的逻辑线
	// 输入参数:	IedRect* pPeerIedRect	对端IED矩形
	// 输入参数:	IedRect* pMainIedRect	主IED矩形
	// 输入参数:	const QString& mainIedName	主IED名称
	// 输入参数:	const QString& peerIedName	对端IED名称
	// 输出参数:	无
	// 返回值:	void
	//************************************
	void AppendPeerLogicLines(IedRect* pPeerIedRect, IedRect* pMainIedRect, const QString& mainIedName, const QString& peerIedName);
	//************************************
	// 函数名称:	AdjustRectListPlateGap
	// 函数全名:	VirtualDiagramBuilder::AdjustRectListPlateGap
	// 函数权限:	private
	// 函数说明:	为IED列表追加压板外边距高度
	// 输入参数:	无
	// 输出参数:	QList<IedRect*>& rectList	待调整的IED矩形列表
	// 返回值:	void
	//************************************
	void AdjustRectListPlateGap(QList<IedRect*>& rectList) const;
	//************************************
	// 函数名称:	AdjustVirtualCircuitLinePosition
	// 函数全名:	VirtualDiagramBuilder::AdjustVirtualCircuitLinePosition
	// 函数权限:	private
	// 函数说明:	调整整张虚回路图的线路与图元位置
	// 输入参数:	无
	// 输出参数:	VirtualSvg& svg	待调整的虚回路图模型
	// 返回值:	void
	//************************************
	void AdjustVirtualCircuitLinePosition(VirtualSvg& svg);
	//************************************
	// 函数名称:	AdjustVirtualCircuitLinePosition
	// 函数全名:	VirtualDiagramBuilder::AdjustVirtualCircuitLinePosition
	// 函数权限:	private
	// 函数说明:	调整单侧IED列表的虚回路线与文本位置
	// 输入参数:	QList<IedRect*>& rectList	待处理的单侧IED列表
	// 输入参数:	IedRect* mainIed	主IED矩形
	// 输入参数:	bool isLeft	是否左侧IED列表
	// 输出参数:	VirtualSvg& svg	更新后的虚回路图模型
	// 返回值:	void
	//************************************
	void AdjustVirtualCircuitLinePosition(VirtualSvg& svg, QList<IedRect*>& rectList, IedRect* mainIed, bool isLeft = true);
	//************************************
	// 函数名称:	AdjustVirtualCircuitPlatePosition
	// 函数全名:	VirtualDiagramBuilder::AdjustVirtualCircuitPlatePosition
	// 函数权限:	private
	// 函数说明:	调整虚回路软压板的位置与高度
	// 输入参数:	const QString& iedName	压板归属IED键值
	// 输入参数:	const QPoint& linePt	线路连接点坐标
	// 输入参数:	const QString& plateDesc	压板描述
	// 输入参数:	const QString& plateRef	压板引用名
	// 输入参数:	quint64 plateCode	压板编码
	// 输入参数:	bool isSrcPlate	是否源端压板
	// 输入参数:	bool isSideSrc	侧边IED是否源端
	// 输入参数:	bool isLeft	是否左侧布局
	// 输出参数:	QHash<QString, PlateRect>& hash	压板矩形缓存表
	// 返回值:	void
	//************************************
	void AdjustVirtualCircuitPlatePosition(QHash<QString, PlateRect>& hash, const QString& iedName, const QPoint& linePt, const QString& plateDesc, const QString& plateRef, quint64 plateCode, bool isSrcPlate, bool isSideSrc, bool isLeft);
};
