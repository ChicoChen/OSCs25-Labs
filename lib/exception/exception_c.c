#include "exception/exception.h"
#include "allocator/simple_alloc.h"
#include "timer/timer.h"
#include "mini_uart.h"
#include "str_utils.h"

#define EXCEPT_TASK_SIZE 24
#define EXCEPT_QUEUE_SIZE 16

typedef enum {
    // lower
        UART,
        TIMER
    // higher
}IrqPriority;

// TODO: a standard link-list template
typedef struct ExceptTask{
    struct ExceptTask *next;
    void (*handler)();
    IrqPriority priority;
}ExceptTask;

typedef struct ExceptQueue{
    struct ExceptQueue *preemptor;
    // struct ExceptQueue *preemptee;
    ExceptTask *head;
}ExceptQueue;

static ExceptQueue* base_queue;

static void init_except_task(ExceptTask *task, void (*handler)(), IrqPriority priority);
static void init_except_queue(ExceptQueue *que, ExceptTask *task);

static ExceptQueue *enqueue_task(ExceptTask *task);
static void exec_queue(ExceptQueue *queue);

static void enableDAIF();
static void disableDAIF();

void init_exception(){
    base_queue = NULL;
    asm volatile(
        "msr vbar_el1, %[table_addr]\n"
        "msr DAIFClr, 0xf\n"
        :
        : [table_addr]"r"(&_exception_vector_table)
        : "memory"
    );
}

void curr_irq_handler(){
    uint32_t source = *CORE0_INTERRUPT_SOURCE;
    ExceptTask* new_task = (ExceptTask *)simple_alloc(EXCEPT_TASK_SIZE);
    
    if(source & 0x01u << 1){
        enable_core_timer(false);
        init_except_task(new_task, timer_interrupt_handler, TIMER);
    }
    else if(source & 0x01u << 8){
        disable_aux_interrupt();
        init_except_task(new_task, uart_except_handler, UART);
    }
    else{
        _send_line_("[unknown irq interrupt]", sync_send_data);
    }

    ExceptQueue *new_queue = enqueue_task(new_task);
    if(new_queue != NULL){
        exec_queue(new_queue);
    }
}

void print_el_message(uint32_t spsr_el1, uint64_t elr_el1, uint64_t esr_el1){
    char reg_content[20];
    _send_string_("spsr_el1: ", sync_send_data);
    _send_line_(itoa(spsr_el1, reg_content, HEX), sync_send_data);
    
    _send_string_("elr_el1: ", sync_send_data);
    _send_line_(itoa(elr_el1, reg_content, HEX), sync_send_data);
    
    _send_string_("esr_el1: ", sync_send_data);
    _send_line_(itoa(esr_el1, reg_content, HEX), sync_send_data);
    return;
}

static ExceptQueue *enqueue_task(ExceptTask *task){
    ExceptQueue *que_iter = base_queue;
    ExceptQueue *prev_queue= NULL;
    while(que_iter && task->priority > que_iter->head->priority){
        prev_queue = que_iter;
        que_iter = que_iter->preemptor;
    }

    // need to create new queue
    if(!que_iter){
        ExceptQueue *new_queue = (ExceptQueue *)simple_alloc(EXCEPT_QUEUE_SIZE);
        init_except_queue(new_queue, task);
        if(!base_queue) base_queue = new_queue;
        if(prev_queue) prev_queue->preemptor = new_queue;
        // new_queue->preemptee = prev_queue;
        return new_queue;
    }

    // insert in existed queue
    ExceptTask *task_iter = que_iter->head;
    while(task_iter-> next && task_iter->next->priority >= task->priority){
        task_iter = task_iter->next;
    }
    task->next = task_iter->next;
    task_iter->next = task;
    return NULL;
}

static void exec_queue(ExceptQueue *queue){
    enableDAIF();
    while(queue->head){
        queue->head->handler();
        queue->head = queue->head->next;
    }

    if(base_queue == queue) base_queue = NULL;
    disableDAIF();
}

static void init_except_task(ExceptTask *task, void (*handler)(), IrqPriority priority){
    task->handler = handler;
    task->priority = priority;
    task->next = NULL;
}

static void init_except_queue(ExceptQueue *que, ExceptTask *task){
    que->head = task;
    que->preemptor = NULL;
    // que->preemptee = NULL;
}

static void enableDAIF(){
    asm volatile("msr DAIFClr, 0xf" :::);
}

static void disableDAIF(){
    asm volatile("msr DAIFSet, 0xf" :::);
}