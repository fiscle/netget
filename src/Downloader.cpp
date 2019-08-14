/********************************************
* @Author:  flk
* @Version: 1.0
* @Date:    2019/08/02
* @File:    Downloader.cpp
* @Desc:    文件下载类
**********************************************/

#include <assert.h>
#include <unistd.h>
#include "Base.h"
#include "AppProtoConnector.h"
#include "HttpProto.h"
#include "FileProto.h"
#include "MdAlg.h"
#include "Downloader.h"

#define SWAP_BIN(b) ((b) == 0 ? 1 : 0)
#define TICK_TIMES 3000
#define TICK_UNIT 1000
#define TRY_AGAIN() do { sleep(1); --trys; continue; } while(0)

/* 主线程传给工作线程参数 */
struct targs_t
{
  union
  {
    int tid;        // 线程序号
    int run;        // 监控线程状态
  };
  AppProto *proto;  // 下载协议对称
  Storer *storer;   // 存储对象 
  long status;
};

Downloader::Downloader()
{
  _proto = NULL;
}

Downloader::~Downloader()
{
  if(_proto)
    delete _proto;
}


/* 下载任务工作单元 */
static void *loader_work(void *p)
{
  int trys = MAX_TRY_TIMES;
  long file_size;
  struct targs_t *args = (struct targs_t *)p;
  struct store_state_t *state = args->storer->GetStoreState();
  struct block_state_t *b_state = &state->blocks[args->tid];
  AppProtoConnector conn;
  
  args->status = ERROR_CODE_OF_DEFAULT;
  if(!conn.Init(args->proto))
  {
    ELOG("tid[%d] initConn fail!\n", args->tid);
    return NULL;
  }

  /* 当文件大小有变化且任务未完成时继续循环下载。
   *   当文件大小未无生变化时重试MAX_TRY_TIMES
   *   因为第一次请求的时文件大小尚未知晓，所以第一次只有一条线程请求，

   */
  while(trys > 0)
  {
    DLOG("tid[%d]::: Req left:%ld, offset:%ld\n", args->tid, b_state->left, b_state->offset);
    args->status = conn.Req(b_state->left, b_state->offset);
    if(args->status < 0)
    {
      ELOG("tid[%d] proto->Req fail!\n", args->tid);
      TRY_AGAIN();
    }

    //args->status = conn.Res(args->storer->GetWritePtr(args->tid), b_state->left, &file_size);
    args->status = conn.Res(args->storer, args->tid, &file_size);
    if(args->status == ERROR_CODE_OF_RELOCATION)
    {
       return NULL;
    }
    else if(args->status == ERROR_CODE_OF_DATA)
    {
      ELOG("tid[%d] proto->Res data fail!\n", args->tid);
      return NULL;
    }
    else if(args->status < 0)
    {
      ELOG("tid[%d] proto->Res fail ret = %ld!\n", args->tid, args->status);
      TRY_AGAIN();
    }
    
    /* 第一次请求 调整状态信息并返回让上层再次分配任务 */
    if(args->tid == 0 && !args->storer->IsAdjusted())
    {
      args->storer->AdjustState(file_size, args->status);
      {
        args->status = ERROR_CODE_OF_ADJUST_STATE;
        ILOG("::: begin to download file, size is %ld Bytes.\n", file_size);
        return NULL;
      }
    }

    /* 更新下载内容 */
    /*
    if((left = args->storer->UpdateState(args->tid, args->status)) < 0)
    {
      args->status = ERROR_CODE_OF_UPDATE_STATE;
      ELOG("tid[%d] UpdateState. ret = %ld\n", args->tid, left);
      return NULL;
    }
    */

    // 完成
    if(b_state->left == 0)
    {
      args->status = 0;
      return NULL;
    }
  }
  return NULL;
}

static void *mon_work(void *p)
{
  struct targs_t *args = (struct targs_t *)p;
  struct download_state_t ds[2] = {{0,0,0},{0,0,0}}, *ds_last = NULL;
  int curr = 0, tick = 0;
  while(1)
  {
    ds[curr] = args->storer->GetDownloadState();
    if(ds_last)
    {
      ILOG(" DOWNLOAD STATE:: total:[%ld]Bytes recv:[%ld] Bytes speed:[%.2f] KByte/second::\n", 
            ds[curr].file_size, ds[curr].curr_size, 
            (ds[curr].curr_size - ds_last->curr_size) / ((TICK_TIMES * TICK_UNIT / 1000000) * 1024.00));
    }
    ds_last = &ds[curr];
    curr = SWAP_BIN(curr);
    
    tick = TICK_TIMES;
    while(--tick > 0)
    {
      usleep(TICK_UNIT);
      if(!args->run)
        return NULL;
    }
  }
  return NULL;
}


long Downloader::Start()
{
  int i, ret, tid;
  long status = ERROR_CODE_OF_DEFAULT;
  // 线程组，前面是下载线程。最后一个是监控线程 
  struct targs_t args[MAX_THREADS+1];
  pthread_t tids[MAX_THREADS+1];
  struct download_state_t ds;

  /* 执行两次，第一次为试探式的请求(因为不知道文件大小) */
  for(i = 0; i < 2; ++i) 
  {
    ds = _storer.GetDownloadState();
    if(ds.file_size > 0 && ds.file_size == ds.curr_size)
    {
       DLOG("in Downloader::Start times[%d] check done.\n", i);
       break;
    }
    DLOG(":: START thread num = [%d].\n", ds.block_num);
    for(tid = 0; tid < ds.block_num; ++tid)
    {
      // 本数据块已经完成
      if(_storer.GetStoreState()->blocks[args->tid].left == 0)
      {
        args[tid].tid = -1; 
        continue;
      }
      args[tid].proto = _proto;
      args[tid].storer = &_storer;
      args[tid].tid = tid;
      ret = pthread_create(&tids[tid], NULL, loader_work, &args[tid]);
      if(ret != 0)
      {
        ELOG("pthread_create [%d] error\n", tid);
        assert(true);
      }
    }

    // 监控线程
    args[MAX_THREADS].proto = _proto;
    args[MAX_THREADS].storer = &_storer;
    args[MAX_THREADS].run = 1;
    ret = pthread_create(&tids[MAX_THREADS], NULL, mon_work, &args[MAX_THREADS]);
    if(ret != 0)
    {
      ELOG("pthread_create [%d] error\n", tid);
      //assert(true);
    }

    for(tid = 0; tid < ds.block_num; ++tid)
    {
      if(args[tid].tid < 0)
        continue;
      pthread_join(tids[tid], NULL);
      if(args[tid].status < 0)
        status = args[tid].status;
    }
   
    // 工程线程结束时，结束监控线程
    args[MAX_THREADS].run = 0;
    pthread_join(tids[MAX_THREADS], NULL);
 
    // 检查结束
    ds = _storer.GetDownloadState();
    if(ds.file_size > 0 && ds.file_size == ds.curr_size)
    {
      _storer.Done();
      return ds.file_size;
    }
    else if(ds.file_size > 0 && ds.curr_size > 0)
    {
      _storer.Sync();
      //return ds.curr_size;
    }

    if(status != ERROR_CODE_OF_ADJUST_STATE)
      break;
  }
  return status;
}

bool Downloader::CheckMd()
{
  char md[MAX_MD_SIZE*2+1];
  char md_remote[MAX_MD_SIZE*2+1];
  int ret, trys = 3;
  AppProtoConnector conn;
  long file_size, status;
  const char *p = NULL;

  p = GetAttr((char *)DOWNLOAD_ATTR_MD5_URL);
  if(!EMPTY_STR(p))
  {
    if(!_proto->ReInit(p))
    {
      printf("Init md5 url fail !\n");
      return false;
    }
    ret = md5_sum(_storer.GetFileName(), md);
    if(ret > 0)
    {
      md[ret] = 0;
      printf("File md5: [%s]\n", md);
    }
    else
    {
      printf("Calc file md5 Fail!\n");
      return false;
    }

    if(!conn.Init(_proto))
    {
      ELOG("in check_md:: initConn fail!\n");
      return false;
    }
  
    while(trys-- > 0)
    {
      status = conn.SingleExch(md_remote, sizeof(md_remote), &file_size);
      if(status > 0)
        break;
    }
    if(status < ret)
    {
      printf("File md5: [%s]\n", md);
      return false;
    }
    printf("Remote file md5: [%.*s].\n", ret, md_remote);
    if(strncmp(md, md_remote, ret) == 0)
    {
      printf("Verify OK\n");
      return true;
    }
    printf("Verify Fail\n");
  }
  else if(!EMPTY_STR(p = GetAttr((char *)DOWNLOAD_ATTR_SHA1_URL)))
  {
    if(_proto->ReInit(p))
    {
      printf("Init sha1 url fail !\n");
      return false;
    }

    ret = sha1_sum(_storer.GetFileName(), md);
    if(ret > 0)
    {
      md[ret] = 0;
      printf("File sha1: [%s]\n", md);
    }
    else
    {
      printf("Calc file sha1 Fail!\n");
      return false;
    }

    if(!conn.Init(_proto))
    {
      ELOG("in check_md:: initConn fail!\n");
      return false;
    }
  
    while(trys-- > 0)
    {
      status = conn.SingleExch(md_remote, sizeof(md_remote), &file_size);
      if(status > 0)
        break;
    }
    if(status < ret)
    {
      printf("File sha1: [%s]\n", md);
      return false;
    }
    printf("Remote file sha1: [%.*s].\n ", ret, md_remote);
    if(strncmp(md, md_remote, ret) == 0)
    {
      printf("Verify OK\n");
      return true;
    }
    printf("Verify Fail\n");
  }
  return false;
}

int Downloader::Download()
{
  time_t start;
  long status;
  const char *file_url = GetAttr((char *)DOWNLOAD_ATTR_FILE_URL);

  if(EMPTY_STR(file_url))
  {
    ELOG("GetAttr file_url NULL\n");
    return ERROR_CODE_OF_ARG;
  }

  if(PROTO_IS_HTTP(file_url))
  {
    _proto = new HttpProto;
  }
  else if(PROTO_IS_FILE(file_url))
  {
    _proto = new FileProto;
  }
  else
  {
    fprintf(stderr, "Parse url, proto not supported!\n");
    return ERROR_CODE_OF_PROTO_NOT_SUPPORTED;
  }

  if(!_proto->Init(file_url))
  {
    fprintf(stderr, "Proto %s Init error!\n", _proto->GetProto());
    return ERROR_CODE_OF_PROTO_INIT;
  }

  if(!_storer.Init(file_url))
  {
    fprintf(stderr, "Storer Init error!\n");
    return ERROR_CODE_OF_STORE_INIT;
  }

  // 开始下载文件 
  start = (int)time(NULL);
  status = Start();

  if(!DOWNLOAD_STATUE_ALL_COMPLETED(status))
  {
    // 下载失败
    printf("Download %s error status(%ld)!\n", file_url, status);
    return (int)status;
  }

  // 下载成功
  Done(file_url, status, (int)time(NULL) - start);

  return 0;
}

void Downloader::Done(const char *file_url, long file_size, int use_sec)
{
  printf("---------------------------------------------\n");
  printf("Download %s ok, file size %ld bytes used %d seconds.\n", file_url, file_size, use_sec);
  printf("Store file [%s] ok.\n", _storer.GetFileName());

  // 获取下载文件对应的md5/sha1等验证吗(如果有指定).
  CheckMd();
}

const char *Downloader::GetProtoAttr(char *name)
{
  return _proto->GetAttr(name);
}

