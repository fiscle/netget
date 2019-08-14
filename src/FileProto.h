/********************************************
* @Author:  flk
* @Version: 1.0
* @Date:    2019/08/02
* @File:    FileProto.h
* @Desc:    FILE协议类
**********************************************/

#ifndef _FileProto_h
#define _FileProto_h

#include <map>
#include <string>
#include "Base.h"
#include "AppProto.h"
#include "Connector.h"

class FileProto : public AppProto
{
public:
  FileProto();
  ~FileProto();
  const char *GetProto();
  int GetDefaultPort();
  int Req(Connector *conns, long len, long offset);
  long Res(Connector *conns, char *write_ptr, long size, long *file_size);
  long Res(Connector *conns, Storer *storer, int block_index, long *file_size);
  bool InitConnector(Connector **conns, int timeout);
  int SingleExch(Connector *conns, char *write_ptr, long size, long *file_size);
  
};

#endif

