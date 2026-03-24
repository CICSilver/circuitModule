#pragma once

#include <QString>

class CircuitConfig;
struct WholeCircuitSvg;

enum WholeBayDiagramBuilderConfig
{
	WHOLE_BAY_TREE_COLUMN_GAP = 650
};

class WholeBayDiagramBuilder
{
public:
	WholeBayDiagramBuilder(CircuitConfig* pCircuitConfig);
	void Generate(const QString& bayName, WholeCircuitSvg& svg, int columnGap);

private:
	CircuitConfig* m_pCircuitConfig;
};
