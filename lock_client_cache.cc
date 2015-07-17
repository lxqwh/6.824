// RPC stubs for clients to talk to lock_server, and cache the locks
// see lock_client_cache.h for protocol details.

#include "lock_client_cache.h"
#include "rpc.h"
#include <sstream>
#include <iostream>
#include <stdio.h>
#include "tprintf.h"


lock_client_cache::lock_client_cache(std::string xdst, 
				     class lock_release_user *_lu)
  : lock_client(xdst), lu(_lu)
{
  rpcs *rlsrpc = new rpcs(0);//服务器的端口任意？
  rlsrpc->reg(rlock_protocol::revoke, this, &lock_client_cache::revoke_handler);
  rlsrpc->reg(rlock_protocol::retry, this, &lock_client_cache::retry_handler);

  const char *hname;
  hname = "127.0.0.1";
  std::ostringstream host;
  host << hname << ":" << rlsrpc->port();//端口自动分配，这里只是获取
  id = host.str();
  
  VERIFY(pthread_mutex_init(&m_, 0) == 0);
  VERIFY(lock_status_[0]==lock_client_cache::NONE);//为什么要这么做，只是为了初始化？
  VERIFY(pthread_cond_init(&wait_retry_, 0) == 0);
  VERIFY(pthread_cond_init(&wait_release_, 0) == 0);  
}

lock_protocol::status
lock_client_cache::acquire(lock_protocol::lockid_t lid)
{
  lock_protocol::status ret = lock_protocol::OK;
  bool try_acquire = false;
  VERIFY(pthread_mutex_lock(&m_)==0);
  //如果不是NONE和FREE的话就要不停地等待下去，直到线程条件被唤醒，并且条件成立
  while (lock_status_[lid] != lock_client_cache::NONE
         && lock_status_[lid] != lock_client_cache::FREE) {
    tprintf("lock_client_cache(%s:%lu): waiting to acquire lock %llu in state %d\n",
            id.c_str(), pthread_self(), lid, lock_status_[lid]);
    VERIFY(pthread_cond_wait(&wait_release_, &m_) == 0);
  }
  
  tprintf("lock_client_cache(%s:%lu): trying acquire of lock %llu in state %d\n",
          id.c_str(),pthread_self(), lid, lock_status_[lid]);
  //Check state，此时的state有两种可能，NONE或者FREE
  lockstate lis = lock_status_[lid];  
  //如果是NONE，则肯定是去请求这个锁，如果是FREE,则是直接锁住，返回OK
  if (lis == lock_client_cache::NONE) {
    try_acquire = true;
    lock_status_[lid] = lock_client_cache::ACQUIRING;
  } else if (lis == lock_client_cache::FREE) {
	  //不用去管这个FREE怎么来的，分离工作，不要混在一块
    lock_status_[lid] = lock_client_cache::LOCKED;
  }
  //当NONE时要进行锁请求
  while (try_acquire) 
  {
    tprintf("lock_client_cache(%s:%lu): sending acq rpc for lock %llu in state %d\n",
            id.c_str(), pthread_self(), lid, lock_status_[lid]);
    // do not hold mutex while making rpc call
    VERIFY(pthread_mutex_unlock(&m_)==0);
    lock_protocol::status r;
    r = cl->call(lock_protocol::acquire, lid, id, ret);
    tprintf("lock_client_cache(%s:%lu): got %d back from rpcc, %d back from server\n",
            id.c_str(), pthread_self(), r, ret);
    VERIFY(r == lock_protocol::OK);
    VERIFY(pthread_mutex_lock(&m_)==0);
    try_acquire = false;
    // need to grab (possibly changed) lis，谁会改它？虽然这段时间没有加锁，但是可以访问的其它锁；对于这个锁，状态是acquire，会阻塞。
    lis = lock_status_[lid];
    tprintf("lock_client_cache(%s:%lu): acquire received %d in state %d for lock %llu\n",
            id.c_str(), pthread_self(), ret, lis, lid);
    if (lis == lock_client_cache::ACQUIRING) {
      if (ret == lock_protocol::OK) {
        lock_status_[lid] = lock_client_cache::LOCKED;
      } else if (ret == lock_protocol::RETRY) {
        //wait for retry_handler invocation
        lock_status_[lid] = lock_client_cache::WAITING;
        while (lock_status_[lid] == lock_client_cache::WAITING) {
          VERIFY(pthread_cond_wait(&wait_retry_, &m_) == 0);
        }
        try_acquire = true;
      } else {
        tprintf("lock_client_cache(%s:%lu):acquire received unexpected error(%d) for lock %llu\n",
                id.c_str(), pthread_self(), ret, lid);
      }
    } else if (lis == lock_client_cache::WAITING) {
      if (ret == lock_protocol::OK) {//从下面看，应该不可能会出现这种情况，只可能是RETEY
        lock_status_[lid] = lock_client_cache::LOCKED;
      } else if (ret == lock_protocol::RETRY) {
        // must have received retry RPC out of order, so just go ahead and retry
        // acq,发生了retry比RETRY更早的情况，即请求的那个还没有返回，retry的已经先到了。
        try_acquire = true;
        lock_status_[lid] = lock_client_cache::ACQUIRING;
      } else {
        tprintf("lock_client_cache(%s:%lu): acquire received unexpected error(%d) for lock %llu\n",
                id.c_str(), pthread_self(), ret, lid);
      }
    } else if (ret != lock_protocol::OK) {
      tprintf("lock_client_cache(%s:%lu): acquire received unexpected error(%d) for lock %llu in state %d\n",
              id.c_str(), pthread_self(), ret, lid, lis);
    }
  }

  VERIFY(pthread_mutex_unlock(&m_)==0);  
  return lock_protocol::OK;
}

lock_protocol::status
lock_client_cache::release(lock_protocol::lockid_t lid)
{
	
	
	
	
	
  return lock_protocol::OK;

}

rlock_protocol::status
lock_client_cache::revoke_handler(lock_protocol::lockid_t lid, 
                                  int &)
{
  int ret = rlock_protocol::OK;
  return ret;
}

rlock_protocol::status
lock_client_cache::retry_handler(lock_protocol::lockid_t lid, 
                                 int &)
{
  int ret = rlock_protocol::OK;
  return ret;
}



