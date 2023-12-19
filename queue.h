#ifndef QUEUE_H
#define QUEUE_H

#include <stdbool.h>

struct queue {
  struct node *head;
  struct node *tail;
};

void init_queue(struct queue *q);
bool enqueue(struct queue *q, void *e);
void *dequeue(struct queue *q);
bool empty_queue(struct queue *q);

#endif /* Queue.h */