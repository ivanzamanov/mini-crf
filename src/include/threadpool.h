#ifndef _H_THREADPOOL
#define _H_THREADPOOL

#include <pthread.h>

#include <deque>
#include <iostream>
#include <vector>

using namespace std;

const int DEFAULT_POOL_SIZE = 10;
const int STARTED = 0;
const int STOPPED = 1;

class Mutex
{
 public:
  Mutex()
    {
      pthread_mutex_init(&m_lock, NULL);
      is_locked = false;
    }
  ~Mutex()
    {
      while(is_locked) { ; }
      unlock(); // Unlock Mutex after shared resource is safe
      pthread_mutex_destroy(&m_lock);
    }
  void lock()
  {
    pthread_mutex_lock(&m_lock);
    is_locked = true;
  }
  void unlock()
  {
    is_locked = false; // do it BEFORE unlocking to avoid race condition
    pthread_mutex_unlock(&m_lock);
  }
  pthread_mutex_t* get_mutex_ptr()
  {
    return &m_lock;
  }
 private:
  pthread_mutex_t m_lock;
  volatile bool is_locked;
};

class CondVar
{
 public:
  CondVar() {
    pthread_cond_init(&m_cond_var, NULL);
  }
  ~CondVar() {
    pthread_cond_destroy(&m_cond_var);
  }
  void wait(pthread_mutex_t* mutex) {
    pthread_cond_wait(&m_cond_var, mutex);
  }
  void signal() {
    pthread_cond_signal(&m_cond_var);
  }
  void broadcast() {
    pthread_cond_broadcast(&m_cond_var);
  }
 private:
  pthread_cond_t m_cond_var;
};

//template<class TClass>
class Task
{
 public:
  //  Task(TCLass::* obj_fn_ptr); // pass an object method pointer
  Task() { }; // pass a free function pointer
  virtual ~Task() { };
  virtual void operator()() = 0;
};

class ThreadPool
{
 public:
  ThreadPool();
  ThreadPool(int pool_size);
  ~ThreadPool();
  int initialize_threadpool();
  int destroy_threadpool();
  void* execute_thread();
  int add_task(Task* task);
 private:
  int m_pool_size;
  Mutex m_task_mutex;
  CondVar m_task_cond_var;
  std::vector<pthread_t> m_threads; // storage for threads
  std::deque<Task*> m_tasks;
  volatile int m_pool_state;
};

template <class ParamT>
class ParamTask : public Task {
 public:
 ParamTask(void (*fn_ptr)(ParamT*), ParamT* arg)
   :m_fn_ptr(fn_ptr), m_arg(arg) { }

  void operator()() {
    (*m_fn_ptr)(m_arg);
    //if (m_arg != NULL) {
    //  delete m_arg;
    //}
  }

 protected:
  void (*m_fn_ptr)(ParamT*);
  ParamT* m_arg;
};

#endif /* _H_THREADPOOL */

