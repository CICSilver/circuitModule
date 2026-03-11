#include "SvgTransformer.h"
#include "Logical/LogicDiagramBuilder.h"
#include "Optical/OpticalDiagramBuilder.h"
#include "Virtual/VirtualDiagramBuilder.h"
#include "Whole/WholeDiagramBuilder.h"

SvgTransformer::SvgTransformer()
{
	m_circuitConfig = CircuitConfig::Instance();
	m_svgGenerator = new QSvgGenerator();
	m_painter = new QPainter;
	m_element_id = 0;
}

SvgTransformer::~SvgTransformer()
{
	if (m_svgGenerator)
		delete m_svgGenerator;
	if (m_painter)
		delete m_painter;
	m_svgGenerator = NULL;
	m_painter = NULL;
}

LogicSvg* SvgTransformer::BuildLogicModelByIedName(const QString& iedName)
{
	LogicDiagramBuilder builder;
	LogicSvg* pSvg = builder.BuildLogicDiagramByIedName(iedName);
	m_errStr = builder.GetErrorString();
	return pSvg;
}

OpticalSvg* SvgTransformer::BuildOpticalModelByIedName(const QString& iedName)
{
	OpticalDiagramBuilder builder;
	OpticalSvg* pSvg = builder.BuildOpticalDiagramByIedName(iedName);
	m_errStr = builder.GetErrorString();
	return pSvg;
}

OpticalSvg* SvgTransformer::BuildOpticalModelByStation()
{
	OpticalDiagramBuilder builder;
	OpticalSvg* pSvg = builder.BuildOpticalDiagramByStation();
	m_errStr = builder.GetErrorString();
	return pSvg;
}

OpticalSvg* SvgTransformer::BuildOpticalModelByBayName(const QString& bayName)
{
	OpticalDiagramBuilder builder;
	OpticalSvg* pSvg = builder.BuildOpticalDiagramByBayName(bayName);
	m_errStr = builder.GetErrorString();
	return pSvg;
}

VirtualSvg* SvgTransformer::BuildVirtualModelByIedName(const QString& iedName)
{
	VirtualDiagramBuilder builder;
	VirtualSvg* pSvg = builder.BuildVirtualDiagramByIedName(iedName);
	m_errStr = builder.GetErrorString();
	return pSvg;
}

WholeCircuitSvg* SvgTransformer::BuildWholeCircuitModelByIedName(const QString& iedName)
{
	WholeDiagramBuilder builder;
	WholeCircuitSvg* pSvg = builder.BuildWholeDiagramByIedName(iedName);
	m_errStr = builder.GetErrorString();
	return pSvg;
}
