#include <sys/types.h>
#include <semaphore.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>

typedef struct {
    int readFd; 
    int writeFd; 
    pid_t pid; 
} SlaveProcess;

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

#define SLAVES 5 
#define BUFFER_SIZE 4096

// taken from pselect limitations
#define MAX_FD 1024

#define BUFFER ( SHM_DEF_SIZE- sizeof(int) )
typedef struct{
    int done;
    char buffer[BUFFER]; /* to make sure we are defining th page structure correctly */
} SharedMemoryStruct;