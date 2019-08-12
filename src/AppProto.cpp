/********************************************
* @Author:  flk
* @Version: 1.0
* @Date:    2019/08/02
* @File:    AppProto.cpp
* @Desc:    应用层协议抽象类
**********************************************/

#include "AppProto.h"

#define APP_PROTO_ATTR_HOST "host"
#define APP_PROTO_ATTR_PORT "port"
#define APP_PROTO_ATTR_PATH "path"
#define APP_PROTO_ATTR_PROTO "proto"

AppProto::AppProto() { }
AppProto::~AppProto() { }
const char *AppProto::GetHost()
{
  return GetAttr((char *)APP_PROTO_ATTR_HOST);
}

const char *AppProto::GetPath()
{
  return GetAttr((char *)APP_PROTO_ATTR_PATH);
}

int AppProto::GetPort()
{
  if(strlen(GetAttr((char *)APP_PROTO_ATTR_PORT)) != 0)
    return atoi(GetAttr((char *)APP_PROTO_ATTR_PORT));
  else
    return GetDefaultPort();
    
}

/* 从url中解释出 proto,host,port,path信息
 *   返回的各个指针直接使用输入buf地址，url被截为多段字符串
 */
static bool ParseUrl(char *url, char **proto, char **host, char **port, char **path)
{
  char *p1 = NULL, *p2 = NULL, *p3 = NULL;
  if(!url)
    return false;

  p1 = url;

  // proto
  if((p2 = strstr(p1, "://")) == NULL || p2 == p1)
    return false;
  *proto = p1;
  *p2 = 0;
  p1 = p2 + 3;

  if(strlen(p1) == 0)
    return false;

  // host
  *host = p1;
  if((p2 = strstr(p1, ":")) != NULL)
  {
     if((p3 = strstr(p1, "/")) == NULL || p3 > p2)
     {
       // port
       *p2 = 0;
       p1 = p2 + 1;
       *port = p1;
     }
  }

  // path
  if((p2 = strstr(p1, "/")) != NULL)
  {
    p1 = p2 + 1;
    *p2 = 0;
    *path = p1;
  }

  return true;
}

bool AppProto::Init(char *url)
{
  char *proto = NULL, *host = NULL, *port = NULL, *path = NULL;
  char buf[MAX_URL_PATH];

  if(!url || strlen(url) == 0)
    return false;

  {
    snprintf(buf, sizeof(buf), "%s", url);
    if(!ParseUrl(buf, &proto, &host, &port, &path))
    {
      return false;
    }
    SetAttr((char *)APP_PROTO_ATTR_PROTO, proto);
    SetAttr((char *)APP_PROTO_ATTR_HOST, host);
    if(port)
    {
      SetAttr((char *)APP_PROTO_ATTR_PORT, port);
    }
    if(path)
    {
      SetAttr((char *)APP_PROTO_ATTR_PATH, path);
    }
  }
  return true;
}

bool AppProto::ReInit(char *url)
{
  _attrs.clear();
  return Init(url);
}

void AppProto::SetAttr(std::string name, std::string value)
{
  _attrs.insert(std::make_pair(name, value));
}

void AppProto::SetAttr(char *name, char *value)
{
  _attrs.insert(std::make_pair(name, value));
}
const char *AppProto::GetAttr(char *name)
{
  return(_attrs[name].data());
}
const char *AppProto::GetAttr(std::string name)
{
  return(_attrs[name].data());
}

void AppProto::Dup(AppProto &src)
{
  std::map<std::string, std::string>::iterator iter;
  _attrs.clear();
  for (iter = src._attrs.begin(); iter != src._attrs.end(); ++iter)
    _attrs.insert(std::make_pair(iter->first, iter->second));
}
