#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>

#define ERROR (-1) 

#define READ_END 0 
#define WRITE_END 1

//based on Linux system standard
#define MAX_FILE_PATH 4096
// maximum of hash is 128 bits
#define MAX_HASH 32


#define SHM_NAME "shmApp"
#define SHM_DEF_SIZE 0x40000
#define SEM_NAME "semApp"