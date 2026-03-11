#pragma once
#include "Common/DiagramBuilderBase.h"

class OpticalDiagramBuilder : public DiagramBuilderBase
{
public:
	//************************************
	// 函数名称:	OpticalDiagramBuilder
	// 函数全名:	OpticalDiagramBuilder::OpticalDiagramBuilder
	// 函数权限:	public
	// 函数说明:	初始化实回路建图器
	// 函数参数:	无
	// 函数返回:	无
	//************************************
	OpticalDiagramBuilder();
	//************************************
	// 函数名称:	~OpticalDiagramBuilder
	// 函数全名:	OpticalDiagramBuilder::~OpticalDiagramBuilder
	// 函数权限:	public
	// 函数说明:	释放实回路建图器
	// 函数参数:	无
	// 函数返回:	无
	//************************************
	~OpticalDiagramBuilder();
	//************************************
	// 函数名称:	BuildOpticalDiagramByIedName
	// 函数全名:	OpticalDiagramBuilder::BuildOpticalDiagramByIedName
	// 函数权限:	public
	// 函数说明:	按IED名称生成实回路图模型
	// 函数参数:	const QString& iedName
	// 函数返回:	OpticalSvg*
	//************************************
	OpticalSvg* BuildOpticalDiagramByIedName(const QString& iedName);
	//************************************
	// 函数名称:	BuildOpticalDiagramByStation
	// 函数全名:	OpticalDiagramBuilder::BuildOpticalDiagramByStation
	// 函数权限:	public
	// 函数说明:	生成站域实回路图模型
	// 函数参数:	无
	// 函数返回:	OpticalSvg*
	//************************************
	OpticalSvg* BuildOpticalDiagramByStation();
	//************************************
	// 函数名称:	BuildOpticalDiagramByBayName
	// 函数全名:	OpticalDiagramBuilder::BuildOpticalDiagramByBayName
	// 函数权限:	public
	// 函数说明:	按间隔名称生成实回路图模型
	// 函数参数:	const QString& bayName
	// 函数返回:	OpticalSvg*
	//************************************
	OpticalSvg* BuildOpticalDiagramByBayName(const QString& bayName);

private:
	//************************************
	// 函数名称:	GenerateOpticalDiagramByIed
	// 函数全名:	OpticalDiagramBuilder::GenerateOpticalDiagramByIed
	// 函数权限:	private
	// 函数说明:	根据IED填充单设备实回路图模型
	// 函数参数:	const IED* pIed
	// 函数参数:	OpticalSvg& svg
	// 函数返回:	void
	//************************************
	void GenerateOpticalDiagramByIed(const IED* pIed, OpticalSvg& svg);
	//************************************
	// 函数名称:	SetArrowStateDirect
	// 函数全名:	OpticalDiagramBuilder::SetArrowStateDirect
	// 函数权限:	private
	// 函数说明:	根据逻辑流向设置直连光纤箭头方向
	// 函数参数:	const QList<LogicCircuit*>& inList
	// 函数参数:	const QList<LogicCircuit*>& outList
	// 函数参数:	const QString& mainIedName
	// 函数参数:	const QString& peerIedName
	// 函数参数:	OpticalCircuitLine* pLine
	// 函数返回:	void
	//************************************
	void SetArrowStateDirect(const QList<LogicCircuit*>& inList, const QList<LogicCircuit*>& outList, const QString& mainIedName, const QString& peerIedName, OpticalCircuitLine* pLine);
	//************************************
	// 函数名称:	SetArrowStateThroughSwitch
	// 函数全名:	OpticalDiagramBuilder::SetArrowStateThroughSwitch
	// 函数权限:	private
	// 函数说明:	根据交换机中转关系设置两段光纤箭头方向
	// 函数参数:	const QList<LogicCircuit*>& inList
	// 函数参数:	const QList<LogicCircuit*>& outList
	// 函数参数:	const QString& mainIedName
	// 函数参数:	const QString& peerIedName
	// 函数参数:	OpticalCircuitLine* pOppLine
	// 函数参数:	OpticalCircuitLine* pMainSwitchLine
	// 函数返回:	void
	//************************************
	void SetArrowStateThroughSwitch(const QList<LogicCircuit*>& inList, const QList<LogicCircuit*>& outList, const QString& mainIedName, const QString& peerIedName, OpticalCircuitLine* pOppLine, OpticalCircuitLine* pMainSwitchLine);
};
