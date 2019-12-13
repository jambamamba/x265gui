#include "MainWindow.h"
#include "ui_MainWindow.h"

#include <QIODevice>
#include <QFileDialog>
#include <QDesktopServices>
#include <QUrl>
#include <QMessageBox>
#include <QImage>
#include <QDebug>
#include <QRgb>

#include <algorithm>
#include <future>

int dec265main(int argc, char** argv, HevcReader *hevcreader, YuvWriter *yuvWriter);

namespace  {

struct __attribute__ ((packed)) ImageMetaData
{
   uint32_t FrameStartSec;
   uint32_t FrameStartUsec;
   uint16_t HSize;
   uint16_t VSize;
   uint32_t FrameNumber;
   uint32_t Sevent;
   uint32_t FrameEndSec;
   uint32_t FrameEndUsec;
   uint32_t Unused;
};
}
MainWindow::MainWindow(QWidget *parent)
   : QMainWindow(parent)
   , ui(new Ui::MainWindow)
   , m_width(600)
   , m_height(700)
   , Hevc([](){
      QApplication::processEvents();
   })
   , Yuv([this](const uint8_t* data, size_t size){
      LoadYuvImage(data, size);
   })
{
   ui->setupUi(this);
   ui->lineEditWidth->setText(QString("%1").arg(m_width));
   ui->lineEditHeight->setText(QString("%1").arg(m_height));

   setAcceptDrops(true);
   setWindowTitle("x265gui 1.0");

   on_lineEditFile_textChanged("");

   Server.Start();

   connect(this, &MainWindow::updateFrame, this, &MainWindow::on_updateFrame, Qt::QueuedConnection);
   connect(&Server, &UdpServer::updateFrame, this, &MainWindow::on_updateFrame, Qt::QueuedConnection);
   connect(&Server, &UdpServer::receivedData, [this](char *buffer, size_t size){
                                                         Hevc.receivedData(buffer, size);
                                                      });

   auto func = [this] {
      char *argv[4] = {
         "dec265",
         "c:/temp/frames2.hevc",
         "--output",
         "c:/temp/frames.yuv"
      };
      dec265main(4, argv, &Hevc, &Yuv);
//      dec265main(4, argv, nullptr, nullptr);
   };
   Future = std::async(std::launch::async, func);
}

MainWindow::~MainWindow()
{
   Server.Stop();
   Future.wait();
   delete ui;
}

void MainWindow::on_updateFrame(const QImage &img) const
{
   QPixmap pix = QPixmap::fromImage(img);
   ui->labelImage->clear();
   ui->labelImage->setPixmap(pix);
}

void MainWindow::on_toolButtonBrowse_clicked()
{
   auto fileName = QFileDialog::getOpenFileName(this,
       tr("Load Arm53 Bundle"), "", tr("rgb files (*.rgb);;raw files (*.raw);;All files (*)"));
   if(!fileName.isEmpty())
   {
      m_imageFile = fileName;
      ui->lineEditFile->setText(fileName);
      LoadRawImage();
   }
}

void MainWindow::LoadYuvImage(const uint8_t *yuv, size_t sz) const
{
   int width = ui->lineEditWidth->text().toInt();
   int height = ui->lineEditHeight->text().toInt();
   QImage img(width, height, QImage::Format::Format_RGB888);

   int frameSize = width * height;
   int chromasize = frameSize;
   {
       int yIndex = 0;
       int uIndex = frameSize;
       int vIndex = frameSize + chromasize;
       int index = 0;
       int Y = 0, U = 0, V = 0;
       for (int j = 0; j < height; j++)
       {
           for (int i = 0; i < width; i++)
           {
               Y = yuv[yIndex++];
               U = yuv[uIndex++];
               V = yuv[vIndex++];
               index++;

               float R = Y + 1.402 * (V - 128);
               float G = Y - 0.344 * (U - 128) - 0.714 * (V - 128);
               float B = Y + 1.772 * (U - 128);

               if (R < 0){ R = 0; } if (G < 0){ G = 0; } if (B < 0){ B = 0; }
               if (R > 255 ){ R = 255; } if (G > 255) { G = 255; } if (B > 255) { B = 255; }

               img.setPixelColor(i, j, qRgb(R, G, B));
           }
       }
   }

   emit updateFrame(img);

  static int frame = 0;
  qDebug() << "Frame " << frame++;
}

void MainWindow::LoadRawImage() const
{
   QFile file(m_imageFile);
   if(!file.exists())
   {
      return;
   }
   if(!file.open(QIODevice::ReadOnly))
   {
       QMessageBox msgBox;
       msgBox.setText(QString("Could not load file: %1").arg(m_imageFile));
       msgBox.exec();
       return;
   }

   char metadata[256];
   size_t sz = file.size();
   file.read(metadata, sizeof(metadata));
   ImageMetaData *metaptr = (ImageMetaData*)metadata;
   int width = metaptr->HSize;
   int height = metaptr->VSize;
   QImage img(width, height, QImage::Format::Format_RGB888);
   for(int x = 0; x < width; ++x)
   {
      for(int y = 0; y < height; ++y)
      {
         char data[3] = {0};
         file.read(data, 3);
         img.setPixelColor(x, y, qRgb(data[2], data[1], data[0]));
         int i = 0;
         i = 1;
      }
   }
   QMatrix rm;
   rm.rotate(90);
   img = img.transformed(rm);

   emit updateFrame(img);
}

void MainWindow::on_lineEditWidth_textChanged(const QString &arg1)
{
   m_width = ui->lineEditWidth->text().toInt();
   LoadRawImage();
//   LoadYuvImage();
}

void MainWindow::on_lineEditHeight_textChanged(const QString &arg1)
{
   m_height = ui->lineEditHeight->text().toInt();
   LoadRawImage();
}

void MainWindow::dragEnterEvent(QDragEnterEvent *e)
{
    if (e->mimeData()->hasUrls()) {
        e->acceptProposedAction();
    }
}

void MainWindow::dropEvent(QDropEvent *e)
{
    foreach (const QUrl &url, e->mimeData()->urls()) {
        QString fileName = url.toLocalFile();
        qDebug() << "Dropped file:" << fileName;
        m_imageFile = fileName;
        ui->lineEditFile->setText(fileName);
        LoadRawImage();
        break;
    }
}

void MainWindow::on_lineEditFile_textChanged(const QString &arg1)
{
}

void MainWindow::on_lineEditFile_editingFinished()
{
   m_imageFile = ui->lineEditFile->text();
}
