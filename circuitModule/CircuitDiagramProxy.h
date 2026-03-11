#pragma once
#include "svgmodel.h"
#include <QString>
#include <QtGlobal>

class LogicDiagramBuilder;
class OpticalDiagramBuilder;
class VirtualDiagramBuilder;
class WholeDiagramBuilder;

typedef LogicSvg LogicDiagramModel;
typedef OpticalSvg OpticalDiagramModel;
typedef VirtualSvg VirtualDiagramModel;
typedef WholeCircuitSvg WholeDiagramModel;

class CircuitDiagramProxy
{
public:
	//************************************
	// 函数名称:	CircuitDiagramProxy
	// 函数全名:	CircuitDiagramProxy::CircuitDiagramProxy
	// 函数权限:	public
	// 函数说明:	初始化各类图模型构建器
	// 输入参数:	无
	// 输出参数:	无
	// 返回值:	无
	//************************************
	CircuitDiagramProxy();
	//************************************
	// 函数名称:	~CircuitDiagramProxy
	// 函数全名:	CircuitDiagramProxy::~CircuitDiagramProxy
	// 函数权限:	public
	// 函数说明:	释放各类图模型构建器
	// 输入参数:	无
	// 输出参数:	无
	// 返回值:	无
	//************************************
	~CircuitDiagramProxy();
	//************************************
	// 函数名称:	BuildLogicDiagramByIedName
	// 函数全名:	CircuitDiagramProxy::BuildLogicDiagramByIedName
	// 函数权限:	public
	// 函数说明:	按IED名称生成逻辑图模型
	// 输入参数:	const QString& iedName
	// 输出参数:	无
	// 返回值:	LogicDiagramModel*
	//************************************
	LogicDiagramModel* BuildLogicDiagramByIedName(const QString& iedName);
	//************************************
	// 函数名称:	BuildLogicDiagramByBayName
	// 函数全名:	CircuitDiagramProxy::BuildLogicDiagramByBayName
	// 函数权限:	public
	// 函数说明:	按间隔名称生成逻辑图模型
	// 输入参数:	const QString& bayName
	// 输出参数:	无
	// 返回值:	LogicDiagramModel*
	//************************************
	LogicDiagramModel* BuildLogicDiagramByBayName(const QString& bayName);
	//************************************
	// 函数名称:	BuildOpticalDiagramByIedName
	// 函数全名:	CircuitDiagramProxy::BuildOpticalDiagramByIedName
	// 函数权限:	public
	// 函数说明:	按IED名称生成光纤图模型
	// 输入参数:	const QString& iedName
	// 输出参数:	无
	// 返回值:	OpticalDiagramModel*
	//************************************
	OpticalDiagramModel* BuildOpticalDiagramByIedName(const QString& iedName);
	//************************************
	// 函数名称:	BuildOpticalDiagramByStation
	// 函数全名:	CircuitDiagramProxy::BuildOpticalDiagramByStation
	// 函数权限:	public
	// 函数说明:	生成全站光纤图模型
	// 输入参数:	无
	// 输出参数:	无
	// 返回值:	OpticalDiagramModel*
	//************************************
	OpticalDiagramModel* BuildOpticalDiagramByStation();
	//************************************
	// 函数名称:	BuildOpticalDiagramByBayName
	// 函数全名:	CircuitDiagramProxy::BuildOpticalDiagramByBayName
	// 函数权限:	public
	// 函数说明:	按间隔名称生成光纤图模型
	// 输入参数:	const QString& bayName
	// 输出参数:	无
	// 返回值:	OpticalDiagramModel*
	//************************************
	OpticalDiagramModel* BuildOpticalDiagramByBayName(const QString& bayName);
	//************************************
	// 函数名称:	BuildVirtualDiagramByIedName
	// 函数全名:	CircuitDiagramProxy::BuildVirtualDiagramByIedName
	// 函数权限:	public
	// 函数说明:	按IED名称生成虚回路图模型
	// 输入参数:	const QString& iedName
	// 输出参数:	无
	// 返回值:	VirtualDiagramModel*
	//************************************
	VirtualDiagramModel* BuildVirtualDiagramByIedName(const QString& iedName);
	//************************************
	// 函数名称:	BuildWholeDiagramByIedName
	// 函数全名:	CircuitDiagramProxy::BuildWholeDiagramByIedName
	// 函数权限:	public
	// 函数说明:	按IED名称生成完整二次回路图模型
	// 输入参数:	const QString& iedName
	// 输出参数:	无
	// 返回值:	WholeDiagramModel*
	//************************************
	WholeDiagramModel* BuildWholeDiagramByIedName(const QString& iedName);

private:
	Q_DISABLE_COPY(CircuitDiagramProxy)
	LogicDiagramBuilder* m_pLogicBuilder;
	OpticalDiagramBuilder* m_pOpticalBuilder;
	VirtualDiagramBuilder* m_pVirtualBuilder;
	WholeDiagramBuilder* m_pWholeBuilder;
};
