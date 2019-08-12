#ifndef _Storer_h
#define _Storer_h

#include <pthread.h>
#include "Base.h"

#define LOCK_DEF(n) pthread_mutex_t n
#define LOCK(p) pthread_mutex_lock(p)
#define UNLOCK(p) pthread_mutex_unlock(p)
#define LOCK_INIT(p) pthread_mutex_init((p), NULL)
#define LOCK_DEL(p) pthread_mutex_destroy(p)

#define _LOCK_DEF() LOCK_DEF(_lock)
#define _LOCK() LOCK(&_lock)
#define _UNLOCK() UNLOCK(&_lock)
#define _LOCK_INIT() LOCK_INIT(&_lock)
#define _LOCK_DEL() LOCK_DEL(&_lock)

// 下载信息
struct download_state_t
{
  long file_size;
  long curr_size;
  int  block_num;
};

// 每块下载状态
struct block_state_t
{
  int id;
  long left;
  long offset;
};

// 下载状态
struct store_state_t
{
  char file_key[MAX_FILE_KEY_SIZE];
  long file_size;
  long curr_size;
  int  block_num;
  long  block_size;
  struct block_state_t blocks[MAX_THREADS];
  char *temp_file_ptr;
};

class Storer
{
public:
  Storer();
  ~Storer();
  struct store_state_t *GetStoreState();
  // 返回当前的下载信息
  struct download_state_t GetDownloadState();
  // 更新状态，文件已完成返回0，失败返回-1,否则返回剩余大小
  long UpdateState(int block_id, long update_size);
  // 清理临时文件和状态文件，打开句柄 
  void Clean();
  // 根据文件大小划分块(分配任务)
  bool AdjustState(long file_size, long recv_size);
  // 块大小管理
  void SetBlockSize(long size);
  long GetBlockSize();
  bool IsAssigned();
  // 初化化状态
  bool Init(char *url, long block_size = DEFAULT_BLOCK_SIZE);
  // 返回指定工作线程的写位置
  char *GetWritePtr(int thread_id);
  // 返回文件名 
  char *GetFileName();
  // 完成下载，更改文件名，删除临时信息
  void Done();
  // 同步内存到文件
  void Sync();

private:
  bool _is_ok;
  long _file_size;
  char _file_name[MAX_FILE_PATH];
  char _state_file[MAX_FILE_PATH];
  char _temp_file[MAX_FILE_PATH];
  long _block_size;
  struct download_state_t _download_state;
  struct store_state_t *_state;
  _LOCK_DEF();
};

#endif

