/********************************************
* @Author:  flk
* @Version: 1.0
* @Date:    2019/08/01
* @File:    Connector.h
* @Desc:    tcp-socket连接通讯类
**********************************************/

#ifndef _Connector_h
#define _Connector_h

#define DEFAULT_TIMEOUT 10

class Connector
{
public:
  Connector();
  ~Connector();
  bool Init(const char *host, int port, int timeout = DEFAULT_TIMEOUT);
  int Connect();
  void DisConnect();
  int Send(char *msg, int len);
  int Recv(char *buf, int size);
  int GetSd();

private:
  int _port;
  char _ip[64];
  int _sd;
  int _timeout;
};

#endif


