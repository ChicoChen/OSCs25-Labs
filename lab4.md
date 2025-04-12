# Lab 4 - Allocator
### Basic concept:

    [low-level --> high level]
    start-up allocator(1) -> page frame allocator(2) -> dynamic allocator(3)

1. start-up allocator
    - Allocate page frame array needed for (2)
    - Reserve memory region for other system component
2. page frame allocator
    - Implemented using Buddy system
    - page-based (4KB) allocation on available memory region
3. dynamic allocator
    - Allocate smaller memory region within allocated page
    - Implemented using ???

### Todo:
    1. Review Buddy system
    2. Decide algorithm for (3)
