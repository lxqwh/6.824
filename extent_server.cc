// the extent server implementation

#include "extent_server.h"
#include <sstream>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

extent_server::extent_server()
{
  VERIFY(pthread_mutex_init(&m_, 0) == 0);
}


int extent_server::put(extent_protocol::extentid_t id, std::string buf, int &)
{
  printf("extent_server: received put request of %llu %s\n",id, buf.c_str());
  extent_server::finfo f;
  f.buf = buf;
  time_t seconds;
  seconds = time(NULL);
  f.a.ctime = seconds;
  f.a.mtime = seconds;
  f.a.size = buf.size()*sizeof(char);
  VERIFY(pthread_mutex_lock(&m_)==0);
  table_[id] = f;
  VERIFY(pthread_mutex_unlock(&m_)==0);
  return extent_protocol::OK;
}

int extent_server::get(extent_protocol::extentid_t id, std::string &buf)
{
  VERIFY(pthread_mutex_lock(&m_)==0);
  printf("extent_server: received get request of %llu\n",id);
  if (table_.count(id) == 0) {
    // if id does not exist, return error
    VERIFY(pthread_mutex_unlock(&m_)==0);
    return extent_protocol::NOENT;
  }
  extent_server::finfo f = table_[id];
  time_t seconds;
  seconds = time(NULL);
  f.a.atime = seconds;//获取时间是这么定义的吗？
  table_[id].a.atime = seconds;//要atime要保存到文件属性吗？
  buf.assign(f.buf);
  printf("extent_server: served get request of %llu %s\n",id, buf.c_str());
  VERIFY(pthread_mutex_unlock(&m_)==0);
  return extent_protocol::OK;
}

int extent_server::getattr(extent_protocol::extentid_t id, extent_protocol::attr &a)
{
  printf("extent_server: received getattr request of %llu\n",id);
  VERIFY(pthread_mutex_lock(&m_)==0);
  if (table_.count(id) == 0) {
    // if id does not exist, return error
    VERIFY(pthread_mutex_unlock(&m_)==0);
    return extent_protocol::NOENT;
  }
  extent_server::finfo f = table_[id];
  a.size = f.a.size;
  a.atime = f.a.atime;
  a.mtime = f.a.mtime;
  a.ctime = f.a.ctime;
  VERIFY(pthread_mutex_unlock(&m_)==0);
  return extent_protocol::OK;
}

int extent_server::remove(extent_protocol::extentid_t id, int &)
{
  printf("extent_server: received removu request of %llu\n",id);
  VERIFY(pthread_mutex_lock(&m_)==0);
  table_.erase(id);
  VERIFY(pthread_mutex_unlock(&m_)==0);
  return extent_protocol::OK;
}