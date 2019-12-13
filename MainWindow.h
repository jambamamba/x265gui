#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMimeData>
#include <QDragEnterEvent>
#include <QDropEvent>

#include "UdpServer.h"
#include "libde265/dec265/hevcreader.h"
#include "libde265/dec265/yuvwriter.h"

namespace Ui {
class MainWindow;
}


class MainWindow : public QMainWindow
{
   Q_OBJECT

public:
   explicit MainWindow(QWidget *parent = nullptr);
   ~MainWindow();

signals:
   void updateFrame(const QImage &img) const;
private slots:
   void on_updateFrame(const QImage &img) const;
   void on_toolButtonBrowse_clicked();
   void on_lineEditWidth_textChanged(const QString &arg1);
   void on_lineEditHeight_textChanged(const QString &arg1);
   void on_lineEditFile_textChanged(const QString &arg1);

   void on_lineEditFile_editingFinished();

protected:
   void LoadRawImage() const;
   void LoadYuvImage(const uint8_t *yuv, size_t sz) const;
   void dragEnterEvent(QDragEnterEvent *e);
   void dropEvent(QDropEvent *e);

private:
   Ui::MainWindow *ui;
   QString m_imageFile;
   int m_width;
   int m_height;
   UdpServer Server;
   HevcReader Hevc;
   YuvWriter Yuv;
   std::future<void> Future;
};

#endif // MAINWINDOW_H
