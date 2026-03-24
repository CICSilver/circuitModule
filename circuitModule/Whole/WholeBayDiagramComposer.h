#pragma once

#include <QString>

class CircuitConfig;
struct WholeCircuitSvg;

enum WholeBayDiagramComposerConfig
{
	WHOLE_BAY_TREE_COLUMN_GAP = 650
};

class WholeBayDiagramComposer
{
public:
	WholeBayDiagramComposer(CircuitConfig* pCircuitConfig);

	void Generate(const QString& bayName, WholeCircuitSvg& svg, int columnGap);

private:
	CircuitConfig* m_pCircuitConfig;
};
