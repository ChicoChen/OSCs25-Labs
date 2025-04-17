#include "template/list.h"
#include "basic_type.h"

void list_init(ListNode *node){
    node->next = NULL;
    node->prev = NULL;
}

void *list_add(ListNode* new, ListNode* prev, ListNode* next){
    new->next = next;
    new->prev = prev;

    if(prev) prev->next = new;
    if(next) next->prev = new;
}

ListNode *list_remove(ListNode* target){
    if(!target) return NULL;
    if(target->prev) target->prev->next = target->next;
    if(target->next) target->next->prev = target->prev;
    return target->next;
}