// the lock server implementation

#include "lock_server.h"
#include <sstream>
#include <stdio.h>
#include <unistd.h>
#include <arpa/inet.h>

lock_server::lock_server():
  nacquire (0)
{
 pthread_mutex_init(&lmap_mutex, NULL);
 pthread_cond_init(&lmap_state_cv, NULL);
}
lock_server::~lock_server(){
	pthread_mutex_destroy(&lmap_mutex);
	pthread_cond_destroy(&lmap_state_cv);
}

lock_protocol::status
lock_server::stat(int clt, lock_protocol::lockid_t lid, int &r)
{
  lock_protocol::status ret = lock_protocol::OK;
  printf("stat request from clt %d\n", clt);
  r = nacquire;
  return ret;
}
lock_protocol::status
lock_server::acquire(int clt, lock_protocol::lockid_t lid, int &r)
{
  lock_protocol::status ret = lock_protocol::OK;
    printf("acquire request from clt %d for lid %llu\n", clt, lid);
//  lockstate lock_state = LOCKFREE;
  pthread_mutex_lock(&lmap_mutex);
  if(tLockMap.count(lid)>0){
	  while(LOCKFREE != tLockMap[lid])
		   pthread_cond_wait(&lmap_state_cv,&lmap_mutex);
  }
  tLockMap[lid] = LOCKED;
  pthread_mutex_unlock(&lmap_mutex);
  return ret;
}
lock_protocol::status
lock_server::release(int clt, lock_protocol::lockid_t lid, int &r)
{
  lock_protocol::status ret = lock_protocol::OK;//可以这么用
    printf("release request from clt %d for lid %llu\n", clt, lid);
//  lockstate lock_state = LOCKFREE;
  pthread_mutex_lock(&lmap_mutex);
  if(tLockMap.count(lid)>0 && tLockMap[lid] == LOCKED ){
	  tLockMap[lid] = LOCKFREE;
      pthread_cond_broadcast(&lmap_state_cv); 
      pthread_mutex_unlock(&lmap_mutex);	  
  }
  else {
  pthread_mutex_unlock(&lmap_mutex);
  ret = lock_protocol::NOENT;
  }
  return ret;
}

