/********************************************
* @Author:  flk
* @Version: 1.0
* @Date:    2019/08/02
* @File:    TcpConnector.cpp
* @Desc:    tcp-socket连接通讯类
**********************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <errno.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "Base.h"
#include "TcpConnector.h"

TcpConnector::TcpConnector()
{
}
TcpConnector::~TcpConnector()
{
}

 /* 判断合法的ipv4地址，并返回整型值
  *   失败时返回0，不支持0.0.0.0的判断
  */
static uint32_t get_and_check_ipv4(const char *ip)
{
  int i = 0, j = 0, k = 0;
  uint32_t v = 0;
  char sv[4];

  for(i = 0; i < 4; ++i)
  {
    k = 0;
    for(j = 0; j < 4; ++j)
    {
      if(j == k && *ip > '0' && *ip < '9')
      {
        sv[k++] = *ip++;
        if(*ip == '\0' && i == 3 && k < 4) // end
        {
          sv[k] = 0;
          v <<= 8;
          v += atoi(sv);
          return v;
        }
      }
      else if(k != 0 && *ip == '.')
      {
        sv[k] = 0;
        v <<= 8;
        v += atoi(sv);
        ++ip;
        k = 0;
        break;
      }
      else
        return 0;
    }
  }
  return 0;
}

/* 域名转换为ipv4地址 */
static uint32_t get_ip_by_host(const char *host, char *ip, int size)
{
  //struct hostent *pHostent = gethostbyname(host);
  struct hostent *pHostent = NULL;
  struct hostent hostent;
  char buf[2048];
  int ret;

  if(gethostbyname_r(host, &hostent, buf, sizeof(buf), &pHostent, &ret) != 0)
    return 0;

  if(pHostent == NULL)
    return 0;

  switch(pHostent->h_addrtype)
  {
    case AF_INET:
      inet_ntop(pHostent->h_addrtype, pHostent->h_addr, ip, size);
      return ntohl((uint32_t)((struct in_addr *)pHostent->h_addr)->s_addr);
      break;
    default:
      return 0;
      break;
  }
  return(0);
}

 /* 获取IP地址 */
static uint32_t get_ip(const char *inAddr, char *buf, int size)
{
  uint32_t ip;
  if((ip = get_and_check_ipv4(inAddr)) > 0)
  {
    if(inAddr != buf)
      strncpy(buf, inAddr, size);
    return(ip);
  }
  else
  {
    return(get_ip_by_host(inAddr, buf, size));
  }
}

bool TcpConnector::Init(int argc, char *argv[])
{
  char ip[64];
  const char *host = NULL;

  LoadAttrs(argc, argv);

  host = GetAttr(TCPCONNECTOR_ATTR_HOST);

  if(EMPTY_STR((char *)host))
  {
    ELOG("TcpConnector Init HOST not spec!\n");
    return false;
  }

  if(GetIntAttr(TCPCONNECTOR_ATTR_PORT) <= 0)
  {
    ELOG("TcpConnector Init PORT = %d fail!\n", GetIntAttr(TCPCONNECTOR_ATTR_PORT));
    return false;
  } 
  
  if(get_ip(host, ip, sizeof(ip)) == 0)
  {
    ELOG("TcpConnector Init get [%s] ip fail!\n", host);
    return false;
  }

  SetAttr(TCPCONNECTOR_ATTR_IPADDR, ip);
  return true;
}

void TcpConnector::DisConnect()
{
}

int TcpConnector::Connect()
{
  struct sockaddr_in psckadd;
  struct linger Linger;
  int on = 1;
  struct timeval  tv;
  int fd = -1;

  tv.tv_sec = GetIntAttr(TCPCONNECTOR_ATTR_TIMEOUT);
  tv.tv_usec = 0;
  const char *ip = GetAttr(TCPCONNECTOR_ATTR_IPADDR);
  int port = GetIntAttr(TCPCONNECTOR_ATTR_PORT);

  if (EMPTY_STR(ip) || port <= 0)
  {
    ELOG("in TcpConnector::Connect arg error!\n");
    return(ERROR_CODE_OF_ARG);
  }

  if(GetFd() > 0) return GetFd(); // connented.

  if ((fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
  {
    ELOG("in TcpConnector::Connect socket error!\n");
    return(ERROR_CODE_OF_SOCKET);
  }

  memset((char *)(&psckadd),'0',sizeof(struct sockaddr_in));
  psckadd.sin_family = AF_INET;
  psckadd.sin_addr.s_addr = inet_addr(ip);
  psckadd.sin_port=htons((uint16_t)port);

  if (connect(fd,(struct sockaddr *)(&psckadd), sizeof(struct sockaddr_in)) < 0)
  {
    ELOG("in TcpConnector::Connect connect IP = [%s] port = [%d] fail!\n", ip, port);
    close(fd);
    return(ERROR_CODE_OF_CONNECT);
  }

  Linger.l_onoff = 1;
  Linger.l_linger = 0;
  if (setsockopt(fd, SOL_SOCKET, SO_LINGER, (char *)&Linger, sizeof(Linger)) != 0)
  {
    WLOG("in TcpConnector::Connect set Linger fail!!\n");
  }

  // 及时发送
  on = 1;
  if (setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, (char *)&on, sizeof(on)))
  {
    WLOG("in TcpConnector::Connect set TCP_NODELAY fail!!\n");
  }

  // 设置发送超时
  if (setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, (char *)&tv, sizeof(struct timeval)))
  {
    WLOG("in TcpConnector::Connect set SO_SNDTIMEO  fail!!\n");
  }

  // 设置超时
  if (setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv, sizeof(struct timeval)))
  {
    WLOG("in TcpConnector::Connect set SO_RCVTIMEO  fail!!\n");
  }
  DLOG("in TcpConnector::Connect %s:%d(%d) Connected.\n", ip, port, fd);

  SetFd(fd);

  return(fd);
}


int TcpConnector::Send(char *msg, int len)
{
  int  rc, fd = GetFd();
  int  send_len = 0;
  
  if (!msg || fd < 0 || len < 0)
  {
    ELOG("in TcpConnector::Send arg error!!\n");
    return(ERROR_CODE_OF_ARG);
  }
  
  for (;;)
  {
    if ((rc = send(fd, &msg[send_len], len - send_len, 0)) != len - send_len)
    {
      if (rc < 0)
      {
        SetFd(-1);
        ELOG("in TcpConnector::Send error!!\n");
        return(ERROR_CODE_OF_SEND);
      }
      else
        send_len += rc;
      if (send_len > len)
      {
        ELOG("in TcpConnector::Send exception!!\n");
        return(ERROR_CODE_OF_SEND);
      }
      usleep(5);
    }
    else
    {
      //DLOG("Send:: [%d][%.*s]\n", len, len, msg);
      return(len);
    }
  }
}

int TcpConnector::Recv(char *buf, int size)
{
  int rc, fd = GetFd();
  
  if (!buf || fd < 0 || size < 0)
  {
    ELOG("in TcpConnector::Recv arg error!!\n");
    return(ERROR_CODE_OF_ARG);
  }

  if ((rc = recv(fd, buf, size, 0)) <= 0)
  {
    SetFd(-1);
    if ((errno != 108) && (errno != 104))
    {
      ELOG("in TcpConnector::Recv rc = %d error!!\n", rc);
      return(ERROR_CODE_OF_RECV);
    }
    else  
    {
      DLOG("in TcpConnector::Recv (%d) Connection is Closed!\n", fd);
      return(0);
    }
  }
  else
  {
    //DLOG("Recv:: [%d][%.*s]\n", rc, rc, buf);
    return(rc);
  }
}

