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
    char shmName[NAME_SIZE]; 
    /*initializing to avoid PVS*/
    char semName[NAME_SIZE]={0};
    // char semDone[NAME_SIZE]={0};
    int shmFd;
    int n;

    for (int i = 0; i < argc; i++) {
        fprintf(stderr, "view %d>> %s\n", i, argv[i]);
    }
    
    // caso error
    if (argc == 2) {
        fprintf(stderr, "Parameters missing...\n");
        exit(ERROR);  
    } 

    // caso pipe
    //use scanf to tokenize
    else if (argc == 1) {
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

    //caso por parámetro
    else {
        strncpy(shmName, argv[1], sizeof(shmName) - 1);
    }
    sem_t * semaphore = sem_open(semName, O_RDONLY, 0);
    // sem_t * done_semaphore = sem_open(semDone, O_RDONLY, 0); || done_semaphore == SEM_FAILED
    if (semaphore == SEM_FAILED ) {
        fprintf(stderr, "Error opening semaphore in view\n");
        exit(ERROR);
    } 

    fprintf(stderr, "Attempting to open shared memory: %s\n", shmName);

    if ((shmFd = shm_open(SHM_NAME, O_RDONLY, S_IRUSR | S_IWUSR)) == ERROR) {
        fprintf(stderr, "Error opening / reading shared memory\n");
        exit(ERROR); 
    }
    
    SharedMemoryStruct * shmData; 
    if ((shmData = mmap(NULL, SHM_DEF_SIZE, PROT_READ, MAP_SHARED, shmFd, 0)) == MAP_FAILED) {
        fprintf(stderr, "Error mapping shared memory\n");
        exit(ERROR); 
    }

        fprintf(stderr, "Shared memory mapped at address: %p\n", (void*)shmData);

    
    //write header 
    printf(HEADER);
    fflush(stdout);

    char buffer[BUFFER_SIZE];
    int j = 0;
    int sem_return = 0;
    while(1){ 
        if ((sem_return = sem_wait(semaphore)) != 0) {
            fprintf(stderr, "sem_wait failed");
        }
        fprintf(stderr,"im here\n");

        size_t len = 0;
        while (shmData->buffer[j + len] != '\n') {
            len++;
        }

        memcpy(buffer, shmData->buffer + j, ++len);
        buffer[len] = 0;
        j += len;       
        
        printf("view>> %s\n", buffer);
        if(atomic_load(&shmData->done) == 1){
            break;
        }
        // if (sem_wait(done_semaphore) == 0) {
        //     fprintf(stderr, "Processing completed in view:)\n");
        // }
    }

    // if (sem_return == -1) {
    //     fprintf(stderr, "sem_wait failed");
    // }

    // Close and unmap everything 
    sem_close(semaphore);
    munmap(shmData, SHM_DEF_SIZE);
    close(shmFd);

    return 0;
}