#include "robot.hpp"

#include <QGraphicsRectItem>
#include <QGraphicsLineItem>
#include <QGraphicsScene>
#include <QPen>
#include <QDebug>
#include "board_file.hpp"

#include <cmath>

Robot::Robot()
	: m_wheelDiameter(16.0),
	m_wheelRadii(3.0),
	m_leftSpeed(0.0),
	m_rightSpeed(0.0),
	m_rangeLength(70.0),
	m_leftTravelDistance(0.0),
	m_rightTravelDistance(0.0),
	m_robot(new QGraphicsRectItem(-m_wheelDiameter / 2.0, -m_wheelDiameter / 2.0, m_wheelDiameter, m_wheelDiameter)),
	m_leftWheel(new QGraphicsEllipseItem(-m_wheelRadii, -m_wheelDiameter / 2.0 - m_wheelRadii, m_wheelRadii * 2, m_wheelRadii)),
	m_rightWheel(new QGraphicsEllipseItem(-m_wheelRadii, m_wheelDiameter / 2.0, m_wheelRadii * 2, m_wheelRadii)),
	m_leftRange(new QGraphicsLineItem()),
	m_frontRange(new QGraphicsLineItem()),
	m_rightRange(new QGraphicsLineItem())
{
	m_robot->setData(0, BoardFile::Fake);
	m_robot->setBrush(Qt::lightGray);
	
	m_leftWheel->setParentItem(m_robot);
	m_rightWheel->setParentItem(m_robot);
	
	m_leftWheel->setBrush(Qt::darkGray);
	m_rightWheel->setBrush(Qt::darkGray);
	
	updateRangeLines();
	
	QPen rangePen(Qt::red, 0, Qt::DotLine);
	m_leftRange->setPen(rangePen);
	m_frontRange->setPen(rangePen);
	m_rightRange->setPen(rangePen);
	
	m_leftRange->setZValue(-0.1);
	m_frontRange->setZValue(-0.1);
	m_rightRange->setZValue(-0.1);
	
	m_time.start();
}

Robot::~Robot()
{
	delete m_robot;
	delete m_leftRange;
	delete m_frontRange;
	delete m_rightRange;
}

void Robot::setWheelDiameter(const double &wheelDiameter)
{
	m_wheelDiameter = wheelDiameter;
}

const double &Robot::wheelDiameter() const
{
	return m_wheelDiameter;
}

void Robot::setWheelRadii(const double &wheelRadii)
{
	m_wheelRadii = wheelRadii;
}

const double &Robot::wheelRadii() const
{
	return m_wheelRadii;
}

void Robot::setLeftSpeed(const double &leftSpeed)
{
	update();
	m_leftSpeed = leftSpeed;
}

const double &Robot::leftSpeed() const
{
	return m_leftSpeed;
}

void Robot::setRightSpeed(const double &rightSpeed)
{
	update();
	m_rightSpeed = rightSpeed;
}

const double &Robot::rightSpeed() const
{
	return m_rightSpeed;
}

void Robot::setLeftTravelDistance(double leftTravelDistance)
{
	m_leftTravelDistance = leftTravelDistance;
}

double Robot::leftTravelDistance() const
{
	return m_leftTravelDistance;
}

void Robot::setRightTravelDistance(double rightTravelDistance)
{
	m_rightTravelDistance = rightTravelDistance;
}

double Robot::rightTravelDistance() const
{
	return m_rightTravelDistance;
}

void Robot::setRangeLength(const double &rangeLength)
{
	m_rangeLength = rangeLength;
	updateRangeLines();
}

const double &Robot::rangeLength() const
{
	return m_rangeLength;
}

double Robot::leftRange() const
{
	return m_leftRange->line().length();
}

double Robot::frontRange() const
{
	return m_frontRange->line().length();
}

double Robot::rightRange() const
{
	return m_rightRange->line().length();
}

void Robot::update()
{
	int milli = m_time.elapsed();
	double sec = milli / 1000.0;
	
	const double theta = m_robot->rotation() / 180.0 * M_PI;
	
	double dl = sec * m_leftSpeed * m_wheelRadii * 2.0 * M_PI;
	double dr = sec * m_rightSpeed * m_wheelRadii * 2.0 * M_PI;
	double dd = (dl + dr) / 2.0;
	m_robot->setRotation((theta + (dr - dl) / m_wheelDiameter) * 180.0 / M_PI);
	m_leftTravelDistance += dl;
	m_rightTravelDistance += dr;
	
	m_robot->setX(m_robot->x() + cos(theta) * dd);
	m_robot->setY(m_robot->y() + sin(theta) * dd);
	
	updateRangeLines();
	
	m_time.restart();
}

QList<QGraphicsItem *> Robot::robot() const
{
	return QList<QGraphicsItem *>() << m_robot << m_leftRange << m_frontRange << m_rightRange;
}

void Robot::updateRangeLines()
{
	m_leftRange->setLine(intersectDistance(m_leftRange, -45.0));
	m_frontRange->setLine(intersectDistance(m_frontRange, 0.0));
	m_rightRange->setLine(intersectDistance(m_rightRange, 45.0));
}

QLineF Robot::intersectDistance(QGraphicsLineItem *item, const double &baseAngle) const
{
	QGraphicsScene *scene = item->scene();
	if(!scene) return QLineF(0, 0, 0, 0);
	
	const double rad = (m_robot->rotation() + baseAngle) / 180.0 * M_PI;
	QLineF line(m_robot->pos(), m_robot->pos() + m_rangeLength *
		QPointF(cos(rad), sin(rad)));
	
	const QLineF unit = line.unitVector();
	const double xs = unit.dx();
	const double ys = unit.dy();
	for(double i = m_wheelDiameter / 2; i < m_rangeLength; i += 1.0) {
		QRectF r(m_robot->x() + i * xs, m_robot->y() + i * ys, xs, ys);
		QList<QGraphicsItem *> items = scene->items(r, Qt::IntersectsItemBoundingRect, Qt::AscendingOrder);
		foreach(QGraphicsItem *t, items) {
			if(t->data(0) == BoardFile::Real) {
				line.setLength(i);
				return line;
			}
		}
	}
	
	return line;
}