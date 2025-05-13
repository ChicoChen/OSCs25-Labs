#include "exception/syscall.h"
#include "exception/exception.h"
#include "thread/thread.h"
#include "allocator/page_allocator.h"
#include "file_sys/initramfs.h"
#include "mini_uart.h"
#include "str_utils.h"

#define NUM_SYSCALL 292

#define SYS_RETURN(result) stack_frame[0] = (uint64_t)result;
#define ENABLE_DAIF asm volatile("msr DAIFClr, 0xf")
#define DISABLE_DAIF asm volatile("msr DAIFSet, 0xf")

typedef void (*SyscallHandler)(uint64_t *);
SyscallHandler handlers[NUM_SYSCALL];

// ----- forward decls -----

void register_all_handlers();
void register_syscall(size_t idx, SyscallHandler handler);

void getpid         (uint64_t *stack_frame);
void uart_read      (uint64_t *stack_frame);
void uart_write     (uint64_t *stack_frame);
void exec           (uint64_t *stack_frame);

// ----- public interface -----

void init_syscalls(){
    for(size_t i = 0; i < NUM_SYSCALL; i++) handlers[i] = NULL;
    register_all_handlers();
}

void invoke_syscall(uint64_t *stack_frame){
    uint64_t syscall = stack_frame[SYSCALL_IDX];
    handlers[syscall](stack_frame);
}

// ----- local functions -----

void register_all_handlers(){
    register_syscall(0, getpid);
    register_syscall(1, uart_read);
    register_syscall(2, uart_write);
    register_syscall(3, exec);
}

void register_syscall(size_t idx, SyscallHandler handler) {
    handlers[idx] = handler;
}

void getpid(uint64_t *stack_frame){
    SYS_RETURN(get_curr_thread()->id);
}

void uart_read(uint64_t *stack_frame){
    ENABLE_DAIF;
    char *buf = (char *)stack_frame[0];
    size_t size = (size_t) stack_frame[1];
    SYS_RETURN(read_to_buf(buf, size));
    DISABLE_DAIF;
}

void uart_write(uint64_t *stack_frame){
    ENABLE_DAIF;
    char *buf = (char *)stack_frame[0];
    size_t size = (size_t) stack_frame[1];
    SYS_RETURN(send_from_buf(buf, size));
    DISABLE_DAIF;
}

/// @brief syscall to execute a program on current thread.
/// @param name name of program.
/// @param args needed arguments.
void exec(uint64_t *stack_frame){
    char *name = (char *)stack_frame[0];
    char **argv = (char **)stack_frame[1];
    
    //clear old program section
    Thread *curr_thread = get_curr_thread();
    page_free(curr_thread->prog);

    // allocate new stack and prog
    void *new_dest = load_program(name);
    stack_frame[ELR_IDX] = (uint64_t)new_dest;
    asm volatile(
        "msr sp_el0, %[user_sp]"
        :
        : [user_sp]"r"(curr_thread->thread_state[STATE_USER_SP])
        :
    );
    return;
}