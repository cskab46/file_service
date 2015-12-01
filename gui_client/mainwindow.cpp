#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QMenu>
#include <QContextMenuEvent>
#include <QStatusBar>

#include "flowlayout.h"
#include "file_entry.h"
#include "create_file_dialog.h"

#include "utils/groups.h"
#include "utils/file_ops.h"
#include "utils/client_ops.h"

MainWindow::MainWindow(QWidget *parent) :
  QMainWindow(parent),
  ui(new Ui::MainWindow),
  connection_error_(false),
  connection_(Connection::Connect(connection_error_)){
  ui->setupUi(this);
  ui->centralwidget->setLayout(new FlowLayout);

  if (connection_error_) {
    ui->statusbar->showMessage("Connection error. Please restart the application.");
  } else {
    ui->statusbar->showMessage("Connected to the server.");
  }
}

MainWindow::~MainWindow() {
  delete ui;
}

void MainWindow::contextMenuEvent(QContextMenuEvent *event) {
  QMenu menu;
  menu.move(event->globalPos());
  menu.addAction("&New file", this, SLOT(NewFile()));
  menu.exec();
}

void MainWindow::NewFile() {
  CreateFileDialog dlg(this);
  connect(&dlg, SIGNAL(accepted(QString,uint)), this, SLOT(CreateFile(QString,uint)));
  dlg.exec();
}

void MainWindow::SetStatusMessage(QString &message) {
  ui->statusbar->showMessage(message);
}

void MainWindow::CreateFile(QString file_name, unsigned int redundancy) {
  ui->centralwidget->layout()->addWidget(new FileEntry(file_name, connection_));
//  SetStatusMessage(QString("Creating file: ") + file_name + " with redundandy " + QString::number(redundancy));
}

void MainWindow::RemoveFile(QString file_name) {
//  SetStatusMessage(QString("Removing file: ") + file_name);
}
