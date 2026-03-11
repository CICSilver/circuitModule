#pragma once
#include "Common/DiagramBuilderBase.h"

class WholeDiagramBuilder : public DiagramBuilderBase
{
public:
	//************************************
	// 函数名称:	WholeDiagramBuilder
	// 函数全名:	WholeDiagramBuilder::WholeDiagramBuilder
	// 函数权限:	public
	// 函数说明:	初始化虚实回路建图器
	// 函数参数:	无
	// 函数返回:	无
	//************************************
	WholeDiagramBuilder();
	//************************************
	// 函数名称:	~WholeDiagramBuilder
	// 函数全名:	WholeDiagramBuilder::~WholeDiagramBuilder
	// 函数权限:	public
	// 函数说明:	释放虚实回路建图器
	// 函数参数:	无
	// 函数返回:	无
	//************************************
	~WholeDiagramBuilder();
	//************************************
	// 函数名称:	BuildWholeDiagramByIedName
	// 函数全名:	WholeDiagramBuilder::BuildWholeDiagramByIedName
	// 函数权限:	public
	// 函数说明:	按IED名称生成虚实回路图模型
	// 函数参数:	const QString& iedName
	// 函数返回:	WholeCircuitSvg*
	//************************************
	WholeCircuitSvg* BuildWholeDiagramByIedName(const QString& iedName);

private:
	//************************************
	// 函数名称:	GenerateWholeDiagramByIed
	// 函数全名:	WholeDiagramBuilder::GenerateWholeDiagramByIed
	// 函数权限:	private
	// 函数说明:	根据IED填充虚实回路图模型
	// 函数参数:	const IED* pIed
	// 函数参数:	WholeCircuitSvg& svg
	// 函数返回:	void
	//************************************
	void GenerateWholeDiagramByIed(const IED* pIed, WholeCircuitSvg& svg);
};
