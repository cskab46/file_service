#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QMenu>
#include <QContextMenuEvent>
#include <QStatusBar>
#include <QTimer>

#include "flowlayout.h"
#include "file_entry.h"
#include "file_dialog.h"
#include "client.h"

#include "utils/groups.h"
#include "utils/file_ops.h"
#include "utils/client_ops.h"

MainWindow::MainWindow(QWidget *parent) :
  QMainWindow(parent),
  ui(new Ui::MainWindow),
  connection_error_(false),
  connection_(Connection::Connect(connection_error_)){
  ui->setupUi(this);
  ui->fileList->setLayout(new FlowLayout);

  if (connection_error_) {
    ui->statusbar->showMessage("Connection error. Please restart the application.");
    return;
  }

  ui->statusbar->showMessage("Connected to the server.");

  ui->menuBar->hide();
  auto menu = ui->menuBar->addMenu("&File");
  auto save = menu->addAction("&Save");
  auto reload = menu->addAction("&Reload");
  auto close = menu->addAction("&Close");

  connect(save, &QAction::triggered, this, &MainWindow::SaveCurrent);
  connect(reload, &QAction::triggered, this, &MainWindow::ReloadCurrent);
  connect(close, &QAction::triggered, this, &MainWindow::CloseCurrent);

  ui->stackedWidget->setCurrentIndex(0);
}

MainWindow::~MainWindow() {
  delete ui;
}

void MainWindow::contextMenuEvent(QContextMenuEvent *event) {
  QMenu menu;
  menu.move(event->globalPos());
  auto nf = menu.addAction("&New file");
  auto existing = menu.addAction("&Add existing");
  auto sync = menu.addAction("&Sync All");

  connect(nf, &QAction::triggered, [this] () {
    FileDialog dlg(FileDialog::kCreateDialog, this);
    connect(&dlg, SIGNAL(Accepted(QString,uint)), this, SLOT(CreateFile(QString,uint)));
    dlg.exec();
  });

  connect(existing, &QAction::triggered, [this] () {
    FileDialog dlg(FileDialog::kOpenDialog, this);
    connect(&dlg, SIGNAL(Accepted(QString)), this, SLOT(LoadFile(QString)));
    dlg.exec();
  });

  connect(sync, &QAction::triggered, [this] () {
    for(auto f : file_entries_.keys()) {
      std::string data;
      if(!HandleReadOp(connection_, f.toStdString(), data)) {
        SetStatusMessage("Could not read file " + f + ". Removing the entry.");
        file_entries_[f]->deleteLater();
        file_entries_.remove(f);
      }
    }
  });
  menu.exec();
}

void MainWindow::SetStatusMessage(QString message) {
  ui->statusbar->showMessage(message);
}

void MainWindow::Open(QString file_name) {
  auto timer = new QTimer(this);
  timer->setSingleShot(true);
  connect(timer, &QTimer::timeout, [this, file_name] () {
    string data;
    if(!HandleReadOp(connection_, file_name.toStdString(), data)) {
      SetStatusMessage("Could not open file " + file_name + ".");
      return;
    }
    current_file_ = file_name;
    ui->menuBar->show();
    ui->textEdit->setText(QString::fromStdString(data));
    ui->stackedWidget->setCurrentIndex(1);
  });
  timer->start();
}

void MainWindow::CloseCurrent() {
  ui->menuBar->hide();
  ui->stackedWidget->setCurrentIndex(0);
}

void MainWindow::SaveCurrent() {
  auto timer = new QTimer;
  timer->setSingleShot(true);
  connect(timer, &QTimer::timeout, [this] () {
    if(!HandleWriteOp(connection_, current_file_.toStdString(),
                      ui->textEdit->toPlainText().toStdString())) {
      SetStatusMessage("Could not write to file " + current_file_ + ".");
      return;
    }
    SetStatusMessage("File " + current_file_ + " written.");
  });
  timer->start();
}

void MainWindow::ReloadCurrent() {
  auto timer = new QTimer;
  timer->setSingleShot(true);
  connect(timer, &QTimer::timeout, [this] () {
    string data;
    if(!HandleReadOp(connection_, current_file_.toStdString(), data)) {
      SetStatusMessage("Could not reload file " + current_file_ + ".");
      return;
    }
    SetStatusMessage("File " + current_file_ + " reloaded.");
    ui->textEdit->setText(QString::fromStdString(data));
  });
  timer->start();
}


void MainWindow::CreateFile(QString file_name, unsigned int redundancy) {
  if (file_entries_.contains(file_name)) {
    SetStatusMessage("File entry already exists. Try reloading it.");
    return;
  }

  if (!HandleCreateOp(connection_, file_name.toStdString(), redundancy)) {
    SetStatusMessage("Failed to create file.");
    return;
  }
  auto fe = new FileEntry(file_name);
  file_entries_[file_name] = fe;
  this->connect(fe, SIGNAL(OpenRequested(QString)), this , SLOT(Open(QString)));
  this->connect(fe, SIGNAL(DeleteRequested(QString)), this , SLOT(RemoveFile(QString)));

  ui->fileList->layout()->addWidget(fe);
  SetStatusMessage("File created.");
}

void MainWindow::LoadFile(QString file_name) {
  if (file_entries_.contains(file_name)) {
    SetStatusMessage("File entry already exists. Try reloading it.");
    return;
  }

  string data;
  if (!HandleReadOp(connection_, file_name.toStdString(), data)) {
    SetStatusMessage("Failed to load file.");
    return;
  }
  auto fe = new FileEntry(file_name);
  file_entries_[file_name] = fe;
  this->connect(fe, SIGNAL(OpenRequested(QString)), this , SLOT(Open(QString)));
  this->connect(fe, SIGNAL(DeleteRequested(QString)), this , SLOT(RemoveFile(QString)));

  ui->fileList->layout()->addWidget(fe);
  SetStatusMessage("File loaded.");
}

void MainWindow::RemoveFile(QString file_name) {
  if (!file_entries_.contains(file_name)) {
    SetStatusMessage("File entry does not exists.");
    return;
  }

  if (!HandleRemoveOp(connection_, file_name.toStdString())) {
    SetStatusMessage("Failed to remove file.");
    return;
  }
  file_entries_[file_name]->deleteLater();
  file_entries_.remove(file_name);
  SetStatusMessage("File removed.");
}
