/********************************************
* @Author:  flk
* @Version: 1.0
* @Date:    2019/08/02
* @File:    FileConnector.cpp
* @Desc:    file连接通讯类
**********************************************/

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include "Base.h"
#include "FileConnector.h"

FileConnector::FileConnector()
{
}
FileConnector::~FileConnector()
{
}

bool FileConnector::Init(int argc, char *argv[])
{
  const char *path = NULL;

  LoadAttrs(argc, argv);

  path = GetAttr(FILECONNECTOR_ATTR_PATH);

  if(EMPTY_STR(path))
  {
    ELOG("FileConnector::Init path is empty!\n");
    return false;
  }
  return true;
}

void FileConnector::DisConnect()
{
}

int FileConnector::Connect()
{
  int fd = GetFd();
  if(fd > 0)
    return fd;
  fd = open(GetAttr(FILECONNECTOR_ATTR_PATH), O_RDONLY);
  if(fd < 0)
    return ERROR_CODE_OF_FILE_OPEN;

  SetIntAttr(FILECONNECTOR_ATTR_FILE_SIZE, lseek(fd, 0, SEEK_END));

  SetFd(fd);
  return(fd);
}

/* 文件内容请求，使用len调节文件offset值
 */
int FileConnector::Send(char *msg, int len)
{
  int fd = GetFd();
  long file_offset;
  if (!msg || fd < 0 || len < 0)
  {
    ELOG("in FileConnector::Recv arg error!!\n");
    return(ERROR_CODE_OF_ARG);
  }
  file_offset = *((long *) msg);
  if(file_offset + len > GetIntAttr(FILECONNECTOR_ATTR_FILE_SIZE))
  {
    ELOG("FileConnector::Send set offset[%ld] + len(%d) too big! when filesize = [%d]\n", file_offset, len, GetIntAttr(FILECONNECTOR_ATTR_FILE_SIZE));
    return ERROR_CODE_OF_ARG;
  }
  SetIntAttr(FILECONNECTOR_ATTR_REQ_LEN, len);
  return lseek(fd, file_offset, SEEK_SET);
}

int FileConnector::Recv(char *buf, int size)
{
  int fd = GetFd();
  
  if (!buf || fd < 0 || size < 0)
  {
    ELOG("in FileConnector::Recv arg error!!\n");
    return(ERROR_CODE_OF_ARG);
  }

  return read(fd, buf, size);
}

