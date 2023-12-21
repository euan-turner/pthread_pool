#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "queue.h"
#include "pthread_pool.h"

void execute_task(struct thread_pool *pool);
void *worker(void *args);

/**
 * @brief Associates a task with its arguments 
 * 
 */
typedef struct {
  task task;
  void *args;
} task_t;

/**
 * @brief Initialise fields in a thread pool
 * @details Worker threads are malloc'd, so call `destroy_pool`
 * to ensure all memory is free'd correctly
 * @param pool
 * @param num_workers 
 * @param max_tasks 
 */
void pool_init(struct thread_pool *pool, int num_workers, int max_tasks) {
  pool->joining = false;
  pool->num_workers = num_workers;
  pool->num_active = 0;
  pool->num_tasks = 0;
  pool->max_tasks = max_tasks;
  init_queue(&pool->tasks);
  pthread_mutex_init(&pool->queue_lock, NULL);
  pthread_mutex_init(&pool->active_lock, NULL);
  pthread_cond_init(&pool->not_empty, NULL);
  pthread_cond_init(&pool->not_full, NULL);
  pool->workers = (pthread_t *) malloc(num_workers * sizeof(pthread_t));

  /* Create worker threads 
  They will pick up tasks as soon as tasks are submitted
  */
  for (int i = 0; i < num_workers; i++) {
    pthread_create(&pool->workers[i], NULL, worker, pool);
  }  
  /* TODO: Wait for threads to initialise before returning */
}

/**
 * @brief Submit a task for execution to thread pool
 * 
 * @param pool 
 * @param task 
 * @param args 
 */
void pool_submit(struct thread_pool *pool, task task, void *args) {
  /* Mutual exclusion on pool queue */
  pthread_mutex_lock(&pool->queue_lock);

  /* Wait until space to add more tasks */
  while (pool->num_tasks >= pool->max_tasks) {
    pthread_cond_wait(&pool->not_full, &pool->queue_lock);
  }

  task_t *t = malloc(sizeof(task_t));
  if (t == NULL) {
    fprintf(stderr, "Malloc failed\n");
    exit(1);
  }
  t->task = task;
  t->args = args;

  if (!enqueue(&pool->tasks, t)) {
    fprintf(stderr, "Enqueue failed\n");
    exit(1);
  }

  pool->num_tasks++;
  
  /* Wake up threads waiting on a task to execute */
  pthread_cond_signal(&pool->not_empty);
  pthread_mutex_unlock(&pool->queue_lock);
}

/**
 * @brief Action of a worker thread, to repeatedly
 * try and dequeue then carry out a task
 * 
 * @param pool 
 */
void execute_task(struct thread_pool *pool) {
  /* Mutual exclusion on pool queue */
  pthread_mutex_lock(&pool->queue_lock);

  /* Wait until queue has tasks */
  while (pool->num_tasks == 0 && !pool->joining) {
    pthread_cond_wait(&pool->not_empty, &pool->queue_lock);
  }

  /* Exit all worker threads when pool is joined and all tasks completed */
  if (pool->joining && pool->num_tasks == 0) {
    pthread_cond_signal(&pool->all_completed);
    pthread_mutex_unlock(&pool->queue_lock);
    pthread_exit(NULL);
  }

  task_t *task = (task_t *) dequeue(&pool->tasks);
  pthread_mutex_lock(&pool->active_lock);
  pool->num_active++;
  pthread_mutex_unlock(&pool->active_lock);
  pool->num_tasks--;

  /* Wake up threads waiting on space to add task */
  pthread_cond_signal(&pool->not_full);
  pthread_mutex_unlock(&pool->queue_lock);

  /* Execute task */
  task->task(task->args);
  free(task);

  pthread_mutex_lock(&pool->active_lock);
  pool->num_active--;
  pthread_mutex_unlock(&pool->active_lock);
}

void pool_join(struct thread_pool *pool) {
  pthread_mutex_lock(&pool->queue_lock);

  /* Signal worker threads to be join when tasks completed */
  pool->joining = true;
  /* Make worker threads retry condition to wait on not-empty */
  pthread_cond_broadcast(&pool->not_empty);

  /* Wait for all active tasks to finish */
  while (pool->num_tasks == 0) {
    pthread_cond_wait(&pool->all_completed, &pool->queue_lock);
  }

  pthread_mutex_unlock(&pool->queue_lock);

  /* Join all workers */
  for (int i = 0; i < pool->num_workers; i++) {
    pthread_join(pool->workers[i], NULL);
  }  
}

void pool_destroy(struct thread_pool *pool, bool join) {
  if (join) {
    pool_join(pool);
  }
  /* TODO: Clean up memory footprint of queue */
  /* Need a node destructor */
  free(pool->workers);
  pthread_mutex_destroy(&pool->queue_lock);
  pthread_mutex_destroy(&pool->active_lock);
  pthread_cond_destroy(&pool->not_empty);
  pthread_cond_destroy(&pool->not_full);
  pthread_cond_destroy(&pool->all_completed);
}

void *worker(void *args) {
  struct thread_pool *pool = (struct thread_pool*) args;
  for (;;) {
    execute_task(pool);
  }
  /* TODO: Check exit logic */
  pthread_exit(NULL);
}

int pool_num_active(struct thread_pool *pool) {
  /* TODO: Check if active needs a lock */
  pthread_mutex_lock(&pool->active_lock);
  int active = pool->num_active;
  pthread_mutex_unlock(&pool->active_lock);
  return active;
}

int  pool_num_inactive(struct thread_pool *pool) {
  return pool->num_workers - pool_num_active(pool);
}




