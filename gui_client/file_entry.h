#ifndef FILE_ENTRY_H
#define FILE_ENTRY_H

#include <QAbstractButton>

namespace Ui {
class FileEntry;
}

class Connection;

class FileEntry : public QWidget {
  Q_OBJECT

public:
  explicit FileEntry(QString file_name, QWidget *parent = 0);
  ~FileEntry();
signals:
  void OpenRequested(QString file_name);
  void DeleteRequested(QString file_name);
protected:
  virtual void mouseDoubleClickEvent(QMouseEvent *event);
  void contextMenuEvent(QContextMenuEvent *event);
  void resizeEvent(QResizeEvent *event);
private:
  Ui::FileEntry *ui;
  QString file_name_;
};

#endif // FILE_ENTRY_H
