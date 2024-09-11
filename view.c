// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http:\/\/www.viva64.com

#include "lib.h"
#include <signal.h>

#define NAME_SIZE 10

int main(int argc, char *argv[]) {
    fprintf(stderr,"%d\n", argc);

    fprintf(stderr, "In view...\n");

    char shmName[NAME_SIZE]={0}; 
    /*initializing to avoid PVS*/
    char semName[NAME_SIZE]={0};
    // char semDone[NAME_SIZE]={0};


    int n; // FixMe: needs a more descriptive name
    

    for (int i = 0; i < argc; i++) {
        fprintf(stderr, "view %d>> %s\n", i, argv[i]);
    }
    
    // caso pipe
    //use scanf to tokenize
    if (argc == 1) {
    /*I set  %9s to avoid the PVS warning */
        if((n = scanf("%9s" ,shmName)) < 0 ){
            fprintf(stderr, "Error reading input\n");
            exit(ERROR);
        }

        fprintf(stderr,"View know shm is: %s\n", shmName);

        if((n = scanf("%9s", semName)) < 0){
            fprintf(stderr, "Error reading input\n");
            exit(ERROR);
        }

        fprintf(stderr, "View knows sem is: %s\n",semName);
    }

    //caso por parámetro -> que pasa con el semaforo?
    else if (argc == 3) { // caso argc == 3 
        strncpy(shmName, argv[1], sizeof(shmName) - 1);
        strncpy(semName, argv[2], sizeof(semName) - 1); // FIxMe: is this how its going to work? 
    } 
    else { 
        ERROR_EXIT("Parameters missing...\n");
    }
    // sem_t * done_semaphore = sem_open(semDone, O_RDONLY, 0); || done_semaphore == SEM_FAILED
    

    fprintf(stderr, "Attempting to open shared memory: %s\n", shmName);

    SharedMemoryStruct *shmData = malloc(sizeof(SharedMemoryStruct));

    // FixMe: consider passing in the path in the function instead of using a constant


    if ((shmData->fd = shm_open(shmName, O_RDONLY, S_IRUSR | S_IWUSR)) == ERROR) {
        ERROR_EXIT("Error opening / reading shared memory\n");
    }

    if ((shmData->shmAddr = mmap(NULL, shmData->bufferSize, PROT_READ, MAP_SHARED, shmData->fd, 0)) == MAP_FAILED) {
        ERROR_EXIT("Error mapping shared memory\n");
    }

    fprintf(stderr, "Shared memory mapped at address: %p\n", (void*)shmData->shmAddr);

    if ((shmData->sem = sem_open(semName, 0)) == SEM_FAILED ) {
        ERROR_EXIT("Error opening shmData->sem in view\n");
    } 
    //write header 
    printf(HEADER); // FixMe: writing header twice?? why is it also in app.c 
    fflush(stdout);

    char buffer[MAX_RES_LENGTH];
    size_t readIdx = 0;
    // int sem_return = 0;
    while(1){ 
        sem_wait(shmData->sem);

        fprintf(stderr,"im here\n");

        // FIxMe: consider using atomic 
        while (readIdx < MAX_RES_LENGTH) {
            size_t len = 0;
            while (readIdx + len < MAX_RES_LENGTH && shmData->shmAddr[readIdx + len] != '\n') {
                len++;
            }

            if (readIdx + len < MAX_RES_LENGTH) {
                memcpy(buffer, &shmData->shmAddr[readIdx], len);
                buffer[len] = '\0';
                printf("view>> %s\n", buffer);
                readIdx += len + 1; // skip newline 
            } else {
                break;
            }    
        }    
        
        if(atomic_load(&shmData->done) == 1){
            break;
        }
    }
    sem_close(shmData->sem);
    munmap(shmData, shmData->bufferSize);
    close(shmData->fd);

    return 0;
}