/********************************************
* @Author:  flk
* @Version: 1.0
* @Date:    2019/08/02
* @File:    AppProtoConnector.cpp
* @Desc:    应用层协议抽象类
**********************************************/
#ifndef _AppProtoConnector_h
#define _AppProtoConnector_h
#include "AppProto.h"
#include "Connector.h"

class AppProtoConnector
{
public:
  AppProtoConnector();
  ~AppProtoConnector();
  bool Init(AppProto *proto, int timeout = DEFAULT_TIMEOUT);
  int Req(long len, long offset);
  long Res(char *write_ptr, long size, long *file_size);
  long Res(Storer *storer, int block_index, long *file_size);
  long SingleExch(char *write_ptr, long size, long *file_size);
private:
  AppProto *_proto;
  Connector *_conns;
};

#endif

