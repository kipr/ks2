#include "console_widget.hpp"

#include <QApplication>
#include <QKeyEvent>
#include <QDebug>

ConsoleWidget::ConsoleWidget(QWidget *parent)
	: QTextEdit(parent), m_process(0)
{
	cmdStr = "";
	curCursorLoc = this->textCursor();
	inputCharCount = 0;
	histLocation = -1;
	tempCmd = "";
	setProcess(0);
}

ConsoleWidget::~ConsoleWidget()
{
	setProcess(0);
}

void ConsoleWidget::setProcess(QIODevice *process)
{
	if(m_process) m_process->disconnect(this);
	m_process = process;
	setReadOnly(!m_process);
	if(!m_process) return;
	clear();
	
	connect(m_process, SIGNAL(readyRead()), this, SLOT(readStandardOut()));
}

QIODevice *ConsoleWidget::process() const
{
	return m_process;
}

void ConsoleWidget::readStandardOut()
{
	QByteArray array = m_process->readAll();
	int i = array.lastIndexOf('\f');
	if(i >= 0) {
		setPlainText("");
		array = array.mid(i + 1);
	}
	insertPlainText(array);
	moveCursor(QTextCursor::End, QTextCursor::KeepAnchor);
	update();
}