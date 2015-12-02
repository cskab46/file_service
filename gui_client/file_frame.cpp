#include "file_frame.h"
#include "ui_file_frame.h"

#include <QFontMetrics>
#include <QMenu>
#include <QContextMenuEvent>

FileFrame::FileFrame(QString file_name, QWidget *parent) :
  QFrame(parent),
  ui(new Ui::FileFrame),
  file_name_(file_name) {
  ui->setupUi(this);
  ui->nameLabel->setToolTip(file_name_);
}

FileFrame::~FileFrame() {
  delete ui;
}

void FileFrame::mouseDoubleClickEvent(QMouseEvent *event) {
  emit OpenRequested(file_name_);
}

void FileFrame::contextMenuEvent(QContextMenuEvent *event) {
  QMenu menu;
  menu.move(event->globalPos());
  auto open = menu.addAction("&Open");
  auto del = menu.addAction("&Delete");

  connect(open, &QAction::triggered,
                 [=]() { emit OpenRequested(file_name_); });
  connect(del, &QAction::triggered,
                 [=]() { emit DeleteRequested(file_name_); });

  menu.exec();
}

void FileFrame::resizeEvent(QResizeEvent *event) {
  auto fm = ui->nameLabel->fontMetrics();
  ui->nameLabel->setText(fm.elidedText(file_name_, Qt::ElideRight,
                                       ui->nameLabel->width()));
}
