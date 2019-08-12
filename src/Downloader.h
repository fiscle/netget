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


class Downloader
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
  long Start(AppProto &proto, Storer &storer);
};
bool check_md(AppProto &proto, const char *store_file, char *md5_url, char *sha1_url);

#endif

