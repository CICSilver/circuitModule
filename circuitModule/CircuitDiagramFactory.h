#pragma once
#include <QtGlobal>

class CircuitDiagramProxy;

class CircuitDiagramFactory
{
public:
	static CircuitDiagramProxy* CreateDiagramProxy();
	static void ReleaseDiagramProxy(CircuitDiagramProxy* pProxy);

private:
	CircuitDiagramFactory();
	~CircuitDiagramFactory();
	Q_DISABLE_COPY(CircuitDiagramFactory)
};
