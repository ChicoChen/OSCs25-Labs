#ifndef FS_ERRORS_H
#define FS_ERRORS_H

#define O_RDONLY    0b000000
#define O_WRONLY    0b000001
#define O_RDWR      0b000010
#define O_CREAT     0b000100

#define UNKNOWN_ERROR           -1
#define FILE_NOT_FOUND          -2
#define ALLOCATION_FAILED       -3
#define OPERATION_NOT_ALLOW     -4
#define OPERATION_NOT_ALLOW     -4

bool readable(int flag){
    return BOOL((flag & O_RDWR) || !(flag & O_WRONLY) ); 
}

bool writeable(int flag){
    return BOOL((flag & O_RDWR) || (flag & O_WRONLY)); 
}

#endif 