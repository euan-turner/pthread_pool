#ifndef PTHREAD_POOL_H
#define PTHREAD_POOL_H

#include <pthread.h>
#include <stdbool.h>

#include "queue.h"

/**
 * @brief The user-supplied task should take in
 * one pointer to some arguments, and return nothing.
 * Typically args is a pointer to a struct containing
 * multiple argument values, and this struct is malloc'd
 * by a user function dispatching tasks to the thread pool,
 * and free'd at the end of the task.
 * 
 */
typedef void (*task)(void *args);

/**
 * @brief A thread pool that manages user-created tasks
 * The pool is not completely concurrently safe - currently
 * if one process is deleting the pool another process submitting to 
 * the pool will have issues
 * 
 */
struct thread_pool {
  bool joining;
  int num_workers;
  int num_active;
  int num_tasks;
  int max_tasks;
  struct queue tasks;
  pthread_mutex_t queue_lock;
  pthread_mutex_t active_lock;
  pthread_cond_t not_empty;
  pthread_cond_t not_full;
  pthread_cond_t all_completed;
  pthread_t *workers;
};

void pool_init(struct thread_pool *pool, int num_workers, int max_tasks);
void pool_submit(struct thread_pool *pool, task task, void *args);
void pool_join(struct thread_pool *pool);
void pool_destroy(struct thread_pool *pool, bool join);
int  pool_num_active(struct thread_pool *pool);
int  pool_num_inactive(struct thread_pool *pool);
#endif /* pthread_pool.h */

/*
- Initialise thread pool
- Add work to the pool, worker threads should start on work as it is added
- Add work to backlog, workers ignore until signalled by user
- Wait for all jobs in the queue to be completed
- Destroy the thread pool, allows currently running threads to finish
- Pause and resume threads in the thread pool
- Number of working threads
*/