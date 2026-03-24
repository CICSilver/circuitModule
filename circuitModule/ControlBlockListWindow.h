#pragma once

#include <QWidget>
#include <QString>

class QTableWidget;

class ControlBlockListWindow : public QWidget
{
public:
	ControlBlockListWindow(QWidget* parent);

	bool LoadByOpticalConnPoint(quint64 opticalCode, const QString& iedName);

private:
	QTableWidget* m_tableWidget;
};
