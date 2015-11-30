#include "create_file_dialog.h"
#include "ui_create_file_dialog.h"

CreateFileDialog::CreateFileDialog(QWidget *parent) :
  QDialog(parent),
  ui(new Ui::CreateFileDialog)
{
  ui->setupUi(this);
}

CreateFileDialog::~CreateFileDialog()
{
  delete ui;
}
