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

void Attr::SetIntAttr(char *name, int value)
{
  _attri[name] = value;
}
int Attr::GetIntAttr(char *name)
{
  return(_attri[name]);
}

void Attr::Dup(Attr &src)
{
  std::map<std::string, std::string>::iterator iter;
  std::map<std::string, int>::iterator iter_int;
  ClearAttr();
  for (iter = src._attrs.begin(); iter != src._attrs.end(); ++iter)
    _attrs.insert(std::make_pair(iter->first, iter->second));
  for (iter_int = src._attri.begin(); iter_int != src._attri.end(); ++iter_int)
    _attri.insert(std::make_pair(iter_int->first, iter_int->second));
}

void Attr::ClearAttr()
{
  _attrs.clear();
  _attri.clear();
}

void Attr::LoadAttrs(int argc, char **argv)
{
  int i = 0;
  for(; i < argc; ++i)
  {
    // prefix --
    if(argv[i][0] == '-' && argv[i][1] == '-' && argv[i][2] != '\0')
    {
      // value
      if((i + 1) < argc && argv[i + 1][0] != '-' && argv[i + 1][1] != '-')
      {
        SetAttr(argv[i] + 2, argv[i+1]);
        ++i;
      }
      else
      {
        SetAttr(argv[i] + 2, (char *)"true");
      }
      
    }
    // prefix ++
    else if(argv[i][0] == '+' && argv[i][1] == '+' && argv[i][2] != '\0')
    {
      // value
      if((i + 1) < argc && argv[i + 1][0] != '-' && argv[i + 1][1] != '-')
      {
        SetIntAttr(argv[i] + 2, atoi(argv[i+1]));
        ++i;
      }
      else
      {
        SetIntAttr(argv[i] + 2, 1);
      }
      
    }
  }
}

void Attr::DumpAttrs(char *buf, int size)
{
}
