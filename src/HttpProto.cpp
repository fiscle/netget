/********************************************
* @Author:  flk
* @Version: 1.0
* @Date:    2019/08/02
* @File:    HttpProto.cpp
* @Desc:    HTTP协议类
**********************************************/

#include "assert.h"
#include "HttpProto.h"

#define HTTP_DEFAULT_PORT 80                         // 默认端口
#define HTTP_PROTO_NAME "HTTP"                       // 协议名称
#define HTTP_DEFAULT_VERSION "1.1"                   // 协议版本号
#define HTTP_DEFAULT_AGENT APPLICATION_NAME    \
                           STRING_BLANK()      \
                           APPLICATION_VERSION \
                           "(linux-gnu)"             // http-agent
#define HTTP_MAX_BUF 1024                            // 请求报文缓冲大小
#define HTTP_MAX_PAYLOAD_SIZE (1024 * 1024 * 10)     // 默认一次最大小载10M
#define HTTP_MAX_RES_HEADER_LEN DEFAULT_BLOCK_SIZE   // 响应报文最大长度，该值保证http header能接收完整.

#define HTTP_PROTO_ATTR_AGENT         "agent"
#define HTTP_PROTO_ATTR_VERSION       "version"

#define HTTP_STATUS_STR(n) HTTP_PROTO_NAME STRING_OF(/)  \
              HTTP_DEFAULT_VERSION STRING_BLANK() STRING_OF(n)
#define GET_HTTP_CONNECTOR(conns) (conns[0])   

#define IS_EMPTY(s) ((s) == NULL || strlen(s) == 0)



HttpProto::HttpProto() {}
HttpProto::~HttpProto() {}

int HttpProto::FormReq(long len, long offset, char *buf, int size)
{
  /* 拼装下载文件的HTTP请求头 */
  return snprintf(buf, size, "GET /%s HTTP/%s\r\n"
          "User-Agent: %s\r\n"
          "Accept: */*\r\n"
          "Range: bytes=%ld-%ld\r\n"
          "Host: %s\r\n" 
          "Connection: Keep-Alive\r\n\r\n",
        GetPath(), 
        IS_EMPTY(GetVersion())? HTTP_DEFAULT_VERSION: GetVersion(),
        IS_EMPTY(GetAgent())? HTTP_DEFAULT_AGENT: GetAgent(),
        offset, offset + len - 1, 
        GetHost());
}

int HttpProto::Req(Connector conns[], long len, long offset)
{
  int ret = 0;
  char buf[HTTP_MAX_BUF];
  Connector &conn = GET_HTTP_CONNECTOR(conns);

  /* HTTP请求头，请申内容限制在HTTP_MAX_PAYLOAD_SIZE大小以内
   *  offset 为文件偏移
   */
  if((ret = conn.Connect()) < 0)
  {
    ELOG("in HttpProto::Req:: conn fail!\n");
    return ret;
  }

  ret = FormReq(MIN_INT(len, HTTP_MAX_PAYLOAD_SIZE), offset, buf, sizeof(buf));
  if((ret = conn.Send(buf, ret)) < 0)
  {
     ELOG("in HttpProto::Req Send!\n");
     return ret;
  }
  DLOG("sd[%d]::::REQ[%.*s]!\n", conn.GetSd(), ret, buf);
  return ret;
}

static char *get_res_data_ptr(char *header)
{
  char *endp = NULL;
  if((endp = strstr(header, "\r\n\r\n")) == NULL)
  {
    return NULL;
  }
  return endp + 4;
}

static int get_file_sizes_from_header(char *header, char *endp, long *file_size, long *off1, long *off2)
{
  char *status_200 = (char *)HTTP_STATUS_STR(200); 
  char *status_206 = (char *)HTTP_STATUS_STR(206);
  char *p;

  if(strncmp(header, status_206, strlen(status_206)) == 0)
  {
    if((p = strstr(header, "Content-Range: bytes")) != NULL && p < endp)
    {
      p += strlen("Content-Range: bytes");
    }
    else if((p = strstr(header, "Content-Range:bytes")) != NULL && p < endp)
    {
      p += strlen("Content-Range:bytes");
    }
    else
      p = NULL;

    if(p)
    {
      /* 去掉可能存在的空格 */
      while(*p == ' ') ++p;
      if(sscanf(p, "%ld-%ld/%ld", off1, off2, file_size) == 3)
      {
        return 0;
      }
    }
  }
  else if(strncmp(header, status_200, strlen(status_200)) == 0)
  {
    if((p = strstr(header, "Content-Length:")) != NULL && p < endp)
    {
      p += strlen("Content-Length:");
      /* 去掉可能存在的空格 */
      while(*p == ' ') ++p;
      *file_size = atol(p);
      *off1 = 0;
      *off2 = *file_size - 1;
      return 0;
    }
  }
  else
  {
    p = header + (strlen(status_200) - 3); // point to status
    if(strncmp(p, "302", 3) == 0)
    {
      if((p = strstr(header, "Location:")) != NULL && p < endp)
      {
        p += strlen("Location:");
        /* 去掉可能存在的空格 */
        while(*p == ' ') ++p;
        endp = strstr(p, "\r\n");
        void relocation_url_and_do_again(char *p, int len);
        relocation_url_and_do_again(p, endp - p);
        return ERROR_CODE_OF_RELOCATION;
      }
    }
    ELOG("in get_file_sizes_from_header: state[%.3s] error!\n", p);
  }

  ELOG("in get_file_sizes_from_header:: get file len error! header=[%.*s]\n", (int)(endp - header), header);
  return ERROR_CODE_OF_DATA;

}


long HttpProto::Res(Connector conns[], char *write_ptr, long size, long *file_size)
{
  int ret;
  long off1, off2, res_file_len, recv_file_len = 0;
  char buf[HTTP_MAX_RES_HEADER_LEN+1], *pfile = NULL;
  Connector &conn = GET_HTTP_CONNECTOR(conns);

  if((ret = conn.Connect()) < 0)
  {
    ELOG("in HttpProto::Res:: conn fail!\n");
    return ret;
  }
  
  /* 读一个http的响应报文，首先默认是取固定的HTTP_MAX_RES_HEADER_LEN字节(保证能读完报文头)
   *   分析读回来的数据，把文件内容区分开来，并从报文头获得本报中的文件内容长度 res_file_len
   *   和文件的总长度。把文件内容写到 write_ptr 
   */
  long offset = 0;
  while(1)
  {
    if((ret = conn.Recv(buf + offset, HTTP_MAX_RES_HEADER_LEN - offset)) < 0)
    {
       ELOG("in HttpProto::Res Recv!\n");
       return ret;
    }
    else if(ret == 0)
    {
      ELOG("in HttpProto::Res Recv 0!\n");
      return 0;
    }
    offset += ret;
  
    buf[offset] = 0;
    if((pfile = get_res_data_ptr(buf)) == NULL)
    {
      if(offset >= HTTP_MAX_RES_HEADER_LEN)
      {
         ELOG("in HttpProto::Res get header!\n");
         return ERROR_CODE_OF_DATA;
      }
      continue;
    }
    break;
  }

  if((ret = get_file_sizes_from_header(buf, pfile, file_size, &off1, &off2)) < 0)
  {
    ELOG("in ::Res Recv get_file_sizes_from_header ret = %d!\n", ret);
    return ret;
  }
  
  // 已经收的文件内容长度
  recv_file_len = offset - (pfile - buf); 
  // 会话报文中应该返回的文件长度
  res_file_len = off2 - off1 + 1;

  memcpy(write_ptr, pfile, recv_file_len);

  DLOG("sd[%d]::::RES1[%.*s]recv-len[%ld]!\n", conn.GetSd(), (int)(pfile - buf), buf, recv_file_len);

  /* 第一次收报文本报中的文件内容就完成 */
  if(recv_file_len == res_file_len)
  {
    DLOG("in ::Res Recv %ld ALL.\n", recv_file_len);
    return recv_file_len;
  }

  /* 继续收HTTP请求中剩余的文件内容 */
  offset = 0;
  while(1)
  {
    if((ret = conn.Recv(write_ptr + recv_file_len + offset, res_file_len - recv_file_len - offset)) < 0)
    {
       ELOG("in HttpProto::Res Recv!\n");
       return ret;
    }
    else if(ret == 0)
    {
      ELOG("in HttpProto::Res Recv 0.\n");
      return 0;
    }
    offset += ret;
    if(offset == res_file_len - recv_file_len)
    {
      DLOG("sd[%d]::::RES2[...]recv-len[%ld]!\n",conn.GetSd(), offset);
      return res_file_len;
    }
    if(offset > res_file_len - recv_file_len)
    {
      ELOG("in HttpProto::Res http exception.\n");
      assert(true);
    }
    else
      continue;
  }
}

int HttpProto::SingleExch(Connector conns[], char *write_ptr, long size, long *file_size)
{
  int ret;
  long status;
  ret = Req(conns, size, 0);
  if(ret < 0)
  {
    ELOG("in HttpProto::SingleExch req fail ret = %d.\n", ret);
    return ret;
  }
  status = Res(conns, write_ptr, size, file_size);
  if(status < 0)
  {
    ELOG("in HttpProto::SingleExch res fail ret = %ld.\n", status);
    return (int)status;
  }
  return (int)status;
}

const char *HttpProto::GetProto()
{
  return (const char *)HTTP_PROTO_NAME;
}

int HttpProto::GetDefaultPort()
{
  return HTTP_DEFAULT_PORT;
}

const char *HttpProto::GetVersion()
{
  return GetAttr((char *)HTTP_PROTO_ATTR_VERSION);
}

const char *HttpProto::GetAgent()
{
  return GetAttr((char *)HTTP_PROTO_ATTR_AGENT);
}

void HttpProto::SetVersion(char *value)
{
  SetAttr((char *)HTTP_PROTO_ATTR_VERSION, value);
}
void HttpProto::SetAgent(char *value)
{
  SetAttr((char *)HTTP_PROTO_ATTR_AGENT, value);
}

bool HttpProto::InitConnector(Connector conns[], int timeout)
{
  Connector &conn = GET_HTTP_CONNECTOR(conns);
  return conn.Init(GetHost(), GetPort(), timeout);
}

