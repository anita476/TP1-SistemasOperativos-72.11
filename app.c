// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http:\/\/www.viva64.com

#include "lib.h"
#include <fcntl.h>
#include <errno.h>
#include <sys/select.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <string.h>
#include <math.h>
#include <sys/stat.h> // moverlo a lib.h, para las macros S_IRUSR y  S_

ssize_t wait_for_ready(pid_t * readFdV, int numSlaves, int * readyV);
int logic_for_num_slaves(int numFiles);
pid_t make_child_process(int * readDescriptor, int * writeDescriptor);
int create_shared_memory(char * shmName, off_t length, const void * address);
void wait_for_view(const char * shmName);
fd_set create_fd_set(int * fdv, int dim);

int main(int argc, char * argv[]) {
    
    // Check if the amount of arguments is valid
    if(argc < 2) {
        printf("Usage: ./md5 path\n");
        fprintf(stderr, "Error in number of arguments passed\n");
        exit(1); 
    }

    // setvbuf(strem, buf, _IONBF, BUFSIZ);
    // _IONBF : unbuffered 
    setvbuf(stdout, NULL, _IONBF, 0); 

    // create file for output 
    const char *  outputFilename = "output.txt";
    FILE * output;
    if ((output = fopen(outputFilename, "w")) == NULL) {
        fprintf(stderr, "Error creating output file");
        exit(1); 
    }
    
     // Local variables for shared memory & semaphore (maybe its better to use a TAD)
    // create shared memory & semaphore 
    char * shmName = SHM_NAME; 
    int shmFd;
    void * shmAddr = 0; 

    //unlink  sem just  in case (to avoid repetition)
    sem_unlink(SEM_NAME);

    sem_t * semaphore = sem_open(SEM_NAME, O_CREAT, 0660, 0); 
    if (semaphore == SEM_FAILED) {
        fprintf(stderr, "Error creating semaphore\n");
        exit(1);
    }


    // Local variables for files 
    int numFiles = argc -1; // esto no se  -> its okey (argv[0] is ./app)
    int numSlaves = logic_for_num_slaves(numFiles); // todo create a logic for numSlaves e.g. para 100 files quiero 10 esclavos, cada uno que procese 10 files 
    printf("%d\n", numFiles);
    // create shared memory 
    shmFd = create_shared_memory(shmName, SHM_DEF_SIZE, shmAddr);
    wait_for_view(shmName); // should we pass length of shm ? && it should wait when shm is already created creo
    
    
    // Select buffers 

    // Vectors -- option 2: childPidV[numSlaves][2]
    pid_t readFdV[numSlaves]; 
    pid_t writeFdV[numSlaves];
    pid_t childPidV[numSlaves];

    // Creating slaves 
    for (int i = 0; i < numSlaves; i++) {
        int *writeP = writeFdV + i;
        int *readP = readFdV + i;
        childPidV[i] = make_child_process(readP, writeP);
        fprintf(stderr, "write[%d]: %d\n",i, writeFdV[i]);

        // give child starting file
        ssize_t first = write(*writeP, argv[i + 1], strlen(argv[i + 1]));
        if(first < 0){
            fprintf(stderr, "Error processing files\n");
            exit(1);
        }
    }
    int nextToProcess = numSlaves + 1;
    int processed= 0;
    while (processed < numFiles){
        int childrenReady[numSlaves];
        int readyCount = wait_for_ready(readFdV,numSlaves, childrenReady);
        for(int i = 0; i< readyCount ; i++){
            int whichChild = childrenReady[i];
            pid_t childPID = childPidV[whichChild]; // we ' lll use it to pass to view :0

            int pipeFd = readFdV[whichChild];

            // use what is on pipe 
            char buffer[BUFFER_SIZE];
            int n;
            if((n = read(pipeFd, buffer, sizeof(buffer)))<0){
                fprintf(stderr, "Problem reading pipe output\n");
                exit(1);
            }
            buffer[n] = 0;
            // write to output file
            if ( fprintf(output, "%s", buffer) < 0){
                fprintf(stderr, "Problem writing result to output file\n");
                exit(1);
            } 
            // write to shared mem
            if(write(shmFd,buffer,n)<0){
                fprintf(stderr, "Error writing to shared memory\n");
                exit(1);
            }
            //raise semaphore so view can read
            sem_post(semaphore);
            // now we need to send new files
            int pipeFd2 = writeFdV[whichChild];
            if(nextToProcess < numFiles){
                 char *filename = argv[nextToProcess++];
                if(write(pipeFd2, filename, strlen(filename)< 0)){
                    fprintf(stderr, "Error assigning file to slave\n");
                    exit(1);
                }
            }
        }
        processed += readyCount;
    }
    //wait for vista to process
    sem_wait(semaphore);
    


    // close output file 
    fclose(output);

    // unlinking shared memory -> munmap is needed
    if (munmap(shmAddr, SHM_DEF_SIZE) == ERROR){
        fprintf(stderr, "Error unmapping shared memory\n");
        exit(1);
    }
    if (shm_unlink(shmName) == ERROR) {
        fprintf(stderr, "Error unlinking shared memory"); 
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
        fprintf(stderr, "CHILD %d\n",getpid());
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
        //return values:
        * readDescriptor = slaveToApp[0];
        * writeDescriptor = appToSlave[1];
        return pid;
    }

    return 0;
}

// create shared memory
/* testear luego */
int create_shared_memory(char * shmName, off_t offset, const void * address) {
    shm_unlink(shmName); //devolverá -1 porque no tiene que existir, y eso es´ta ok
    int shmFd = shm_open(shmName, O_CREAT | O_RDWR | O_EXCL, S_IRUSR | S_IWUSR); // faltaron modos, ahi lo puse
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
    return shmFd;
}



fd_set create_fd_set(int * fdv, int dim) {
    fd_set toReturn;
    // To avoid "trash"
    FD_ZERO(&toReturn);
    for (int i = 0; i < dim; i++) {
        FD_SET(fdv[i], &toReturn);
    }
    return toReturn;
}


void wait_for_view(const char * shmName) {
    printf("%s", shmName);
    sleep(2); 
    fflush(stdout);
    
}


ssize_t wait_for_ready(pid_t * readFdV, int numSlaves, int * readyV){
    fd_set readFdSet = create_fd_set(readFdV, numSlaves);
    int howMany = 0;

    if(select(MAX_FD, &readFdSet, NULL, NULL, NULL)< 0){
        return ERROR;
    }
    for(int i = 0; i < numSlaves; i++){
        if(FD_ISSET(readFdV[i], &readFdSet)){
            readyV[howMany++] = i;
        }
    }
    return howMany;
}


int logic_for_num_slaves(int numFiles){
    int logic = floor( numFiles / 10 );
    if(logic < 1){
        return numFiles;
    }
    else{
        return (SLAVES * ( logic ));
    }
    
}