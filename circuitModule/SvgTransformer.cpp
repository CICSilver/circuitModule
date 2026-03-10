#include "SvgTransformer.h"

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
