#ifndef LINKED_LIST_H
#define LINKED_LIST_H

#define GET_OFFSET(type, member) ((uint64_t) &((type *)0)->member)
#define GET_CONTAINER(ptr, type, member) ((type *)((addr_t)ptr - GET_OFFSET(type, member)))

#define LISTNODE_SIZE 16
typedef struct ListNode{
    struct ListNode *prev;
    struct ListNode *next;
} ListNode;

void list_init(ListNode *node);
void list_add(ListNode* new, ListNode* prev, ListNode* next);
ListNode *list_remove(ListNode* target);

#endif