#pragma once
#include <QtGlobal>

class CircuitDiagramProxy;
class LogicDiagramBuilder;
class OpticalDiagramBuilder;
class VirtualDiagramBuilder;
class WholeDiagramBuilder;

class CircuitDiagramFactory
{
public:
	//************************************
	// 函数名称:	CreateDiagramProxy
	// 函数全名:	CircuitDiagramFactory::CreateDiagramProxy
	// 函数权限:	public
	// 函数说明:	创建对外建图代理
	// 函数参数:	无
	// 函数返回:	CircuitDiagramProxy*
	//************************************
	static CircuitDiagramProxy* CreateDiagramProxy();
	//************************************
	// 函数名称:	ReleaseDiagramProxy
	// 函数全名:	CircuitDiagramFactory::ReleaseDiagramProxy
	// 函数权限:	public
	// 函数说明:	释放对外建图代理
	// 函数参数:	CircuitDiagramProxy* pProxy
	// 函数返回:	void
	//************************************
	static void ReleaseDiagramProxy(CircuitDiagramProxy* pProxy);
	//************************************
	// 函数名称:	CreateLogicDiagramBuilder
	// 函数全名:	CircuitDiagramFactory::CreateLogicDiagramBuilder
	// 函数权限:	public
	// 函数说明:	创建逻辑图建图器
	// 函数参数:	无
	// 函数返回:	LogicDiagramBuilder*
	//************************************
	static LogicDiagramBuilder* CreateLogicDiagramBuilder();
	//************************************
	// 函数名称:	ReleaseLogicDiagramBuilder
	// 函数全名:	CircuitDiagramFactory::ReleaseLogicDiagramBuilder
	// 函数权限:	public
	// 函数说明:	释放逻辑图建图器
	// 函数参数:	LogicDiagramBuilder* pBuilder
	// 函数返回:	void
	//************************************
	static void ReleaseLogicDiagramBuilder(LogicDiagramBuilder* pBuilder);
	//************************************
	// 函数名称:	CreateOpticalDiagramBuilder
	// 函数全名:	CircuitDiagramFactory::CreateOpticalDiagramBuilder
	// 函数权限:	public
	// 函数说明:	创建实回路建图器
	// 函数参数:	无
	// 函数返回:	OpticalDiagramBuilder*
	//************************************
	static OpticalDiagramBuilder* CreateOpticalDiagramBuilder();
	//************************************
	// 函数名称:	ReleaseOpticalDiagramBuilder
	// 函数全名:	CircuitDiagramFactory::ReleaseOpticalDiagramBuilder
	// 函数权限:	public
	// 函数说明:	释放实回路建图器
	// 函数参数:	OpticalDiagramBuilder* pBuilder
	// 函数返回:	void
	//************************************
	static void ReleaseOpticalDiagramBuilder(OpticalDiagramBuilder* pBuilder);
	//************************************
	// 函数名称:	CreateVirtualDiagramBuilder
	// 函数全名:	CircuitDiagramFactory::CreateVirtualDiagramBuilder
	// 函数权限:	public
	// 函数说明:	创建虚回路建图器
	// 函数参数:	无
	// 函数返回:	VirtualDiagramBuilder*
	//************************************
	static VirtualDiagramBuilder* CreateVirtualDiagramBuilder();
	//************************************
	// 函数名称:	ReleaseVirtualDiagramBuilder
	// 函数全名:	CircuitDiagramFactory::ReleaseVirtualDiagramBuilder
	// 函数权限:	public
	// 函数说明:	释放虚回路建图器
	// 函数参数:	VirtualDiagramBuilder* pBuilder
	// 函数返回:	void
	//************************************
	static void ReleaseVirtualDiagramBuilder(VirtualDiagramBuilder* pBuilder);
	//************************************
	// 函数名称:	CreateWholeDiagramBuilder
	// 函数全名:	CircuitDiagramFactory::CreateWholeDiagramBuilder
	// 函数权限:	public
	// 函数说明:	创建虚实回路建图器
	// 函数参数:	无
	// 函数返回:	WholeDiagramBuilder*
	//************************************
	static WholeDiagramBuilder* CreateWholeDiagramBuilder();
	//************************************
	// 函数名称:	ReleaseWholeDiagramBuilder
	// 函数全名:	CircuitDiagramFactory::ReleaseWholeDiagramBuilder
	// 函数权限:	public
	// 函数说明:	释放虚实回路建图器
	// 函数参数:	WholeDiagramBuilder* pBuilder
	// 函数返回:	void
	//************************************
	static void ReleaseWholeDiagramBuilder(WholeDiagramBuilder* pBuilder);

private:
	CircuitDiagramFactory();
	~CircuitDiagramFactory();
	Q_DISABLE_COPY(CircuitDiagramFactory)
};
