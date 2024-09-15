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
 
    printf(HEADER); // FixMe: writing header twice?? why is it also in app.c 
    fflush(stdout);

    char buffer[MAX_RES_LENGTH];
    size_t read_position = 0;

    // sem_wait(shm_data->done_semaphore);
    int sem_value;
    
    while (1) {
        // if (sem_wait(shm_data->sync_semaphore) != 0) {
        //     perror("Failed to wait on sync semaphore");
        //     break;
        // }
    
        sem_wait(shm_data->sync_semaphore);
        //If we hit a null character, there is no more data to read, so exit
        // if (shm_data->shm_addr[read_position] == '\0') {
        //     fprintf(stderr, "No more data to read. Exiting...\n");
        //     // sem_post(shm_data->sync_semaphore);
        //     break;
        // }

        size_t len = 0;
        while (shm_data->shm_addr[read_position + len] != '\n') {
            len++;
        }

        // while (read_position < shm_data->current_position) {
        //     fprintf(stderr, "shm_data current_pos: %d\n\n", shm_data->current_position);


        //     size_t remaining = shm_data->current_position - read_position;
        //     size_t to_read = (remaining < MAX_RES_LENGTH - 1) ? remaining : (MAX_RES_LENGTH - 1);
            
        if (len > 0) {
            memcpy(buffer, shm_data->shm_addr + read_position, len);
            buffer[len] = '\0';
            read_position += len + 1;  // +1 to skip the newline

            printf("view>> %s\n", buffer);
            fflush(stdout);
        }
        
        // sem_post(shm_data->sync_semaphore);

    }

    // sem_post(shm_data->done_semaphore);


    fprintf(stderr, "View process: All data read, exiting.\n");

    close_resources(shm_data);
    free(shm_data);

    fprintf(stderr, "All done in view!\n");

    return 0;
}