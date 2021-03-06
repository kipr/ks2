#define _USE_MATH_DEFINES

#include "touch_dial.hpp"

#include <cmath>
#include <QPainter>
#include <QMouseEvent>
#include <QDebug>

// Qt specifies degrees in 1/16th angle increments
#define DEG_TO_ARCDEG(x) ((x) * 16)

TouchDial::TouchDial(QWidget *parent)
	: QWidget(parent),
	m_minimumValue(-1.0),
	m_maximumValue(1.0),
	m_value(0.0),
	m_label(0xFFFF),
	m_readOnly(false)
{
	updateDial();
}

TouchDial::~TouchDial()
{
}

void TouchDial::setMinimumValue(const double &minimumValue)
{
	m_minimumValue = minimumValue;
}

const double &TouchDial::minimumValue() const
{
	return m_minimumValue;
}

void TouchDial::setMaximumValue(const double &maximumValue)
{
	m_maximumValue = maximumValue;
	updateDial();
}

const double &TouchDial::maximumValue() const
{
	return m_maximumValue;
}

void TouchDial::setValue(const double &value)
{
	m_value = value;
	if(m_value < m_minimumValue) m_value = m_minimumValue;
	if(m_value > m_maximumValue) m_value = m_maximumValue;
	emit valueChanged(m_value);
	updateDial();
}

const double &TouchDial::value() const
{
	return m_value;
}

void TouchDial::setLabel(const quint16 &label)
{
	m_label = label;
	update();
}

const quint16 &TouchDial::label() const
{
	return m_label;
}

void TouchDial::setReadOnly(const bool& readOnly)
{
	m_readOnly = readOnly;
}

const bool &TouchDial::readOnly() const
{
	return m_readOnly;
}

void TouchDial::paintEvent(QPaintEvent *)
{
	const int w = width();
	const int h = height();
	
	const int d = qMin(w, h) / 1.1;
	const int r = d / 2;
	
	const int xoff = (w - d) / 2;
	const int yoff = (h - d) / 2;
	
	QPainter p(this);
	
  p.setRenderHints(QPainter::Antialiasing);
	p.drawPixmap(0, 0, m_dialPixmap);
	
	p.setPen(Qt::black);
	p.setBrush(Qt::black);
	QFont font = p.font();
	font.setPixelSize(r / 3);
	p.setFont(font);
	
	// This is to get rid of "-0" nonsense
	double printValue = m_value;
	if(printValue < 0.0 && printValue >= -0.5) printValue = 0.0;
	
	p.drawText(xoff + r * 0.4 + 0.5, yoff + r * 1.3 + 0.5, r * 1.2, r * 0.6,
		Qt::AlignHCenter | Qt::AlignVCenter, QString().sprintf("%.0f", printValue));
	
	if(m_label != 0xFFFF) {
		p.setPen(Qt::white);
		p.drawText(xoff, yoff, d, d,
			Qt::AlignHCenter | Qt::AlignVCenter, QString::number(m_label));
	}
}

void TouchDial::mouseMoveEvent(QMouseEvent *event)
{
	if(m_readOnly) return;
	
	const int w = width();
	const int h = height();
	
	const int d = qMin(w, h);
	const int r = d / 2;
	
	const int xoff = (w - d) / 2;
	const int yoff = (h - d) / 2;
	
	QPointF localPos = event->localPos();
	double x = (localPos.x() - xoff - r);
	double y = (localPos.y() - yoff - r);
	
	double angle = atan(y / x);

	if(x < 0.0) angle += M_PI;
	else if(y < 0.0) angle += 2 * M_PI;
	else angle -= M_PI / 4;
	
	double value = angleToValue(angle) * (m_maximumValue - m_minimumValue) + m_minimumValue;
	
	// Correct dead zone
	if(angle < (3.0 * M_PI) / 4.0 && angle > M_PI / 2.0 && value > 0.5) value = m_minimumValue;

	setValue(value);
	
	update();
}

void TouchDial::resizeEvent(QResizeEvent *event)
{
	updateDial();
}

inline double TouchDial::valueToAngle(const double &value)
{
	return (M_PI / 4.0) * (6.0 * value - 1.0) + M_PI;
}

inline double TouchDial::angleToValue(const double &angle)
{
	double value = (4.0 * angle - 3 * M_PI) / (6.0 * M_PI);
	if(value < 0.0) value += 1.5;
	return value;
}

void TouchDial::updateDial()
{
	const int w = width();
	const int h = height();
	
	if(w < 1 || h < 1) return;
	
	m_dialPixmap = QPixmap(w, h);
	m_dialPixmap.fill(Qt::transparent);
	
	const int d = qMin(w, h);
	const int r = d / 2;
	
	const int xoff = (w - d) / 2;
	const int yoff = (h - d) / 2;
	
	QPainter p(&m_dialPixmap);
	p.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing);
	p.setPen(QPen(Qt::white, r / 25));
	
	p.setBrush(Qt::black);
	p.drawPie(xoff, yoff, d, d, DEG_TO_ARCDEG(-45), DEG_TO_ARCDEG(270));
	
	p.setBrush(Qt::red);
	
	const double angle = valueToAngle((m_value - m_minimumValue) / (m_maximumValue - m_minimumValue));
	p.setPen(QPen(Qt::red, r / 12));
	p.drawLine(xoff + r, yoff + r, xoff + r * (1 + cos(angle)), yoff + r * (1 + sin(angle)));
	p.setPen(QPen(Qt::white, r / 20));
	p.drawEllipse(QPoint(xoff + r, yoff + r), r / 4, r / 4);
}