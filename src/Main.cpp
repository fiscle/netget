#include <time.h>
#include "Base.h"
#include "Downloader.h"
#include "HttpProto.h"

#define MAX_ARG_NUM  64

static void help(int argc, char *argv[])
{
  printf("Usage: %s --file-url value  [--md5-url value] [--sha1-url value]\n", argv[0]);
  printf("     --file-url 下载url地址\n");
  printf("     --md5-url 下载文件对应的md5校验文件\n");
  printf("     --sha1-url 下载文件对应的sha1校验文件\n");
}

static bool CheckArgs(int argc, char *argv[])
{
  int i = 1;
  for(; i < argc - 1; ++i)
  {
    if(strcmp("--file-url", argv[i]) == 0 && (i + 1) < argc)
      return true;
  }
  return false;
}

int main(int argc, char *argv[])
{
  int ret, trys = 2;
  int ex_argc = 0;
  char *ex_argv[MAX_ARG_NUM];
  const char *p = NULL;

  if(!CheckArgs(argc, argv))
  {
    fprintf(stderr, "in %s: arg error!\n", argv[0]);
    help(argc, argv);
    return -1;
  }

  while(trys-- > 0)
  {
    Downloader downloader;
    downloader.LoadAttrs(argc, argv);
    downloader.LoadAttrs(ex_argc, ex_argv);
    ret = downloader.Download();
    // HTTP重定向再执行一次下载
    if(ret == ERROR_CODE_OF_RELOCATION)
    {
      p = downloader.GetProtoAttr((char *)HTTP_PROTO_ATTR_LOCATION);
      if(!EMPTY_STR(p))
      {
         ex_argv[ex_argc++] = StrDup("--" DOWNLOAD_ATTR_FILE_URL);
         ex_argv[ex_argc++] = StrDup(p);
         continue;
      }
    }
    break;
  }

  while(ex_argc)
  {
    StrFree(ex_argv[--ex_argc]);
  }
  
  return ret;
}

