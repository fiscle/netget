/********************************************
* @Author:  flk
* @Version: 1.0
* @Date:    2019/08/02
* @File:    HttpProto.h
* @Desc:    HTTP协议类
**********************************************/

#ifndef _HttpProto_h
#define _HttpProto_h

#include <map>
#include <string>
#include "Base.h"
#include "AppProto.h"
#include "Connector.h"

class HttpProto : public AppProto
{
public:
  HttpProto();
  ~HttpProto();
  const char *GetProto();
  int GetDefaultPort();
  int Req(Connector conn[], long len, long offset);
  long Res(Connector conn[], char *write_ptr, long size, long *file_size);
  bool InitConnector(Connector conn[], int timeout);
  void SetAgent(char *value);
  void SetVersion(char *value);
  int SingleExch(Connector conn[], char *write_ptr, long size, long *file_size);
private:
  const char *GetAgent();
  const char *GetVersion();
  int FormReq(long len, long offset, char *buf, int size);
  
};

#endif

