/********************************************
* @Author:  flk
* @Version: 1.0
* @Date:    2019/08/02
* @File:    Base.h
* @Desc:    通用基础模块
**********************************************/

#ifndef _Base_h
#define _Base_h

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>

#define DEFAULT_BLOCK_SIZE 1024
#define MAX_FILE_PATH 1024                     // 文件路径的最大长度
#define MAX_URL_PATH 2048                      // url的最大长度
#define MAX_FILE_KEY_SIZE MAX_URL_PATH         // 文件关键字的最大长度，用以区分下载内容唯一性 
#define MAX_THREADS 10                         // 最大支持多少个进程同时下载 
#define MAX_TRY_TIMES 10                       // 下载失败后尝试次数

#define ERROR_CODE_OFFSET                       -33000
#define ERROR_CODE_OF_DEFAULT                   (ERROR_CODE_OFFSET - 1)
#define ERROR_CODE_OF_ARG                       (ERROR_CODE_OFFSET - 2)
#define ERROR_CODE_OF_SOCKET                    (ERROR_CODE_OFFSET - 3)
#define ERROR_CODE_OF_SEND                      (ERROR_CODE_OFFSET - 4)
#define ERROR_CODE_OF_RECV                      (ERROR_CODE_OFFSET - 5)
#define ERROR_CODE_OF_ACCESS_FILE               (ERROR_CODE_OFFSET - 6)
#define ERROR_CODE_OF_MMAP                      (ERROR_CODE_OFFSET - 7)
#define ERROR_CODE_OF_DATA                      (ERROR_CODE_OFFSET - 8)
#define ERROR_CODE_OF_CONNECT                   (ERROR_CODE_OFFSET - 9)
#define ERROR_CODE_OF_PART_DONE                 (ERROR_CODE_OFFSET - 10)
#define ERROR_CODE_OF_RELOCATION                (ERROR_CODE_OFFSET - 11)
#define ERROR_CODE_OF_PROTO_NOT_SUPPORTED       (ERROR_CODE_OFFSET - 12)
#define ERROR_CODE_OF_PROTO_INIT                (ERROR_CODE_OFFSET - 13)
#define ERROR_CODE_OF_STORE_INIT                (ERROR_CODE_OFFSET - 14)
#define ERROR_CODE_OF_ADJUST_STATE              (ERROR_CODE_OFFSET - 14)
#define ERROR_CODE_OF_UPDATE_STATE              (ERROR_CODE_OFFSET - 14)


#define APPLICATION_NAME "download-file"
#define APPLICATION_VERSION "1.0"


// 日志级别 0-4 分别为: 不写,写错误,写警告,写INFO,写DEBUG
#define DEBUG_LEVEL 4 // [ 0 - 4 ]              


#if DEBUG_LEVEL > 0
#define ELOG(...) fprintf(stderr, __VA_ARGS__)
#else
#define ELOG(...)
#endif

#if DEBUG_LEVEL > 1
#define WLOG(...) printf(__VA_ARGS__)
#else
#define WLOG(...)
#endif

#if DEBUG_LEVEL > 2
#define ILOG(...) printf(__VA_ARGS__)
#else
#define ILOG(...)
#endif

#if DEBUG_LEVEL > 3
#define DLOG(...) printf(__VA_ARGS__)
#else
#define DLOG(...)
#endif

#define SELOG(...) do { \
         fprintf(stderr, "ERROR: [%s]!\n", strerror(errno)) ; \
         fprintf(stderr, __VA_ARGS__); } while(0)

#define INIT_FUNC() DLOG(":: [%d] in %s.\n", __LINE__, __FUNCTION__)
#define MIN_INT(a, b) ((a) > (b) ? (b): (a))
#define STRING_BLANK() " "
#define STRING_OF(s) #s
#define EMPTY_STR(s) ((s) == NULL || strlen(s) == 0)

/* 执行shell命令行 */
void shell_cmd(const char *fmt, ...);

/* 返回指定目录的文件名，指针指向path内地址.*/
const char *file_basename(const char *path);

char *StrDup(const char *s);
void StrFree(char *s);

#endif

