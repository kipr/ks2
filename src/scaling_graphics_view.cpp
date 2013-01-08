#include "scaling_graphics_view.hpp"

ScalingGraphicsView::ScalingGraphicsView(QWidget *parent)
	: QGraphicsView(parent)
{
}

void ScalingGraphicsView::resizeEvent(QResizeEvent *event)
{
	QGraphicsView::resizeEvent(event);
	const QRectF rect = sceneRect();
	const qreal minW = qMin(width(), height());
	const qreal maxS = qMax(rect.width(), rect.height());
	QTransform transform;
	transform.scale(minW / maxS * 0.9, minW / maxS * 0.9);
	setTransform(transform);
}