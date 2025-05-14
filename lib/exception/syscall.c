#include "exception/syscall.h"
#include "exception/exception.h"
#include "thread/thread.h"
#include "allocator/rc_region.h"
#include "file_sys/initramfs.h"
#include "mailbox/mailbox.h"
#include "memory_region.h"
#include "mini_uart.h"
#include "str_utils.h"
#include "utils.h"

#define NUM_SYSCALL 292

#define SET_RETURN_VALUE(result) stack_frame[0] = (uint64_t)result;
#define ENABLE_DAIF asm volatile("msr DAIFClr, 0xf")
#define DISABLE_DAIF asm volatile("msr DAIFSet, 0xf")

typedef void (*SyscallHandler)(uint64_t *);
SyscallHandler handlers[NUM_SYSCALL];

// ----- forward decls -----

void register_all_handlers();
void register_syscall(size_t idx, SyscallHandler handler);

void getpid_s         (uint64_t *stack_frame);
void uart_read_s      (uint64_t *stack_frame);
void uart_write_s     (uint64_t *stack_frame);
void exec_s           (uint64_t *stack_frame);
void fork_s           (uint64_t *stack_frame);
void exit_s           (uint64_t *stack_frame);
void mbox_call_s      (uint64_t *stack_frame);
void kill_s             (uint64_t *stack_frame);

extern void _fork_entry; // delared in exception_s.S 
// ----- public interface -----

void init_syscalls(){
    for(size_t i = 0; i < NUM_SYSCALL; i++) handlers[i] = NULL;
    register_all_handlers();
}

void invoke_syscall(uint64_t *stack_frame){
    ENABLE_DAIF;
    uint64_t syscall = stack_frame[SYSCALL_IDX];
    handlers[syscall](stack_frame);
    DISABLE_DAIF;
}

// ----- local functions -----

void register_all_handlers(){
    register_syscall(0, getpid_s);
    register_syscall(1, uart_read_s);
    register_syscall(2, uart_write_s);
    register_syscall(3, exec_s);
    register_syscall(4, fork_s);
    register_syscall(5, exit_s);
    register_syscall(6, mbox_call_s);
    register_syscall(7, kill_s);
}

void register_syscall(size_t idx, SyscallHandler handler) {
    handlers[idx] = handler;
}

void getpid_s(uint64_t *stack_frame){
    SET_RETURN_VALUE(get_curr_thread()->id);
}

void uart_read_s(uint64_t *stack_frame){
    // ENABLE_DAIF;
    char *buf = (char *)stack_frame[0];
    size_t size = (size_t) stack_frame[1];
    SET_RETURN_VALUE(read_to_buf(buf, size));
    // DISABLE_DAIF;
}

void uart_write_s(uint64_t *stack_frame){
    // ENABLE_DAIF;
    char *buf = (char *)stack_frame[0];
    size_t size = (size_t) stack_frame[1];
    SET_RETURN_VALUE(send_from_buf(buf, size));
    // DISABLE_DAIF;
}

/// @brief syscall to execute a program on current thread.
/// @param name name of program.
/// @param args needed arguments.
void exec_s(uint64_t *stack_frame){
    char *name = (char *)stack_frame[0];
    char **argv = (char **)stack_frame[1];
    
    //clear old program section
    Thread *curr_thread = get_curr_thread();
    rc_free(curr_thread->prog);

    // allocate new stack and prog
    RCregion *new_prog = load_program(name);
    stack_frame[ELR_IDX] = (uint64_t)new_prog->mem;
    asm volatile(
        "msr sp_el0, %[user_sp]"
        :
        : [user_sp]"r"(curr_thread->thread_state[STATE_USER_SP])
        :
    );
}

void fork_s(uint64_t *stack_frame){
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
    uint64_t k_sp_offset = (uint64_t)stack_frame - p_kstack_top;
    uint64_t c_kstack_top = GET_STACK_TOP((uint64_t)child ->thread_state[STATE_KERNEL_SP], KERNEL_STACK_SIZE);
    child->thread_state[STATE_KERNEL_SP] = c_kstack_top + k_sp_offset;
    memcpy((void *)child->thread_state[STATE_KERNEL_SP], stack_frame, KERNEL_STACK_SIZE - k_sp_offset);

    // set after copy, child's return value remains 0
    SET_RETURN_VALUE(child->id);
    add_ref(parent->prog);
    
    // child will start at nextline of lower_sync_handler();
    child->thread_state[STATE_LR] = (uint64_t)&_fork_entry;
}

void exit_s(uint64_t *stack_frame){
    terminate_thread();
}

void mbox_call_s(uint64_t *stack_frame){
    char ch = (char)stack_frame[0];
    unsigned int mbox = (unsigned int)stack_frame[1];
    SET_RETURN_VALUE(mailbox_call(ch, mbox));
}

void kill_s(uint64_t *stack_frame){
    int pid = stack_frame[0];
    kill_thread(pid);
}