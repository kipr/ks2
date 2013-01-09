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

#include "main_window.hpp"
#include "ui_main_window.h"

#include "kovan_kmod_sim.hpp"
#include "kovan_button_provider.hpp"
#include "robot.hpp"
#include "simulator.hpp"
#include "board_file.hpp"
#include "server_thread.hpp"
#include "kovan_regs_p.hpp"
#include "heartbeat.hpp"

#include <kovanserial/kovan_serial.hpp>
#include <kovanserial/tcp_server.hpp>
#include <kovanserial/udp_advertiser.hpp>

#include <QTimer>
#include <QGraphicsItem>
#include <QThreadPool>
#include <QProcess>
#include <QDir>
#include <QDebug>

#include <cmath> // tmp

MainWindow::MainWindow(QWidget *parent)
	: QMainWindow(parent),
	ui(new Ui::MainWindow),
	m_robot(new Robot()),
	m_buttonProvider(0),
	m_kmod(new Kovan::KmodSim(this)),
	m_heartbeat(new Heartbeat(this)),
	m_process(0)
{
	ui->setupUi(this);
	
	m_analogs[0] = ui->analog0;
	m_analogs[1] = ui->analog1;
	m_analogs[2] = ui->analog2;
	m_analogs[3] = ui->analog3;
	m_analogs[4] = ui->analog4;
	m_analogs[5] = ui->analog5;
	m_analogs[6] = ui->analog6;
	m_analogs[7] = ui->analog7;
	
	m_digitals[0] = ui->digital0;
	m_digitals[1] = ui->digital1;
	m_digitals[2] = ui->digital2;
	m_digitals[3] = ui->digital3;
	m_digitals[4] = ui->digital4;
	m_digitals[5] = ui->digital5;
	m_digitals[6] = ui->digital6;
	m_digitals[7] = ui->digital7;
	
	m_motors[0] = ui->motor0;
	m_motors[1] = ui->motor1;
	m_motors[2] = ui->motor2;
	m_motors[3] = ui->motor3;
		
	m_servos[0] = ui->servo0;
	m_servos[1] = ui->servo1;
	m_servos[2] = ui->servo2;
	m_servos[3] = ui->servo3;
	
	for(quint16 i = 0; i < 4; ++i) {
		m_motors[i]->setLabel(i);
		m_motors[i]->setReadOnly(true);
		m_servos[i]->setLabel(i);
		m_servos[i]->setReadOnly(true);
		
		m_motors[i]->setMinimumValue(-100);
		m_motors[i]->setMaximumValue(100);
		
		m_servos[i]->setMinimumValue(0);
		m_servos[i]->setMaximumValue(2047);
	}
	
	// connect(m_kmod, SIGNAL(stateChanged(State)), SLOT(update()));
	
	ui->sim->setScene(BoardFile::load("2013.board"));
	
	if(ui->sim->scene())
	foreach(QGraphicsItem *item, m_robot->robot())
		ui->sim->scene()->addItem(item);
		
	m_robot->robot()[0]->setRotation(45);
	
	QTimer *timer = new QTimer(this);
	connect(timer, SIGNAL(timeout()), SLOT(update()));
	timer->start(50);
	
	connect(ui->a, SIGNAL(pressed()), SLOT(buttonPressed()));
	connect(ui->b, SIGNAL(pressed()), SLOT(buttonPressed()));
	connect(ui->c, SIGNAL(pressed()), SLOT(buttonPressed()));
	connect(ui->x, SIGNAL(pressed()), SLOT(buttonPressed()));
	connect(ui->y, SIGNAL(pressed()), SLOT(buttonPressed()));
	connect(ui->z, SIGNAL(pressed()), SLOT(buttonPressed()));
	
	connect(ui->a, SIGNAL(released()), SLOT(buttonReleased()));
	connect(ui->b, SIGNAL(released()), SLOT(buttonReleased()));
	connect(ui->c, SIGNAL(released()), SLOT(buttonReleased()));
	connect(ui->x, SIGNAL(released()), SLOT(buttonReleased()));
	connect(ui->y, SIGNAL(released()), SLOT(buttonReleased()));
	connect(ui->z, SIGNAL(released()), SLOT(buttonReleased()));
	
	connect(ui->actionStop, SIGNAL(activated()), SLOT(stop()));
	connect(ui->actionQuit, SIGNAL(activated()), QCoreApplication::instance(), SLOT(quit()));
	
	m_kmod->setup();
	
	TcpServer *serial = new TcpServer;
	if(serial->bind(KOVAN_SERIAL_PORT)) {
		perror("bind");
	}
	if(!serial->listen(2)) {
		perror("listen");
	}
	m_server = new ServerThread(serial);
	connect(m_server, SIGNAL(run(QString)), SLOT(run(QString)));
	m_server->start();
	
	m_buttonProvider = new Kovan::ButtonProvider(m_kmod, this);
	ui->extras->connect(m_buttonProvider, SIGNAL(extraShownChanged(bool)), SLOT(setVisible(bool)));
	connect(m_buttonProvider,
		SIGNAL(buttonTextChanged(::Button::Type::Id, QString)),
		SLOT(textChanged(::Button::Type::Id, QString)));
	
	ui->actionStop->setEnabled(false);
	
	updateAdvert();
}

MainWindow::~MainWindow()
{
	stop();
	m_server->stop();
	m_server->wait();
	delete m_server;
	delete m_robot;
	delete ui;
}

void MainWindow::buttonPressed()
{
	QObject *from = sender();
	if(!from) return;	
	
	::Button::Type::Id id = ::Button::Type::A;
	if(from == ui->b) id = ::Button::Type::B;
	else if(from == ui->c) id = ::Button::Type::C;
	else if(from == ui->x) id = ::Button::Type::X;
	else if(from == ui->y) id = ::Button::Type::Y;
	else if(from == ui->z) id = ::Button::Type::Z;
	
	m_buttonProvider->setPressed(id, true);
}

void MainWindow::buttonReleased()
{
	QObject *from = sender();
	if(!from) return;	
	
 	::Button::Type::Id id = ::Button::Type::A;
	if(from == ui->b) id = ::Button::Type::B;
	else if(from == ui->c) id = ::Button::Type::C;
	else if(from == ui->x) id = ::Button::Type::X;
	else if(from == ui->y) id = ::Button::Type::Y;
	else if(from == ui->z) id = ::Button::Type::Z;
	
	m_buttonProvider->setPressed(id, false);
}

void MainWindow::textChanged(::Button::Type::Id id, const QString &text)
{
	switch(id) {
	case ::Button::Type::A:
		ui->a->setText(text);
		break;
	case ::Button::Type::B:
		ui->b->setText(text);
		break;
	case ::Button::Type::C:
		ui->c->setText(text);
		break;
	case ::Button::Type::X:
		ui->x->setText(text);
		break;
	case ::Button::Type::Y:
		ui->y->setText(text);
		break;
	case ::Button::Type::Z:
		ui->z->setText(text);
		break;
	default: break;
	}
}

void MainWindow::update()
{
	m_buttonProvider->refresh();
	m_robot->update();
	
	Kovan::State &s = m_kmod->state();

	static const int motors[4] = {
		MOTOR_PWM_0,
		MOTOR_PWM_1,
		MOTOR_PWM_2,
		MOTOR_PWM_3
	};

	unsigned short modes = s.t[PID_MODES];
	for(int i = 0; i < 4; ++i) {
		unsigned char mode = modes >> ((3 - i) << 1);
		double val = 0.0;
		bool pwm = false;
		if(mode == 0) { // pwm
			val = s.t[MOTOR_PWM_0 + i] / 2600.0;
			pwm = true;
			if(val > 1.0) val = 1.0;
		} else if(mode == 2) { // speed
			val = (s.t[(GOAL_SPEED_0_HIGH + i)] << 16 | s.t[(GOAL_SPEED_0_LOW + i)]) / 1000.0;
		} else if(mode == 3) {
			
		}
		
		
		const double m = 2.5;
		int port = unfixPort(i);
		if(port == 0) {
			m_robot->setLeftSpeed(val * (pwm ? m : 1.0));
		} else if(port == 2) {
			m_robot->setRightSpeed(val * (pwm ? m : 1.0));
		}
		
		m_motors[port]->setValue(val * 100.0);
		// m_servos[i]->setValue();
	}
	
	static const int analogs[8] = {
		AN_IN_0,
		AN_IN_1,
		AN_IN_2,
		AN_IN_3,
		AN_IN_4,
		AN_IN_5,
		AN_IN_6,
		AN_IN_7
	};
	
	s.t[analogs[0]] = m_robot->leftRange() / m_robot->rangeLength() * 1024.0;
	s.t[analogs[1]] = m_robot->frontRange() / m_robot->rangeLength() * 1024.0;
	s.t[analogs[2]] = m_robot->rightRange() / m_robot->rangeLength() * 1024.0;
	
	for(int i = 0; i < 8; ++i) {
		m_analogs[i]->setText(QString::number(s.t[analogs[i]]));
		m_digitals[i]->setText(s.t[DIG_IN] << (7 - i) ? "1" : "0");
	}
	
	ui->scrollArea->update();
}

void MainWindow::finished(int exitCode)
{
	ui->actionStop->setEnabled(false);
	stop();
}

void MainWindow::run(const QString &executable)
{
	raise();
	stop();
	reset();
	m_process = new QProcess();
	connect(m_process, SIGNAL(finished(int, QProcess::ExitStatus)),
		SLOT(finished(int)));
	QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
	// TODO: This will only work on OS X
#ifdef Q_OS_MAC
	env.insert("DYLD_LIBRARY_PATH", QDir::currentPath() + "/prefix/usr/lib:" + env.value("DYLD_LIBRARY_PATH"));
	env.insert("DYLD_LIBRARY_PATH", QDir::currentPath() + "/prefix/usr:" + env.value("DYLD_LIBRARY_PATH"));
#endif
	m_process->setProcessEnvironment(env);
	m_process->start(executable, QStringList());
	if(!m_process->waitForStarted(10000)) stop();
	ui->actionStop->setEnabled(true);
	ui->console->setProcess(m_process);
}

void MainWindow::stop()
{
	update();
	if(!m_process) return;
	ui->console->setProcess(0);
	m_process->terminate();
	if(!m_process->waitForFinished(5000)) m_process->kill();
	delete m_process;
	m_process = 0;
	ui->actionStop->setEnabled(false);
	m_kmod->reset();
}

void MainWindow::updateAdvert()
{
	QString version = (QString::number(SIMULATOR_VERSION_MAJOR) + "." + QString::number(SIMULATOR_VERSION_MINOR));
	#if defined(Q_OS_WIN)
	version += " for Windows";
	#elif defined(Q_OS_MAC)
	version += " for Mac OS X";
	#else
	version += " for *nix";
	#endif
	
	Advert ad(tr("N/A").toAscii(),
		version.toAscii(),
		tr("2D Simulator").toAscii(),
		tr("Simulator").toAscii());
	m_heartbeat->setAdvert(ad);
}

int MainWindow::unfixPort(int port)
{
	switch(port) {
	case 0: return 1;
	case 1: return 0;
	case 2: return 3;
	case 3: return 2;
	}
	return port;
}

void MainWindow::reset()
{
	m_buttonProvider->reset();
	m_robot->setLeftSpeed(0.0);
	m_robot->setRightSpeed(0.0);
	m_robot->setLeftTravelDistance(0.0);
	m_robot->setRightTravelDistance(0.0);
}