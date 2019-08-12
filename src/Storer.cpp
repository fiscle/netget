#include <assert.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <unistd.h>
#include <pthread.h>
#include "Base.h"
#include "Storer.h"

#define mmap_open_state_file(f) (struct store_state_t *)mmap_open((f), sizeof(struct store_state_t))
#define mmap_close_state_file(p) mmap_close((p), sizeof(struct store_state_t))
#define mmap_sync_state_file(p) mmap_sync((p), sizeof(struct store_state_t))

#define FILE_NOT_WRITE(f) (access((f), W_OK) != 0)
#define FILE_NOT_EXEC(f) (access((f), X_OK) != 0)

#define OPEN_FILE_MODE (S_IRUSR | S_IWUSR)

// 下载文件存放路径
static char g_download_path[MAX_FILE_PATH] = "./download";

// 文件名
static char *get_file_name(char *file, char *buf, int size)
{
  snprintf(buf, size, "%s/%s", g_download_path, file);
  return buf;
}

// 获取状态文件名称, 下载完成这个文件会被删除
static char *get_state_file_name(char *file, char *buf, int size)
{
  snprintf(buf, size, "%s/.state.%s~", g_download_path, file);
  return buf;
}

// 获取临时文件名称, 下载完成这个文件会更名为正式文件名
static char *get_temp_file_name(char *file, char *buf, int size)
{
  snprintf(buf, size, "%s/%s~", g_download_path, file);
  return buf;
}

// 下载文件对应的唯一区别值，用于区别下唯一性
static char *get_file_key(char *path, char *file_key, int bufsize)
{
  int len = strlen(path);
  if(len > bufsize)
    len = bufsize;
  snprintf(file_key, len , path);
  return file_key;
}

long get_fd_size(int fd)
{
  INIT_FUNC();
  struct stat sb;
  if(fstat(fd, &sb) == -1)
  {
    //SELOG("in get_fd_szie fd [%d].\n", fd);
    return -1;
  }
  return (long)sb.st_size;
}

static long get_file_szie(char *file)
{
  INIT_FUNC();
  struct stat sb;
  if(stat(file, &sb) == -1)
  {
    //SELOG("in get_file_szie file [%s].\n", file);
    return -1;
  }
  return (long)sb.st_size;
}

// 打开设置文件大小以便后续映射到内存
static int mmap_prepare_file(char *file, long size)
{
  int fd;

  /* 如果文件已经比要求大小大(要不状态文件已经存，
   *   要不临时文件已经存:预开辟的临时文件比实际大)，
   * 则直接返回
   */
  if(get_file_szie(file) >= size)
    return 0;

  fd = open(file, O_WRONLY | O_CREAT, OPEN_FILE_MODE);
  if(fd < 0)
  {
    ELOG("mmap_prepare_file open %s fail.\n", file);
    return ERROR_CODE_OF_ACCESS_FILE;
  }
  lseek(fd, size - 1, SEEK_SET);
  write(fd, "\0", 1);
  close(fd);
  return 0;
}

// 关闭映射
static void mmap_close(void *p, long size)
{
  INIT_FUNC();
  if(p)
    munmap(p, size);
}


// 把文件映射到内存
static void *mmap_open(char *file, long size)
{
  INIT_FUNC();
  int fd = -1;
  void *p = NULL;

  if(mmap_prepare_file(file, size) < 0)
    return NULL;
  
  fd = open(file, O_RDWR, OPEN_FILE_MODE);
  DLOG("open file[%s]%d\n", file, fd);
  if(fd < 0)
    return NULL;

  p = (char *)mmap(NULL, (size_t)size, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
  assert(p);

  return p;
}
static void *mmap_reopen(void *old_p, long old_s, char *file, long size)
{
  INIT_FUNC();
  int fd = -1;
  void *p = NULL;

  mmap_close(old_p, old_s);

  if(mmap_prepare_file(file, size) < 0)
    return NULL;
  
  fd = open(file, O_RDWR, OPEN_FILE_MODE);
  DLOG("open file[%s]%d\n", file, fd);
  if(fd < 0)
    return NULL;

  p = (char *)mmap(NULL, (size_t)size, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
  assert(p);

  return p;
}

// 同步到文件
static int mmap_sync(void *p, long len)
{
  INIT_FUNC();
  return msync(p, len, MS_SYNC);
}

/*
static void copy_state(struct store_state_t *src, struct store_state_t *dst)
{
  memcpy(dst->file_key, src->file_key, MAX_FILE_KEY_SIZE);
  dst->file_size = src->file_size;
  dst->curr_size = src->curr_size;
  dst->block_num = src->block_num;
  dst->block_size = src->block_size;
  memcpy(dst->blocks, src->blocks, sizeof(struct block_state_t) * MAX_THREADS);
  dst->temp_file_ptr = src->temp_file_ptr;
}
*/

// 初始化状态文件
struct store_state_t *init_state_file(char *file_key, char *state_file_name, char *temp_file_name, long block_size)
{
  struct store_state_t *pstate = NULL;
  pstate = mmap_open_state_file(state_file_name);
  if(!pstate)
  {
    return NULL;
  }

  pstate->temp_file_ptr = (char *)mmap_open(temp_file_name, block_size);
  if(!pstate->temp_file_ptr)
  {
    mmap_close_state_file(pstate);
    return NULL;
  }

  // 状态文件以这个file_key为标识，续传时用于区别不同文件
  memcpy(pstate->file_key, file_key, MAX_FILE_KEY_SIZE);
  pstate->file_size = 0;
  pstate->curr_size = 0;
  pstate->block_num = 1;
  pstate->block_size = block_size;
  pstate->blocks[0].id = 0;;
  pstate->blocks[0].left = block_size;
  pstate->blocks[0].offset = 0;
  return pstate;
}


Storer::Storer()
{
  _is_ok = false;
  _LOCK_INIT();
}
Storer::~Storer()
{
  _LOCK_DEL();
}

bool Storer::Init(char *url, long block_size)
{
  char file_key[MAX_FILE_KEY_SIZE];

  if(!url || strlen(url) == 0)
    return false;

  getcwd(g_download_path, sizeof(g_download_path));
  snprintf(g_download_path + strlen(g_download_path), 
      sizeof(g_download_path) - strlen(g_download_path), "/%s", "download");
  shell_cmd("mkdir -p %s", g_download_path);

  if(FILE_NOT_EXEC(g_download_path))
    return false;

  _file_size = 0;
  get_file_name(file_basename(url), _file_name, sizeof(_file_name));
  get_state_file_name(file_basename(url), _state_file, sizeof(_state_file));
  get_temp_file_name(file_basename(url), _temp_file, sizeof(_temp_file));
  _block_size = block_size;
  get_file_key(_file_name, file_key, sizeof(file_key));

  /* 打开状态文件和临时文件，并映射到内存
   *   如果两文件都存在，并且file_key和大小检查都一至则继续使用当前状态文件续传文件
   *   否则，清除所有文件，重新打开两个文件
   */
  if(FILE_NOT_WRITE(_state_file)
     || FILE_NOT_WRITE(_temp_file)
     || get_file_szie(_state_file) != sizeof(struct store_state_t))
  {
    _state = NULL;
    Clean();
    _state = init_state_file(file_key, _state_file, _temp_file, _block_size);
  }
  else
  {
     _state = mmap_open_state_file(_state_file);
 
    if(!_state)
    {
      ELOG("in Storer::Init mmap_open exist file(%s)size(%ld) fail!\n", _state_file, sizeof(struct store_state_t));
      return false;
    }

    if(strcmp(file_key, _state->file_key) == 0 && get_file_szie(_temp_file) == _state->file_size)
    {
      _state->temp_file_ptr = (char *)mmap_open(_temp_file, _state->file_size);
      if(!_state->temp_file_ptr)
      {
        ELOG("in Storer::Init mmap_open exist file(%s)size(%ld) fail!\n", _temp_file, _state->file_size);
        mmap_close_state_file(_state);
        return NULL;
      }
    }
    else
    {
      Clean();
      _state = init_state_file(file_key, _state_file, _temp_file, _block_size);
    }
   
  }

  if(!_state)
  {
    ELOG("in Storer::Init mmap_open file(%s)size(%ld) fail!\n", _state_file, sizeof(struct store_state_t));
    return false;
  }

  return true;
}

void Storer::SetBlockSize(long size)
{
  _block_size = size;
}

long Storer::GetBlockSize()
{
  return _block_size;
}

struct download_state_t Storer::GetDownloadState()
{
  struct download_state_t ds = {0, 0, 0};
  if(!_state)
  {
    return ds;
  }
  _LOCK();
  if(_is_ok)
  {
    ds.file_size = _download_state.file_size;
    ds.curr_size = _download_state.curr_size;
    ds.block_num = _download_state.block_num;
    return ds;
  }
  ds.file_size = _state->file_size;
  ds.curr_size = _state->curr_size;
  ds.block_num = _state->block_num;
  _UNLOCK();

#if (DEBUG_LEVEL > 3)
  {
    int i = 0;
    DLOG("##### file_size = %ld  curr_size = %ld #######\n", ds.file_size, ds.curr_size);
    for(i = 0; i < ds.block_num; ++i)
      DLOG("  ### block[%d] left = %ld  offset = %ld #####\n", i, _state->blocks[i].left, _state->blocks[i].offset);
  }
#endif

  return ds;
}

struct store_state_t *Storer::GetStoreState()
{
  return _state;
}

// 更新状态，文件已完成返回0，失败返回-1,否则返回剩余大小
long Storer::UpdateState(int block_id, long update_size)
{
  if(!_state)
    return ERROR_CODE_OF_ARG;
  _LOCK();
  _state->curr_size += update_size;
  _UNLOCK();
  _state->blocks[block_id].offset += update_size;
  _state->blocks[block_id].left -= update_size;
  return _state->blocks[block_id].left;
}

void Storer::Clean()
{
  if(_state)
  {
    if(_state->temp_file_ptr)
      mmap_close(_state->temp_file_ptr, _file_size);
    mmap_close_state_file(_state);
  }
  shell_cmd("rm -f %s %s", _temp_file, _state_file);
}

void Storer::Sync()
{
  if(_state)
  {
    if(_state->temp_file_ptr)
      mmap_sync(_state->temp_file_ptr, _file_size);
    mmap_sync_state_file(_state);
  }
}
void Storer::Done()
{
  _is_ok = true;
  if(_state)
  {
    _file_size = _state->file_size;
    _download_state.file_size = _state->file_size;
    _download_state.curr_size = _state->curr_size;
    _download_state.block_num = _state->block_num;

    if(_state->temp_file_ptr)
      mmap_close(_state->temp_file_ptr, _file_size);
    mmap_close_state_file(_state);
  }
  else
  {
    _download_state.file_size = _file_size;
    _download_state.curr_size = 0;
    _download_state.block_num = 0;
  }

  if(truncate(_temp_file, (off_t)_file_size) != 0)
  {
    SELOG("in Storer::Done truncate file[%s] to %ld\n", _temp_file, _file_size);
  }
  shell_cmd("mv %s %s", _temp_file, _file_name);
  shell_cmd("rm -f %s", _state_file);
}

static int calc_block_num(long file_size)
{
  int i = 0;
  for(i = 0; i < MAX_THREADS; ++i)
  {
    if(file_size < 1024 * 1024 * 5 * (1 << i))
    //if(file_size < 1024 * 1024 * (1 << i))
    //if(file_size < 1024 * 10 * (1 << i))
    //if(file_size < 1024 * 50 * (1 << i))
    //if(file_size < 1024 * 200 * (1 << i))
      return i + 1;
  }
  return MAX_THREADS;
}

bool Storer::AdjustState(long file_size, long recv_size)
{
  int i = 0;
  if(!_state)
    return false;

  // reopen temp_file
  _state->temp_file_ptr = (char *)mmap_reopen(_state->temp_file_ptr, _file_size, _temp_file, file_size);
  /*
  mmap_sync(_state->temp_file_ptr, _file_size);
  mmap_close(_state->temp_file_ptr, _file_size);
  _state->temp_file_ptr = (char *)mmap_open(_file_name, file_size);
  */
  if(!_state->temp_file_ptr)
  {
    ELOG("in AdjustState reopen %s fail. %ld to %ld\n", _file_name, _file_size, file_size);
    mmap_close_state_file(_state);
    return false;
  }

  _file_size = file_size;
  _state->file_size = file_size;
  _state->block_num = calc_block_num(_state->file_size);

  _state->block_size = _file_size / _state->block_num;

  if(file_size % _state->block_num)
    ++_state->block_size;

  _state->curr_size = recv_size;
  // excell first ignored
  if(_state->curr_size > _state->block_size)
    _state->curr_size = _state->block_size;

  for(i = 0; i < _state->block_num; ++i)
  {
    _state->blocks[i].id = i;
    _state->blocks[i].left = _state->block_size;
    _state->blocks[i].offset = _state->block_size * i;
  }

  // have recved
  _state->blocks[0].left = _state->block_size - _state->curr_size;
  _state->blocks[0].offset = _state->curr_size;
  
  // last block
  if(_state->block_num > 1)
  {
    //_state->blocks[_state->block_num - 1].left = _state->block_size - file_size % _state->block_num;
    _state->blocks[_state->block_num - 1].left = file_size - _state->block_size * (_state->block_num - 1);
  }

  mmap_sync_state_file(_state);


  return true;

}

char *Storer::GetFileName()
{
  return _file_name;
}

char *Storer::GetWritePtr(int thread_id)
{
  if(!_state)
  {
    ELOG("in GetWritePtr, _state null!\n");
    return NULL;
  }
  if(!_state->temp_file_ptr)
  {
    ELOG("in GetWritePtr, temp_file_ptr null!\n");
    return NULL;
  }
  return _state->temp_file_ptr + _state->blocks[thread_id].offset;
}

