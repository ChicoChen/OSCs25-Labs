#ifndef LINKED_LIST_H
#define LINKED_LIST_H

#define LISTNODE_SIZE 16
typedef struct ListNode
{
    struct ListNode *prev;
    struct ListNode *next;
} ListNode;

void list_init(ListNode *node);
void list_add(ListNode* new, ListNode* prev, ListNode* next);
ListNode *list_remove(ListNode* target);

#endif