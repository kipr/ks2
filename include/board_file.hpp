#ifndef _BOARD_FILE_HPP_
#define _BOARD_FILE_HPP_

#include <QString>

class QGraphicsScene;

class BoardFile
{
public:
	enum Type
	{
		Real = 0,
		Tape,
		Fake
	};
	
	static QGraphicsScene *load(const QString &path);
	
private:
	static void parse(const QString &contents, QGraphicsScene *scene);
	static void error(const quint32 &line, const QString &id);
	static void error(const quint32 &line, const quint16 &expecting, const quint16 &got);
};

#endif
