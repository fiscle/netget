/********************************************
* @Author:  flk
* @Version: 1.0
* @Date:    2019/08/02
* @File:    AppProtoConnector.cpp
* @Desc:    应用层协议抽象类
**********************************************/
#include "AppProtoConnector.h"

bool AppProtoConnector::Init(AppProto *proto, int timeout)
{
  _proto = proto;
  return _proto->InitConnector(_conns, timeout);
}

int AppProtoConnector::Req(long len, long offset)
{
  return _proto->Req(_conns, len, offset);
}

long AppProtoConnector::Res(char *write_ptr, long size, long *file_size)
{
  return _proto->Res(_conns, write_ptr, size, file_size);
}

long AppProtoConnector::SingleExch(char *write_ptr, long size, long *file_size)
{
  return _proto->SingleExch(_conns, write_ptr, size, file_size);
}

long AppProtoConnector::Res(Storer *storer, int block_index, long *file_size)
{
  return _proto->Res(_conns, storer, block_index, file_size);
}

