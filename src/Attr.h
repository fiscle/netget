/********************************************
* @Author:  flk
* @Version: 1.0
* @Date:    2019/08/12
* @File:    Attr.h
* @Desc:    应用层协议抽象类
**********************************************/
#ifndef _Attr_h
#define _Attr_h

#include <map>
#include <string>
#include "Base.h"

class Attr
{
public:
  void SetAttr(std::string name, std::string value);
  void SetAttr(char *name, char *value);
  const char *GetAttr(std::string name);
  const char *GetAttr(char *name);
  void Dup(Attr &src);
  void ClearAttr();
  void SetIntAttr(char *name, int value);
  int GetIntAttr(char *name);
  void LoadAttrs(int argc, char **argv);
  void DumpAttrs(char *buf, int size);
private:
  std::map<std::string, std::string> _attrs;
  std::map<std::string, int> _attri;
};

#endif

