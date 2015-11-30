#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

namespace Ui {
class MainWindow;
}

#include "utils/connection.h"

class MainWindow : public QMainWindow
{
  Q_OBJECT

public:
  explicit MainWindow(QWidget *parent = 0);
  ~MainWindow();
  void contextMenuEvent(QContextMenuEvent *event);

public slots:
  void NewDocument();
  void SetStatusMessage(QString &message);

private:
  Ui::MainWindow *ui;
  bool connection_error_;
  Connection connection_;
};

#endif // MAINWINDOW_H
