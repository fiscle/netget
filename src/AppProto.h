/********************************************
* @Author:  flk
* @Version: 1.0
* @Date:    2019/08/02
* @File:    AppProto.h
* @Desc:    应用层协议抽象类
**********************************************/
#ifndef _AppProto_h
#define _AppProto_h

#include <map>
#include <string>
#include "Base.h"
#include "Storer.h"
#include "Connector.h"

#define PROTO_IS_HTTP(url) (strncmp((url), "http://", strlen("http://")) == 0)
#define PROTO_IS_HTTPS(url) (strncmp((url), "https://", strlen("https://")) == 0)
#define PROTO_IS_FTP(url) (strncmp((url), "ftp://", strlen("ftp://")) == 0)

class AppProto
{
public:
  AppProto();
  virtual ~AppProto();
  // 初始化，把url解释为, http,host,path,port几部分内容
  bool Init(const char *ufl);
  bool ReInit(const char *ufl);
  void SetAttr(std::string name, std::string value);
  void SetAttr(char *name, char *value);
  const char *GetAttr(std::string name);
  const char *GetAttr(char *name);
  const char *GetHost();
  const char *GetPath();
  int GetPort();
  void Dup(AppProto &src);
  Connector &GetConnector(int i);
  // 请求下载文件
  virtual int Req(Connector conns[], long len, long offset) = 0;
  // 报协议解释后的文件内容写到write_ptr位置
  virtual long Res(Connector conns[], char *write_ptr, long size, long *file_size) = 0;
  virtual long Res(Connector conns[], Storer *storer, int block_index, long *file_size) = 0;
  // 执行一个交互
  virtual int SingleExch(Connector conn[], char *write_ptr, long size, long *file_size) = 0;
  // 返回协议名称
  virtual const char *GetProto() = 0;
  // 返回应用层默认端口
  virtual int GetDefaultPort() = 0;
  // 初始化连接
  virtual bool InitConnector(Connector conns[], int timeout) = 0;

private:
  // 保存协议相关的属性
  std::map<std::string, std::string> _attrs;
};

#endif

