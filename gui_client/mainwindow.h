#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMap>

#include "utils/connection.h"

namespace Ui {
class MainWindow;
}

class FileEntry;


class MainWindow : public QMainWindow
{
  Q_OBJECT

public:
  explicit MainWindow(QWidget *parent = 0);
  ~MainWindow();
  void contextMenuEvent(QContextMenuEvent *event);

signals:
  void FileWritten(QString file_name, bool result);
  void FileRead(QString file_name, QString data, bool result);
  void FileCreated(QString file_name, bool result);
  void FileRemoved(QString file_name, bool result);


public slots:
  void SetStatusMessage(QString message);

  void Open(QString file_name);
  void CloseCurrent();
  void SaveCurrent();
  void ReloadCurrent();

  void CreateFile(QString file_name, unsigned int redundancy);
  void LoadFile(QString file_name);
  void RemoveFile(QString file_name);

private:
  Ui::MainWindow *ui;
  bool connection_error_;
  Connection connection_;
  QMap<QString, FileEntry*> file_entries_;
  QString current_file_;
};

#endif // MAINWINDOW_H
