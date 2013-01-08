#ifndef _KOVAN_BUTTON_PROVIDER_HPP_
#define _KOVAN_BUTTON_PROVIDER_HPP_

#include <QObject>

class AbstractTextButton;

namespace Kovan
{
	class ButtonProvider : public QObject
	{
	Q_OBJECT
	public:
		enum ButtonId {
			A,
			B,
			C,
			X,
			Y,
			Z
		};
		
		ButtonProvider(QObject *parent = 0);
		~ButtonProvider();
		
		virtual bool isExtraShown() const;
		virtual QString text(const ButtonProvider::ButtonId& id) const;
		virtual bool setPressed(const ButtonProvider::ButtonId& id, bool pressed) const;
		
	public slots:
		virtual void reset();
		virtual void refresh();
	
	signals:
		void buttonTextChanged(const ButtonProvider::ButtonId& id, const QString& text);
		void extraShownChanged(const bool& shown);
	
	private:
		AbstractTextButton *lookup(const ButtonProvider::ButtonId& id) const;
	};
}

#endif
