#ifndef _LIGHT_HPP_
#define _LIGHT_HPP_

#include <QGraphicsEllipseItem>

class Light : public QGraphicsEllipseItem
{
public:
	Light();
	~Light();
	
	bool isOn() const;
	
protected:
	virtual void mousePressEvent(QGraphicsSceneMouseEvent *event);
	virtual void mouseMoveEvent(QGraphicsSceneMouseEvent *event);
	virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);
	virtual void mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event);
	
private:
	bool m_grabbed;
	
	bool m_on;
	QGraphicsEllipseItem *m_gradient;
};

#endif
