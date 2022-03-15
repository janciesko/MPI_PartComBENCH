/*
//@HEADER
// ************************************************************************
//
//                        Partix 1.0
//              Copyright Type Date and Name (2022)
//
// Questions? Contact Jan Ciesko (jciesko@sandia.gov)
//
// ************************************************************************
//@HEADER
*/

#include <cassert>
#include <map>
#include <thread.h>
#include <malloc.h>

#define SUCCEED(val) assert(val == 0)
#define MAX_TASKS_PER_TW 4096

partix_mutex_t global_mutex;
partix_mutex_t context_mutex;
partix_config_t *global_conf;

typedef struct {
  aligned_t thread;
  partix_task_args_t args;
} thread_handle_t;

/* We need this to implement taskwait*/
typedef struct {
  thread_handle_t threadHandle[MAX_TASKS_PER_TW];
  int context_task_counter;
} partix_handle_t;

typedef std::map<size_t, partix_handle_t *> partix_context_map_t;
partix_context_map_t context_map;

__attribute__((noinline)) size_t get_context() {
  debug("get_context");
  const unsigned int level = 2;
  size_t addr =
      (size_t)__builtin_extract_return_addr(__builtin_frame_address(level));
  return addr;
}

thread_handle_t *register_task(size_t context) {
  partix_context_map_t::iterator it;
  partix_mutex_enter(&context_mutex);
  it = context_map.find(context);
  if (it != context_map.end()) {
    partix_handle_t *context_handle = it->second;
    if(context_handle->context_task_counter >= MAX_TASKS_PER_TW)
    {
      debug("Error: context_handle->context_task_counter > MAX_TASKS_PER_TW");
      //TBD: return here an invalid threadHandle and stop generating tasks
      exit(1);
    }
    debug("register_task, it != context_map.end()");
    partix_mutex_exit(&context_mutex);
    return &context_handle
                ->threadHandle[context_handle->context_task_counter++];
  } else {
    partix_handle_t *context_handle =
        (partix_handle_t *)calloc(1, sizeof(partix_handle_t));
    const auto it_insert = context_map.insert(
        std::pair<size_t, partix_handle_t *>(context, context_handle));
    if (it_insert.second) { /*Insert successful*/
    }
    debug("register_task, it == context_map.end()");
    partix_mutex_exit(&context_mutex);
    return &context_handle
                ->threadHandle[context_handle->context_task_counter++];
  }
  assert(false); // DO NOT REACH HERE
}

void partix_mutex_enter() {
  debug("partix_mutex_enter");
  int ret = qthread_lock(&global_mutex);
  SUCCEED(ret);
}

void partix_mutex_exit() {
  int ret = qthread_unlock(&global_mutex);
  debug("partix_mutex_exit");
  SUCCEED(ret);
}

void partix_mutex_enter(partix_mutex_t *m) {
  debug("partix_mutex_enter");
  int ret = qthread_lock(m);
  SUCCEED(ret);
}

void partix_mutex_exit(partix_mutex_t *m) {
  int ret = qthread_unlock(m);
  debug("partix_mutex_exit");
  SUCCEED(ret);
}

void partix_mutex_init(partix_mutex_t *m) {
  /*EMPTY*/
}

void partix_mutex_destroy(partix_mutex_t *m) {
  /*EMPTY*/
}

int partix_executor_id(void) {
  debug("partix_executor_id");
  return qthread_id();
};

void partix_library_init(void) {
  debug("partix_library_init");
  int ret = qthread_init(global_conf->num_threads);
  SUCCEED(ret);
  partix_mutex_init(&global_mutex);
  partix_mutex_init(&context_mutex);
}

void partix_library_finalize(void) {
  debug("partix_library_finalize");
  partix_mutex_destroy(&global_mutex);
  partix_mutex_destroy(&context_mutex);
  qthread_finalize();
}

void partix_thread_create(void (*f)(partix_task_args_t *), void *args,
                     aligned_t *handle) {
  debug("partix_thread_create");
  int ret = qthread_fork((long unsigned int (*)(void *))f, args, handle);
  SUCCEED(ret);
}

void partix_thread_join(aligned_t * handle) {
  debug("partix_thread_join");
  int ret = qthread_readFF(NULL, handle);
  SUCCEED(ret);
}

__attribute__((noinline)) void partix_task(void (*f)(partix_task_args_t *),
                                           void *user_args) {
  size_t context = get_context();
  thread_handle_t *threadhandle = register_task(context);
  partix_task_args_t *partix_args = &threadhandle->args;
  partix_args->user_task_args = user_args;
  partix_args->conf = global_conf;
  debug("partix_task");
  partix_thread_create(f, partix_args, &threadhandle->thread);
}

__attribute__((noinline)) void partix_taskwait() {
  size_t context = get_context();
  partix_context_map_t::iterator it;
  it = context_map.find(context);
  if (it == context_map.end()) {
    debug("partix_taskwait, it == context_map.end()");
    return;
  }
  debug("partix_taskwait, it != context_map.end()");

  partix_handle_t *context_handle = it->second;
  for (int i = 0; i < context_handle->context_task_counter; ++i) {
    debug("partix_taskwait, partix_thread_join");
    partix_thread_join(&context_handle->threadHandle[i].thread);
  }
  free(context_handle);
  partix_mutex_enter(&context_mutex);
  context_map.erase(it);
  partix_mutex_exit(&context_mutex);
}
