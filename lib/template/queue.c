#include "template/queue.h"

void queue_init(Queue *que){
    que->front = NULL;
    que->back = NULL;
    que->size = 0;
}

void queue_push(Queue *que, ListNode *new){
    if(queue_empty(que)){
        que->back = new;
        que->front = new;
    }
    else{
        list_add(new, que->back, NULL);
        que->back = new;
    }

    que->size++;
}

ListNode* queue_pop(Queue *que){
    if(queue_empty(que)) return NULL;

    ListNode *result = que->front;
    que->front = list_remove(result);
    que->size--;
    if(que->size == 0) que->back = NULL;
    return result;
}

void queue_erase(Queue *que, ListNode *target){
    if(!target || !que) return;

    if(target == que->front){
        queue_pop(que);
        return;
    }

    if(target == que->back){
        que->back = target->prev;
    }
    list_remove(target);
    que->size--;
}

ListNode *queue_head(Queue *que){
    return que->front;
}

size_t get_queue_size(Queue *que){
    return que->size;
}

bool queue_empty(Queue *que){
    return BOOL(que->size == 0);
}