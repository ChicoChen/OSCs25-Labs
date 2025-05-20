#include "thread/signals.h"
#include "allocator/page_allocator.h"
#include "thread/thread.h"
#include "utils.h"

void exit_signal_handler();

SignalHandler default_signal_handlers[NUM_SIGNALS] = {
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        kill_curr_thread
    };

// void init_sigal_handler(SignalHandler *handler, HandlerFunc func){
//     handler->registed_func = func;
//     handler->temp_sp = NULL;
// }

void handle_signal(uint64_t *trap_frame){
    Thread *curr_thread = get_curr_thread();
    int signal = curr_thread->last_signal;
    curr_thread->last_signal = NUM_SIGNALS; // prevent current execution get preempted by same handler

    // no registered function, use default
    if(curr_thread->signal_handlers[signal] == NULL){
        default_signal_handlers[signal]();
        return;
    }

    // otherwise, use registered handler
    SignalHandler handler = curr_thread->signal_handlers[signal];
    // assign temp_user_stack
    uint64_t origin_user_sp;
    asm volatile("mrs %[user_sp], sp_el0": [user_sp]"=r"(origin_user_sp)::);
    uint64_t *temp_stack = (uint64_t *)(GET_STACK_BOTTOM((uint64_t)page_alloc(SIGNAL_HANDLER_STACK_SIZE), SIGNAL_HANDLER_STACK_SIZE));
    
    // save trap_frame and original user_sp for furture restore
    *temp_stack = origin_user_sp;
    temp_stack -= TRAP_FRAME_NUM_ELEMENTS;
    temp_stack = (uint64_t *)memcpy((void *)temp_stack, (void *)trap_frame, TRAP_FRAME_SIZE);
    asm volatile("msr sp_el0, %[new_user_sp]":: [new_user_sp]"r"(temp_stack):);

    trap_frame[LR_IDX] = (uint64_t)exit_signal_handler; // configure lr

    // modify to elr_el1 on stack, jump to handler
    trap_frame[ELR_IDX] = (uint64_t)handler;
    trap_frame[SPSR_IDX] = 0x340;
}

/// @brief leaving block for registered signal handler, runs in EL0
void exit_signal_handler(){
    asm volatile(
        "mov x8, #10\n\t"
        "svc #0\n\t"
    );
}