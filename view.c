// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http:\/\/www.viva64.com

#include "lib.h"


int main(int argc, char *argv[]) {
    // fprintf(stderr,"%d\n", argc);
    fprintf(stderr, "\n\nConnected to view...\n\n");

    /*initializing to avoid PVS*/
    char shmName[NAME_SIZE] = {0}; 
    // char semName[NAME_SIZE] = {0};
    // char semDoneName[NAME_SIZE]= {0};    

    // for (int i = 0; i < argc; i++) {
    //     fprintf(stderr, "view %d>> %s\n", i, argv[i]);
    // }

    if (argc == 2) {
        strncpy(shmName, argv[1], sizeof(shmName) - 1);
        // strncpy(semName, argv[2], sizeof(semName) - 1);
        // strncpy(semDoneName, argv[3], sizeof(semDoneName) - 1);
    } else if (argc == 1) { // caso pipe 
        // I set  %9s to avoid the PVS warning
        // otra opcion: hacer scanf(%9s %9s %9s) == 3 
        if(scanf("%9s", shmName) < 0) { // Scanf auto terminates with 0
            ERROR_EXIT("Error reading shmpath");
        }
        fprintf(stderr,"View know shm is: %s\n", shmName);

        // if(scanf("%9s", semName) < 0) {
        //     ERROR_EXIT("Error reading semName\n");
        //     exit(ERROR);
        // }

        // fprintf(stderr, "View knows sem is: %s\n",semName);

        // if(scanf("%9s", semDoneName) < 0) {
        //     ERROR_EXIT("Error reading semDoneName\n");
        // }
        
        // fprintf(stderr, "View knows sem is: %s\n",semName);
    } else {
        fprintf(stderr, "Usage: %s /shm\n", argv[0]);
        exit(ERROR);
    }  

    // fprintf(stderr, "Attempting to open shared memory: %s\n", shmName);

    SharedMemoryStruct *shmData = malloc(sizeof(SharedMemoryStruct));

    if ((shmData->fd = shm_open(shmName, O_RDONLY, S_IRUSR | S_IWUSR)) == ERROR) {
        ERROR_EXIT("Error opening shared memory in view\n");
    }

    if ((shmData->shmAddr = mmap(NULL, SHM_DEF_SIZE, PROT_READ, MAP_SHARED, shmData->fd, 0)) == MAP_FAILED) {
        ERROR_EXIT("Error mapping shared memory from view\n");
    }

    // // fprintf(stderr, "Shared memory mapped at address: %p\n", (void*)shmData->shmAddr);
    // fprintf(stderr, "Attempting to open semaphore: %s\n", shmData->semName);

    // no funciona shmData->semName 
    if ((shmData->sem = sem_open(SEM_PATH, 0)) == SEM_FAILED) {
        ERROR_EXIT("Error opening shmData->sem in view\n");
    } 

    if ((shmData->semDone = sem_open(SEM_DONE_PATH, 0)) == SEM_FAILED) {
        ERROR_EXIT("Error opening shmData->semDone in view\n");
    } 
 
    printf(HEADER); // FixMe: writing header twice?? why is it also in app.c 
    fflush(stdout);

    char buffer[MAX_RES_LENGTH];
    size_t readIdx = 0;

    if (sem_wait(shmData->semDone) != 0) {
        fprintf(stderr, "Creo que esta bien..\n");
    }

    while (1) {
        // fprintf(stderr, "Waiting for semaphore signal to start reading...\n");

        // Wait for the semaphore to signal that new data is available
        if (sem_wait(shmData->sem) != 0) {
            ERROR_EXIT("Error waiting on semaphore");
        }

        // fprintf(stderr, "Received signal, reading...\n");

        // If we hit a null character, there is no more data to read, so exit
        if (shmData->shmAddr[readIdx] == '\0') {
            fprintf(stderr, "No more data to read. Exiting...\n");
            break;
        }

        size_t len = 0;
        while (shmData->shmAddr[readIdx + len] != '\n') {
            len++;
        }

        memcpy(buffer, shmData->shmAddr + readIdx, ++len);  // Include newline
        buffer[len] = '\0';  // Null-terminate the string
        readIdx += len;

        printf("view>> %s", buffer);
        fflush(stdout);

        sem_post(shmData->sem);

        // if (sem_trywait(shmData->semDone) == 0) {
        //     fprintf(stderr, "Processing complete. Exiting...\n");
        //     break;
        // }
    }

    fprintf(stderr, "Out of loop\n");

    if (sem_post(shmData->semDone) == 0) {
        fprintf(stderr, "Processing complete. Exiting...\n");
    }

    fprintf(stderr, "View process: All data read, exiting.\n");

    sem_close(shmData->sem);
    sem_close(shmData->semDone);

    munmap(shmData->shmAddr, SHM_DEF_SIZE);
    close(shmData->fd);
    free(shmData);

    fprintf(stderr, "All done in view!\n");

    return 0;
}