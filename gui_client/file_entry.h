#ifndef FILE_ENTRY_H
#define FILE_ENTRY_H

#include <QWidget>

namespace Ui {
class FileEntry;
}

class Connection;

class FileEntry : public QWidget
{
  Q_OBJECT

public:
  explicit FileEntry(QString file_name, Connection &con, QWidget *parent = 0);
  ~FileEntry();

private:
  Ui::FileEntry *ui;
  QString file_name_;
  Connection &connection_;
};

#endif // FILE_ENTRY_H
