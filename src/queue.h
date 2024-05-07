#ifndef QUEUE_H
#define QUEUE_H
#include <stdlib.h>

typedef struct QueueElement {
  void *data;
  struct QueueElement *next;
} QueueElement;

typedef struct Queue {
  QueueElement *front, *back;
  size_t size;
  size_t element_size;
  void (*data_destructor)(void **data);
} Queue;

#define QUEUE_NO_ERROR 0x0
#define QUEUE_PARAMS_ERROR 0x2
#define QUEUE_EMPTY 0x3

int queue_constructor(Queue **const queue, const size_t element_size,
                      void (*data_destructor)(void **data));
int queue_pop_front(Queue *const queue, void **const element);
int queue_push_back(Queue *const queue, void *const element);
int queue_destructor(Queue **const queue);

#endif
