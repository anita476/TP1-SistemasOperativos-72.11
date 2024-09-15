#include <stdatomic.h>
#include <sys/types.h>
#include <semaphore.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>

#define ERROR (-1) 

#define READ_END 0 
#define WRITE_END 1

#define MAX_MD5_LENGTH 32
#define MAX_PID_LENGTH 20
#define MAX_FILEPATH_LENGTH 4096
#define EXTRA_CHARS 4 // Two spaces and a newline and one more just in case 
#define MAX_RES_LENGTH (MAX_FILEPATH_LENGTH + MAX_MD5_LENGTH + MAX_PID_LENGTH)

#define SHM_PATH "/shm"
#define SEM_SYNC_PATH "/sync_semaphore"
#define SEM_DONE_PATH "/done_semaphore"

#define SHM_DEF_SIZE 0x40000

#define MIN_SLAVES 5
#define MAX_SLAVES 20
#define MIN_FILES_PER_SLAVE 1
#define AVG_FILES_PER_SLAVE 2

#define NAME_SIZE 10

#define HEADER "PID\tFILE\t\t\tHASH\n"

typedef struct {
    pid_t pid; 
    int app_to_slave[2];
    int slave_to_app[2]; 
} SlaveProcessInfo;

typedef struct {
    char *shm_addr; 
    sem_t *sync_semaphore; 
    sem_t *done_semaphore;
    size_t buffer_size; 
    int shm_fd; 
} SharedMemoryContext;

SharedMemoryContext *open_resources(const char *shm_path);
void check_error(int return_value, const char *message);
SharedMemoryContext *create_resources(int num_files);
void close_resources(SharedMemoryContext *shm);
void destroy_resources(SharedMemoryContext *shm);
