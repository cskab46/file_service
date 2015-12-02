#ifndef FILE_FRAME_H
#define FILE_FRAME_H

#include <QFrame>
#include <QString>

namespace Ui {
class FileFrame;
}

class FileFrame : public QFrame
{
  Q_OBJECT

public:
  explicit FileFrame(QString file_name, QWidget *parent = 0);
  ~FileFrame();

signals:
  void OpenRequested(QString file_name);
  void DeleteRequested(QString file_name);
protected:
  virtual void mouseDoubleClickEvent(QMouseEvent *event);
  void contextMenuEvent(QContextMenuEvent *event);
  void resizeEvent(QResizeEvent *event);
private:
  Ui::FileFrame *ui;
  QString file_name_;
};

#endif // FILE_FRAME_H
