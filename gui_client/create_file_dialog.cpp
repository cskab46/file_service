#include "create_file_dialog.h"
#include "ui_create_file_dialog.h"

#include <QPushButton>

CreateFileDialog::CreateFileDialog(QWidget *parent) :
  QDialog(parent),
  ui(new Ui::CreateFileDialog) {
  ui->setupUi(this);
  connect(ui->buttonBox, SIGNAL(accepted()), this, SLOT(Accept()));
  connect(ui->buttonBox, SIGNAL(rejected()), this, SLOT(reject()));

  ui->buttonBox->button(ui->buttonBox->Ok)->setEnabled(false);
  ui->nameLineEdit->setValidator(new QRegExpValidator(QRegExp("[^?<>:*|\"]+"), this));
  connect(ui->nameLineEdit, SIGNAL(textChanged(QString)), this, SLOT(HandleText(QString)));
}

CreateFileDialog::~CreateFileDialog() {
  delete ui;
}

void CreateFileDialog::Accept() {
  emit accepted(ui->nameLineEdit->text(), ui->redundancySpinBox->value());
}

void CreateFileDialog::HandleText(const QString &text) {
  ui->buttonBox->button(ui->buttonBox->Ok)->setEnabled(!text.isEmpty());
}
