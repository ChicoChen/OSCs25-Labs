### Threading system

In this lab, the term `Thread` and `process` can be used interchangeably.

1. create data structure for thread:

    ```
    Thread
    ├─ Thread ID (uint32)
    ├─ Priority (uint32)
    ├─ Program (reference counted region to place user program's code)
    ├─ Task (function pointer and its arguemnts)
    ├─ saved state (use to restored CPU state upon context switching)
    └─ Queue interface (`ListNode`)
    ```
    - initial state of `saved state`

2. Context switching:

    - `r19 ~ r28` are callee-saved, others is either saved on stack or temporary
    - If we switch `sp`, after function return it will eventually read target `pc` and hence fulfill context switching.

3. ARM's sp and execution level(EL_x):

    aarch64 will automatically switch to `sp_elx` when switching to `EL_x`, hence I use separated `kernel_stack` and `user_stack` for a user program.

4. Forked process:

    Create a copy of current process/thread's user's stack, kernel's stack and program state, and share the same program(.text section) with parent.

    The reason we need reference count on code section in memeroy is that: even if stacks are copied upon forking a new child process, parent process and child process need to share that code region. As long as those return address stored on the stack are not modified.

    I use reference count to track if a code region become abandoned, and reclaim it while reclaiming last related process.

5. Refine exception enqueue and handle mechanism:

    In previous design, exception block enqueued insterrupt to preserve usable task in memory pool. But after context-switch, the timer-interrupt remains in the head of `ExceptQueue`, blocking other interrupt from executing.

    My solution is separate different thread's exception tasks, encapsulate as `ExceptWorkLoad` which track thread's interrupt mask and enqueued but un-handled exceptions.