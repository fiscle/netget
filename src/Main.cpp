#include <time.h>
#include "Base.h"
#include "Downloader.h"
#include "HttpProto.h"
#include "Storer.h"

static char g_file_url[MAX_URL_PATH] = "";
static char g_relocation_url[MAX_URL_PATH] = "";
static char g_md5_url[MAX_URL_PATH] = "";
static char g_sha1_url[MAX_URL_PATH] = "";

void relocation_url_and_do_again(char *p, int len)
{
  if(len >= MAX_URL_PATH)
    return;
  strncpy(g_relocation_url, p, len);
}


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
      snprintf(g_file_url, sizeof(g_file_url), "%s", argv[++i]);
    else if(strcmp("--md5-url", argv[i]) == 0 && (i + 1) < argc)
      snprintf(g_md5_url, sizeof(g_md5_url), "%s", argv[++i]);
    else if(strcmp("--sha1-url", argv[i]) == 0 && (i + 1) < argc)
      snprintf(g_sha1_url, sizeof(g_sha1_url), "%s", argv[++i]);
    else
      return false;
  }
  if(strlen(g_file_url) > 0)
    return true;
  return false;
}


static int wrapper(int argc, char *argv[])
{
  AppProto *appProto = NULL;
  Storer storer;
  Downloader downloader;
  time_t start;
  long status;
  char *app_name = NULL;

  app_name = argv[0];

  if(PROTO_IS_HTTP(g_file_url))
  {
    appProto = new HttpProto;
  }
  else
  {
    fprintf(stderr, "in %s: parse url proto not supported!\n", app_name);
    return -1;
  }

  if(!appProto->Init(g_file_url))
  {
    fprintf(stderr, "in %s: %s init error!\n", app_name, appProto->GetProto());
    delete appProto;
    return -1;
  }

  if(!storer.Init(g_file_url))
  {
    fprintf(stderr, "in %s: store init error!\n", app_name);
    delete appProto;
    return -1;
  }

  // 开始下载文件 
  start = (int)time(NULL);
  status = downloader.Start(*appProto, storer);

  if(!DOWNLOAD_STATUE_ALL_COMPLETED(status))
  {
    // 下载失败
    printf("Download %s error status(%ld)!\n", g_file_url, status);
    delete appProto;
    return (int)status;
  }

  // 下载成功
  printf("---------------------------------------------\n");
  printf("Download %s ok, file size %ld bytes used %d seconds.\n", g_file_url, status ,int(time(NULL) - start));
  printf("Store file [%s] ok.\n", storer.GetFileName());

  // 获取下载文件对应的md5/sha1等验证吗(如果有指定).
  check_md(*appProto, storer.GetFileName(), g_md5_url, g_md5_url);

  delete appProto;
   
  return 0;
}


int main(int argc, char *argv[])
{
  int ret, trys = 2;
  if(!CheckArgs(argc, argv))
  {
    fprintf(stderr, "in %s: arg error!\n", argv[0]);
    help(argc, argv);
    return -1;
  }

  while(trys-- > 0)
  {
    ret = wrapper(argc, argv);
    // HTTP重定向再执行一次下载
    if(ret < 0 && strlen(g_relocation_url) > 0)
    {
      printf("URL relocation to %s!\n", g_relocation_url);
      strncpy(g_file_url, g_relocation_url, sizeof(g_file_url));
      g_relocation_url[0] = 0;
      continue;
    }
    break;
  }
  return ret;
}

