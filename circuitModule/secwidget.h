#ifndef SECWIDGET_H
#define SECWIDGET_H

#include <QWidget>
#include <QString>
#include "svgmodel.h"
#include "InteractiveSvgItem.h"
class QGraphicsView;
class QGraphicsScene;

class SecWidget : public QWidget
{
	Q_OBJECT

public:
	explicit SecWidget(QWidget *parent = NULL);
	~SecWidget();

	void displayCircuit(const QString& srcIed, const QString& destIed);
	void displayCircuit(const MapLine& line);

private:
	QGraphicsView* m_view;
	QGraphicsScene* m_scene;
};

#endif // SECWIDGET_H
