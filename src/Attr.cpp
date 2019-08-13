/********************************************
* @Author:  flk
* @Version: 1.0
* @Date:    2019/08/12
* @File:    Attr.cpp
* @Desc:    Attrs
**********************************************/

#include "Attr.h"

void Attr::SetAttr(std::string name, std::string value)
{
  //_attrs.insert(std::make_pair(name, value));
  _attrs[name] = value;
}

void Attr::SetAttr(char *name, char *value)
{
  //_attrs.insert(std::make_pair(name, value));
  _attrs[name] = value;
}
const char *Attr::GetAttr(char *name)
{
  return(_attrs[name].data());
}
const char *Attr::GetAttr(std::string name)
{
  return(_attrs[name].data());
}

void Attr::Dup(Attr &src)
{
  std::map<std::string, std::string>::iterator iter;
  _attrs.clear();
  for (iter = src._attrs.begin(); iter != src._attrs.end(); ++iter)
    _attrs.insert(std::make_pair(iter->first, iter->second));
}

