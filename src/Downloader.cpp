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
};

Downloader::Downloader() { }

Downloader::~Downloader() { }


/* 下载任务工作单元 */
static void *loader_work(void *p)
{
  int ret, trys = MAX_TRY_TIMES;
  long len, file_size, left;
  struct targs_t *args = (struct targs_t *)p;
  struct store_state_t *state = args->storer->GetStoreState();
  struct block_state_t *b_state = &state->blocks[args->tid];
  AppProtoConnector conn;
  
  if(!conn.Init(args->proto, DEFAULT_TIMEOUT))
  {
    ELOG("tid[%d] initConn fail!\n", args->tid);
    return NULL;
  }

  /* 当文件大小有变化且任务未完成时继续循环下载。
   *   当文件大小未无生变化时重试MAX_TRY_TIMES
   *   因为第一次请求的时文件大小尚未知晓，所以第一次只有一条线程请求，
   *   且第一次完成后(已获得文件大小),主动返回上层，让上层重新分配多任务下载 
   */
  while(trys > 0)
  {
    DLOG("tid[%d]::: Req left:%ld, offset:%ld\n", args->tid, b_state->left, b_state->offset);
    ret = conn.Req(b_state->left, b_state->offset);
    if((ret) < 0)
    {
      ELOG("tid[%d] proto->Req fail!\n", args->tid);
      TRY_AGAIN();
    }

    len = conn.Res(args->storer->GetWritePtr(args->tid), b_state->left, &file_size);
    if((len) < 0)
    {
      if(len == ERROR_CODE_OF_RELOCATION)
      {
         return NULL;
      }
      if(len == ERROR_CODE_OF_DATA)
      {
        ELOG("tid[%d] proto->Res data fail!\n", args->tid);
        return NULL;
      }
      ELOG("tid[%d] proto->Res fail ret = %ld!\n", args->tid, len);
      TRY_AGAIN();
    }
    
    /* 第一次请求 调整状态信息并返回让上层再次分配任务 */
    if(args->tid == 0 &&  b_state->offset == 0)
    {
      ILOG("::: begin to download file, size is %ld Bytes.\n", file_size);
      args->storer->AdjustState(file_size, len);
      return NULL;
    }

    /* 更新下载内容 */
    if((left = args->storer->UpdateState(args->tid, len)) < 0)
    {
      ELOG("tid[%d] UpdateState. ret = %ld\n", args->tid, left);
      return NULL;
    }

    // 完成
    if(left == 0)
    {
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


long Downloader::Start(AppProto &proto, Storer &storer)
{
  int i, ret, tid;
  // 线程组，前面是下载线程。最后一个是监控线程 
  struct targs_t args[MAX_THREADS+1];
  pthread_t tids[MAX_THREADS+1];
  struct download_state_t ds;

  /* 执行两次，第一次为试探式的请求(因为不知道文件大小) */
  for(i = 0; i < 2; ++i) 
  {
    ds = storer.GetDownloadState();
    if(ds.file_size > 0 && ds.file_size == ds.curr_size)
    {
       DLOG("in Downloader::Start times[%d] check done.\n", i);
       break;
    }
    DLOG(":: START thread num = [%d].\n", ds.block_num);
    for(tid = 0; tid < ds.block_num; ++tid)
    {
      // 本数据块已经完成
      if(storer.GetStoreState()->blocks[args->tid].left == 0)
      {
        args[tid].tid = -1; 
        continue;
      }
      args[tid].proto = &proto;
      args[tid].storer = &storer;
      args[tid].tid = tid;
      ret = pthread_create(&tids[tid], NULL, loader_work, &args[tid]);
      if(ret != 0)
      {
        ELOG("pthread_create [%d] error\n", tid);
        assert(true);
      }
    }

    // 监控线程
    args[MAX_THREADS].proto = &proto;
    args[MAX_THREADS].storer = &storer;
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
    }
   
    // 工程线程结束时，结束监控线程
    args[MAX_THREADS].run = 0;
    pthread_join(tids[MAX_THREADS], NULL);

 
    // 检查结束
    ds = storer.GetDownloadState();
    if(ds.file_size > 0 && ds.file_size == ds.curr_size)
    {
      storer.Done();
      return ds.file_size;
    }
    else if(ds.file_size > 0 && ds.curr_size > 0)
    {
      storer.Sync();
      //return ds.curr_size;
    }
    else
    {
      return ERROR_CODE_OF_DEFAULT;
    }
  }
  return ERROR_CODE_OF_PART_DONE;
}

bool check_md(AppProto &proto, const char *store_file, char *md5_url, char *sha1_url)
{
  char md[MAX_MD_SIZE*2+1];
  char md_remote[MAX_MD_SIZE*2+1];
  int ret, trys = 3;
  AppProtoConnector conn;
  long file_size, status;

  if(strlen(md5_url) > 0)
  {
    if(!proto.ReInit(md5_url))
    {
      printf("Init md5 url fail !\n");
      return false;
    }
    ret = md5_sum(store_file, md);
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

    if(!conn.Init(&proto, DEFAULT_TIMEOUT))
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
    printf("Remote file md5: [%.*s], ", ret, md_remote);
    if(strncmp(md, md_remote, ret) == 0)
    {
      printf("Verify OK\n");
      return true;
    }
    printf("Verify Fail\n");
  }
  else if(strlen(sha1_url) > 0)
  {
    if(!proto.ReInit(sha1_url))
    {
      printf("Init sha1 url fail !\n");
      return false;
    }

    ret = sha1_sum(store_file, md);
    if(ret > 0)
    {
      md[ret] = 0;
      printf("File md5: [%s]\n", md);
    }
    else
    {
      printf("Calc file sha1 Fail!\n");
      return false;
    }

    if(!conn.Init(&proto, DEFAULT_TIMEOUT))
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
    printf("Remote file sha1: [%.*s], ", ret, md_remote);
    if(strncmp(md, md_remote, ret) == 0)
    {
      printf("Verify OK\n");
      return true;
    }
    printf("Verify Fail\n");
  }
  return false;
}
