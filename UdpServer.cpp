#include "UdpServer.h"

#include <QImage>
#include <QDebug>

#include <stdio.h>
#include <errno.h>
#include <string.h>
#ifdef WIN32
#include <winsock2.h>
#else
#include <fcntl.h>
#include <netinet/in.h>
#endif
#include <sys/types.h>
#include <unistd.h> /* for close() for socket */
#include <stdlib.h>
#include <future>

namespace  {

int lastError()
{
#ifdef WIN32
    return WSAGetLastError();
#else
    return errno;
#endif
}
int sockInit(void)
{
#ifdef WIN32
   WSADATA wsa_data;
   return WSAStartup(MAKEWORD(1,1), &wsa_data);
#else
    return 0;
#endif
}

int sockQuit(void)
{
#ifdef WIN32
   return WSACleanup();
#else
    return 0;
#endif
}
}//namespace

UdpServer::UdpServer()
   : QObject(nullptr)
{
   sockInit();
}

UdpServer::~UdpServer()
{
   sockQuit();
}

void UdpServer::Start()
{
   Future = std::async(std::launch::async, [this]{
      Server(Buffer);
   });

}

void UdpServer::Stop()
{
   Stopped = true;
   Future.wait();
}
bool UdpServer::WriteJpegToFile(std::vector<unsigned char> &jpeg) const
{
   if(jpeg.size() == 0)
   {
      return false;
   }

   unsigned char *jpegdata = (unsigned char*)malloc(jpeg.size());
   for(int i = 0; i < jpeg.size(); ++i)
   {
      jpegdata[i] = jpeg.at(i);
   }

   QImage img = QImage::fromData(jpegdata, jpeg.size(), "JPEG");
   emit updateFrame(img);

   FILE* fp = fopen("c:/temp/foo.jpg", "wb");
   fwrite(jpegdata, jpeg.size(), 1, fp);
   fclose(fp);
   return true;
}
bool UdpServer::ParseJpegFrame(std::vector<unsigned char> &data) const
{
   /*JPEG files (compressed images) start with an image marker which always contains the marker code
    * hex values FF D8 FF. It does not have a length of the file embedded, thus we need to find
    * JPEG trailer, which is FF D9.*/
   if(data.size() < 3)
   { return false;}

   static std::vector<unsigned char> jpeg;
   if((data.at(0) == 0xff && data.at(1) == 0xd8 && data.at(2) == 0xff))
   {
      WriteJpegToFile(jpeg);
      jpeg.clear();
   }
   while(data.size() > 0)
   {
      jpeg.push_back(data.at(0));
      data.erase(data.begin());
   }
   return true;
}

int UdpServer::Server(std::vector<unsigned char> &data) const
{
  int sock;
  struct sockaddr_in sa;
  char buffer[8192];
  ssize_t recsize;
  unsigned int fromlen;

  memset(&sa, 0, sizeof sa);
  sa.sin_family = AF_INET;
  sa.sin_addr.s_addr = htonl(INADDR_ANY);
//  sa.sin_addr.s_addr = inet_addr("192.168.1.25");
  sa.sin_port = htons(7878);
  fromlen = sizeof sa;

  sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
  if (bind(sock, (struct sockaddr *)&sa, sizeof sa) == -1) {
    perror("error bind failed");
    close(sock);
    exit(EXIT_FAILURE);
  }

  int iResult;
  u_long iMode = 1;//1: non-blocking, 0: blocking
#ifdef WIN32
  iResult = ioctlsocket(sock, FIONBIO, &iMode);
#else
  int flags = fcntl(sock, F_GETFL, 0);
  fcntl(sock, F_SETFL, flags | O_NONBLOCK);
#endif

  while (!Stopped)
  {
    recsize = recvfrom(sock, buffer, sizeof buffer, 0, (struct sockaddr*)&sa, &fromlen);
    if (recsize < 0)
    {
        int err = lastError();
       if(10035 == err || EWOULDBLOCK == err || EAGAIN == err)
       {
          usleep(1000 * 10);
          continue;
       }
      fprintf(stderr, "[%i] %s\n", err, strerror(err));
      exit(EXIT_FAILURE);
    }
    qDebug() << "datagram: " << recsize;
    emit receivedData(buffer, recsize);

  }
}
