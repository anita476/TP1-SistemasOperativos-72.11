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

    // int fd = -1;
    // sem_t *semaphore = SEM_FAILED;
    SlaveProcess *slaves = malloc(sizeof(SlaveProcess) * numSlaves);
    if (slaves == NULL) {
    ERROR_EXIT("Failed to allocate memory for slaves");
    }


    // initilize shared memory & semaphore
    SharedMemoryStruct *shmStruct = create_shared_memory_and_semaphore(numFiles);

    wait_for_view();
    
    // create slaves and communicate
    for (int i = 0; i < numSlaves; i++) {
        slaves[i].pid = create_slave_process(&slaves[i].readFd, &slaves[i].writeFd);

        // // Set both read and write file descriptors to non-blocking mode
        // int flags = fcntl(slaves[i].readFd, F_GETFL, 0);
        // fcntl(slaves[i].readFd, F_SETFL, flags | O_NONBLOCK);
        // flags = fcntl(slaves[i].writeFd, F_GETFL, 0);
        // fcntl(slaves[i].writeFd, F_SETFL, flags | O_NONBLOCK);

        // fprintf(stderr, "Created slave with PID %d, readFd: %d, writeFd: %d\n", 
        //         slaves[i].pid, slaves[i].readFd, slaves[i].writeFd);

        // give child starting file
        if (send_file_to_slave(&slaves[i], argv[i+1]) < 0) {
            ERROR_EXIT("Error sending initial file to slave");
        }
    }

    distribute_files_to_slaves(slaves, numSlaves, numFiles, argv, shmStruct, output);

    close_all_resources(shmStruct, output, slaves, numSlaves);
    free(slaves);

    return 0;
}


SharedMemoryStruct * create_shared_memory_and_semaphore(int numFiles){
    SharedMemoryStruct *shmStruct = malloc(sizeof(SharedMemoryStruct));

    // FixMe: consider passing in the path in the function instead of using a constant
    shm_unlink(SHM_PATH);

    shmStruct->fd = shm_open(SHM_PATH, O_CREAT | O_RDWR | O_EXCL, S_IRUSR | S_IWUSR);

    if (shmStruct->fd == ERROR) {
        ERROR_EXIT("Error creating shared memory");
    }

    if (ftruncate(shmStruct->fd, sizeof(SharedMemoryStruct)) == ERROR) {
        ERROR_EXIT("Error truncating shared memory");
    }

    shmStruct->shmAddr = mmap(NULL, sizeof(SharedMemoryStruct), PROT_READ | PROT_WRITE, MAP_SHARED, shmStruct->fd, 0);
    if (shmStruct->shmAddr == MAP_FAILED) {
        ERROR_EXIT("Error mapping shared memory");
    }

    shmStruct->bufferSize = numFiles * MAX_RES_LENGTH;

    sem_unlink(SEM_PATH);
    shmStruct->sem = sem_open(SEM_PATH, O_CREAT | O_EXCL, S_IRUSR | S_IWUSR, 1); 
    if (shmStruct->sem == SEM_FAILED) {
        ERROR_EXIT("Error creating semaphore");
    }
    

    sem_unlink(SEM_DONE_PATH);
    shmStruct->semDone = sem_open(SEM_DONE_PATH, O_CREAT | O_EXCL, S_IRUSR | S_IWUSR, 0); 
    if (shmStruct->semDone == SEM_FAILED) {
        ERROR_EXIT("Error creating semaphore");
    }

    return shmStruct;
}


// Returns pid's of child processes 
pid_t create_slave_process(int *readFd, int *writeFd) {
    int appToSlave[2]; 
    int slaveToApp[2];

    // Iniciate pipes - check  -1 return -> cant fork :C
    if(pipe(appToSlave) < 0 || pipe(slaveToApp) < 0) {
        ERROR_EXIT("Error creating pipes"); 
    }
    // fprintf(stderr,"APP to SLAVE pipe: readEnd: %d, writeEnd: %d\n",appToSlave[0],appToSlave[1]);
    // fprintf(stderr, "SLAVE to APP pipe: readEnd: %d, writeEnd: %d\n",slaveToApp[0],slaveToApp[1]);

    // // set pips to unbuffered mode
    // FILE *app_to_slave_write = fdopen(appToSlave[WRITE_END], "w");
    // FILE *slave_to_app_read = fdopen(slaveToApp[READ_END], "r");
    // if (app_to_slave_write == NULL || slave_to_app_read == NULL) {
    //     fprintf(stderr, "Error creating FILE streams");
    //     return ERROR;
    // }
    // setvbuf(app_to_slave_write, NULL, _IONBF, 0);
    // setvbuf(slave_to_app_read, NULL, _IONBF, 0);
    
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
        // fprintf(stderr,"I closed fd: %d and %d in child\n", appToSlave[WRITE_END],slaveToApp[READ_END]);

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
        // fprintf(stderr,"I closed fd: %d and %d in parent\n", appToSlave[READ_END],slaveToApp[WRITE_END]);
        //return values:
        *readFd = slaveToApp[READ_END];
        *writeFd = appToSlave[WRITE_END];
    }

    return pid;
}

// !!! FixMe REVISAR!!!
// aca no se si hacer que devuelva -1, o que haga un exit directamente desde esta funcion (como lo hicimos para las otras funcioens) -> las funciones modulares no deberían hacer exit mejor
// better name: write in pipe for slave? 
int send_file_to_slave(SlaveProcess *slave, const char *filename) {
    ssize_t bytesWritten = write(slave->writeFd, filename, strlen(filename));
    // fprintf(stderr, "Writing file %s to slave w pid %d from fd write: %d\n", filename, slave->pid, slave->writeFd);
    if (bytesWritten < 0){
        ERROR_EXIT("Error writing to slave");
    }
    if (write(slave->writeFd, "\n", 1) != 1) {
        ERROR_EXIT("Error writing to slave");
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
    int logic = floor(numFiles / 10);
    if (logic < 1) return numFiles;
    else { 
        fprintf(stderr, "Slaves num: %d\n", SLAVES * logic);
        return (SLAVES * logic);
    };

}

void distribute_files_to_slaves(SlaveProcess *slaves, int numSlaves, int numFiles, char *files[], SharedMemoryStruct *shmStruct, FILE * outputFile) {
    int nextToProcess = numSlaves;
    int processed = 0;
    int writtenCount = 0;

    while (processed < numFiles) {
        int readySlaves[numSlaves];
        int readyCount = wait_for_ready(slaves, numSlaves, readySlaves);

        for (int i = 0; i < readyCount; i++) {
            int whichSlave = readySlaves[i];
            SlaveProcess *slave = &slaves[whichSlave];

            char buffer[MAX_RES_LENGTH];
            ssize_t bytesRead = read(slave->readFd, buffer, sizeof(buffer) - 1);

            if (bytesRead > 0) {
                fprintf(stderr, "Bytes read: %ld\n", bytesRead);
                buffer[bytesRead] = '\0';
                fprintf(stderr, "Read from slave %d: %s", slave->pid, buffer);

                // Write to output file
                if (fprintf(outputFile, "%s", buffer) < 0) {
                    ERROR_EXIT("Error writing to output file");
                }
                fflush(outputFile);
                fprintf(stderr, "Waiting on semaphore...\n");
                if (sem_wait(shmStruct->sem) != 0) {
                    ERROR_EXIT("Error waiting on semaphore");
                }
                fprintf(stderr, "Semaphore acquired, about to write...\n");

                writtenCount += sprintf(shmStruct->shmAddr + writtenCount, 
                                            "%d: %s", slave->pid, buffer);

                
                fprintf(stderr, "Data written to shared memory\n");

                sem_post(shmStruct->sem);
                fprintf(stderr, "Posted to sem for view to read\n");

                processed++;

                if (nextToProcess < numFiles) {
                    // no se si debe ser un + 1 o no 
                    if (send_file_to_slave(slave, files[nextToProcess]) < 0) {
                        fprintf(stderr, "Error sending file to slave %d\n", slave->pid);
                        exit(ERROR);
                    }
                    nextToProcess++;
                }
            } else if (bytesRead == 0) {
                fprintf(stderr, "Slave %d has closed its pipe\n", slave->pid);
            } else {
                fprintf(stderr, "Error reading from slave %d: %s\n", slave->pid, strerror(errno));
            }
        }
    }

    // Signal that we're done processing
    fprintf(stderr, "Done processing, posting to semDone\n");
    sem_post(shmStruct->semDone);
    fprintf(stderr, "Done posting\n");

    // Close all slave pipes
    for (int i = 0; i < numSlaves; i++) {
        fprintf(stderr, "Closing pipes for slave %d (readFd: %d, writeFd: %d)\n", 
                slaves[i].pid, slaves[i].readFd, slaves[i].writeFd);
        close(slaves[i].readFd);
        close(slaves[i].writeFd);
    }

    fprintf(stderr, "Content of shared memory:\n%s\n", shmStruct->shmAddr);
}


void wait_for_view() {
    sleep(2);
    printf("%s\n", SHM_PATH);
    printf("%s\n",SEM_PATH);
    fflush(stdout);
}

void close_all_resources(SharedMemoryStruct *shmStruct, FILE *output, SlaveProcess *slaves, int numSlaves) {
    // close output file 
    fclose(output);

    shm_unlink(SHM_PATH);

    if (sem_close(shmStruct->sem) == ERROR) {
        ERROR_EXIT("Error closing semaphore");
    }
    if (sem_unlink(SEM_PATH) == ERROR) {
        ERROR_EXIT("Error unlinking semaphore\n"); 
    }

    if (sem_close(shmStruct->semDone) == ERROR) {
        ERROR_EXIT("Error closing semaphore");
    }
    if (sem_unlink(SEM_DONE_PATH) == ERROR) {
        ERROR_EXIT("Error unlinking semaphore\n"); 
    }

    // unlinking shared memory -> munmap is needed
    if (munmap(shmStruct->shmAddr, SHM_DEF_SIZE) == ERROR) {
        ERROR_EXIT("Error unmapping shared memory\n");
    }
    // FixMe: delete later
    else {
        fprintf(stderr, "I unmapped shm\n");
    }

    if (close(shmStruct->fd) == ERROR) { //if i unlink view cant access anymore (namespace is deleted)
        ERROR_EXIT("Error closing shared memory"); 
    }


    for (int i = 0; i < numSlaves; i++) {
        close(slaves[i].readFd);
        close(slaves[i].writeFd);
    }

    free(shmStruct);

}
