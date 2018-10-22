
#ifndef DIALOG_PREFERENCES_H_
#define DIALOG_PREFERENCES_H_

#include <QDialog>

class DialogPreferences : public QDialog {
	Q_OBJECT
public:
	DialogPreferences(QWidget *parent = nullptr, Qt::WindowFlags f = Qt::WindowFlags());
	~DialogPreferences() noexcept override = default;
};

#endif
