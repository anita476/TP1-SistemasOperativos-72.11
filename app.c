// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http:\/\/www.viva64.com

#include "lib.h"
#include "app.h"

#include <sys/select.h>
#include <math.h>   
#include <signal.h>

#define OUTPUT_FILE "output.txt"

int main(int argc, char * argv[]) {
    signal(SIGPIPE,SIG_IGN);

    
    // Check if the amount of arguments is valid
    if(argc < 2) {
        fprintf(stderr, "Usage: %s <file1> <file2>...\n", argv[0]);
        exit(EXIT_FAILURE); 
    }

    setvbuf(stdout, NULL, _IONBF, 0);

    int numFiles = argc - 1;
    int numSlaves = calculate_num_slaves(numFiles);

    FILE * output = fopen(OUTPUT_FILE, "w");
    if (output == NULL) {
        ERROR_EXIT("Error creating output file");
    }
    fprintf(output, HEADER);

    int shmFd = -1;
    SharedMemoryStruct *shmAddr = NULL;
    sem_t *semaphore = SEM_FAILED;
    SlaveProcess slaves[numSlaves];

    // initilize shared memory & semaphore
    create_shared_memory_and_semaphore(&shmFd, &shmAddr, SHM_NAME, &semaphore, SEM_NAME);
    wait_for_view();
    
    // create slaves and communicate
    for (int i = 0; i < numSlaves; i++) {
        slaves[i].pid = create_slave_process(&slaves[i].readFd, &slaves[i].writeFd);

        // give child starting file
        if (send_file_to_slave(&slaves[i], argv[i+1]) < 0) {
            ERROR_EXIT("Error sending initial file to slave");
        }
    }

    distribute_files_to_slaves(slaves, numSlaves, numFiles, argv, semaphore, shmAddr);

    close_all_resources(shmAddr, shmFd, semaphore, output);

    return 0;
}


void create_shared_memory_and_semaphore(int *shmFd, SharedMemoryStruct **shmAddr, const char *shmName, sem_t **semaphore, const char *semName){
    size_t totalSize = sizeof(SharedMemoryStruct) + BUFFER_SIZE;
    shm_unlink(shmName);

    *shmFd = shm_open(SHM_NAME, O_CREAT | O_RDWR | O_EXCL, S_IRUSR | S_IWUSR);
    if (*shmFd == ERROR) {
        ERROR_EXIT("Error creating shared memory");
    }
    if (ftruncate(*shmFd, totalSize) == ERROR) {
        ERROR_EXIT("Error truncating shared memory");
    }
    *shmAddr = mmap(NULL, totalSize, PROT_READ | PROT_WRITE, MAP_SHARED, *shmFd, 0);
    if (*shmAddr == MAP_FAILED) {
        ERROR_EXIT("Error mapping shared memory");
    }

    atomic_store(&(*shmAddr)->done, 0);

    sem_unlink(semName);
    *semaphore = sem_open(SEM_NAME, O_CREAT | O_EXCL, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP, 1); // important to set it to 1
    if (semaphore == SEM_FAILED) {
        ERROR_EXIT("Error creating semaphore");
    }
}

// Returns pid's of child processes 
pid_t create_slave_process(int *readFd, int *writeFd) {
    int appToSlave[2]; 
    int slaveToApp[2];

    // Iniciate pipes - check  -1 return -> cant fork :C
    if(pipe(appToSlave) < 0 || pipe(slaveToApp) < 0) {
        ERROR_EXIT("Error creating pipes"); 
    }
    fprintf(stderr,"APP to SLAVE pipe: readEnd: %d, writeEnd: %d\n",appToSlave[0],appToSlave[1]);
    fprintf(stderr, "SLAVE to APP pipe: readEnd: %d, writeEnd: %d\n",slaveToApp[0],slaveToApp[1]);

    // set pips to unbuffered mode
    FILE *app_to_slave_write = fdopen(appToSlave[WRITE_END], "w");
    FILE *slave_to_app_read = fdopen(slaveToApp[READ_END], "r");
    if (app_to_slave_write == NULL || slave_to_app_read == NULL) {
        fprintf(stderr, "Error creating FILE streams");
        return ERROR;
    }
    setvbuf(app_to_slave_write, NULL, _IONBF, 0);
    setvbuf(slave_to_app_read, NULL, _IONBF, 0);
    
    // Fork parent process
    pid_t pid = fork();

    // Invalid process
    if(pid < 0) {
        ERROR_EXIT("Error forking process");
    }
    
    // Child process 
    if (pid == 0) {
        // Close child redundant file descriptors 
        if (close(appToSlave[WRITE_END]) || close(slaveToApp[READ_END]) ) {
            ERROR_EXIT("Error closing pipe ends");
        }
        fprintf(stderr,"I closed fd: %d and %d in child\n", appToSlave[WRITE_END],slaveToApp[READ_END]);

        // for pipe created "for app" (for reading input data)-> where slave reads, its "stdin" 
        // for pipe crearted "for slave" (for writing result data) -> where slave writes, its "stdout"
        if(dup2(appToSlave[READ_END], STDIN_FILENO) < 0 || dup2(slaveToApp[WRITE_END], STDOUT_FILENO) < 0) {
            ERROR_EXIT("Error duping file descriptors");
        }

        // // closing unnecessary fd's 
        // close(appToSlave[READ_END]);
        // close(slaveToApp[WRITE_END]);
        // fprintf(stderr,"I closed fd: %d and %d in child\n", appToSlave[READ_END],slaveToApp[WRITE_END]);
        
        char *const argv[] = {"./slave", NULL};
        
        if(execv("./bin/slave", argv) == ERROR) {
            ERROR_EXIT("Error executing slave program");
        }
    }

    // Parent process
    else {
        // Close parent redundant file descriptors
        if (close(appToSlave[READ_END] || close(slaveToApp[WRITE_END]))) {
            ERROR_EXIT("Error closing pipe ends");
        }
        fprintf(stderr,"I closed fd: %d and %d in parent\n", appToSlave[READ_END],slaveToApp[WRITE_END]);
        //return values:
        *readFd = slaveToApp[READ_END];
        *writeFd = appToSlave[WRITE_END];
    }

    return pid;
}

// !!! FixMe REVISAR!!!
// aca no se si hacer que devuelva -1, o que haga un exit directamente desde esta funcion (como lo hicimos para las otras funcioens) -> las funciones modulares no deberían hacer exit mejor
int send_file_to_slave(SlaveProcess *slave, const char *filename) {
    ssize_t bytesWritten = write(slave->writeFd, filename, strlen(filename));
    if (bytesWritten < 0 || write(slave->writeFd, "\n", 1) != 1) {
        fprintf(stderr, "Error writing to slave");
        return ERROR;
    }
    return 0;
}

ssize_t wait_for_ready(SlaveProcess *slaves, int numSlaves, int *readySlaves) {
    fd_set readFdSet;
    FD_ZERO(&readFdSet);

    int maxFd = -1;

    // Finds the highest fd & add each slave's file descriptor 
    for (int i = 0; i < numSlaves; i++) {
        FD_SET(slaves[i].readFd, &readFdSet);
        if (slaves[i].readFd > maxFd) {
            maxFd = slaves[i].readFd;
        }
    } 
    int selectRes; 
    if ((selectRes = select(maxFd + 1, &readFdSet, NULL, NULL, NULL)) < 0) {
        ERROR_EXIT("Error selecting ready slaves");
    }

    int readyCount = 0;

    // checks which slaves are ready and adds them to readySlaves
    for (int i = 0; i < numSlaves && readyCount < selectRes; i++) {
        // Checks if the file descriptor is part of the set
        if (FD_ISSET(slaves[i].readFd, &readFdSet)) {
            readySlaves[readyCount++] = i;
        }
    }
    return readyCount;
}


// FixMe: find a better logic for the number of files sent at once 
int calculate_num_slaves(int numFiles) {
    // int logic = floor(numFiles / 10);
    // if (logic < 1) return numFiles;
    // else { 
    //     fprintf(stderr, "Slaves num: %d\n", SLAVES * logic);
    //     return (SLAVES * logic);
    // };

    return numFiles < 10 ? numFiles : numFiles / 10;
}

void distribute_files_to_slaves(SlaveProcess *slaves, int numSlaves, int numFiles, char *files[], sem_t *semaphore, SharedMemoryStruct *shmAddr) {
    int nextToProcess = numSlaves;
    int processed = 0;
    int bytesWritten = 0;

    // considerar: no entrar al loop si numSlaves == numFiles
    // There are still files to process
    while (processed < numFiles) {
        int readySlaves[numSlaves];
        int readyCount = wait_for_ready(slaves, numSlaves, readySlaves);
        // fprintf(stderr, "Processed: %d\n", processed);

        for(int i = 0; i < readyCount; i++) {
            int whichSlave = readySlaves[i];
            // pid_t childPID = childPidV[whichSlave]; // we ' lll use it to pass to view :0
            // int pipeFd = readFdV[whichSlave];

            SlaveProcess *slave = &slaves[whichSlave];

            // use what is on pipe to read result from slave
            char buffer[BUFFER_SIZE];
            ssize_t bytesRead;
            fprintf(stderr, "Getting ready 2 read from %d\n", slave->readFd);

            if((bytesRead = read(slave->readFd, buffer, sizeof(buffer))) < 0) {
                ERROR_EXIT("Error reading from slave");
            }
            if(bytesRead < BUFFER_SIZE ){
                buffer[bytesRead] = '\0';
            }
            else{
                ERROR_EXIT("Buffer size exceeded\n");
            }

            // char result[BUFFER_SIZE + 4];
            //append every part of  result 
            // snprintf(result, sizeof(result), "%s",buffer);
            // write to output file

            // CRITICAL SECTION!! 
            // FixMe: idk if to include error checking for sem_wait 
            sem_wait(semaphore);
            if (bytesWritten + bytesRead < BUFFER_SIZE) {
                memcpy(shmAddr->buffer + bytesWritten, buffer, bytesRead);
                bytesWritten += bytesRead;
            } else {
                fprintf(stderr, "Shared memory buffer overflow, data lost\n");
                sem_post(semaphore);  // semaphore is released even if  it fails
                return;
            }
            sem_post(semaphore);

            // write to shared mem
            // bytesWritten += snprintf(shmAddr->buffer + bytesWritten,  BUFFER - bytesWritten, "%s", result);

            //raise semaphore so view can read
            // int n = sem_post(semaphore);
            // if(n == -1){
            //     fprintf(stderr,"No process was woken up\n");
            // }
            // else{
            //     fprintf(stderr, "A process was woken up: n = %d\n",n);
            // }
            processed++;

            if(nextToProcess < numFiles) {
                // FixMe: include + 1 or nah? bc argv[0] is ./app so argv should be + 1
                if (send_file_to_slave(slave, files[nextToProcess + 1]) < 0) {
                    fprintf(stderr, "Error sending file to slave %d", whichSlave);
                    exit(ERROR);
                }
                nextToProcess++;
            }

            // else if (processed == numFiles) {
            //     //im done processing, i should signal EOF
            //     char * buf = "-1"; 
            //     write(shmFd,buf, 2);
            // }

            if (processed == numFiles) {
                atomic_store(&shmAddr->done, 1);
                sem_post(semaphore);
            }
        }
    }

    // if (sem_post(done_semaphore) == -1) {
    // fprintf(stderr, "sem_post done_semaphore failed");
    // }
    // atomic_store(&shmAddr->done, 1);
    // sem_post(semaphore);

    fprintf(stderr, "Content of shared memory:\n%s\n",shmAddr->buffer);
    // here we should close the rest of the pipes!
    for (int i = 0; i < numSlaves; i++) {
        fprintf(stderr, "Closing slave PID's %d pipes. Readfd: %d, WriteFd: %d\n", slaves[i].pid, slaves[i].readFd, slaves[i].writeFd);
        close(slaves[i].readFd);
        close(slaves[i].writeFd);   
    }
    
    // check if file descriptor still refers to terminal? idk why lucas does it yet, he uses isatty
}




void wait_for_view() {
    sleep(2);
    printf("%s\n", SHM_NAME);
    printf("%s\n",SEM_NAME);
    fflush(stdout);
}

void close_all_resources(SharedMemoryStruct *shmAddr, int shmFd, sem_t *semaphore, FILE *output) {
    // close output file 
    fclose(output);

    // unlinking shared memory -> munmap is needed
    if (munmap(shmAddr, sizeof(SharedMemoryStruct) + BUFFER_SIZE) == ERROR) {
        ERROR_EXIT("Error unmapping shared memory\n");
    }
    // FixMe: delete later
    else {
        fprintf(stderr, "I unmapped shm\n");
    }

    if (close(shmFd) == ERROR) { //if i unlink view cant access anymore (namespace is deleted)
        ERROR_EXIT("Error closing shared memory"); 
    }
    if (sem_close(semaphore) == ERROR) {
        ERROR_EXIT("Error closing semaphore");
    }
    if (sem_unlink(SEM_NAME) == ERROR) {
        ERROR_EXIT("Error unlinking semaphore\n"); 
    }
}