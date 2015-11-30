#ifndef CREATE_FILE_DIALOG_H
#define CREATE_FILE_DIALOG_H

#include <QDialog>

namespace Ui {
class CreateFileDialog;
}

class CreateFileDialog : public QDialog
{
  Q_OBJECT

public:
  explicit CreateFileDialog(QWidget *parent = 0);
  ~CreateFileDialog();

private:
  Ui::CreateFileDialog *ui;
};

#endif // CREATE_FILE_DIALOG_H
