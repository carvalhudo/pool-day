#include <pthread.h>
#include <stdlib.h>

#include "queue.h"

#define THREAD_SAFE_ZONE(mutex, code)  \
  pthread_mutex_lock(mutex); \
  code \
  pthread_mutex_unlock(mutex);

struct task_queue_t {
  task_t *tail;
  task_t *head;
  pthread_mutex_t mutex;
};

int queue_size(task_queue_t *queue) {
  int size = 0;

  THREAD_SAFE_ZONE(&queue->mutex, {
    for_each_task(curr, queue) {
      size++;
    }
  })

  return size;
}

task_t *create_task(void (*task)(void *), void *param) {
  task_t *node;

  node = (task_t *)malloc(sizeof(task_t));
  if (node) {
    node->next = node->prev = NULL;
    node->task = task;
    node->param = param;
  }

  return node;
}

void enqueue(task_queue_t *queue, task_t *elem) {
  THREAD_SAFE_ZONE(&queue->mutex, {
    // if the queue is empty
    if (!queue->head && !queue->tail) {
      queue->tail = queue->head = elem;
    } else {
      queue->tail->prev = elem;
      elem->next = queue->tail;
      queue->tail = elem;
    }
  })
}

task_t *dequeue(task_queue_t *queue) {
  task_t *to_del;

  THREAD_SAFE_ZONE(&queue->mutex, {
    to_del = queue->head;

    // if the list is not empty
    if (to_del) {
      queue->head = to_del->prev;
      // if the queue has more than one element
      if (to_del->prev) {
        to_del->prev->next = NULL;
        to_del->prev = NULL;
      } else {
        queue->head = queue->tail = NULL;
      }
    }
  })

  return to_del;
}

void init_queue(task_queue_t **queue) {
  *queue = malloc(sizeof(task_queue_t));
  if (*queue) {
    (*queue)->head = (*queue)->tail = NULL;
  }

  pthread_mutex_init(&(*queue)->mutex, NULL);
}

void destroy_queue(task_queue_t *queue) {
  for_each_task_safe(curr, queue) {
    task_t *node = dequeue(queue);

    free(node);
  }

  pthread_mutex_destroy(&queue->mutex);
  free(queue);
}
