/********************************************
* @Author:  flk
* @Version: 1.0
* @Date:    2019/08/02
* @File:    FileProto.cpp
* @Desc:    FILE协议类
**********************************************/

#include "assert.h"
#include "FileProto.h"
#include "FileConnector.h"

#define FILE_DEFAULT_PORT 0                          // 默认端口
#define FILE_PROTO_NAME "FILE"                       // 协议名称
#define FILE_MAX_PAYLOAD_SIZE (4096 * 16)            // 默认一次拷贝块的大小

#define GET_FILE_CONNECTOR(conns) (conns[0])

FileProto::FileProto() {}
FileProto::~FileProto() {}

int FileProto::Req(Connector *conns, long len, long offset)
{
  int ret = 0;
  Connector &conn = GET_FILE_CONNECTOR(conns);
  long file_offset = offset;

  if((ret = conn.Connect()) < 0)
  {
    ELOG("in FileProto::Req:: conn fail!\n");
    return ret;
  }

  if((ret = conn.Send((char *)&file_offset, len)) < 0)
  {
     ELOG("in FileProto::Req Send!\n");
     return ret;
  }
  DLOG("sd[%d]::::REQ offset = [%ld]!\n", conn.GetFd(), offset);
  return ret;
}

long FileProto::Res(Connector *conns, Storer *storer, int block_index, long *file_size)
{
  int ret;
  long offset = 0, req_len = 0;
  Connector &conn = GET_FILE_CONNECTOR(conns);
  char *write_ptr = storer->GetWritePtr(block_index);

  if((ret = conn.Connect()) < 0)
  {
    ELOG("in FileProto::Res:: conn fail!\n");
    return ret;
  }
  
  *file_size = conn.GetIntAttr(FILECONNECTOR_ATTR_FILE_SIZE);
  if(!storer->IsAdjusted())
     return 0;
  
  req_len = conn.GetIntAttr(FILECONNECTOR_ATTR_REQ_LEN);
  offset = 0;
  while(1)
  {
    if((ret = conn.Recv(write_ptr + offset, req_len - offset)) < 0)
    {
       ELOG("in FileProto::Res Recv!\n");
       return ret;
    }
    else if(ret == 0)
    {
      ELOG("in FileProto::Res Recv 0.\n");
      return 0;
    }
    offset += ret;
    storer->UpdateState(block_index, ret);
    if(offset == req_len)
    {
      DLOG("sd[%d]::::RES[...]recv-len[%ld]!\n",conn.GetFd(), offset);
      return req_len;
    }
    if(offset > req_len)
    {
      ELOG("in FileProto::Res exception.\n");
      assert(true);
    }
    else
      continue;
  }
}

long FileProto::Res(Connector *conns, char *write_ptr, long size, long *file_size)
{
  int ret;
  long offset = 0, req_len = 0;
  Connector &conn = GET_FILE_CONNECTOR(conns);

  if((ret = conn.Connect()) < 0)
  {
    ELOG("in FileProto::Res:: conn fail!\n");
    return ret;
  }

  *file_size = conn.GetIntAttr(FILECONNECTOR_ATTR_FILE_SIZE);

  req_len = conn.GetIntAttr(FILECONNECTOR_ATTR_REQ_LEN);
  if(req_len > size)
    req_len = size;

  offset = 0;
  while(1)
  {
    if((ret = conn.Recv(write_ptr + offset, req_len - offset)) < 0)
    {
       ELOG("in FileProto::Res Recv!\n");
       return ret;
    }
    else if(ret == 0)
    {
      ELOG("in FileProto::Res Recv 0.\n");
      return 0;
    }
    offset += ret;
    if(offset == req_len)
    {
      DLOG("sd[%d]::::RES[...]recv-len[%ld]!\n",conn.GetFd(), offset);
      return req_len;
    }
    if(offset > req_len)
    {
      ELOG("in FileProto::Res exception.\n");
      assert(true);
    }
    else
      continue;
  }
}

int FileProto::SingleExch(Connector *conns, char *write_ptr, long size, long *file_size)
{
  int ret;
  long status;
  ret = Req(conns, size, 0);
  if(ret < 0)
  {
    ELOG("in FileProto::SingleExch req fail ret = %d.\n", ret);
    return ret;
  }
  status = Res(conns, write_ptr, size, file_size);
  if(status < 0)
  {
    ELOG("in FileProto::SingleExch res fail ret = %ld.\n", status);
    return (int)status;
  }
  return (int)status;
}

const char *FileProto::GetProto()
{
  return (const char *)FILE_PROTO_NAME;
}

int FileProto::GetDefaultPort()
{
  return FILE_DEFAULT_PORT;
}

bool FileProto::InitConnector(Connector **conns, int timeout)
{
  const int max_buf = MAX_FILE_PATH, max_arg = 4;
  int argc = 0, i = 0;
  char *argv[max_arg];
  char buf[max_arg][max_buf];

  *conns = new FileConnector[1];
  snprintf(buf[argc++], max_buf, "--%s", FILECONNECTOR_ATTR_PATH);
  snprintf(buf[argc++], max_buf, "%s", GetAttr(APP_PROTO_ATTR_PATH));

  for(i = 0; i < argc; ++i)
    argv[i] = buf[i];
  
  return (*conns)[0].Init(argc, argv);
}

