// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

#include "lib.h"

int main(int argc, char *argv[]) {
    // Initializing to avoid PVS warning    
    char shm_path[NAME_SIZE] = {0}; 

    if (argc == 2) {
        strncpy(shm_path, argv[1], sizeof(shm_path) - 1);
    } 
    
    else if (argc == 1) {
        // %9s to avoid PVS warning
        check_error(scanf("%9s", shm_path) < 0, "Failed to read shm_path");
    } 
    
    else {
        fprintf(stderr, "Usage: %s <sharedMemoryPath>\n", argv[0]);
        exit(EXIT_FAILURE);
    }  

    SharedMemoryContext *shm_data = open_resources(shm_path);
    check_error(shm_data == NULL, "Failed to allocate memory for shared memory in view");
 
    printf(HEADER);
    fflush(stdout);

    char buffer[MAX_RES_LENGTH];
    size_t read_position = 0;

    int sem_value;
    sem_getvalue(shm_data->sync_semaphore, &sem_value);
    sem_getvalue(shm_data->done_semaphore, &sem_value);
    
    while (1) {
        sem_wait(shm_data->done_semaphore);
        sem_wait(shm_data->sync_semaphore);

        if (shm_data->shm_addr[read_position] == '\0') {
            sem_post(shm_data->sync_semaphore);
            break;
        }

        size_t len = 0;
        while (shm_data->shm_addr[read_position + len] != '\n' && shm_data->shm_addr[read_position + len] != '\0') {
            len++;
        }
                
        if (len > 0) {
            memcpy(buffer, shm_data->shm_addr + read_position, len);
            buffer[len] = '\0';
            read_position += len + 1;  // +1 to skip the newline

            printf("%s\n", buffer);
            fflush(stdout);
        }
        
        sem_post(shm_data->sync_semaphore);
    }
    
    sem_getvalue(shm_data->sync_semaphore, &sem_value);
    sem_getvalue(shm_data->done_semaphore, &sem_value);

    close_resources(shm_data);
    free(shm_data);

    return 0;
}