// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http:\/\/www.viva64.com

#include "lib.h"

#include <errno.h>
#include <sys/select.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <sys/stat.h> // moverlo a lib.h, para las macros S_IRUSR y  S_

int make_child_process();
int create_shared_memory(char * shmName, off_t length);
void wait_for_view();
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

    // Local variables for shared memory & semaphore (maybe its better to use a TAD)

    // create shared memory & semaphore 
    char * shmName = SHM_NAME; 
    int shmFd;
    void * shmAddr; 

    char * semRead;
    sem_t semReadAddr = SEM_READ_NAME; 
    char * semClose; 
    sem_t semCloseAddr = SEM_CLOSE_NAME; 

    // create file for output 
    char * outputFilename = "output.txt";
    FILE * output;
    if ((output = fopen(outputFilename, "w")) == NULL) {
        fprintf(stderr, "Error creating output file");
        exit(ERROR); 
    }

    wait_for_view();

    // Local variables for files 
    int maxNumFiles = argc-1; // esto no se 
    int numSlaves = logic_for_num_slaves(maxNumFiles); // create a logic for numSlaves e.g. para 100 files quiero 10 esclavos, cada uno que procese 10 files 

    // create shared memory 
    create_shared_memory(shmName, shmFd, shmAddr);

    char * semRead = SEM_READ_NAME; // put these in the header
    char * semExit = SEM_EXIT_NAME; 

    // create named semaphores 
    sem_t semReadAddr = sem_open(semRead, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR, 0);
    if (semReadAddr == SEM_FAILED) {
        fprintf(stderr, "Error creating READ semaphore");
        exit(ERROR);
    }

    sem_t semExitAddr = sem_open(semExit, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR, 0);
    if (semExitAddr == SEM_FAILED) {
        fprintf(stderr, "Error creating EXIT semaphore");
        exit(ERROR);
    }


    // printf("%d", make_child_process());

    // Select buffers 

    // Vectors -- option 2: childPidV[numSlaves][2]
    pid_t readFdV[numSlaves]; 
    pid_t writeFdV[numSlaves];
    pid_t childPidV[numSlaves]

    // Creating slaves 
    for (int i = 0; i < numSlaves; i++) {
        childPidV[i] = make_child_process(readFdV[i], writeFdV[i]);

        // give child starting files 

        // fd_set 
    }
    // all is well 
    fd_set readFdSet = create_fd_set(readFdV, numSlaves);
    fd_set writeFdSet = create_fd_set(writeFdV, numSlaves);

    


    // unlink semaphore
    if (sem_unlink(sem_name) == ERROR) {
        fprintf(stderr, "Error unlinking semaphore"); 
        exit(ERROR); 
    }


    // close everything 

    // close output file 

    // close/destroy semaphores

    // unlinking shared memory 
    if (shm_unlink(shmName) == ERROR) {
        fprintf(stderr, "Error unlinking shared memory"); 
        exit(ERROR); 
    }

    // close semaphore
    if (sem_close(sem_addr) == ERROR) {
        fprintf(stderr, "Error closing semaphore")
        exit(ERROR); 
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
int create_shared_memory(char * shmName, off_t shmSize) {
    shm_unlink(shmName); // si no existe, que devuelve? o si no es exitoso el unlinking, manejar error
    int shmFd = shm_open(shmName, O_CREAT | O_RDWR | O_EXCL, S_IRUSR | S_IWUSR); // faltaron modos, ahi lo puse
    if(shmFd == ERROR) {
        fprintf(stderr, "Error creating shared memory");
        return ERROR; 
    }
    if (ftruncate(shmFd, length) == ERROR) {
        fprintf(stderr, "Error truncating shared memory");
        return ERROR;
    }
    void * address = mmap(NULL, shmSize, PROT_READ | PROT_WRITE, MAP_SHARED, shmFd , 0);
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


void wait_for_view() {
    printf("%s", NAME_SHM);
    sleep(2); 
    printf("\n");
}