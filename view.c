// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http:\/\/www.viva64.com

#include "lib.h"

#define NAME_SIZE 10

int main(int argc, char *argv[]) {

    fprintf(stderr, "In view...\n");
    char shmName[NAME_SIZE]; 
    int shmFd;
    int n;

    for (int i = 0; i < argc; i++) {
        fprintf(stderr, "view %d>> %s\n", i, argv[i]);
    }

    // caso error
    if (argc == 2) {
        fprintf(stderr, "Parameters missing...");
        exit(ERROR);  
    } 
    
    // caso pipe
    else if (argc == 1) {
        if((n = read(STDIN_FILENO, shmName, sizeof(shmName))) < 0) {
            fprintf(stderr, "Error reading input");
            exit(ERROR);
        }
        shmName[n] = 0; // fixed this erro 
    }

    //caso por parámetro
    else {
        strncpy(shmName, argv[1], sizeof(shmName)-1);
    }

    // cambio por el momento O_RDONLY

    if ((shmFd = shm_open(shmName, O_RDWR, S_IRUSR | S_IWUSR)) == ERROR) {
        fprintf(stderr, "Error opening / reading shared memory");
        exit(ERROR); 
    }
    
    char * shmData; 
    if ((shmData = mmap(NULL, SHM_DEF_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shmFd, 0)) == MAP_FAILED) {
        fprintf(stderr, "Error mapping shared memory");
        exit(ERROR); 
    }

    sem_t * semaphore = sem_open(SEM_NAME, O_RDONLY, 0);
    if (semaphore == SEM_FAILED) {
        fprintf(stderr, "Error opening semaphore in view");
        exit(ERROR);
    } 

    // telling app that we are reading!! 
    if (sem_wait(semaphore) == ERROR) {
        fprintf(stderr, "Error in sem_wait in view");
        exit(ERROR);
    }
 
    // no print buffering
	setvbuf(stdout, NULL, _IONBF, 0);

    // probamos ver que hay dentor de shmData
    // printf("view!! %s", (char *)shmData);

    printf("Calculating md5 hash...\n");
    int done = 0; 
    for (int i = 0; !done; i++) {
        sem_wait(semaphore);

        printf("\n File: %s | MD5: %s | PID: %d\n", )
    }
    
    char buffer[BUFFER_SIZE];

    // n = 0;
    // // Toma un md5 y cierra el semáforo hasta que termina de procesarlo
    // while(!(sem_wait(semaphore))) {
    //     if (shmData[n] == 0) {
    //         break; 
    //     }
    //     int len = 0;
    //     while (shmData[n] != '\n') {
    //         len++;
    //     }

    //     memcpy(buffer, shmData+n, ++len);
    //     buffer[len] = 0;
    //     n+=len;
    //     printf("MD_5 HASH: %s\n", buffer);
    // }

    // printf("FILE - MD5_HASH - CHILD_PID"); 

    // termino de pasar todo, mando un sem_post a app para indicarle que ya termino 

    if (sem_post(semaphore) == ERROR) {
        fprintf(stderr, "Error in sem_post in view");
        exit(ERROR);
    }

    // closing shared memory 
    if (munmap(shmData, SHM_DEF_SIZE) == ERROR) {
        fprintf(stderr, "Error unmapping shared memory");
        exit(ERROR); 
    }
    
    if (close(shmFd) == ERROR) {
        fprintf(stderr, "Error closing shared memory");
        exit(ERROR); 
    }

    return 0;

}