#pragma once

#include <QObject>

#include <future>
#include <vector>

class UdpServer : public QObject
{
   Q_OBJECT
public:
   UdpServer();
   ~UdpServer();
   void Start();
   void Stop();

signals:
   void updateFrame(const QImage &img) const;
   void receivedData(char *buffer, size_t size) const;
protected:
   int Server(std::vector<unsigned char> &data) const;
   void DecodeHevc(char *buffer, size_t size) const;
   bool ParseJpegFrame(std::vector<unsigned char> &data) const;
   bool WriteJpegToFile(std::vector<unsigned char> &jpeg) const;

   std::future<void> Future;
   std::vector<unsigned char> Buffer;
   bool Stopped = false;
};
