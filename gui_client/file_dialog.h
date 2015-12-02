#ifndef FILE_DIALOG_H
#define FILE_DIALOG_H

#include <QDialog>

namespace Ui {
class FileDialog;
}

class FileDialog : public QDialog
{
  Q_OBJECT

public:
  enum FileDialogType {kCreateDialog, kOpenDialog};

  explicit FileDialog(FileDialogType type, QWidget *parent = 0);
  ~FileDialog();
signals:
  void Accepted(QString name, unsigned int redundancy);
  void Accepted(QString name);

private slots:
  void Accept();
  void HandleText(const QString &text);

private:
  Ui::FileDialog *ui;
  FileDialogType type_;
};

#endif // FILE_DIALOG_H
