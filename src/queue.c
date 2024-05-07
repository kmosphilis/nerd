#include "queue.h"

/**
 * @brief Constructs a dynamic Queue.
 *
 * @param queue A pointer to a Queue * to save the new Queue *.
 * @param element_size The size of the future elements to be stored in the
 * Queue.
 * @param data_destructor (Optional) A function that requires a pointer to the
 * void *, that deallocates the element.
 *
 * @return QUEUE_NO_ERROR if no error occurs, or QUEUE_PARAMS_ERROR if queue is
 * NULL.
 */
int queue_constructor(Queue **const queue, const size_t element_size,
                      void (*data_destructor)(void **data)) {
  if (!queue)
    return QUEUE_PARAMS_ERROR;

  *queue = (Queue *)malloc(sizeof(Queue));
  (*queue)->element_size = element_size;
  (*queue)->size = 0;
  (*queue)->front = NULL;
  (*queue)->back = NULL;
  (*queue)->data_destructor = data_destructor;

  return QUEUE_NO_ERROR;
}

/**
 * @brief Dequeues, pops the front element of the Queue.
 *
 * @param queue The Queue to remove the front element.
 * @param element A pointer to a void *, to save the popped element. If NULL is
 * given, the element will be destructed by the given data_destructor function.
 *
 * @return QUEUE_NO_ERROR if no error occurs, QUEUE_PARAMS_ERROR if queue is
 * NULL, or QUEUE_EMPTY if queue is empty.
 */
int queue_pop_front(Queue *const queue, void **const element) {
  if (!queue)
    return QUEUE_PARAMS_ERROR;

  if (!queue->front)
    return QUEUE_EMPTY;

  QueueElement *current_element = queue->front;
  if (queue->front == queue->back)
    queue->back = NULL;
  queue->front = current_element->next;
  if (element)
    *element = current_element->data;
  else if (queue->data_destructor)
    queue->data_destructor(&current_element->data);
  free(current_element);
  --queue->size;

  return QUEUE_NO_ERROR;
}

/**
 * @brief Destructs a dynamic Queue by emptying and deallocating it.
 *
 * @param queue A pointer to a Queue *.
 *
 * @return QUEUE_NO_ERROR if no error occurs, or QUEUE_PARAMS_ERROR if queue, or
 * the Queue * is NULL.
 */
int queue_destructor(Queue **const queue) {
  if (!queue || !*queue)
    return QUEUE_PARAMS_ERROR;
  while ((*queue)->front != NULL) {
    queue_pop_front(*queue, NULL);
  }
  free(*queue);
  *queue = NULL;

  return QUEUE_NO_ERROR;
}

/**
 * @brief Enqueues, pushes a new element at the back of the Queue.
 *
 * @param queue The Queue to add the element.
 * @param element The element to be added.
 *
 * @return QUEUE_NO_ERROR if no error occurs, or QUEUE_PARAMS_ERROR if queue is
 * NULL.
 */
int queue_push_back(Queue *const queue, void *const element) {
  if (!queue)
    return QUEUE_PARAMS_ERROR;
  ++queue->size;
  if (!queue->back) {
    queue->back = (QueueElement *)malloc(sizeof(QueueElement));
  } else {
    queue->back->next = (QueueElement *)malloc(sizeof(QueueElement));
    queue->back = queue->back->next;
  }

  queue->back->data = element;
  queue->back->next = NULL;

  if (!queue->front)
    queue->front = queue->back;

  return QUEUE_NO_ERROR;
}
