#include "file_entry.h"
#include "ui_file_entry.h"

FileEntry::FileEntry(QString file_name, Connection &con, QWidget *parent) :
  QWidget(parent),
  ui(new Ui::FileEntry),
  file_name_(file_name),
  connection_(con) {
  ui->setupUi(this);
}

FileEntry::~FileEntry() {
  delete ui;
}
