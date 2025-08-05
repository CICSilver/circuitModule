#include "pugixml/pugixml.hpp"
#include "circuitconfig.h"
#include "mainwindow.h"
#include "math.h"
#include "SvgTransformer.h"
#include "customsvgitem.h"
#include "basemodel.h"

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

using std::string;
void printMemoryUsage() {
	MEMORYSTATUSEX statex;

	statex.dwLength = sizeof(statex);
	GlobalMemoryStatusEx(&statex);

    DWORDLONG totalMemoryUsed = statex.ullTotalPhys - statex.ullAvailPhys;

    qDebug() << "Total memory used: " << totalMemoryUsed / 1024 / 1024 << " MB.";
}

QColor get_color_by_hex(uint hex)
{
	return QColor((hex >> 16) & 0xFF, (hex >> 8) & 0xFF, hex & 0xFF);
}

void drawArrowHeader(QPainter& painter, const QPoint& endPoint, double arrowAngle);
void svg_generator_test()
{
	QSvgGenerator generator;
	generator.setViewBox(QRect(0, 0, 800, 600));
	generator.setFileName("test.svg");
	generator.setSize(QSize(800, 600));
	QPainter painter;
	painter.begin(&generator);

	const uint red = 0xff0000;
	const uint grey = 0x233f4f;
	const uint gseColor = 0xd3603d;
	const uint smvColor = 0x00b0f0;

	QPen pen;
	pen.setStyle(Qt::SolidLine);
	pen.setColor(get_color_by_hex(gseColor));
	painter.setPen(pen);

	QPoint pt1(50, 50);
	QPoint pt2(200, 200);

	double vec_x = pt2.x() - pt1.x();
	double vec_y = pt2.y() - pt1.y();
	double direction = atan2(-vec_y, vec_x);
	double angle = direction * (180 / M_PI);
	painter.drawLine(pt1, pt2);
	drawArrowHeader(painter, pt2, angle);


	painter.end();                 
}
double AngleToRadians(double angle)
{
	return angle * M_PI / 180.0;
}
void drawArrowHeader(QPainter& painter, const QPoint& endPoint, double arrowAngle)
{
	double arrowHeadLineAngle = 150;
	int arrowHeadLength = 10;
	QPointF leftArrowPoint = endPoint + QPointF(
		arrowHeadLength * cos(AngleToRadians(arrowAngle + arrowHeadLineAngle)),
		-arrowHeadLength * sin(AngleToRadians(arrowAngle + arrowHeadLineAngle)));
	QPointF rightArrowPoint = endPoint + QPointF(
		arrowHeadLength * cos(AngleToRadians(arrowAngle - arrowHeadLineAngle)),
		-arrowHeadLength * sin(AngleToRadians(arrowAngle - arrowHeadLineAngle)));
	painter.drawLine(endPoint, leftArrowPoint);
	painter.drawLine(endPoint, rightArrowPoint);
}

void scd_xpath_test()
{
	QString scdPath = QCoreApplication::applicationDirPath() + "/big_scd.scd";
	QElapsedTimer timer;
	timer.start();
	pugi::xml_document doc;
	pugi::xml_parse_result result = doc.load_file(scdPath.toLocal8Bit());
	qDebug() << QString::fromLocal8Bit("pugi 读取文件耗时：%1").arg(timer.elapsed());

}

int main(int argc, char* argv[]) {
	QApplication app(argc, argv);
	QString iedName = "IMM2201L1";
	// 解析SCD文件
	//QString scdPath = QCoreApplication::applicationDirPath() + "/scd_test.scd";
	//QString configPath = QCoreApplication::applicationDirPath() + "/circuit_config.csv";


	// 读取配置文件
	CircuitConfig* pCircuitConfig = CircuitConfig::Instance();
	pCircuitConfig->LoadCimeFile();
	SvgTransformer transformer;
	// 单设备文件生成测试
	transformer.GenerateSvgByIedName(iedName);

	//// 生成SVG文件
	//QList<QString> pathList;
	//pathList
	//	<< QCoreApplication::applicationDirPath() + "/logic"		// 逻辑链路
	//	<< QCoreApplication::applicationDirPath() + "/optical"		// 光纤链路
	//	<< QCoreApplication::applicationDirPath() + "/virtual"		// 虚链路
	//	<< QCoreApplication::applicationDirPath() + "/whole";		// 整体链路
	//foreach(QString path, pathList)
	//{
	//	QDir dir(path);
	//	if (!dir.exists())
	//	{
	//		dir.mkpath(path);
	//	}
	//}
	//
	//foreach(IED * pIed, pCircuitConfig->GetIedList())
	//{
	//	QString path;
	//	if (pIed->name.contains("SW"))
	//		continue;
	//	// 逻辑链路
	//	transformer.GenerateLogicSvg(pIed, pathList.at(0) + "/" + pIed->name + "_logic_circuit.svg");
	//	// 光纤链路
	//	transformer.GenerateOpticalSvg(pIed, pathList.at(1) + "/" + pIed->name + "_optical_circuit.svg");
	//	// 虚回路
	//	transformer.GenerateVirtualSvg(pIed, pathList.at(2) + "/" + pIed->name + "_virtual_circuit.svg");
	//	// 整体链路
	//	//transformer.GenerateWholeCircuitSvg(pIed, pathList.at(3) + "/" + pIed->name + "_whole_circuit.svg");
	//}
	qDebug() << "SVG files generated successfully.";


	// 显示SVG文件
	//QGraphicsScene scene;
	//scene.setSceneRect(0, 0, 2160, 1440);
	//QString path = pathList.at(0) + "/" + iedName + "_logic_circuit.svg";
	//CustomSvgItem* svgItem = new CustomSvgItem();
	//svgItem->setSharedRenderer(new QSvgRenderer(path));
	//svgItem->LoadSvg(path);
	//scene.addItem(svgItem);
	//scene.update();

	//QGraphicsView view(&scene);
	//view.setRenderHint(QPainter::Antialiasing);
	//view.setViewportUpdateMode(QGraphicsView::BoundingRectViewportUpdate);
	//view.setBackgroundBrush(Qt::black);
	//view.resize(1920, 1080);
	//view.setDragMode(QGraphicsView::ScrollHandDrag);
	//view.scene()->setItemIndexMethod(QGraphicsScene::NoIndex);
	//view.show();


	//MainWindow mainWindow;
	//mainWindow.show();

	
	return app.exec();
}
