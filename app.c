// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http:\/\/www.viva64.com

#include "lib.h"

#include <errno.h>
#include <sys/select.h>
#include <sys/mman.h>

int make_child_process();
int create_shared_memory(char * shmName, off_t offset);
fd_set createFdSet(int * fdv, int dim);

int main(int argc, char * argv[]) {
    // Check if the amount of arguments is valid
    if(argc < 2) {
        fprintf(stderr, "Usage: ./md5 path\n");
        exit(1); 
    }
    printf("%d", make_child_process());
    exit(0);
}

int make_child_process(int * readDescriptor, int * writeDescriptor) {
    // Create file descriptors for the pipes
    int appToSlave[2];
    int slaveToApp[2];

    // Iniciate pipes - check  -1 return -> cant fork :C
    if(pipe(appToSlave) < 0|| pipe(slaveToApp) < 0) {
        fprintf(stderr, "Error creating pipe");
        return ERROR; 
    }
    
    // Fork parent process
    pid_t pid = fork();

    // Invalid process
    if(pid < 0) {
        fprintf(stderr, "Error forking parent process");
        return ERROR;
    }
    
    // Child process 
    if (pid == 0) {
        // Close child redundant file descriptors 
        if (close(appToSlave[WRITE_END]) || close(slaveToApp[READ_END])) {
            fprintf(stderr, "Error closing pipe ends");
            return ERROR; 
        }
        
        // for pipe created "for app" (for reading input data)-> where slave reads, its "stdin" 
        // for pipe crearted "for slave" (for writing result data) -> where slave writes, its "stdout"
        if(dup2(appToSlave[READ_END], STDIN_FILENO) < 0 || dup2(slaveToApp[WRITE_END], STDOUT_FILENO) < 0) {
            fprintf(stderr, "Error duping input and output");
            return ERROR;
        }
        
        char * const argv[] = {"./slave", NULL};
        
        if(execv("./slave", argv)) {
            fprintf(stderr, "Error in execv");
            return ERROR;
        }
    }

    // Parent process
    else {
        // Close parent redundant file descriptors
        if (close(appToSlave[READ_END] || close(slaveToApp[WRITE_END]))) {
            fprintf(stderr, "Error closing pipe ends");
            return ERROR; 
        }

        //return values:
        * readDescriptor = slaveToApp[0];
        * writeDescriptor = appToSlave[1];

        return pid;
    }

    return 0;

}

/* testear luego */
int create_shared_memory(char * shmName, off_t offsets) {
    shm_unlink(shmName);
    int shmFD = shm_open(shmName, O_RDWR | O_CREATE | O_EXCL);
    if(shmFD == ERROR) {
        return ERROR; 
    }
    if (ftruncate(shmFD, offset) == ERROR) {
        return ERROR;
    }
    void * address = mmap(NULL, offset, PROT_READ | PROT_WRITE, MAP_SHARED, shmFD,0);
    if(address == MAP_FAILED) {
        return ERROR;
    }
    return 0;
}

fd_set createFdSet(int * fdv, int dim) {
    fd_set toReturn;
    // To avoid "trash"
    FD_ZERO(&toReturn);
    for (int i = 0; i < dim; i++) {
        FD_SET(fdv[i], &toReturn);
    }
    return toReturn;
}