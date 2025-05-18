#include "exception/exception.h"
#include "exception/syscall.h"
#include "timer/timer.h"
#include "allocator/dynamic_allocator.h"
#include "template/list.h"
#include "mini_uart.h"
#include "str_utils.h"
#include "utils.h"

static void enableDAIF() { asm volatile("msr DAIFClr, 0xf" :::); }
static void disableDAIF() { asm volatile("msr DAIFSet, 0xf" :::); }

typedef enum {
    // lower
    DEFAULT,
    UART,
    TIMER
    // higher
}IrqPriority;

#define EXCEPT_TASK_SIZE (20 + LISTNODE_SIZE)
typedef struct ExceptTask{
    struct ExceptTask *next;
    void (*handler)();
    ListNode node;
    IrqPriority priority;
}ExceptTask;

#define EXCEPT_QUEUE_SIZE (16 + LISTNODE_SIZE)
typedef struct ExceptQueue{
    struct ExceptQueue *preemptor;
    ExceptTask *head;
    ListNode node;
}ExceptQueue;

// ----- local obj pools-----
#define TASK_POOL_LEN 512
static ExceptTask task_pool[TASK_POOL_LEN];
static ExceptTask *task_free_list = NULL;

#define QUEUE_POOL_LEN 16
static ExceptQueue queue_pool[QUEUE_POOL_LEN];
static ExceptQueue *queue_free_list;

static ExceptWorkload curr_workload;
// ----- Declare Local Functions -----
static ExceptQueue *enqueue_task(ExceptTask *task);
static void exec_queue(ExceptQueue *queue);

static void init_except_task(ExceptTask *task);
static void init_except_queue(ExceptQueue *que);

static ExceptTask *query_task();
static ExceptQueue *query_queue();
static void free_task(ExceptTask *done);
static void free_queue(ExceptQueue *empty);
// ----- Defining Public Functions -----
void init_exception(){
    asm volatile(
        "msr vbar_el1, %[table_addr]\n"
        "msr DAIFClr, 0xf\n"
        :
        : [table_addr]"r"(&_exception_vector_table)
        : "memory"
    );

    for(int i = TASK_POOL_LEN - 1; i >= 0; i--){
        init_except_task(task_pool + i);
        
        ListNode *next_node = (task_free_list)? &task_free_list->node: NULL;
        list_add(&(task_pool + i)->node, NULL, next_node);
        task_free_list = task_pool + i;
    }

    for(int i = QUEUE_POOL_LEN - 1; i >= 0; i--){
        init_except_queue(queue_pool + i);

        ListNode *next_node = (queue_free_list)? &queue_free_list->node: NULL; 
        list_add(&(queue_pool + i)->node, NULL, next_node);
        queue_free_list = queue_pool + i;
    }

    init_workload(&curr_workload);
    swap_event_workload(&curr_workload);

    init_syscalls();
}

/// @brief Handle and routing all irq from EL0 and EL1
/// @note will disable recieved interrupt's signal until being handled, keeps it from recurring
void irq_handler(){
    uint32_t source = *CORE0_INTERRUPT_SOURCE;
    ExceptTask* new_task = query_task();
    if(!new_task) {
        _send_line_("[ERROR][except handler]: can't find avail ExceptTask", sync_send_data);
        return;
    }
    
    if(source & (0x01u << 1)){
        config_core_timer(false);
        new_task->handler = timer_interrupt_handler;
        new_task->priority = TIMER;
    }
    else if(source & (0x01u << 8)){
        disable_aux_interrupt();

        new_task->handler = uart_except_handler;
        new_task->priority = UART;
    }
    else{
        _send_line_("[unknown irq interrupt]", sync_send_data);
        return;
    }

    ExceptQueue *new_queue = enqueue_task(new_task);
    if(new_queue != NULL){
        exec_queue(new_queue);
    }
}

void lower_sync_handler(void *stack_frame){
    uint64_t *register_state = (uint64_t *)stack_frame;
    uint64_t EC = (register_state[ESR_IDX] >> 26) & (0b111111);
    if(EC == SVC) invoke_syscall(register_state);
    else print_el_message(register_state[SPSR_IDX], register_state[ELR_IDX], register_state[ESR_IDX]);
}


void init_workload(ExceptWorkload *workload){
    workload->base_queue = NULL;
    workload->timer_mask = false;
    workload->uart_mask = false;
}

void free_workload(ExceptWorkload *workload){
    //! workload is allocated using dyna_alloc in init_thread(),
    //! causing except system now depends on memory allocation system.
    if(!workload->base_queue){
        free_queue(workload->base_queue); //? does queue empty and no preemptor_queue?
    }
    dyna_free(workload);
}

ExceptWorkload *get_curr_workload(){
    return &curr_workload;
}

void swap_event_workload(ExceptWorkload *new_workload){
    curr_workload.base_queue = new_workload->base_queue;
    config_core_timer(!new_workload->timer_mask);
    if(new_workload->uart_mask) disable_aux_interrupt();
    else enable_aux_interrupt();
}

void print_el_message(uint32_t spsr_el1, uint64_t elr_el1, uint64_t esr_el1){
    char reg_content[20];
    _send_line_("\n!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!", sync_send_data);
    _send_string_("spsr_el1: ", sync_send_data);
    _send_line_(itoa(spsr_el1, reg_content, HEX), sync_send_data);
    
    _send_string_("elr_el1: ", sync_send_data);
    _send_line_(itoa(elr_el1, reg_content, HEX), sync_send_data);
    
    _send_string_("esr_el1: ", sync_send_data);
    _send_line_(itoa(esr_el1, reg_content, HEX), sync_send_data);
    while(true){}
    return;
}

// ----- local functions -----

/** 
 * enqueue handler's execution order according to its priority.
 *
 * @param task, the newly encounted interrupt/exception
 * @return address of newly created queue, NULL in no queue created
 **/ 
static ExceptQueue *enqueue_task(ExceptTask *task){
    ExceptQueue *que_iter = curr_workload.base_queue;
    ExceptQueue *prev_queue= NULL;
    while(que_iter && task->priority > que_iter->head->priority){
        prev_queue = que_iter;
        que_iter = que_iter->preemptor;
    }

    // need to create new queue
    if(!que_iter){
        ExceptQueue *new_queue = query_queue();
        new_queue->head = task;
        
        if(!curr_workload.base_queue) {
            curr_workload.base_queue = new_queue;   
        }
        if(prev_queue) prev_queue->preemptor = new_queue;
        return new_queue;
    }

    // else: insert in existed queue
    ExceptTask *prev_task = que_iter->head;
    while(prev_task-> next && prev_task->next->priority >= task->priority){
        prev_task = prev_task->next;
    }
    task->next = prev_task->next;
    prev_task->next = task;
    return NULL;
}

/** 
 * Execute all handler in a queue while interrupted enabled.
 *
 * @param queue queue to be executed
 **/
static void exec_queue(ExceptQueue *queue){
    while(queue->head){
        enableDAIF();
        queue->head->handler();
        disableDAIF();
        
        ExceptTask *done = queue->head;
        queue->head = queue->head->next;
        free_task(done);
    }

    free_queue(queue);
    if(curr_workload.base_queue == queue) curr_workload.base_queue = NULL;
}

static void init_except_task(ExceptTask *task){
    task->handler = NULL;
    task->next = NULL;
    node_init(&task->node);
    task->priority = DEFAULT;
}

static void init_except_queue(ExceptQueue *que){
    que->head = NULL;
    que->preemptor = NULL;
    node_init(&que->node);
}

static ExceptTask *query_task(){
    if(!task_free_list) return NULL;

    ExceptTask *avail = task_free_list;
    task_free_list = (avail->node.next)? GET_CONTAINER(avail->node.next, ExceptTask, node): NULL;
    list_remove(&avail->node);
    return avail;
}

static ExceptQueue *query_queue(){
    if(!queue_free_list) return NULL;
    
    ExceptQueue *avail = queue_free_list;
    queue_free_list = (avail->node.next)? GET_CONTAINER(avail->node.next, ExceptQueue, node): NULL;
    list_remove(&avail->node);
    return avail;
}

static void free_task(ExceptTask *done){
    init_except_task(done);
    list_add(&done->node, NULL, &task_free_list->node);
    task_free_list = done;
}

static void free_queue(ExceptQueue *empty){
    init_except_queue(empty);
    list_add(&empty->node, NULL, &queue_free_list->node);
    queue_free_list = empty;
}