#include "pugixml/pugixml.hpp"
#include "circuitconfig.h"
#include "mainwindow.h"
#include "math.h"
#include "SvgTransformer.h"
#include "basemodel.h"
#include "InteractiveSvgItem.h"
#include "directwidget.h"
#include "RtdbClient.h"
#include "../CircuitModuleApi.h"

#include <QApplication>
#include <QDir>
#include <QFile>
#include <QXmlStreamReader>
#include <QDomDocument>
#include <string>
#include <QDebug>
#include <QElapsedTimer>
#include <QtSvg>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsSvgItem>
#include <QSvgRenderer>
#include <QMouseEvent>

#ifdef _WIN32
#include <windows.h>
#endif
using std::string;

// 简单的 RTDB 读取测试：演示如何打开实时库并读取一个模拟量通道
static void RunRtdbReadTest()
{
	RtdbClient& client = RtdbClient::Instance();
	// 以只读方式打开实时库
	if (!client.refresh()) {
		qWarning() << "RTDB refresh failed";
		return;
	}
	const CRtdbEleModelStation* station = client.stationModel();
	foreach(CRtdbEleModelVoltLevel * pVoltLevel, station->m_listVoltLevel)
	{
		qDebug() << "[Volt Level] " << pVoltLevel->m_pVoltInfo->levelDesc;
		foreach(CRtdbEleModelBay * pBay, pVoltLevel->m_listBay)
		{
			qDebug() << "\t[Bay] " << pBay->m_pBayInfo->bayName;
			foreach(CRtdbEleModelIed * pIed, pBay->m_listIed)
			{
				qDebug() << "\t\t[IED] " << pIed->m_pIedHead->iedName;
			}
		}
	}
}

void showSingleSvg(const QString& svgPath) {
	QGraphicsScene* scene = new QGraphicsScene();
	InteractiveSvgMapItem* item = new InteractiveSvgMapItem(svgPath);
	scene->addItem(item);
	QRectF itemRect = item->boundingRect();
	scene->setSceneRect(itemRect);
	QGraphicsView* view = new QGraphicsView(scene);
	view->setDragMode(QGraphicsView::NoDrag);
	view->setRenderHint(QPainter::Antialiasing);
	view->setViewportUpdateMode(QGraphicsView::BoundingRectViewportUpdate);
	view->setBackgroundBrush(Qt::black);
	
	QSizeF sz = itemRect.size();
	int w = int(std::min<double>(sz.width() + 20, 1800));
	int h = int(std::min<double>(sz.height() + 20, 1000));
	if (w < 800) w = 800; if (h < 600) h = 600;
	view->resize(w, h);
	view->show();
}

void generateSvg()
{
	//SVG 生成
	CircuitConfig* pCircuitConfig = CircuitConfig::Instance();
	SvgTransformer transformer;
	QList<QString> pathList;
	pathList
		<< QCoreApplication::applicationDirPath() + "/logic"
		<< QCoreApplication::applicationDirPath() + "/optical"
		<< QCoreApplication::applicationDirPath() + "/virtual";
	// << QCoreApplication::applicationDirPath() + "/whole";		
	foreach(QString path, pathList)
	{
		QDir dir(path);
		if (!dir.exists())
		{
			dir.mkpath(path);
		}
	}
	foreach(IED * pIed, pCircuitConfig->GetIedList())
	{
		QString path;
		if (pIed->name.contains("SW"))
			continue;
		transformer.GenerateLogicSvg(pIed, pathList.at(0) + "/" + pIed->name + "_logic_circuit.svg");
		transformer.GenerateOpticalSvg(pIed, pathList.at(1) + "/" + pIed->name + "_optical_circuit.svg");
		transformer.GenerateVirtualSvg(pIed, pathList.at(2) + "/" + pIed->name + "_virtual_circuit.svg");
		//transformer.GenerateWholeCircuitSvg(pIed, pathList.at(3) + "/" + pIed->name + "_whole_circuit.svg");
	}
	qDebug() << "SVG files generated successfully.";
}

int main(int argc, char* argv[]) {
	QApplication app(argc, argv);
	QString iedName = "MIB5012A";


	CircuitConfig* cfg = CircuitConfig::Instance();
	if (!cfg->LoadRTDB()) {
		qWarning() << "LoadRTDB failed";
	}
	DirectWidget* testWidget = new DirectWidget(NULL);
	SvgTransformer transformer;

	// 虚回路测试入口
	VirtualSvg* vsvg = transformer.BuildVirtualModelByIedName(iedName);
	if (vsvg) {
		testWidget->ParseFromVirtualSvg(*vsvg);
		delete vsvg;
	}

	//// 光纤链路测试入口
	//OpticalSvg* osvg = transformer.BuildOpticalModelByIedName(iedName);
	//if (osvg) {
	//	testWidget->ParseFromOpticalSvg(*osvg);
	//	delete osvg;
	//}

	//// 逻辑链路测试入口
	//LogicSvg* lsvg = transformer.BuildLogicModelByIedName(iedName);
	//if (lsvg) {
	//	testWidget->ParseFromLogicSvg(*lsvg);
	//	delete lsvg;
	//}

	testWidget->resize(1200, 800);
	testWidget->show();

	//QGraphicsScene* scene = new QGraphicsScene();
	//CircuitModuleWidget mainWindow(true);
	//if (QGraphicsView* opticalView = mainWindow.findChild<QGraphicsView*>("opticalView")) {
	//	opticalView->setScene(scene);
	//	opticalView->setDragMode(QGraphicsView::NoDrag);
	//	opticalView->setRenderHint(QPainter::Antialiasing);
	//	opticalView->setViewportUpdateMode(QGraphicsView::BoundingRectViewportUpdate);
	//	opticalView->setBackgroundBrush(Qt::black);
	//}
	//mainWindow.show();

	return app.exec();
}
