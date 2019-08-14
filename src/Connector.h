/********************************************
* @Author:  flk
* @Version: 1.0
* @Date:    2019/08/01
* @File:    Connector.h
* @Desc:    连接通讯类
**********************************************/

#ifndef _Connector_h
#define _Connector_h

#include "Attr.h"
#define DEFAULT_TIMEOUT 10

class Connector : public Attr
{
public:
  Connector();
  virtual ~Connector();
  virtual int Connect() = 0;
  virtual void DisConnect() = 0;
  virtual int Send(char *msg, int len) = 0;
  virtual int Recv(char *buf, int size) = 0;
  virtual bool Init(int argc, char *argv[]) = 0;
  int GetFd();
  void SetFd(int fd);
private:
  int _fd;
};

#endif


