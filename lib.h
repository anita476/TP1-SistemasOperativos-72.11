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
// maximum of hash is 128 bits
#define MAX_BUFFER_LENGTH 1024

#define MAX_MD5_LENGTH 32
#define MAX_PID_LENGTH 20
#define MAX_FILEPATH_LENGTH 4096
#define EXTRA_CHARS 4 // two spaces and a newline and one more just in case 
#define MAX_RES_LENGTH (MAX_FILEPATH_LENGTH + MAX_MD5_LENGTH + MAX_PID_LENGTH)

#define SHM_PATH "/shm"
#define SEM_PATH "/sem"

#define SHM_DEF_SIZE 0x40000

#define MIN_SLAVES 5

typedef struct {
    int readFd; 
    int writeFd; 
    pid_t pid; 
} SlaveProcess;

typedef struct{
    char *shmAddr; 
    sem_t *sem; 
    size_t bufferSize; 
    int fd; 

    atomic_int done;
    // char buffer[BUFFER_SIZE]; /* to make sure we are defining th page structure correctly */

} SharedMemoryStruct;

#define ERROR_EXIT(msg) do {perror(msg); exit(EXIT_FAILURE); } while (0)

#define HEADER "PID\t\t\tFILE\t\t\t\tHASH\n"