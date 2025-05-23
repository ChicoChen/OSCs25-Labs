#include "exception/syscall.h"
#include "exception/exception.h"
#include "thread/thread.h"
#include "thread/signals.h"
#include "allocator/rc_region.h"
#include "allocator/page_allocator.h"
#include "file_sys/initramfs.h"
#include "mailbox/mailbox.h"
#include "memory_region.h"
#include "mini_uart.h"

#include "str_utils.h"
#include "utils.h"
#include "timer/timer.h"

#define NUM_SYSCALL 16

#define SET_RETURN_VALUE(result) trap_frame[0] = (uint64_t)result;

typedef void (*SyscallHandler)(uint64_t *);
SyscallHandler handlers[NUM_SYSCALL];

// ----- forward decls -----

void register_syscall_handlers();
void register_syscall(size_t idx, SyscallHandler handler);

void getpid_s         (uint64_t *trap_frame);
void uart_read_s      (uint64_t *trap_frame);
void uart_write_s     (uint64_t *trap_frame);
void exec_s           (uint64_t *trap_frame);
void fork_s           (uint64_t *trap_frame);
void exit_s           (uint64_t *trap_frame);
void mbox_call_s      (uint64_t *trap_frame);
void kill_s           (uint64_t *trap_frame);
void signal_s         (uint64_t *trap_frame);
void kill_with_signal_s(uint64_t *trap_frame);
void sigreturn_s      (uint64_t *trap_frame);

extern void _fork_entry; // delared in exception_s.S 
// ----- public interface -----

void init_syscalls(){
    for(size_t i = 0; i < NUM_SYSCALL; i++) handlers[i] = NULL;
    register_syscall_handlers();
}

void invoke_syscall(uint64_t *trap_frame){
    ENABLE_DAIF;
    uint64_t syscall = trap_frame[SYSCALL_IDX];
    handlers[syscall](trap_frame);    
    DISABLE_DAIF;
}

// ----- local functions -----

void register_syscall_handlers(){
    register_syscall(0, getpid_s);
    register_syscall(1, uart_read_s);
    register_syscall(2, uart_write_s);
    register_syscall(3, exec_s);
    register_syscall(4, fork_s);
    register_syscall(5, exit_s);
    register_syscall(6, mbox_call_s);
    register_syscall(7, kill_s);
    register_syscall(8, signal_s);
    register_syscall(9, kill_with_signal_s);
    register_syscall(10, sigreturn_s);
}

void register_syscall(size_t idx, SyscallHandler handler) {
    handlers[idx] = handler;
}

void getpid_s(uint64_t *trap_frame){
    SET_RETURN_VALUE(get_curr_thread()->id);
}

void uart_read_s(uint64_t *trap_frame){
    char *buf = (char *)trap_frame[0];
    size_t size = (size_t) trap_frame[1];
    SET_RETURN_VALUE(read_to_buf(buf, size));
}

void uart_write_s(uint64_t *trap_frame){
    char *buf = (char *)trap_frame[0];
    size_t size = (size_t) trap_frame[1];
    SET_RETURN_VALUE(send_from_buf(buf, size));
}

/// @brief syscall to execute a program on current thread.
/// @param name name of program.
/// @param args needed arguments.
void exec_s(uint64_t *trap_frame){
    char *name = (char *)trap_frame[0];
    char **argv = (char **)trap_frame[1];
    
    //clear old program section
    Thread *curr_thread = get_curr_thread();
    rc_free(curr_thread->prog);

    // allocate new stack and prog
    RCregion *new_prog = load_program(name);
    trap_frame[ELR_IDX] = (uint64_t)new_prog->mem;
    asm volatile(
        "msr sp_el0, %[user_sp]"
        :
        : [user_sp]"r"(curr_thread->thread_state[STATE_USER_SP])
        :
    );
}

void fork_s(uint64_t *trap_frame){
    config_core_timer(false);
    Thread *parent = get_curr_thread();
    Thread *child = make_thread(parent->task, parent->args, parent->prog);
    SET_RETURN_VALUE(0);
    
    // directly copy parent's user stack to child
    uint64_t p_ustack_top = GET_STACK_TOP((uint64_t)parent->thread_state[STATE_USER_SP], USER_STACK_SIZE);
    uint64_t c_ustack_top = GET_STACK_TOP((uint64_t)child ->thread_state[STATE_USER_SP], USER_STACK_SIZE);
    memcpy((void *)c_ustack_top, (void *)p_ustack_top, USER_STACK_SIZE);
    
    // calculate position of child's user_sp by offset from parent's user_stack
    uint64_t u_sp_offset;
    asm volatile(
        "mrs %[user_sp], sp_el0"
        : [user_sp]"=r"(u_sp_offset)
        ::
    );
    u_sp_offset -= p_ustack_top;
    child->thread_state[STATE_USER_SP] = c_ustack_top + u_sp_offset;

    // copy the kernel stack before enter lower_sync_handler()
    uint64_t p_kstack_top = GET_STACK_TOP((uint64_t)parent->thread_state[STATE_KERNEL_SP], KERNEL_STACK_SIZE);
    uint64_t k_sp_offset = (uint64_t)trap_frame - p_kstack_top;
    uint64_t c_kstack_top = GET_STACK_TOP((uint64_t)child ->thread_state[STATE_KERNEL_SP], KERNEL_STACK_SIZE);
    child->thread_state[STATE_KERNEL_SP] = c_kstack_top + k_sp_offset;
    memcpy((void *)child->thread_state[STATE_KERNEL_SP], trap_frame, KERNEL_STACK_SIZE - k_sp_offset);
    
    // also copy signal handlers
    for(size_t i = 0; i < NUM_SIGNALS; i++){
        child->signal_handlers[i] = parent->signal_handlers[i];
    }

    // set after copy, child's return value remains 0
    SET_RETURN_VALUE(child->id);
    add_ref(parent->prog);
    
    // child will start at nextline of lower_sync_handler();
    child->thread_state[STATE_LR] = (uint64_t)&_fork_entry;
    config_core_timer(true);
}

void exit_s(uint64_t *trap_frame){
    kill_curr_thread();
}

void mbox_call_s(uint64_t *trap_frame){
    char ch = (char)trap_frame[0];
    unsigned int mbox = (unsigned int)trap_frame[1];
    SET_RETURN_VALUE(mailbox_call(ch, mbox));
}

void kill_s(uint64_t *trap_frame){
    int pid = trap_frame[0];
    if(pid == get_curr_thread()->id) {
        kill_curr_thread();
        return;
    }

    Queue *location;
    Thread *found = find_thread(pid, &location);
    if(found) kill_thread(found, location);
}

void signal_s(uint64_t *trap_frame){
    int signal = (int)trap_frame[0];
    SignalHandler handler_func = (SignalHandler)trap_frame[1];

    curr_thread_regis_signal(signal, handler_func);
}

void kill_with_signal_s(uint64_t *trap_frame){
    int pid = trap_frame[0];
    int signal = trap_frame[1];

    Queue *location;
    Thread *found = find_thread(pid, &location);
    if(found)found->last_signal = signal;
}

void sigreturn_s(uint64_t *trap_frame){
    DISABLE_DAIF;
    uint64_t temp_stack;
    asm volatile("mrs %[user_sp], sp_el0": [user_sp]"=r"(temp_stack)::);
    temp_stack = GET_STACK_TOP(temp_stack, SIGNAL_HANDLER_STACK_SIZE);

    uint64_t *stack_bottom = (uint64_t *)(GET_STACK_BOTTOM(temp_stack, SIGNAL_HANDLER_STACK_SIZE));
    asm volatile("msr sp_el0, %[original_sp]":: [original_sp]"r"(*stack_bottom):);
    
    stack_bottom -= TRAP_FRAME_NUM_ELEMENTS;
    memcpy((void *)trap_frame, (void *)stack_bottom, TRAP_FRAME_SIZE);
    
    page_free((void *)temp_stack);
    ENABLE_DAIF;
}