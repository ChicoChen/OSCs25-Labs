#ifndef FS_ERRORS_H
#define FS_ERRORS_H

#define O_RDWR      00000
#define O_RDONLY    00001
#define O_WRONLY    00002
#define O_CREAT     00100

#define FILE_READABLE(flag) ((flag & 0b111) != 2)
#define FILE_WRITEABLE(flag) ((flag & 0b111) != 1)

#define UNKNOWN_ERROR           -1
#define FILE_NOT_FOUND          -2
#define ALLOCATION_FAILED       -3
#define OPERATION_NOT_ALLOW     -4
#define OPERATION_NOT_ALLOW     -4

#endif 