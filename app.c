// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http:\/\/www.viva64.com

#include "lib.h"

#include <errno.h>
#include <sys/select.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <sys/stat.h> // moverlo a lib.h, para las macros S_IRUSR y  S_

int make_child_process();
int create_shared_memory(char * shmName, off_t length, const void * address);
void wait_for_view(char * shmName);
fd_set createFdSet(int * fdv, int dim);

int main(int argc, char * argv[]) {
    // Check if the amount of arguments is valid
    if(argc < 2) {
        printf("Usage: ./md5 path\n");
        fprintf(stderr, "Error in number of arguments passed");
        exit(1); 
    }

    // setvbuf(strem, buf, _IONBF, BUFSIZ);
    // _IONBF : unbuffered 
    setvbuf(stdout, NULL, _IONBF, 0); 

    // create file for output 
    char * const outputFilename = "output.txt";
    FILE * output;
    if ((output = fopen(outputFilename, "w")) == NULL) {
        fprintf(stderr, "Error creating output file");
        exit(1); 
    }
    
     // Local variables for shared memory & semaphore (maybe its better to use a TAD)
    // create shared memory & semaphore 
    char * shmName = SHM_NAME; 
    int shmFd;
    void * shmAddr; 

    //unlink  sem just  in case (to avoid repetition)
    sem_unlink(SEM_NAME);

    sem_t semaphore = sem_open(SEM_NAME, O_CREAT, 0660, 0); 
    if (semaphore == SEM_FAILED) {
        fprintf(stderr, "Error creating semaphore\n");
        exit(1);
    }


    // Local variables for files 
    int numFiles = argc-1; // esto no se  -> its okey (argv[0] is ./app)
    int numSlaves = logic_for_num_slaves(numFiles); // todo create a logic for numSlaves e.g. para 100 files quiero 10 esclavos, cada uno que procese 10 files 

    // create shared memory 
    create_shared_memory(shmName, shmFd, shmAddr);

    wait_for_view(shmName); // should we pass length of shm ? && it should wait when shm is already created creo
    
    
    // Select buffers 

    // Vectors -- option 2: childPidV[numSlaves][2]
    pid_t readFdV[numSlaves]; 
    pid_t writeFdV[numSlaves];
    pid_t childPidV[numSlaves];

    // Creating slaves 
    for (int i = 0; i < numSlaves; i++) {
        childPidV[i] = make_child_process(readFdV[i], writeFdV[i]);

        // give child starting files 

        // fd_set 
    }
    // all is well 
    fd_set readFdSet = create_fd_set(readFdV, numSlaves);
    fd_set writeFdSet = create_fd_set(writeFdV, numSlaves);

    


    // unlink semaphore -> why first unlink before close?
    if (sem_unlink(SEM_NAME) == ERROR) {
        fprintf(stderr, "Error unlinking semaphore"); 
        exit(1); 
    }

    // close everything 


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

// create shared memory
/* testear luego */
int create_shared_memory(char * shmName, off_t offset, const void * address) {
    if(shm_unlink(shmName)!= 0){ // si no existe, que devuelve? o si no es exitoso el unlinking, manejar error -> no hace nada si no existe 
        fprintf(stderr,"Error unlinking preexisting shared memory");
        return ERROR;
    }
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
    return 0;
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
    printf("\n");
}