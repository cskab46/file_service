#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QMenu>
#include <QContextMenuEvent>
#include <QStatusBar>

#include "flowlayout.h"
#include "file_entry.h"

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
  menu.addAction("&New document", this, SLOT(NewDocument()));
  menu.exec();
}

void MainWindow::NewDocument() {
  ui->centralwidget->layout()->addWidget(new FileEntry("Teste.txt", connection_));
}

void MainWindow::SetStatusMessage(QString &message) {
  ui->statusbar->showMessage(message);
}
