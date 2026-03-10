#include "CircuitDiagramFactory.h"
#include "CircuitDiagramProxy.h"

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
