#include "file_entry.h"
#include "ui_file_entry.h"

#include <QFontMetrics>
#include <QMenu>
#include <QContextMenuEvent>

FileEntry::FileEntry(QString file_name, QWidget *parent) :
  QWidget(parent),
  ui(new Ui::FileEntry),
  file_name_(file_name) {
  ui->setupUi(this);
  ui->nameLabel->setToolTip(file_name_);
}

FileEntry::~FileEntry() {
  delete ui;
}

void FileEntry::mouseDoubleClickEvent(QMouseEvent *event) {
  emit OpenRequested(file_name_);
}

void FileEntry::contextMenuEvent(QContextMenuEvent *event) {
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

void FileEntry::resizeEvent(QResizeEvent *event) {
  auto fm = ui->nameLabel->fontMetrics();
  ui->nameLabel->setText(fm.elidedText(file_name_, Qt::ElideRight,
                                       ui->nameLabel->width()));
}
