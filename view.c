// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http:\/\/www.viva64.com

#include "lib.h"


int main(int argc, char *argv[]) {
    fprintf(stderr, "\n\nConnected to view...\n\n");

    /*initializing to avoid PVS*/
    char shm_path[NAME_SIZE] = {0}; 

    if (argc == 2) {
        strncpy(shm_path, argv[1], sizeof(shm_path) - 1);
    } else if (argc == 1) { // caso pipe 
        // I set  %9s to avoid the PVS warning
        check_error(scanf("%9s", shm_path) < 0, "Failed to read shm_path");
        
        fprintf(stderr,"View know shm is: %s\n", shm_path);
    } else {
        fprintf(stderr, "Usage: %s /shm\n", argv[0]);
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
    fprintf(stderr, "in view> Semaphore sync value: %d\n", sem_value);
    sem_getvalue(shm_data->done_semaphore, &sem_value);
    fprintf(stderr, "in view> Semaphore done value: %d\n", sem_value);
    
    while (1) {
        sem_wait(shm_data->done_semaphore);
        sem_wait(shm_data->sync_semaphore);

        if (shm_data->shm_addr[read_position] == '\0') {
            fprintf(stderr, "All processing complete. Exiting...\n");
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

            printf("view>> %s\n", buffer);
            fflush(stdout);
        }
        
        sem_post(shm_data->sync_semaphore);

    }
    

    fprintf(stderr, "View process: All data read, exiting.\n");

    sem_getvalue(shm_data->sync_semaphore, &sem_value);
    fprintf(stderr, "in view> Semaphore sync value: %d\n", sem_value);
    sem_getvalue(shm_data->done_semaphore, &sem_value);
    fprintf(stderr, "in view> Semaphore done value: %d\n", sem_value);

    close_resources(shm_data);
    free(shm_data);

    fprintf(stderr, "All done in view!\n");

    return 0;
}