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
#include "light.hpp"
#include "simulator.hpp"
#include "board_file.hpp"
#include "server_thread.hpp"
#include "kovan_regs_p.hpp"
#include "heartbeat.hpp"

#ifdef WIN32
#include <winsock2.h>
#endif

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
	m_light(new Light()),
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
	// m_analogs[7] = ui->analog7;
	
	m_digitals[0] = ui->digital0;
	m_digitals[1] = ui->digital1;
	// m_digitals[2] = ui->digital2;
	// m_digitals[3] = ui->digital3;
	// m_digitals[4] = ui->digital4;
	// m_digitals[5] = ui->digital5;
	// m_digitals[6] = ui->digital6;
	// m_digitals[7] = ui->digital7;
	
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
	ui->sim->setSceneRect(0.0, 0.0, 275.0, 275.0);

	
	if(ui->sim->scene())
	foreach(QGraphicsItem *item, m_robot->robot())
		ui->sim->scene()->addItem(item);
		
	m_robot->robot()[0]->setRotation(45);
	
	if(ui->sim->scene()) ui->sim->scene()->addItem(m_light);
	
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
	
	connect(ui->actionReset, SIGNAL(activated()), SLOT(reset()));

	bool ret = m_kmod->setup();
	if (!ret) qWarning() << "m_kmod->setup() failed.  (main_window.cpp : " << __LINE__ << ")";

	TcpServer *serial = new TcpServer;
	if(!serial->bind(KOVAN_SERIAL_PORT)) {
		perror("bind");
#ifdef WIN32
		qCritical() << WSAGetLastError();
#endif
	}
	if(!serial->listen(2)) {
		perror("listen");
#ifdef WIN32
		qCritical() << WSAGetLastError();
#endif
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

#define TIMEDIV (1.0 / 13000000) // 13 MHz clock
#define PWM_PERIOD_RAW 0.02F
#define SERVO_MAX_RAW 0.0025f
#define SERVO_MIN_RAW 0.0005f
#define PWM_PERIOD ((unsigned int)(PWM_PERIOD_RAW / TIMEDIV))
#define SERVO_MAX (SERVO_MAX_RAW / TIMEDIV)
#define SERVO_MIN (SERVO_MIN_RAW / TIMEDIV)

void MainWindow::update()
{
	if(!m_process) return;
	
	m_buttonProvider->refresh();
	m_robot->update();
	
	Kovan::State &s = m_kmod->state();


	static const int motors[4] = {
		MOTOR_PWM_0,
		MOTOR_PWM_1,
		MOTOR_PWM_2,
		MOTOR_PWM_3
	};
	
	static const int servos[4] = {
		SERVO_COMMAND_0,
		SERVO_COMMAND_1,
		SERVO_COMMAND_2,
		SERVO_COMMAND_3
	};

	unsigned short modes = s.t[PID_MODES];

	static const int GOAL_EPSILON = 20;
	static const int MOTOR_SCALE = 500;

	for(int i = 0; i < 4; ++i) {

		unsigned char mode = (modes >> ((3 - i) * 2)) & 0x3;
		double val = 0.0;
		bool pwm = false;
		int code = 0;

		int pos_goal = ((int)s.t[GOAL_POS_0_HIGH + i] << 16) | s.t[GOAL_POS_0_LOW + i];
		int pos_err = 0;

		// TODO: are left/right switched?
		if (i == 1){
			 pos_err = pos_goal - MOTOR_SCALE*(int)m_robot->rightTravelDistance();
		}else if (i == 3){
			 pos_err = pos_goal - MOTOR_SCALE*(int)m_robot->leftTravelDistance();
		}


		int desired_speed = s.t[(GOAL_SPEED_0_HIGH + i)] << 16 | s.t[(GOAL_SPEED_0_LOW + i)];

		switch(mode){

		case 0: // pwm

			code = (s.t[MOTOR_DRIVE_CODE_T] >> ((3 - i) * 2)) & 0x3;
			val = s.t[MOTOR_PWM_0 + i] / 2600.0;
			if(code == 1) val = -val;
			else if(code != 2) val = 0.0;
			pwm = true;
			if(val > 1.0) val = 1.0;
			break;

		case 1: // position

			if ((pos_err > 0 && pos_err < GOAL_EPSILON)
							|| (pos_err < 0 && pos_err > -GOAL_EPSILON)){
				val = 0.0;
			}else{

				val = pos_err / 2000.0;

				if (val > 1.0){
					val = 1.0;
				}else if (val < -1.0){
					val = -1.0;
				}
			}
			break;

		case 2: // speed

			val = (desired_speed) / 1000.0;
			break;

		case 3: // position at speed

			if ((pos_err > 0 && pos_err < GOAL_EPSILON)
							|| (pos_err < 0 && pos_err > -GOAL_EPSILON)
							|| (pos_err < 0 && desired_speed > 0)
							|| (pos_err > 0 && desired_speed < 0)){
				val = 0.0;
			}else{
				val = (desired_speed) / 1000.0;
			}
			break;
		}
		
		const double m = 2.5;
		int port = unfixPort(i);
		if(port == 2) {
			m_robot->setLeftSpeed(val * (pwm ? m : 1.0));
		} else if(port == 0) {
			m_robot->setRightSpeed(val * (pwm ? m : 1.0));
		}
		
		m_motors[port]->setValue(val * 100.0);
		m_servos[i]->setValue(2048.0 * ((s.t[servos[i]] << 8) - SERVO_MIN) / (SERVO_MAX - SERVO_MIN));
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
	
	s.t[analogs[0]] = m_robot->leftRange() / m_robot->rangeLength() * 1023.0;
	s.t[analogs[1]] = m_robot->frontRange() / m_robot->rangeLength() * 1023.0;
	s.t[analogs[2]] = m_robot->rightRange() / m_robot->rangeLength() * 1023.0;
	
	
	setDigital(0, s.t[analogs[0]] < 150);
	setDigital(1, s.t[analogs[2]] < 150);

	const double lRad = M_PI * (m_robot->robot()[0]->rotation() + 45.0) / 180.0;
	const double rRad = M_PI * (m_robot->robot()[0]->rotation() - 45.0) / 180.0;

	const double lightSensorDisplacement = 15.0;

	const QPointF leftLightSensorPos = m_robot->robot()[0]->pos() +
			lightSensorDisplacement * QPointF(cos(lRad), sin(lRad));

	const QPointF rightLightSensorPos = m_robot->robot()[0]->pos() +
			lightSensorDisplacement * QPointF(cos(rRad), sin(rRad));

	QLineF leftLightline(leftLightSensorPos, m_light->pos());
	QLineF rightLightline(rightLightSensorPos, m_light->pos());

	double leftLightValue = leftLightline.length() / 50.0 * 1023.0;
	if(leftLightValue > 1023.0) leftLightValue = 1023.0;
	s.t[analogs[3]] = m_light->isOn() ? leftLightValue : 1023.0;

	double rightLightValue = rightLightline.length() / 50.0 * 1023.0;
	if(rightLightValue > 1023.0) rightLightValue = 1023.0;
	s.t[analogs[4]] = m_light->isOn() ? rightLightValue : 1023.0;

	for(int i = 0; i < 8; ++i) {
		if(i < 7) m_analogs[i]->setText(QString::number(s.t[analogs[i]]));
		if(i < 2) m_digitals[i]->setText(s.t[DIG_IN] & (1 << (7 - i)) ? "0" : "1");
	}
	
	s.t[analogs[5]] = m_robot->leftReflectance() * 1023.0;
	s.t[analogs[6]] = m_robot->rightReflectance() * 1023.0;

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
	
	m_process->kill();
	m_process->waitForFinished();
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
	m_robot->reset();
	m_light->reset();
	m_robot->robot()[0]->setRotation(45);
}

void MainWindow::setDigital(int port, bool on)
{
	Kovan::State &s = m_kmod->state();
	if(!on) s.t[DIG_IN] |= 1 << (7 - port);
	else s.t[DIG_IN] &= ~(1 << (7 - port));
}
