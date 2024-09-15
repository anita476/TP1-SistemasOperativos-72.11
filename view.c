// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

#include "lib.h"

int main(int argc, char *argv[]) {
    // Initializing to avoid PVS warning    
    char shm_path[NAME_SIZE] = {0}; 

    if (argc == 2) {
        strncpy(shm_path, argv[1], sizeof(shm_path) - 1);
    } 
    
    // Case: pipe
    else if (argc == 1) {
        // %9s to avoid PVS warning
        check_error(scanf("%9s", shm_path) < 0, "Failed to read shm_path");
    } 
    
    else {
        fprintf(stderr, "Usage: %s /shm\n", argv[0]);
        exit(EXIT_FAILURE);
    }  

    SharedMemoryContext *shm_data = open_resources(shm_path);
    check_error(shm_data == NULL, "Failed to allocate memory for shared memory in view");
 
    printf(HEADER);
    fflush(stdout);

    char buffer[MAX_RES_LENGTH];
    size_t read_position = 0;

    while (1) {
        if (sem_wait(shm_data->sync_semaphore) != 0) {
            perror("Failed to wait on sync semaphore");
            break;
        }

        while (read_position < shm_data->current_position) {
            size_t remaining = shm_data->current_position - read_position;
            size_t to_read = (remaining < MAX_RES_LENGTH - 1) ? remaining : (MAX_RES_LENGTH - 1);
            
            memcpy(buffer, shm_data->shm_addr + read_position, to_read);
            buffer[to_read] = '\0';

            printf("view>> %s", buffer);
            fflush(stdout);

            read_position += to_read;
        }
        
        sem_post(shm_data->sync_semaphore);

        if (sem_trywait(shm_data->done_semaphore) == 0) {
            break;
        }
    }

    close_resources(shm_data);
    free(shm_data);

    return 0;
}