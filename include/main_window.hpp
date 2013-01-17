/**************************************************************************
 * ks2 - A 2D simulator for the Kovan Robot Controller                    *
 * Copyright (C) 2012 KISS Institute for Practical Robotics               *
 *                                                                        *
 * This program is free software: you can redistribute it and/or modify   *
 * it under the terms of the GNU General Public License as published by   *
 * the Free Software Foundation, either version 3 of the License, or      *
 * (at your option) any later version.                                    *
 *                                                                        *
 * This program is distributed in the hope that it will be useful,        *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 * GNU General Public License for more details.                           *
 *                                                                        *
 * You should have received a copy of the GNU General Public License      *
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.  *
 **************************************************************************/

#ifndef _MAIN_WINDOW_HPP_
#define _MAIN_WINDOW_HPP_

#include <QMainWindow>

#include "button_ids.hpp"

namespace Ui
{
	class MainWindow;
}

class TouchDial;
class QLabel;
class QProcess;
class Robot;
class Light;
class Heartbeat;
class ServerThread;

namespace Kovan
{
	class KmodSim;
	class ButtonProvider;
}

class MainWindow : public QMainWindow
{
Q_OBJECT
public:
	MainWindow(QWidget *parent = 0);
	~MainWindow();
	
private slots:
	void buttonPressed();
	void buttonReleased();
	void textChanged(::Button::Type::Id id, const QString &text);
	void update();
	void reset();
	
	void finished(int exitCode);
	
	void run(const QString &executable);
	void stop();
	
private:
	void updateAdvert();
	int unfixPort(int port);
	void setDigital(int port, bool on);
	
	Ui::MainWindow *ui;
	
	QLabel *m_analogs[8];
	QLabel *m_digitals[8];
	TouchDial *m_motors[4];
	TouchDial *m_servos[4];
	Robot *m_robot;
	Light *m_light;
	
	ServerThread *m_server;
	
	Kovan::ButtonProvider *m_buttonProvider;
	Kovan::KmodSim *m_kmod;
	
	Heartbeat *m_heartbeat;
	
	QProcess *m_process;
};

#endif
