#include "thread/thread.h"
#include "allocator/dynamic_allocator.h"
#include "template/queue.h"
#include "memory_region.h"
#include "basic_type.h"

#define THREAD_SIZE (24 + LISTNODE_SIZE)
typedef struct{
    uint32_t id;
    Task task;
    void *args;
    uint32_t priority;
    ListNode node;
}Thread;

static Queue run_queue;
static Queue wait_queue;
static Thread *idle_thread = NULL;
static uint32_t total_thread = 0;

// ----- forward decls -----

// ----- public interface -----

void init_thread_sys(){
    queue_init(&run_queue);
    queue_init(&wait_queue);
    // make idle thread and assign id 0
}

void *make_thread(Task assigned_func, void *args, uint32_t priority){
    Thread *new_thread = (Thread *)kmalloc(THREAD_SIZE);
    
    new_thread->id = total_thread++;
    new_thread->task = assigned_func;
    new_thread->args = args;
    new_thread->priority = priority;
    
    // insert into queue
    list_init(&new_thread->node);
    queue_push(&run_queue, &new_thread->node);

    return new_thread;
}

void schedule(){
    // save context of current thread

    ListNode* preemptee = queue_pop(&run_queue);
    queue_push(&run_queue, preemptee);

    // switch to execute new thread
    // reset timer interrput again
}

void preempt_callback(void *args){
    schedule();
}

// ----- local functions -----

void idle(void *args){
    while(true){
        kill_zombies();
        schedule();
    }
}