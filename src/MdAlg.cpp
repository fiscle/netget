#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include "MdAlg.h"


template <typename T>
MdAlg<T>::MdAlg()
{
  _isInit = false;
}

template <typename T>
T *MdAlg<T>::GetCtx()
{
  return &_c;
}

template <typename T>
bool MdAlg<T>::AlgUpdate(const unsigned char *data, unsigned long len)
{
  return (1 == Update_cb(GetCtx(), data, len));
}

template <typename T>
bool MdAlg<T>::AlgFinal(unsigned char *md)
{
  return (1 == Final_cb(md, GetCtx()));
}

template <typename T>
bool MdAlg<T>::Calc(const char *file, unsigned char *md)
{
  const int block_size = 4096;
  unsigned char buf[block_size];
  int fd;
  ssize_t len;

  if(!AlgInit())
    return false;

  if((fd = open(file, O_RDONLY)) < 0)
    return false;

  while(1)
  {
    len = read(fd, buf, block_size);
    if(len <= 0)
      break;
    if(!Update_cb(GetCtx(), buf, len))
      goto ERR;
  }
  if(!Final_cb(md, GetCtx()))
    goto ERR;

  close(fd);
  return true;
ERR:
  close(fd);
  return false;
}

static int ex_str(const unsigned char *a, int len, unsigned char *b)
{
  static const unsigned char *hex =(const unsigned char *)"0123456789abcdef";
  int i = 0;
  if(!a || len <= 0 || !b)
    return 0;

  for (i = 0; i < len; i++)
  {
     b[i + i] = hex[(a[i] & 0xf0) >> 4];
     b[i + i + 1] = hex[a[i] & 0x0f];
  }
  return len * 2;
}

template <typename T>
int MdAlg<T>::CalcBcd(const char *file, char *md)
{
  unsigned char buf[MAX_MD_SIZE];
  if(!Calc(file, buf))
    return -1;
  return ex_str(buf, GetMdLen(), (unsigned char *)md);
}


// MD5 Alg 
int MdAlg_md5::GetMdLen()
{
  return ALG_MD_LEN_MD5;
}

MdAlg_md5::MdAlg_md5()
{
  _isInit = false;
}
bool MdAlg_md5::AlgInit()
{
  if(_isInit)
    return _isInit;

  Init_cb = MD5_Init;
  Update_cb = MD5_Update;
  Final_cb = MD5_Final;

  if(1 == Init_cb(GetCtx()));
    _isInit = true;
  return _isInit;
}

int MdAlg_md5::CalcBcd(const char *file, char *md)
{
  return MdAlg<MD5_CTX>::CalcBcd(file, md);
}


// SHA1 Alg
int MdAlg_sha1::GetMdLen()
{
  return ALG_MD_LEN_SHA1;
}
MdAlg_sha1::MdAlg_sha1()
{
  _isInit = false;
}
bool MdAlg_sha1::AlgInit()
{
  if(_isInit)
    return _isInit;

  Init_cb = SHA1_Init;
  Update_cb = SHA1_Update;
  Final_cb = SHA1_Final;

  if(1 == Init_cb(GetCtx()));
    _isInit = true;
  return _isInit;
}

int MdAlg_sha1::CalcBcd(const char *file, char *md)
{
  return MdAlg<SHA_CTX>::CalcBcd(file, md);
}


// md5调用函数
int md5_sum(const char *file, char *md)
{
  MdAlg_md5 alg;
  return alg.CalcBcd(file, md);
}

// sha1调用函数
int sha1_sum(const char *file, char *md)
{
  MdAlg_sha1 alg;
  return alg.CalcBcd(file, md);
}

