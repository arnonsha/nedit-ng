
#include "DialogPreferences.h"
#include "BorderLayout.h"
#include <QVBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QStackedWidget>
#include <QDialogButtonBox>

DialogPreferences::DialogPreferences(QWidget *parent, Qt::WindowFlags f) : QDialog(parent, f) {

	auto title         = new QLabel(tr("Page Title"));
	auto search        = new QLineEdit;
	auto categories    = new QListWidget;
	auto stackedWidget = new QStackedWidget;
	auto buttons       = new QDialogButtonBox;

	buttons->addButton(QDialogButtonBox::Close);
	buttons->addButton(QDialogButtonBox::Apply);
	buttons->addButton(QDialogButtonBox::Help);

	auto centerLayout = new QVBoxLayout;
	centerLayout->addWidget(title);
	centerLayout->addWidget(stackedWidget);

	auto leftLayout = new QVBoxLayout;
	leftLayout->addWidget(search);
	leftLayout->addWidget(categories);

	auto layout = new BorderLayout;
	layout->add(centerLayout,  BorderLayout::Center);
	layout->add(leftLayout,    BorderLayout::West);
	layout->addWidget(buttons, BorderLayout::South);

	this->setLayout(layout);
}
