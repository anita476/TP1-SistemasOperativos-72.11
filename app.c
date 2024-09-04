// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http:\/\/www.viva64.com

#include "lib.h"
#include <unistd.h>
#include <errno.h>
#include <sys/select.h>

int make_child_process();
// create shared memory
// create fd set for select 

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






