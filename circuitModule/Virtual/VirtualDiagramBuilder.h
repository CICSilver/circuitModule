#pragma once
#include "Common/DiagramBuilderBase.h"

class VirtualDiagramBuilder : public DiagramBuilderBase
{
public:
	//************************************
	// 函数名称:	VirtualDiagramBuilder
	// 函数全名:	VirtualDiagramBuilder::VirtualDiagramBuilder
	// 函数权限:	public
	// 函数说明:	初始化虚回路建图器
	// 函数参数:	无
	// 函数返回:	无
	//************************************
	VirtualDiagramBuilder();
	//************************************
	// 函数名称:	~VirtualDiagramBuilder
	// 函数全名:	VirtualDiagramBuilder::~VirtualDiagramBuilder
	// 函数权限:	public
	// 函数说明:	释放虚回路建图器
	// 函数参数:	无
	// 函数返回:	无
	//************************************
	~VirtualDiagramBuilder();
	//************************************
	// 函数名称:	BuildVirtualDiagramByIedName
	// 函数全名:	VirtualDiagramBuilder::BuildVirtualDiagramByIedName
	// 函数权限:	public
	// 函数说明:	按IED名称生成虚回路图模型
	// 函数参数:	const QString& iedName
	// 函数返回:	VirtualSvg*
	//************************************
	VirtualSvg* BuildVirtualDiagramByIedName(const QString& iedName);
	VirtualSvg* BuildVirtualDiagramByIedPair(const QString& mainIedName, const QString& peerIedName);

private:
	//************************************
	// 函数名称:	GenerateVirtualDiagramByIed
	// 函数全名:	VirtualDiagramBuilder::GenerateVirtualDiagramByIed
	// 函数权限:	private
	// 函数说明:	根据IED填充虚回路图模型
	// 函数参数:	const IED* pIed
	// 函数参数:	VirtualSvg& svg
	// 函数返回:	void
	//************************************
	void GenerateVirtualDiagramByIed(const IED* pIed, VirtualSvg& svg);
	//************************************
	// 函数名称:	AdjustVirtualCircuitLinePosition
	// 函数全名:	VirtualDiagramBuilder::AdjustVirtualCircuitLinePosition
	// 函数权限:	private
	// 函数说明:	批量调整虚回路线和附属图元位置
	// 函数参数:	VirtualSvg& svg
	// 函数返回:	void
	//************************************
	void AdjustVirtualCircuitLinePosition(VirtualSvg& svg);
	//************************************
	// 函数名称:	AdjustVirtualCircuitLinePosition
	// 函数全名:	VirtualDiagramBuilder::AdjustVirtualCircuitLinePosition
	// 函数权限:	private
	// 函数说明:	调整单侧IED的虚回路线和文本布局
	// 函数参数:	VirtualSvg& svg
	// 函数参数:	QList<IedRect*>& rectList
	// 函数参数:	IedRect* mainIed
	// 函数参数:	bool isLeft
	// 函数返回:	void
	//************************************
	void AdjustVirtualCircuitLinePosition(VirtualSvg& svg, QList<IedRect*>& rectList, IedRect* mainIed, bool isLeft = true);
	//************************************
	// 函数名称:	AdjustVirtualCircuitPlatePosition
	// 函数全名:	VirtualDiagramBuilder::AdjustVirtualCircuitPlatePosition
	// 函数权限:	private
	// 函数说明:	根据线路端点更新压板位置和高度
	// 函数参数:	QHash<QString, PlateRect>& hash
	// 函数参数:	const QString& iedName
	// 函数参数:	const QPoint& linePt
	// 函数参数:	const QString& plateDesc
	// 函数参数:	const QString& plateRef
	// 函数参数:	quint64 plateCode
	// 函数参数:	bool isSrcPlate
	// 函数参数:	bool isSideSrc
	// 函数参数:	bool isLeft
	// 函数返回:	void
	//************************************
	void AdjustVirtualCircuitPlatePosition(QHash<QString, PlateRect>& hash, const QString& iedName, const QPoint& linePt, const QString& plateDesc, const QString& plateRef, quint64 plateCode, bool isSrcPlate, bool isSideSrc, bool isLeft);
};
