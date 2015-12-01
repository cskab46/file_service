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

public slots:
  void NewFile();
  void SetStatusMessage(QString &message);

  void CreateFile(QString file_name, unsigned int redundancy);
  void RemoveFile(QString file_name);

private:
  Ui::MainWindow *ui;
  bool connection_error_;
  Connection connection_;
  QMap<QString, FileEntry*> file_entries_;
};

#endif // MAINWINDOW_H
