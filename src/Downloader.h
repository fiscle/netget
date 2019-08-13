/********************************************
* @Author:  flk
* @Version: 1.0
* @Date:    2019/08/02
* @File:    Downloader.h
* @Desc:    文件下载类
**********************************************/

#ifndef _Downloader_h
#define _Downloader_h

#include "Storer.h"
#include "AppProto.h"
#include "Attr.h"

#define DOWNLOAD_ATTR_FILE_URL       "file-url"
#define DOWNLOAD_ATTR_MD5_URL        "md5-url"
#define DOWNLOAD_ATTR_SHA1_URL       "sha1-url"

class Downloader : Attr
{
public:
  Downloader();
  ~Downloader();
  /* 开始下载文件，返回:
   *   全部完成: >0 返回文件大小
   *   部分成功: = ERROR_CODE_OF_PART_DONE 
   *   完全失败: <0 其它
   */
#define DOWNLOAD_STATUE_ALL_COMPLETED(ret)  ((ret) > 0)
#define DOWNLOAD_STATUE_PART_COMPLETED(ret)  ((ret) == ERROR_CODE_OF_PART_DONE)
#define DOWNLOAD_STATUE_ERROR(ret)  ((ret) < 0)
  int Download();
  long Start();
  void LoadAttrs(int argc, char **argv);
  const char *GetProtoAttr(char *name);
private:
  void Done(const char *file_url, long file_size, int use_sec);
  bool CheckMd();
  AppProto *_proto;
  Storer _storer;
};

#endif

