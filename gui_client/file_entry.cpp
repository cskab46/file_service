#include "file_entry.h"
#include "ui_file_entry.h"

#include <QFontMetrics>
#include <QMenu>
#include <QContextMenuEvent>

FileEntry::FileEntry(QString file_name, Connection &con, QWidget *parent) :
  QWidget(parent),
  ui(new Ui::FileEntry),
  file_name_(file_name),
  connection_(con) {
  ui->setupUi(this);
  auto fm = ui->nameLabel->fontMetrics();

  ui->nameLabel->setText(fm.elidedText(file_name, Qt::ElideLeft, ui->nameLabel->width()));
  connect(this, SIGNAL(clicked()), this, SLOT(Open()));
}

FileEntry::~FileEntry() {
  delete ui;
}

void FileEntry::mouseDoubleClickEvent(QMouseEvent *event) {
  emit clicked();
}

void FileEntry::contextMenuEvent(QContextMenuEvent *event) {
  QMenu menu;
  menu.move(event->globalPos());
  menu.addAction("&Open", this, SLOT(Open()));
  menu.addAction("&Delete", this, SLOT(Delete()));

  menu.exec();
}

void FileEntry::Open() {

}

void FileEntry::Delete() {
  deleteLater();
}
