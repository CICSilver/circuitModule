#include "pugixml/pugixml.hpp"
#include "circuitconfig.h"
#include "mainwindow.h"
#include "math.h"
#include "SvgTransformer.h"
#include "customsvgitem.h"
#include "basemodel.h"
#include "InteractiveSvgItem.h"

#include <QApplication>
#include <QDir>
#include <QFile>
#include <QXmlStreamReader>
#include <QDomDocument>
#include <string>
#include <QDebug>
#include <QElapsedTimer>
#include <windows.h>
#include <QtSvg>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsSvgItem>
#include <QSvgRenderer>
#include <QMouseEvent>
#include "RtdbClient.h"

using std::string;

// 简单的 RTDB 读取测试：演示如何打开实时库并读取一个模拟量通道
static void RunRtdbReadTest()
{
	RtdbClient client;
	// 以只读方式打开实时库
	if (!client.open(RTDB_OPEN_RO)) {
		qWarning() << "RTDB open failed";
		return;
	}

	UINT64 code = 0x1060b0001; // 示例编码
	const qulonglong kDemoAnalogCode = code;

	double val = 0.0;
	if (client.getAnalog(code, val, false)) {
		qDebug() << QString::fromLocal8Bit("RTDB 模拟量读取成功 code=") << QString::number(code) << ", value=" << val;
	} else {
		qWarning() << QString::fromLocal8Bit("RTDB 模拟量读取失败 code=") << QString::number(code);
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

int main(int argc, char* argv[]) {
	QApplication app(argc, argv);
	QString iedName = "IMT2201L1";
	//QString scdPath = QCoreApplication::applicationDirPath() + "/scd_test.scd";
	//QString configPath = QCoreApplication::applicationDirPath() + "/circuit_config.csv";
	QElapsedTimer timer;

	CircuitConfig* pCircuitConfig = CircuitConfig::Instance();
	pCircuitConfig->LoadCimeFile();
	SvgTransformer transformer;
	//transformer.GenerateSvgByIedName(iedName);

	// 实时库测试
	RunRtdbReadTest();

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



	//QString svgPath = QCoreApplication::applicationDirPath() + "/PT2202A_virtual_circuit.svg";
	//showSingleSvg(svgPath);


	QGraphicsScene* scene = new QGraphicsScene();
	MainWindow mainWindow;
	// 将场景接入到 UI 中的 opticalView（通过对象名）
	if (QGraphicsView* opticalView = mainWindow.findChild<QGraphicsView*>("opticalView")) {
		opticalView->setScene(scene);
		opticalView->setDragMode(QGraphicsView::NoDrag);
		opticalView->setRenderHint(QPainter::Antialiasing);
		opticalView->setViewportUpdateMode(QGraphicsView::BoundingRectViewportUpdate);
		opticalView->setBackgroundBrush(Qt::black);
	}
	mainWindow.show();

	
	return app.exec();
}
