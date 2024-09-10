// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http:\/\/www.viva64.com

#include "lib.h"

#include <sys/select.h>
#include <errno.h>
#include <math.h>   
#include <signal.h>

#define OUTPUT_FILE "output.txt"

//int create_shared_memory(char * shmName, off_t length, SharedMemoryStruct * address);
ssize_t wait_for_ready(SlaveProcess * slaves, int numSlaves, int * readyV);
pid_t make_child_process(int * readDescriptor, int * writeDescriptor);
int send_file_to_slave(SlaveProcess *slave, const char *filename);
// fd_set create_fd_set(int * fdv, int dim);
int logic_for_num_slaves(int numFiles);
void wait_for_view();

int main(int argc, char * argv[]) {
    signal(SIGPIPE,SIG_IGN);
    
    // Check if the amount of arguments is valid
    if(argc < 2) {
        printf("Usage: ./md5 path\n");
        fprintf(stderr, "Error in number of arguments passed\n");
        exit(1); 
    }

    int numFiles = argc - 1;
    int numSlaves = logic_for_num_slaves(numFiles);

    // Initialize resources
    int shmFd = -1;
    SharedMemoryStruct * shmAddr = NULL;
    sem_t * semaphore = SEM_FAILED;
    FILE * output = NULL;
    SlaveProcess slaves[numSlaves];

    // Open output file  
    if ((output = fopen(OUTPUT_FILE, "w")) == NULL) {
        fprintf(stderr, "Error creating output file");
        exit(1); 
    }
    fprintf(output,HEADER);

    // Create shared memory
/*     shmFd = create_shared_memory(SHM_NAME, SHM_DEF_SIZE, shmAddr);
    
    if (shmFd == ERROR) {
        fprintf(stderr, "Error creating shared memory");
        exit(ERROR);
    } */
    shm_unlink(SHM_NAME); //devolverá -1 porque no tiene que existir, y eso esta ok
    
    shmFd = shm_open(SHM_NAME, O_CREAT | O_RDWR | O_EXCL, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
    if(shmFd == ERROR) {
        fprintf(stderr, "Error creating shared memory");
        exit(1);
    }

    if (ftruncate(shmFd, SHM_DEF_SIZE) == ERROR) {
        fprintf(stderr, "Error truncating shared memory");
        exit(1);
    }

    shmAddr = mmap(NULL, SHM_DEF_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shmFd , 0);
    if(shmAddr == MAP_FAILED) {
        fprintf(stderr, "Error mapping shared memory");
        exit(1);
    }
    shmAddr->done = 0;
    

    // Create semaphores
    sem_unlink(SEM_NAME);
    if ((semaphore = sem_open(SEM_NAME, O_CREAT, 0660, 0)) == SEM_FAILED) {
        fprintf(stderr, "Error creating semaphore\n");
        exit(1);
    }
    
    
    wait_for_view(); 
    

    for (int i = 0; i < numSlaves; i++) {

        slaves[i].pid = make_child_process(&slaves[i].readFd, &slaves[i].writeFd);

        // give child starting file
        if (send_file_to_slave(&slaves[i], argv[i+1]) < 0) {
            fprintf(stderr, "Error sending intiial file to slave %d", i);
            exit(ERROR);
        }
    }

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

            if((bytesRead = read(slave->readFd, buffer, sizeof(buffer))) < 0) {
                fprintf(stderr, "Error reading pipe output (from slave)\n");
                exit(1);
            }
            if(bytesRead < BUFFER_SIZE){
            buffer[bytesRead] = '\0';
            }
            else{
                fprintf(stderr,"Buffer size exceeded\n");
                exit(1);
            }

            char result[BUFFER_SIZE + 4];
            //append every part of  result 
            snprintf(result, sizeof(result), "%s",buffer);
            // write to output file
            if (fprintf(output, "%s", result) < 0) {
                fprintf(stderr, "Error writing result to output file");
                exit(1);
            } 

            // write to shared mem
           bytesWritten += snprintf(shmAddr->buffer + bytesWritten,  BUFFER - bytesWritten, "%s", result);

            //raise semaphore so view can read
            int n = sem_post(semaphore);
            if(n == -1){
                fprintf(stderr,"No process was woken up\n");
            }
            else{
                fprintf(stderr, "A process was woken up: n = %d\n",n);
            }
            processed++;

            if(nextToProcess < numFiles) {
                if (send_file_to_slave(slave, argv[nextToProcess + 1]) < 0) {
                    fprintf(stderr, "Error sending file to slave %d", whichSlave);
                    exit(ERROR);
                }
                nextToProcess++;
            }

            else if (processed == numFiles) {
                //im done processing, i should signal EOF
                char * buf = "-1"; 
                write(shmFd,buf, 2);
            }
        }
    }
    shmAddr->done = 1;
    sem_post(semaphore);
    fprintf(stderr, "Content of shared memory:\n%s\n",shmAddr->buffer);
    // here we should close the rest of the pipes!
    for (int i= 0; i < numSlaves; i++) {
        fprintf(stderr, "Closing slave %d pipes. Readfd: %d, WriteFd: %d\n", slaves[i].pid, slaves[i].readFd, slaves[i].writeFd);
        close(slaves[i].readFd);
        close(slaves[i].writeFd);   
    }
    
    // check if file descriptor still refers to terminal? idk why lucas does it yet, he uses isatty
    
    // close output file 
    fclose(output);

    // unlinking shared memory -> munmap is needed
    if (munmap(shmAddr, SHM_DEF_SIZE) == ERROR ) {
        fprintf(stderr, "Error unmapping shared memory\n");
        exit(1);
    }
    else {
        fprintf(stderr, "I unmapped shm\n");
    }

    if (close(shmFd) == ERROR) { //if i unlink view cant access anymore (namespace is deleted)
        fprintf(stderr, "Error closing shared memory"); 
        exit(1); 
    }

    // close semaphore
    if (sem_close(semaphore) == ERROR) {
        fprintf(stderr, "Error closing semaphore");
        exit(1); 
    }

    // unlink semaphore -> why first unlink before close?
    if (sem_unlink(SEM_NAME) == ERROR) {
        fprintf(stderr, "Error unlinking semaphore\n"); 
        exit(1); 
    }

    exit(0);
}

// Returns pid's of child processes 
pid_t make_child_process(int * readDescriptor, int * writeDescriptor) {
    // Create file descriptors for the pipes
    int appToSlave[2]; 
    int slaveToApp[2];

    // Iniciate pipes - check  -1 return -> cant fork :C
    if(pipe(appToSlave) < 0 || pipe(slaveToApp) < 0) {
        fprintf(stderr, "Error creating pipe");
        return ERROR; 
    }
    fprintf(stderr,"APP to SLAVE pipe: readEnd: %d, writeEnd: %d\n",appToSlave[0],appToSlave[1]);
    fprintf(stderr, "SLAVE to APP pipe: readEnd: %d, writeEnd: %d\n",slaveToApp[0],slaveToApp[1]);
    
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
        fprintf(stderr,"I closed fd: %d and %d in child\n", appToSlave[WRITE_END],slaveToApp[READ_END]);

        // for pipe created "for app" (for reading input data)-> where slave reads, its "stdin" 
        // for pipe crearted "for slave" (for writing result data) -> where slave writes, its "stdout"
        if(dup2(appToSlave[READ_END], STDIN_FILENO) < 0 || dup2(slaveToApp[WRITE_END], STDOUT_FILENO) < 0) {
            fprintf(stderr, "Error duping input and output");
            return ERROR;
        }
        
        char * const argv[] = {"./slave", NULL};
        
        if(execv("./bin/slave", argv)) {
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
        fprintf(stderr,"I closed fd: %d and %d in parent\n", appToSlave[READ_END],slaveToApp[WRITE_END]);
        //return values:
        * readDescriptor = slaveToApp[0];
        * writeDescriptor = appToSlave[1];
        return pid;
    }

    return 0;
}

// create shared memory
/* testear luego */
/* int create_shared_memory(char * shmName, off_t offset, SharedMemoryStruct * address) {
    shm_unlink(shmName); //devolverá -1 porque no tiene que existir, y eso esta ok
    
    int shmFd = shm_open(shmName, O_CREAT | O_RDWR | O_EXCL, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
    if(shmFd == ERROR) {
        fprintf(stderr, "Error creating shared memory");
        return ERROR; 
    }

    if (ftruncate(shmFd, offset) == ERROR) {
        fprintf(stderr, "Error truncating shared memory");
        return ERROR;
    }

    address = mmap(NULL, offset, PROT_READ | PROT_WRITE, MAP_SHARED, shmFd , 0);
    if(address == MAP_FAILED) {
        fprintf(stderr, "Error mapping shared memory");
        return ERROR;
    }
    address->done = 0;
    return shmFd;
} */

void wait_for_view() {
    sleep(2);
    printf("%s\n", SHM_NAME);
    printf("%s\n",SEM_NAME);
    fflush(stdout);
}

ssize_t wait_for_ready(SlaveProcess *slaves, int numSlaves, int * readySlaves) {
    fd_set readFdSet;
    int readyCount = 0;
    int maxFd = -1;

    // initialize fd_set
    FD_ZERO(&readFdSet);

    // Finds the highest fd & add each slave's file descriptor 
    for (int i = 0; i < numSlaves; i++) {
        FD_SET(slaves[i].readFd, &readFdSet);
        if (slaves[i].readFd > maxFd) {
            maxFd = slaves[i].readFd;
        }
    } 
    int selectRes; 
    if ((selectRes = select(maxFd + 1, &readFdSet, NULL, NULL, NULL)) < 0) {
        fprintf(stderr, "Select error in wait_for_ready");
        return ERROR;
    }

    // checks which slaves are ready and adds them to readySlaves
    for (int i = 0; i < numSlaves && readyCount < selectRes; i++) {
        // Checks if the file descriptor is part of the set
        if (FD_ISSET(slaves[i].readFd, &readFdSet)) {
            readySlaves[readyCount++] = i;
        }
    }
    return readyCount;
}

int logic_for_num_slaves(int numFiles) {
    int logic = floor(numFiles / 10);
    if (logic < 1) return numFiles;
    else { 
        fprintf(stderr, "Slaves num: %d\n", SLAVES * logic);
        return (SLAVES * logic);
    };
}

// aca no se si hacer que devuelva -1, o que haga un exit directamente desde esta funcion (como lo hicimos para las otras funcioens) -> las funciones modulares no deberían hacer exit mejor
int send_file_to_slave(SlaveProcess *slave, const char *filename) {
    ssize_t bytesWritten = write(slave->writeFd, filename, strlen(filename));
    if (bytesWritten < 0) {
        fprintf(stderr, "Error processing files");
        return ERROR; 
    }
    if (write(slave->writeFd, "\n", 1) != 1) {
        fprintf(stderr, "Error writing to slave");
        return ERROR;
    }
    return 0;
}