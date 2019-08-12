#ifndef _MdAlg_h
#define _MdAlg_h


#include <openssl/md5.h>
#include <openssl/sha.h>
#include "Base.h"

#define MAX_MD_SIZE 256
#define ALG_MD_LEN_MD5 16
#define ALG_MD_LEN_SHA1 20

template <typename T>
class MdAlg
{
public:
  MdAlg();
  virtual bool AlgInit() = 0;
  virtual int GetMdLen() = 0;
  bool Calc(const char *file, unsigned char *md);
  int CalcBcd(const char *file, char *md);
  bool _isInit;

  int (*Init_cb)(T *c);
  int (*Update_cb)(T *c, const void *data, unsigned long len);
  int (*Final_cb)(unsigned char *md, T *c);
  T *GetCtx();

private:
  bool AlgUpdate(const unsigned char *data, unsigned long len);
  bool AlgFinal(unsigned char *md);

  T _c;
};

class MdAlg_md5 : public MdAlg<MD5_CTX>
{
public:
  MdAlg_md5();
  int GetMdLen();
  bool AlgInit();
  int CalcBcd(const char *file, char *md);
};

class MdAlg_sha1 : public MdAlg<SHA_CTX>
{
public:
  MdAlg_sha1();
  int GetMdLen();
  bool AlgInit();
  int CalcBcd(const char *file, char *md);
};

// md5调用函数
int md5_sum(const char *file, char *md);

// sha1调用函数
int sha1_sum(const char *file, char *md);

#endif

