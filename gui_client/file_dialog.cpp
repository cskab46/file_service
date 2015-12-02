#include "file_dialog.h"
#include "ui_file_dialog.h"

#include <QPushButton>

FileDialog::FileDialog(FileDialogType type, QWidget *parent) :
  QDialog(parent),
  ui(new Ui::FileDialog),
  type_(type){
  ui->setupUi(this);
  connect(ui->buttonBox, SIGNAL(accepted()), this, SLOT(Accept()));
  connect(ui->buttonBox, SIGNAL(rejected()), this, SLOT(reject()));

  ui->buttonBox->button(ui->buttonBox->Ok)->setEnabled(false);
  ui->nameLineEdit->setValidator(new QRegExpValidator(QRegExp("[^?<>:*|\"]+"), this));
  connect(ui->nameLineEdit, SIGNAL(textChanged(QString)), this, SLOT(HandleText(QString)));
  if (type_ == kOpenDialog) {
  }
  switch(type_) {
  case kOpenDialog:
    ui->redundancy->hide();
    setWindowTitle("Open File Dialog");
    break;
  case kCreateDialog:
    setWindowTitle("Create File Dialog");
    break;
  default:
    break;
  };
}

FileDialog::~FileDialog() {
  delete ui;
}

void FileDialog::Accept() {
  switch(type_) {
  case kOpenDialog:
    emit Accepted(ui->nameLineEdit->text());
    break;
  case kCreateDialog:
    emit Accepted(ui->nameLineEdit->text(), ui->redundancySpinBox->value());
    break;
  default:
    break;
  };
}

void FileDialog::HandleText(const QString &text) {
  ui->buttonBox->button(ui->buttonBox->Ok)->setEnabled(!text.isEmpty());
}
