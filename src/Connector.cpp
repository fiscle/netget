/********************************************
* @Author:  flk
* @Version: 1.0
* @Date:    2019/08/02
* @File:    Connector.cpp
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
#include "Connector.h"

Connector::Connector()
{
  _port = 0;
  _ip[0] = 0;
  _sd = -1;
  _timeout = DEFAULT_TIMEOUT;
}
Connector::~Connector()
{
  if(_sd > 0)
  {
    close(_sd);
    _sd = -1;
  }
}

int Connector::GetSd()
{
  return _sd;
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
  struct hostent *pHostent = gethostbyname(host);
  if(pHostent == NULL)
    return(0);

  switch(pHostent->h_addrtype)
  {
    case AF_INET:
      inet_ntop(pHostent->h_addrtype, pHostent->h_addr, ip, size);
      return ntohl((uint32_t)((struct in_addr *)pHostent->h_addr)->s_addr);
      break;
    default:
      return(0);
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

bool Connector::Init(const char *host, int port, int timeout)
{
  if(get_ip(host, _ip, sizeof(_ip)) == 0)
    return false;
  _port = port;
  _timeout = timeout;
  return true;
}

void Connector::DisConnect()
{
  if(_sd > 0)
  {
    close(_sd);
    _sd = -1;
  }
}

int Connector::Connect()
{
  struct sockaddr_in psckadd;
  struct linger Linger;
  int on = 1;
  struct timeval  tv;
  int sd = -1;

  tv.tv_sec = _timeout;
  tv.tv_usec = 0;

  if (strlen(_ip) == 0 || _port <= 0)
  {
    ELOG("in Connector::Connect arg error!\n");
    return(ERROR_CODE_OF_ARG);
  }

  if(_sd > 0) return _sd; // connented.

  if ((sd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
  {
    ELOG("in Connector::Connect socket error!\n");
    return(ERROR_CODE_OF_SOCKET);
  }

  memset((char *)(&psckadd),'0',sizeof(struct sockaddr_in));
  psckadd.sin_family = AF_INET;
  psckadd.sin_addr.s_addr = inet_addr(_ip);
  psckadd.sin_port=htons((uint16_t)_port);

  if (connect(sd,(struct sockaddr *)(&psckadd), sizeof(struct sockaddr_in)) < 0)
  {
    ELOG("in Connector::Connect connect IP = [%s] port = [%d] fail!\n", _ip, _port);
    close(sd);
    return(ERROR_CODE_OF_CONNECT);
  }

  Linger.l_onoff = 1;
  Linger.l_linger = 0;
  if (setsockopt(sd, SOL_SOCKET, SO_LINGER, (char *)&Linger, sizeof(Linger)) != 0)
  {
    WLOG("in Connector::Connect set Linger fail!!\n");
  }

  // 及时发送
  on = 1;
  if (setsockopt(sd, IPPROTO_TCP, TCP_NODELAY, (char *)&on, sizeof(on)))
  {
    WLOG("in Connector::Connect set TCP_NODELAY fail!!\n");
  }

  // 设置发送超时
  if (setsockopt(sd, SOL_SOCKET, SO_SNDTIMEO, (char *)&tv, sizeof(struct timeval)))
  {
    WLOG("in Connector::Connect set SO_SNDTIMEO  fail!!\n");
  }

  // 设置超时
  if (setsockopt(sd, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv, sizeof(struct timeval)))
  {
    WLOG("in Connector::Connect set SO_RCVTIMEO  fail!!\n");
  }
  DLOG("in Connector::Connect %s:%d(%d) Connected.\n", _ip, _port, sd);

  _sd = sd;

  return(sd);
}


int Connector::Send(char *msg, int len)
{
  int  rc;
  int  send_len = 0;
  
  if (!msg || _sd < 0 || len < 0)
  {
    ELOG("in Connector::Send arg error!!\n");
    return(ERROR_CODE_OF_ARG);
  }
  
  for (;;)
  {
    if ((rc = send(_sd, &msg[send_len], len - send_len, 0)) != len - send_len)
    {
      if (rc < 0)
      {
        _sd = -1;
        ELOG("in Connector::Send error!!\n");
        return(ERROR_CODE_OF_SEND);
      }
      else
        send_len += rc;
      if (send_len > len)
      {
        ELOG("in Connector::Send exception!!\n");
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

int Connector::Recv(char *buf, int size)
{
  int rc;
  
  if (!buf || _sd < 0 || size < 0)
  {
    ELOG("in Connector::Recv arg error!!\n");
    return(ERROR_CODE_OF_ARG);
  }

  if ((rc = recv(_sd, buf, size, 0)) <= 0)
  {
     _sd = -1;
    if ((errno != 108) && (errno != 104))
    {
      ELOG("in Connector::Recv rc = %d error!!\n", rc);
      return(ERROR_CODE_OF_RECV);
    }
    else  
    {
      _sd = -1;
      DLOG("in Connector::Recv %s:%d(%d) Connection is Closed!\n", _ip, _port, _sd);
      return(0);
    }
  }
  else
  {
    //DLOG("Recv:: [%d][%.*s]\n", rc, rc, buf);
    return(rc);
  }
}

