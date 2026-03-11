#include "CircuitDiagramFactory.h"
#include "CircuitDiagramProxy.h"
#include "Logical/LogicDiagramBuilder.h"
#include "Optical/OpticalDiagramBuilder.h"
#include "Virtual/VirtualDiagramBuilder.h"
#include "Whole/WholeDiagramBuilder.h"

CircuitDiagramProxy* CircuitDiagramFactory::CreateDiagramProxy()
{
	return new CircuitDiagramProxy();
}

void CircuitDiagramFactory::ReleaseDiagramProxy(CircuitDiagramProxy* pProxy)
{
	if (pProxy)
	{
		delete pProxy;
	}
}

LogicDiagramBuilder* CircuitDiagramFactory::CreateLogicDiagramBuilder()
{
	return new LogicDiagramBuilder();
}

void CircuitDiagramFactory::ReleaseLogicDiagramBuilder(LogicDiagramBuilder* pBuilder)
{
	if (pBuilder)
	{
		delete pBuilder;
	}
}

OpticalDiagramBuilder* CircuitDiagramFactory::CreateOpticalDiagramBuilder()
{
	return new OpticalDiagramBuilder();
}

void CircuitDiagramFactory::ReleaseOpticalDiagramBuilder(OpticalDiagramBuilder* pBuilder)
{
	if (pBuilder)
	{
		delete pBuilder;
	}
}

VirtualDiagramBuilder* CircuitDiagramFactory::CreateVirtualDiagramBuilder()
{
	return new VirtualDiagramBuilder();
}

void CircuitDiagramFactory::ReleaseVirtualDiagramBuilder(VirtualDiagramBuilder* pBuilder)
{
	if (pBuilder)
	{
		delete pBuilder;
	}
}

WholeDiagramBuilder* CircuitDiagramFactory::CreateWholeDiagramBuilder()
{
	return new WholeDiagramBuilder();
}

void CircuitDiagramFactory::ReleaseWholeDiagramBuilder(WholeDiagramBuilder* pBuilder)
{
	if (pBuilder)
	{
		delete pBuilder;
	}
}
