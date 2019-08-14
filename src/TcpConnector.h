/********************************************
* @Author:  flk
* @Version: 1.0
* @Date:    2019/08/01
* @File:    TcpConnector.h
* @Desc:    tcp-socket连接通讯类
**********************************************/

#ifndef _TcpConnector_h
#define _TcpConnector_h

#include "Connector.h"

#define TCPCONNECTOR_ATTR_PORT       (char*)"port"
#define TCPCONNECTOR_ATTR_IPADDR     (char*)"ipaddr"
#define TCPCONNECTOR_ATTR_TIMEOUT    (char*)"timeout"
#define TCPCONNECTOR_ATTR_HOST       (char*)"host"

class TcpConnector : public Connector
{
public:
  TcpConnector();
  ~TcpConnector();
  int Connect();
  void DisConnect();
  int Send(char *msg, int len);
  int Recv(char *buf, int size);
  bool Init(int argc, char *argv[]);
};

#endif


