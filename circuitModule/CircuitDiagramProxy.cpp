#include "CircuitDiagramProxy.h"
#include "SvgTransformer.h"

CircuitDiagramProxy::CircuitDiagramProxy()
{
	m_pTransformer = new SvgTransformer();
}

CircuitDiagramProxy::~CircuitDiagramProxy()
{
	if (m_pTransformer)
	{
		delete m_pTransformer;
	}
	m_pTransformer = NULL;
}

LogicDiagramModel* CircuitDiagramProxy::BuildLogicDiagramByIedName(const QString& iedName)
{
	return m_pTransformer->BuildLogicModelByIedName(iedName);
}

OpticalDiagramModel* CircuitDiagramProxy::BuildOpticalDiagramByIedName(const QString& iedName)
{
	return m_pTransformer->BuildOpticalModelByIedName(iedName);
}

OpticalDiagramModel* CircuitDiagramProxy::BuildOpticalDiagramByStation()
{
	return m_pTransformer->BuildOpticalModelByStation();
}

OpticalDiagramModel* CircuitDiagramProxy::BuildOpticalDiagramByBayName(const QString& bayName)
{
	return m_pTransformer->BuildOpticalModelByBayName(bayName);
}

VirtualDiagramModel* CircuitDiagramProxy::BuildVirtualDiagramByIedName(const QString& iedName)
{
	return m_pTransformer->BuildVirtualModelByIedName(iedName);
}

WholeDiagramModel* CircuitDiagramProxy::BuildWholeDiagramByIedName(const QString& iedName)
{
	return m_pTransformer->BuildWholeCircuitModelByIedName(iedName);
}
