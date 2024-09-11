#include <sys/types.h>
#include <semaphore.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <stdatomic.h>

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
#define BUFFER_SIZE 1024

// taken from pselect limitations
#define MAX_FD 1024

#define BUFFER ( SHM_DEF_SIZE- sizeof(int) ) // when do we use this? 

typedef struct {
    int readFd; 
    int writeFd; 
    pid_t pid; 
} SlaveProcess;

typedef struct{
    atomic_int done;
    char buffer[BUFFER_SIZE]; /* to make sure we are defining th page structure correctly */
} SharedMemoryStruct;

#define ERROR_EXIT(msg) do {perror(msg); exit(EXIT_FAILURE); } while (0)

#define HEADER "PID\t\t\tFILE\t\t\t\tHASH\n"