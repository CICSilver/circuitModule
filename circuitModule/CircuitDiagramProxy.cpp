#include "CircuitDiagramProxy.h"
#include "CircuitDiagramFactory.h"
#include "Logical/LogicDiagramBuilder.h"
#include "Optical/OpticalDiagramBuilder.h"
#include "Virtual/VirtualDiagramBuilder.h"
#include "Whole/WholeDiagramBuilder.h"

CircuitDiagramProxy::CircuitDiagramProxy()
{
	m_pLogicBuilder = CircuitDiagramFactory::CreateLogicDiagramBuilder();
	m_pOpticalBuilder = CircuitDiagramFactory::CreateOpticalDiagramBuilder();
	m_pVirtualBuilder = CircuitDiagramFactory::CreateVirtualDiagramBuilder();
	m_pWholeBuilder = CircuitDiagramFactory::CreateWholeDiagramBuilder();
}

CircuitDiagramProxy::~CircuitDiagramProxy()
{
	CircuitDiagramFactory::ReleaseLogicDiagramBuilder(m_pLogicBuilder);
	CircuitDiagramFactory::ReleaseOpticalDiagramBuilder(m_pOpticalBuilder);
	CircuitDiagramFactory::ReleaseVirtualDiagramBuilder(m_pVirtualBuilder);
	CircuitDiagramFactory::ReleaseWholeDiagramBuilder(m_pWholeBuilder);
	m_pLogicBuilder = NULL;
	m_pOpticalBuilder = NULL;
	m_pVirtualBuilder = NULL;
	m_pWholeBuilder = NULL;
}

LogicDiagramModel* CircuitDiagramProxy::BuildLogicDiagramByIedName(const QString& iedName)
{
	if (!m_pLogicBuilder)
	{
		return NULL;
	}
	return m_pLogicBuilder->BuildLogicDiagramByIedName(iedName);
}

LogicDiagramModel* CircuitDiagramProxy::BuildLogicDiagramByBayName(const QString& bayName)
{
	if (!m_pLogicBuilder)
	{
		return NULL;
	}
	return m_pLogicBuilder->BuildLogicDiagramByBayName(bayName);
}

OpticalDiagramModel* CircuitDiagramProxy::BuildOpticalDiagramByIedName(const QString& iedName)
{
	if (!m_pOpticalBuilder)
	{
		return NULL;
	}
	return m_pOpticalBuilder->BuildOpticalDiagramByIedName(iedName);
}

OpticalDiagramModel* CircuitDiagramProxy::BuildOpticalDiagramByStation()
{
	if (!m_pOpticalBuilder)
	{
		return NULL;
	}
	return m_pOpticalBuilder->BuildOpticalDiagramByStation();
}

OpticalDiagramModel* CircuitDiagramProxy::BuildOpticalDiagramByBayName(const QString& bayName)
{
	if (!m_pOpticalBuilder)
	{
		return NULL;
	}
	return m_pOpticalBuilder->BuildOpticalDiagramByBayName(bayName);
}

VirtualDiagramModel* CircuitDiagramProxy::BuildVirtualDiagramByIedName(const QString& iedName)
{
	if (!m_pVirtualBuilder)
	{
		return NULL;
	}
	return m_pVirtualBuilder->BuildVirtualDiagramByIedName(iedName);
}

WholeDiagramModel* CircuitDiagramProxy::BuildWholeDiagramByIedName(const QString& iedName)
{
	if (!m_pWholeBuilder)
	{
		return NULL;
	}
	return m_pWholeBuilder->BuildWholeDiagramByIedName(iedName);
}
