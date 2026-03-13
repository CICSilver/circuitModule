#pragma once

#include <QGraphicsItem>
#include <QRectF>
#include <QtGlobal>

class QPainter;
class QStyleOptionGraphicsItem;
class QWidget;

class directItemBase : public QGraphicsItem
{
public:
	enum ItemType
	{
		Plate = 0,
		CircuitLine,
		Icon,
		Text,
		Ied
	};

	virtual ~directItemBase()
	{
	}
	virtual QRectF boundingRect() const = 0;
	virtual void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) = 0;
	ItemType itemType() const
	{
		return static_cast<ItemType>(m_itemType);
	}

protected:
	quint16 m_itemType;
};
