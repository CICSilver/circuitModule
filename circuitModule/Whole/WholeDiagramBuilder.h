#pragma once
#include "Common/DiagramBuilderBase.h"

class VirtualSvg;

class WholeDiagramBuilder : public DiagramBuilderBase
{
public:
	WholeDiagramBuilder();
	~WholeDiagramBuilder();
	//************************************
	// 函数名称:	BuildWholeDiagramByIedName
	// 函数全名:	WholeDiagramBuilder::BuildWholeDiagramByIedName
	// 函数权限:	public
	// 函数说明:	按IED名称生成全图模型
	// 输入参数:	const QString& iedName	IED名称
	// 输出参数:	无
	// 返回值:	WholeCircuitSvg*
	//************************************
	WholeCircuitSvg* BuildWholeDiagramByIedName(const QString& iedName);
	//************************************
	// 函数名称:	BuildWholeDiagramByBayName
	// 函数全名:	WholeDiagramBuilder::BuildWholeDiagramByBayName
	// 函数权限:	public
	// 函数说明:	按间隔名称生成全图模型
	// 输入参数:	const QString& bayName	间隔名称
	// 输出参数:	无
	// 返回值:	WholeCircuitSvg*
	//************************************
	WholeCircuitSvg* BuildWholeDiagramByBayName(const QString& bayName);
	WholeCircuitSvg* BuildWholeDiagramByBayName(const QString& bayName, int columnGap);

private:
	//************************************
	// 函数名称:	GenerateWholeDiagramByIed
	// 函数全名:	WholeDiagramBuilder::GenerateWholeDiagramByIed
	// 函数权限:	private
	// 函数说明:	根据IED对象生成全图模型
	// 输入参数:	const IED* pIed	当前IED对象
	// 输出参数:	WholeCircuitSvg& svg	生成的全图模型
	// 返回值:	void
	//************************************
	void GenerateWholeDiagramByIed(const IED* pIed, WholeCircuitSvg& svg);
	//************************************
	// 函数名称:	GenerateWholeDiagramByBay
	// 函数全名:	WholeDiagramBuilder::GenerateWholeDiagramByBay
	// 函数权限:	private
	// 函数说明:	根据间隔名称生成全图模型
	// 输入参数:	const QString& bayName	间隔名称
	// 输出参数:	WholeCircuitSvg& svg	生成的全图模型
	// 返回值:	void
	//************************************
	void GenerateWholeDiagramByBay(const QString& bayName, WholeCircuitSvg& svg);
	void GenerateWholeDiagramByBay(const QString& bayName, WholeCircuitSvg& svg, int columnGap);
	//************************************
	// 函数名称:	TakeOverVirtualSvg
	// 函数全名:	WholeDiagramBuilder::TakeOverVirtualSvg
	// 函数权限:	private
	// 函数说明:	接管虚回路图中的图元所有权
	// 输入参数:	VirtualSvg& virtualSvg	源虚回路图模型
	// 输出参数:	WholeCircuitSvg& wholeSvg	接管后的全图模型
	// 返回值:	void
	//************************************
	void TakeOverVirtualSvg(VirtualSvg& virtualSvg, WholeCircuitSvg& wholeSvg);
	//************************************
	// 函数名称:	RebuildVirtualLayout
	// 函数全名:	WholeDiagramBuilder::RebuildVirtualLayout
	// 函数权限:	private
	// 函数说明:	重建全图中的虚回路布局
	// 输入参数:	无
	// 输出参数:	WholeCircuitSvg& svg	待重建的全图模型
	// 返回值:	void
	//************************************
	void RebuildVirtualLayout(WholeCircuitSvg& svg);
	//************************************
	// 函数名称:	ExpandSideDistance
	// 函数全名:	WholeDiagramBuilder::ExpandSideDistance
	// 函数权限:	private
	// 函数说明:	按分组装饰需求扩展主IED与两侧IED间距
	// 输入参数:	无
	// 输出参数:	WholeCircuitSvg& svg	需要调整间距的全图模型
	// 返回值:	void
	//************************************
	void ExpandSideDistance(WholeCircuitSvg& svg);
	//************************************
	// 函数名称:	ShiftRectList
	// 函数全名:	WholeDiagramBuilder::ShiftRectList
	// 函数权限:	private
	// 函数说明:	整体平移一组IED矩形
	// 输入参数:	QList<IedRect*>& rectList	待平移的IED矩形列表
	// 输入参数:	int offsetX	水平偏移量
	// 输出参数:	无
	// 返回值:	void
	//************************************
	void ShiftRectList(QList<IedRect*>& rectList, int offsetX) const;
	//************************************
	// 函数名称:	ShiftAllIeds
	// 函数全名:	WholeDiagramBuilder::ShiftAllIeds
	// 函数权限:	private
	// 函数说明:	整体平移全图中的IED矩形
	// 输入参数:	int offsetX	水平偏移量
	// 输出参数:	WholeCircuitSvg& svg	待平移的全图模型
	// 返回值:	void
	//************************************
	void ShiftAllIeds(WholeCircuitSvg& svg, int offsetX) const;
	//************************************
	// 函数名称:	GetMaxRightEdge
	// 函数全名:	WholeDiagramBuilder::GetMaxRightEdge
	// 函数权限:	private
	// 函数说明:	获取全图图元的最右边界
	// 输入参数:	const WholeCircuitSvg& svg	待统计的全图模型
	// 输出参数:	无
	// 返回值:	qreal
	//************************************
	qreal GetMaxRightEdge(const WholeCircuitSvg& svg) const;
	//************************************
	// 函数名称:	GetMinLeftEdge
	// 函数全名:	WholeDiagramBuilder::GetMinLeftEdge
	// 函数权限:	private
	// 函数说明:	获取全图图元的最左边界
	// 输入参数:	const WholeCircuitSvg& svg	待统计的全图模型
	// 输出参数:	无
	// 返回值:	qreal
	//************************************
	qreal GetMinLeftEdge(const WholeCircuitSvg& svg) const;
	//************************************
	// 函数名称:	ResetVirtualState
	// 函数全名:	WholeDiagramBuilder::ResetVirtualState
	// 函数权限:	private
	// 函数说明:	重置虚回路重建前的连接状态与压板缓存
	// 输入参数:	无
	// 输出参数:	WholeCircuitSvg& svg	待重置的全图模型
	// 返回值:	void
	//************************************
	void ResetVirtualState(WholeCircuitSvg& svg);
	//************************************
	// 函数名称:	ResetConnectIndex
	// 函数全名:	WholeDiagramBuilder::ResetConnectIndex
	// 函数权限:	private
	// 函数说明:	重置单个IED矩形的连接点索引
	// 输入参数:	IedRect* pRect	待重置的IED矩形
	// 输出参数:	无
	// 返回值:	void
	//************************************
	void ResetConnectIndex(IedRect* pRect) const;
	//************************************
	// 函数名称:	ClearVirtualLines
	// 函数全名:	WholeDiagramBuilder::ClearVirtualLines
	// 函数权限:	private
	// 函数说明:	清空一组IED矩形上的虚回路线
	// 输入参数:	无
	// 输出参数:	QList<IedRect*>& rectList	待清空的IED矩形列表
	// 返回值:	void
	//************************************
	void ClearVirtualLines(QList<IedRect*>& rectList);
	//************************************
	// 函数名称:	RebuildVirtualLinesForRectList
	// 函数全名:	WholeDiagramBuilder::RebuildVirtualLinesForRectList
	// 函数权限:	private
	// 函数说明:	为单侧IED列表重建虚回路线
	// 输入参数:	QList<IedRect*>& rectList	待处理的单侧IED列表
	// 输入参数:	IedRect* pMainIed	主IED矩形
	// 输入参数:	bool isLeft	是否左侧IED列表
	// 输出参数:	WholeCircuitSvg& svg	更新后的全图模型
	// 返回值:	void
	//************************************
	void RebuildVirtualLinesForRectList(WholeCircuitSvg& svg, QList<IedRect*>& rectList, IedRect* pMainIed, bool isLeft);
	//************************************
	// 函数名称:	BuildVirtualLineGeometry
	// 函数全名:	WholeDiagramBuilder::BuildVirtualLineGeometry
	// 函数权限:	private
	// 函数说明:	计算单条虚回路的几何参数
	// 输入参数:	IedRect* pSideRect	侧边IED矩形
	// 输入参数:	IedRect* pMainIed	主IED矩形
	// 输入参数:	bool isLeft	是否位于主IED左侧
	// 输入参数:	bool isSideSource	侧边IED是否为源端
	// 输入参数:	bool groupedMode	是否使用分组布局
	// 输入参数:	int valWidth	测量值文本宽度
	// 输入参数:	int startOffset	起始偏移量
	// 输入参数:	VirtualCircuitLine* pVtLine	待填写的虚回路线对象
	// 输出参数:	int& startValX	起点测量值X坐标
	// 输出参数:	int& endValX	终点测量值X坐标
	// 输出参数:	int& descRectX	描述框X坐标
	// 输出参数:	int& descRectWidth	描述框宽度
	// 返回值:	void
	//************************************
	void BuildVirtualLineGeometry(IedRect* pSideRect, IedRect* pMainIed, bool isLeft, bool isSideSource, bool groupedMode, int valWidth, int startOffset, VirtualCircuitLine* pVtLine, int& startValX, int& endValX, int& descRectX, int& descRectWidth) const;
	//************************************
	// 函数名称:	AdjustVirtualPlatePosition
	// 函数全名:	WholeDiagramBuilder::AdjustVirtualPlatePosition
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
	void AdjustVirtualPlatePosition(QHash<QString, PlateRect>& hash, const QString& iedName, const QPoint& linePt, const QString& plateDesc, const QString& plateRef, quint64 plateCode, bool isSrcPlate, bool isSideSrc, bool isLeft);
	//************************************
	// 函数名称:	FillOpticalSidePorts
	// 函数全名:	WholeDiagramBuilder::FillOpticalSidePorts
	// 函数权限:	private
	// 函数说明:	提取经交换机场景一侧的端口文本
	// 输入参数:	const OpticalCircuit* pOpticalCircuit	光纤回路对象
	// 输入参数:	const QString& iedName	IED名称
	// 输出参数:	QString& iedPort	IED侧端口文本
	// 输出参数:	QString& switchPort	交换机侧端口文本
	// 返回值:	bool
	//************************************
	bool FillOpticalSidePorts(const OpticalCircuit* pOpticalCircuit, const QString& iedName, QString& iedPort, QString& switchPort) const;
	//************************************
	// 函数名称:	FillDirectPorts
	// 函数全名:	WholeDiagramBuilder::FillDirectPorts
	// 函数权限:	private
	// 函数说明:	提取直连场景两端端口文本
	// 输入参数:	const OpticalCircuit* pOpticalCircuit	光纤回路对象
	// 输入参数:	const QString& leftIedName	左侧IED名称
	// 输入参数:	const QString& rightIedName	右侧IED名称
	// 输出参数:	QString& leftPort	左侧端口文本
	// 输出参数:	QString& rightPort	右侧端口文本
	// 返回值:	bool
	//************************************
	bool FillDirectPorts(const OpticalCircuit* pOpticalCircuit, const QString& leftIedName, const QString& rightIedName, QString& leftPort, QString& rightPort) const;
	//************************************
	// 函数名称:	GetFirstValidVirtualCircuit
	// 函数全名:	WholeDiagramBuilder::GetFirstValidVirtualCircuit
	// 函数权限:	private
	// 函数说明:	获取逻辑线中的首个有效虚回路
	// 输入参数:	const LogicCircuitLine* pLogicLine	待检查的逻辑线
	// 输出参数:	无
	// 返回值:	VirtualCircuit*
	//************************************
	VirtualCircuit* GetFirstValidVirtualCircuit(const LogicCircuitLine* pLogicLine) const;
	//************************************
	// 函数名称:	GetGroupMode
	// 函数全名:	WholeDiagramBuilder::GetGroupMode
	// 函数权限:	private
	// 函数说明:	判定逻辑线的虚实分组模式
	// 输入参数:	const LogicCircuitLine* pLogicLine	待判定的逻辑线
	// 输出参数:	QString& switchIedName	分组中的交换机名称
	// 返回值:	WholeGroupMode
	//************************************
	WholeGroupMode GetGroupMode(const LogicCircuitLine* pLogicLine, QString& switchIedName) const;
	//************************************
	// 函数名称:	CreateIedRectWithSize
	// 函数全名:	WholeDiagramBuilder::CreateIedRectWithSize
	// 函数权限:	private
	// 函数说明:	按指定尺寸创建设备矩形
	// 输入参数:	const QString& iedName	IED名称
	// 输入参数:	int x	矩形左上角X坐标
	// 输入参数:	int y	矩形左上角Y坐标
	// 输入参数:	int width	矩形宽度
	// 输入参数:	int height	矩形高度
	// 输入参数:	quint32 borderColor	边框颜色
	// 输出参数:	无
	// 返回值:	IedRect*
	//************************************
	IedRect* CreateIedRectWithSize(const QString& iedName, int x, int y, int width, int height, quint32 borderColor) const;
	//************************************
	// 函数名称:	CreateLogicLine
	// 函数全名:	WholeDiagramBuilder::CreateLogicLine
	// 函数权限:	private
	// 函数说明:	创建一条逻辑连接线对象
	// 输入参数:	LogicCircuit* pLogicCircuit	逻辑回路对象
	// 输入参数:	IedRect* pSrcRect	源IED矩形
	// 输入参数:	IedRect* pDestRect	目标IED矩形
	// 输出参数:	无
	// 返回值:	LogicCircuitLine*
	//************************************
	LogicCircuitLine* CreateLogicLine(LogicCircuit* pLogicCircuit, IedRect* pSrcRect, IedRect* pDestRect) const;
	//************************************
	// 函数名称:	AdjustRectExtendHeight
	// 函数全名:	WholeDiagramBuilder::AdjustRectExtendHeight
	// 函数权限:	private
	// 函数说明:	按回路数量调整矩形扩展高度
	// 输入参数:	IedRect* pRect	待调整的IED矩形
	// 输出参数:	无
	// 返回值:	void
	//************************************
	void AdjustRectExtendHeight(IedRect* pRect) const;
	//************************************
	// 函数名称:	GetRectOuterBottom
	// 函数全名:	WholeDiagramBuilder::GetRectOuterBottom
	// 函数权限:	private
	// 函数说明:	获取矩形外扩后的底部坐标
	// 输入参数:	const IedRect* pRect	待计算的IED矩形
	// 输出参数:	无
	// 返回值:	int
	//************************************
	int GetRectOuterBottom(const IedRect* pRect) const;
	//************************************
	// 函数名称:	AttachBayTreeLogicLines
	// 函数全名:	WholeDiagramBuilder::AttachBayTreeLogicLines
	// 函数权限:	private
	// 函数说明:	挂接bay树节点与父节点间的逻辑线
	// 输入参数:	struct WholeBayTreeNode* pNode	待挂线的bay树节点
	// 输出参数:	无
	// 返回值:	void
	//************************************
	void AttachBayTreeLogicLines(struct WholeBayTreeNode* pNode);
	//************************************
	// 函数名称:	BuildBayTreeVirtualLinesForRect
	// 函数全名:	WholeDiagramBuilder::BuildBayTreeVirtualLinesForRect
	// 函数权限:	private
	// 函数说明:	为bay树节点构建虚回路线
	// 输入参数:	IedRect* pOwnerRect	当前bay树节点矩形
	// 输出参数:	WholeCircuitSvg& svg	更新后的全图模型
	// 返回值:	void
	//************************************
	void BuildBayTreeVirtualLinesForRect(WholeCircuitSvg& svg, IedRect* pOwnerRect);
	//************************************
	// 函数名称:	BuildGroupDecor
	// 函数全名:	WholeDiagramBuilder::BuildGroupDecor
	// 函数权限:	private
	// 函数说明:	构建全图中的分组装饰
	// 输入参数:	无
	// 输出参数:	WholeCircuitSvg& svg	更新后的全图模型
	// 返回值:	void
	//************************************
	void BuildGroupDecor(WholeCircuitSvg& svg);
	//************************************
	// 函数名称:	BuildGroupDecorByLogicLine
	// 函数全名:	WholeDiagramBuilder::BuildGroupDecorByLogicLine
	// 函数权限:	private
	// 函数说明:	按单条逻辑线构建分组装饰
	// 输入参数:	LogicCircuitLine* pLogicLine	当前逻辑线对象
	// 输出参数:	WholeCircuitSvg& svg	更新后的全图模型
	// 返回值:	void
	//************************************
	void BuildGroupDecorByLogicLine(WholeCircuitSvg& svg, LogicCircuitLine* pLogicLine);
	//************************************
	// 函数名称:	BuildGroupPortTexts
	// 函数全名:	WholeDiagramBuilder::BuildGroupPortTexts
	// 函数权限:	private
	// 函数说明:	生成分组装饰中的端口文本
	// 输入参数:	const LogicCircuitLine* pLogicLine	当前逻辑线对象
	// 输出参数:	WholeGroupDecor* pGroupDecor	待填写的分组装饰对象
	// 返回值:	void
	//************************************
	void BuildGroupPortTexts(WholeGroupDecor* pGroupDecor, const LogicCircuitLine* pLogicLine);
	//************************************
	// 函数名称:	BuildGroupPortLayout
	// 函数全名:	WholeDiagramBuilder::BuildGroupPortLayout
	// 函数权限:	private
	// 函数说明:	计算分组装饰端口文本布局
	// 输入参数:	无
	// 输出参数:	WholeGroupDecor* pGroupDecor	待更新布局的分组装饰对象
	// 返回值:	void
	//************************************
	void BuildGroupPortLayout(WholeGroupDecor* pGroupDecor) const;
	//************************************
	// 函数名称:	IsGroupDirectionRight
	// 函数全名:	WholeDiagramBuilder::IsGroupDirectionRight
	// 函数权限:	private
	// 函数说明:	判断分组箭头方向是否朝右
	// 输入参数:	const LogicCircuitLine* pLogicLine	待判断的逻辑线对象
	// 输出参数:	无
	// 返回值:	bool
	//************************************
	bool IsGroupDirectionRight(const LogicCircuitLine* pLogicLine) const;
};
