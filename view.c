// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http:\/\/www.viva64.com

#include "lib.h"
#include <signal.h>

#define NAME_SIZE 10

int main(int argc, char *argv[]) {
    fprintf(stderr,"%d\n", argc);

    fprintf(stderr, "In view...\n");
    char shmName[NAME_SIZE]; 
    char semName[NAME_SIZE];
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
        if((n = scanf("%s", shmName)) < 0) {
            fprintf(stderr, "Error reading input\n");
            exit(ERROR);
        }
        fprintf(stderr,"View know shm is: %s\n", shmName);
        if((n = scanf("%s",semName)) < 0){
            fprintf(stderr, "Error reading input\n");
            exit(ERROR);
        }
        fprintf(stderr, "View knows sem is: %s\n",semName);
    }

    //caso por parámetro
    else {
        strncpy(shmName, argv[1], sizeof(shmName)-1);
    }

    sem_t * semaphore = sem_open(semName, O_RDONLY, 0);
    if (semaphore == SEM_FAILED) {
        fprintf(stderr, "Error opening semaphore in view\n");
        exit(ERROR);
    } 

    if ((shmFd = shm_open(SHM_NAME, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR)) == ERROR) {
        fprintf(stderr, "Error opening / reading shared memory\n");
        exit(ERROR); 
    }
    
    SharedMemoryStruct * shmData; 
    if ((shmData = mmap(NULL, SHM_DEF_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shmFd, 0)) == MAP_FAILED) {
        fprintf(stderr, "Error mapping shared memory\n");
        exit(ERROR); 
    }
    
    //write header 
    printf(HEADER);
    fflush(stdout);

    char buffer[BUFFER_SIZE];
    int j = 0;
    while(sem_wait(semaphore) == 0){ 
        fprintf(stderr,"im here\n");

        size_t len = 0;
        while (shmData->buffer[j + len] != '\n')
        {
            len++;
        }
        memcpy(buffer, shmData->buffer + j, ++len);
        buffer[len] = 0;
        j += len;
        printf("%s", buffer);
        if(shmData->done == 1){
            printf("Hello boo\n");
            break;
        }
    }
    sem_close(semaphore);
    munmap(shmData, SHM_DEF_SIZE);
    close(shmFd);
    return 0;

}