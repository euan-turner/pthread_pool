#include <stdio.h>
#include <stdlib.h>

#include "queue.h"

struct node {
  void *elem;
  struct node *next;
};

void init_queue(struct queue *q) {
  q->head = NULL;
  q->tail = NULL;
}

bool enqueue(struct queue *q, void *e) {
  struct node *n = malloc(sizeof(struct node));
  if (n == NULL) {
    return false;
  }
  n->elem = e;
  n->next = NULL;
  if (!empty_queue(q)) {
    q->tail->next = n;
    q->tail = n;
  } else {
    q->head = n;
    q->tail = n;
  }
  return true;
}

void *dequeue(struct queue *q) {
  if (empty_queue(q)) {
    return NULL;
  }
  struct node *n = q->head;
  q->head = n->next;
  if (q->head == NULL) {
    q->tail = NULL;
  }
  void *e = n->elem;
  free(n);
  return e;
}

bool empty_queue(struct queue *q) {
  return q->head == NULL;
}