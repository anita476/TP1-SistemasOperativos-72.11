// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http:\/\/www.viva64.com

#include "lib.h"

int main(int argc, char * argv[]) {
    // Check if the amount of arguments is valid
    if(argc == 0) {
        fprintf(stderr, "Usage: ./md5 path\n");
        exit(1); 
    }
    exit(0);
}



int make_child_process() {
    // Create file descriptors for the pipes
    int appToSlave[2];
    int slaveToApp[2];

    // Iniciate pipes - check  -1 return -> cant fork :C
    if(pipe(appToSlave) < 0|| pipe(slaveToApp) < 0) {
        fprintf(stderr, "Error creating pipe");
        return ERROR; 
    
    // Fork parent process
    pid_t pid = fork();

    // Invalid process
    if(pid < 0) {
        fprintf(stderr, "Error forking parent process");
        return ERROR;
    }
    
    // Child process 
    if (pid > 0) {
        // Close child file descriptors 
        if (close(appToSlave[WRITE_END]) || close(slaveToApp[READ_END])) {
            fprintf(stderr, "Error closing pipe ends");
            return ERROR; 
        }
        // excve("./bin/slave", fd) etc etc
    }
    
    // Parent process
    else {
        // Close parent file descriptors
        if (close(appToSlave[READ_END] || close(slaveToApp[WRITE_END]))) {
            fprintf(stderr, "Error closing pipe ends");
            return ERROR; 
        }
        
        
    }

}