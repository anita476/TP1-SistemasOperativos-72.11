// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http:\/\/www.viva64.com

#include "lib.h"
#include <signal.h>

#define NAME_SIZE 10

int main(int argc, char *argv[]) {
    // fprintf(stderr,"%d\n", argc);

    // fprintf(stderr, "In view...\n");

    char shmName[NAME_SIZE]={0}; 
    /*initializing to avoid PVS*/
    char semName[NAME_SIZE]={0};
    // char semDone[NAME_SIZE]={0};    

    // for (int i = 0; i < argc; i++) {
    //     fprintf(stderr, "view %d>> %s\n", i, argv[i]);
    // }
    
    // caso pipe
    //use scanf to tokenize
    if (argc == 1) {
    /*I set  %9s to avoid the PVS warning */
        int read = 0;
        if((read = scanf("%9s", shmName)) < 0 ){ // scanf auto terminates with 0
            ERROR_EXIT("Error reading shmpath");
        }

        fprintf(stderr,"View know shm is: %s\n", shmName);

        if((read = scanf("%9s", semName)) < 0){
            fprintf(stderr, "Error reading input\n");
            exit(ERROR);
        }

        fprintf(stderr, "View knows sem is: %s\n",semName);
    }

    //caso por parámetro -> que pasa con el semaforo?
    else if (argc == 3) { // caso argc == 3 
        strncpy(shmName, argv[1], sizeof(shmName) - 1);
    } 
    else { 
        ERROR_EXIT("Parameters missing...\n");
    }    

    fprintf(stderr, "Attempting to open shared memory: %s\n", shmName);

    SharedMemoryStruct *shmData = malloc(sizeof(SharedMemoryStruct));

    if ((shmData->fd = shm_open(shmName, O_RDONLY, S_IRUSR | S_IWUSR)) == ERROR) {
        ERROR_EXIT("Error opening shared memory in view\n");
    }

    if ((shmData->shmAddr = mmap(NULL, sizeof(SharedMemoryStruct), PROT_READ, MAP_SHARED, shmData->fd, 0)) == MAP_FAILED) {
        ERROR_EXIT("Error mapping shared memory from view\n");
    }

    fprintf(stderr, "Shared memory mapped at address: %p\n", (void*)shmData->shmAddr);

    if ((shmData->sem = sem_open(semName, 0)) == SEM_FAILED ) {
        ERROR_EXIT("Error opening shmData->sem in view\n");
    } 

    if ((shmData->semDone = sem_open(SEM_DONE_PATH, 0)) == SEM_FAILED ) {
        ERROR_EXIT("Error opening shmData->sem in view\n");
    } 
    //write header 
    printf(HEADER); // FixMe: writing header twice?? why is it also in app.c 
    fflush(stdout);

    char buffer[MAX_RES_LENGTH];
    size_t readIdx = 0;

    sem_wait(shmData->semDone);

    while (1) {
        // fprintf(stderr, "Waiting for semaphore signal to start reading...\n");

        // Wait for the semaphore to signal that new data is available
        if (sem_wait(shmData->sem) != 0) {
            ERROR_EXIT("Error waiting on semaphore");
        }

        fprintf(stderr, "Received signal, reading...\n");
        if (shmData->shmAddr[readIdx] == 0) {
            break; 
        }
        size_t len = 0;
        while (shmData->shmAddr[readIdx + len] != '\n') {
            len++;
        }

        memcpy(buffer, shmData->shmAddr + readIdx, ++len);  // Include newline
        buffer[len] = '\0';  // Null-terminate the string
        readIdx += len + 1;

        printf("view>> %s", buffer);
        fflush(stdout);

        if (sem_trywait(shmData->semDone) == 0) {
            fprintf(stderr, "Processing complete. Exiting...\n");
            break;
        }
    
    }
    fprintf(stderr, "View process: All data read, exiting.\n");


    sem_close(shmData->sem);
    sem_close(shmData->semDone);

    munmap(shmData->shmAddr, SHM_DEF_SIZE);
    close(shmData->fd);
    free(shmData);

    return 0;
}