// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http:\/\/www.viva64.com

// ./app files/* | ./vista

terminal1> 
// ./app files/* 
terminal2>
// ./vista 

#include "lib.h"
#include <string.h>
#include <semaphore.h>

#define NAME_SIZE 10


int main(int argc, char *argv[]) {

    int shmFd; 
    char shmName[NAME_SIZE];  

    if (argc > 1) {
        strncpy(shmName, argv[1], sizeof(shmName)-1);
    }

    else {
        if((n = read(STDIN_FILENO, shmName, sizeof(shmName) - 1)) < 0) {
            
        }
    }

  
    if ((shmFd = shm_open(shmName, O_RDONLY, S_IRSUR)) == ERROR) {
        fprintf(stderr, "Error opening / reading shared memory");
        exit(ERROR); 
    }
    
    void * shmData; 
    if ((shmData = mmap(NULL, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shmFd, 0)) == MAP_FAILED) {
        fprintf(stderr, "Error mapping shared memory");
        exit(ERROR); 
    }

    printf("FILE - MD5_HASH - CHILD_PID"); 

    // logic for semaphores 
    // veo si sem_wait esta bloqueado (no hay semData nuevo)
    // cuando esta listo (paso un arg para leer), copio el contenido a un buffer

    // termino de pasar todo, mando un sem_post a app para indicarle que ya termino 


    // closing shared memory 
    if (munmap(mmapAddr, SHM_SIZE) == ERROR) {
        fprintf(stderr, "Error unmapping shared memory");
        exit(ERROR); 
    }
    if (close(shmFd) == ERROR) {
        fprintf(stderr, "Error closing shared memory");
        exit(ERROR); 
    }

    return 0;

}