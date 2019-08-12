/********************************************
* @Author:  flk
* @Version: 1.0
* @Date:    2019/08/02
* @File:    Base.cpp
* @Desc:    通用基础模块
**********************************************/

#include "Base.h"
#include <stdarg.h>

/* 返回指定目录的文件名，指针指向path内地址.
 */
char *file_basename(char *path)
{
  char *p = NULL;
  if(!path)
    return (char *)"unknow";
  if(path[strlen(path)-1] == '/')
    path[strlen(path)-1] = 0;
  if((p = strrchr(path, '/')))
    ++p;
  else
    p = path;
  if(strlen(p) == 0)
    return (char *)"unknow";
  return p;
}

/* 执行shell命令行
 */
void shell_cmd(const char *fmt, ...)
{
  va_list 	args;
  char cmd[2048];

  va_start(args, fmt);
  vsprintf(cmd, fmt, args);
  va_end(args);

  DLOG("cmd:: [%s]\n", cmd);
  system(cmd);
}

