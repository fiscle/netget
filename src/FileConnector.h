/********************************************
* @Author:  flk
* @Version: 1.0
* @Date:    2019/08/01
* @File:    FileConnector.h
* @Desc:    file连接通讯类
**********************************************/

#ifndef _FileConnector_h
#define _FileConnector_h

#include "Connector.h"

#define FILECONNECTOR_ATTR_PATH       (char*)"path"
#define FILECONNECTOR_ATTR_FILE_SIZE  (char*)"filesize"
#define FILECONNECTOR_ATTR_REQ_LEN    (char*)"reqlen"

class FileConnector : public Connector
{
public:
  FileConnector();
  ~FileConnector();
  int Connect();
  void DisConnect();
  int Send(char *msg, int len);
  int Recv(char *buf, int size);
  bool Init(int argc, char *argv[]);
  long GetFileSize();
  int GetReqLen();
};

#endif


