### Threading system
1. create data structure for thread
    ```
    Thread
    ├─ Thread ID (uint32)
    ├─ Priority (uint32)
    ├─ Task (function pointer and its arguemnts)
    ├─ saved state (use to restored CPU state upon context switching)
    └─ Queue interface (`ListNode`)
    ```
    - Then what's the initial state of `saved state`

2. Context switching
    - `r19 ~ r28` are callee-saved, others is either saved on stack or temporary
    - If we switch `sp`, after function return it will eventually read target `pc` and hence fulfill context switching.