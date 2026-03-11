#pragma once
#include "Common/DiagramBuilderBase.h"

class LogicDiagramBuilder : public DiagramBuilderBase
{
public:
	//************************************
	// 函数名称:	LogicDiagramBuilder
	// 函数全名:	LogicDiagramBuilder::LogicDiagramBuilder
	// 函数权限:	public
	// 函数说明:	初始化逻辑图构建器
	// 输入参数:	无
	// 输出参数:	无
	// 返回值:	无
	//************************************
	LogicDiagramBuilder();
	//************************************
	// 函数名称:	~LogicDiagramBuilder
	// 函数全名:	LogicDiagramBuilder::~LogicDiagramBuilder
	// 函数权限:	public
	// 函数说明:	释放逻辑图构建器
	// 输入参数:	无
	// 输出参数:	无
	// 返回值:	无
	//************************************
	~LogicDiagramBuilder();
	//************************************
	// 函数名称:	BuildLogicDiagramByIedName
	// 函数全名:	LogicDiagramBuilder::BuildLogicDiagramByIedName
	// 函数权限:	public
	// 函数说明:	按IED名称生成逻辑图模型
	// 输入参数:	const QString& iedName
	// 输出参数:	无
	// 返回值:	LogicSvg*
	//************************************
	LogicSvg* BuildLogicDiagramByIedName(const QString& iedName);
	//************************************
	// 函数名称:	BuildLogicDiagramByBayName
	// 函数全名:	LogicDiagramBuilder::BuildLogicDiagramByBayName
	// 函数权限:	public
	// 函数说明:	按间隔名称生成逻辑图模型
	// 输入参数:	const QString& bayName
	// 输出参数:	无
	// 返回值:	LogicSvg*
	//************************************
	LogicSvg* BuildLogicDiagramByBayName(const QString& bayName);

private:
	//************************************
	// 函数名称:	GenerateLogicDiagramByIed
	// 函数全名:	LogicDiagramBuilder::GenerateLogicDiagramByIed
	// 函数权限:	private
	// 函数说明:	生成单IED逻辑图模型
	// 输入参数:	const IED* pIed
	// 输入参数:	LogicSvg& svg
	// 输出参数:	无
	// 返回值:	void
	//************************************
	void GenerateLogicDiagramByIed(const IED* pIed, LogicSvg& svg);
	//************************************
	// 函数名称:	GenerateLogicDiagramByBay
	// 函数全名:	LogicDiagramBuilder::GenerateLogicDiagramByBay
	// 函数权限:	private
	// 函数说明:	生成间隔逻辑图模型
	// 输入参数:	const QString& bayName
	// 输入参数:	LogicSvg& svg
	// 输出参数:	无
	// 返回值:	void
	//************************************
	void GenerateLogicDiagramByBay(const QString& bayName, LogicSvg& svg);
	//************************************
	// 函数名称:	AdjustLogicCircuitLinePosition
	// 函数全名:	LogicDiagramBuilder::AdjustLogicCircuitLinePosition
	// 函数权限:	private
	// 函数说明:	调整单IED逻辑图连线位置
	// 输入参数:	LogicSvg& svg
	// 输出参数:	无
	// 返回值:	void
	//************************************
	void AdjustLogicCircuitLinePosition(LogicSvg& svg);
};
