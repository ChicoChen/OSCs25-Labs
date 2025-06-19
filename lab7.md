! Only some features in this lab support running on rasberry-pi 3:
- VFS
- TMPFS
- File system operation via syscall

Others only tested on qemu.

1. VFS's concept

    Virtual file system is the abstraction of whole file system in order to maintain a uniform interface to manipulate file objects under different file system.

    The File systems can mounted under each other, Hence VFS use a tree structure to represent all types of file object as nodes. Though some file system's can have nothing to do with tree structure, this can be resolve by including function pointer in our node structure. which is basically polymorphism in c. Each different file system need to implement its own member functions that VFS specified, and assign it to function pointers in `VnodeOperations` and `FileOperations`.

    Further more, to track the mounting between file system. Another structure `Mount` is included, you can think of it as a chain that links the mounted node and the root node of the mounting system.

2. Tmpfs

    The basic file system of my kerenl is tmpfs, which is literally tree-structured file nodes residing in memory.

3. About path parsing

    Since searching file on vfs need to be able to cross the boundery created by mounting, this task is mainly processed by vfs's operations/functions. The actual file systems only implement minimum part of this function (only look for children that directly below a node).

    Also relative parsing require bidirectional movement. Beside going from the mounting root to where it mounted, the tree iterator will always step to mounting root if possible.

    Also two fs mounted under a same point is prohibited, and unmount operation is currently not implemented.