/********************************************
* @Author:  flk
* @Version: 1.0
* @Date:    2019/08/02
* @File:    Connector.cpp
* @Desc:    连接通讯类
**********************************************/
#include <unistd.h>
#include "Connector.h"

Connector::Connector()
{
  _fd = -1;
}
Connector::~Connector()
{
  if(_fd > 0)
    close(_fd);
}
int Connector::GetFd()
{
  return _fd;
}
void Connector::SetFd(int fd)
{
  _fd = fd;
}


