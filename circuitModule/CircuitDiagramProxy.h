#pragma once
#include "svgmodel.h"
#include <QString>
#include <QtGlobal>

class SvgTransformer;

typedef LogicSvg LogicDiagramModel;
typedef OpticalSvg OpticalDiagramModel;
typedef VirtualSvg VirtualDiagramModel;
typedef WholeCircuitSvg WholeDiagramModel;

class CircuitDiagramProxy
{
public:
	CircuitDiagramProxy();
	~CircuitDiagramProxy();

	LogicDiagramModel* BuildLogicDiagramByIedName(const QString& iedName);
	OpticalDiagramModel* BuildOpticalDiagramByIedName(const QString& iedName);
	OpticalDiagramModel* BuildOpticalDiagramByStation();
	OpticalDiagramModel* BuildOpticalDiagramByBayName(const QString& bayName);
	VirtualDiagramModel* BuildVirtualDiagramByIedName(const QString& iedName);
	WholeDiagramModel* BuildWholeDiagramByIedName(const QString& iedName);

private:
	Q_DISABLE_COPY(CircuitDiagramProxy)
	SvgTransformer* m_pTransformer;
};
