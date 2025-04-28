#ifndef QUEUE_H
#define QUEUE_H

#include "template/list.h"
#include "basic_type.h"

typedef struct{
    ListNode *front;
    ListNode *back;
    size_t size;
}Queue;

void queue_init(Queue *que);

void queue_push(Queue *que, ListNode *new);
ListNode* queue_pop(Queue *que);

size_t get_queue_size(Queue *que);
bool queue_empty(Queue *que);

// void free_queue(Queue *que);

#endif