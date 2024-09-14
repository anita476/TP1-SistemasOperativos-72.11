#include "lib.h"
#include "app.h"

#define OUTPUT_FILE "output.txt"

int main(int argc, char * argv[]) {
    signal(SIGPIPE, SIG_IGN);

    if(argc < 2) {
        fprintf(stderr, "Usage: %s <file1> <file2>...\n", argv[0]);
        exit(EXIT_FAILURE); 
    }

    int numFiles = argc - 1;
    int numSlaves = (numFiles > MAX_SLAVES) ? numFiles / 10 : 
                    (numFiles <= MIN_SLAVES) ? numFiles : 
                    numFiles / 2;

    FILE * output = fopen(OUTPUT_FILE, "w");
    if (output == NULL) {
        ERROR_EXIT("Error creating output file");
    }
    fprintf(output, HEADER);

    SlaveProcess *slaves = malloc(sizeof(SlaveProcess) * numSlaves);
    if (slaves == NULL) {
        ERROR_EXIT("Failed to allocate memory for slaves");
    }

    setvbuf(stdout, NULL, _IONBF, 0);

    // Create shared memory and semaphores
    int shmFd = shm_open(SHM_PATH, O_CREAT | O_RDWR | O_EXCL, S_IRUSR | S_IWUSR);
    if (shmFd == ERROR) {
        ERROR_EXIT("Error creating shared memory");
    }
    if (ftruncate(shmFd, SHM_DEF_SIZE) == ERROR) {
        ERROR_EXIT("Error truncating shared memory");
    }
    char *shmAddr = mmap(NULL, SHM_DEF_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shmFd, 0);
    if (shmAddr == MAP_FAILED) {
        ERROR_EXIT("Error mapping shared memory");
    }

    sem_t *sem = sem_open(SEM_PATH, O_CREAT | O_EXCL, S_IRUSR | S_IWUSR, 1);
    sem_t *semDone = sem_open(SEM_DONE_PATH, O_CREAT | O_EXCL, S_IRUSR | S_IWUSR, 0);
    if (sem == SEM_FAILED || semDone == SEM_FAILED) {
        ERROR_EXIT("Error creating semaphores");
    }

    // Create slaves
    int curr_slave, curr_id = 1;
    for(curr_slave = 0; curr_slave < numSlaves && curr_id != 0; curr_slave++) {
        int appToSlave[2], slaveToApp[2];
        if(pipe2(appToSlave, O_CLOEXEC) < 0 || pipe2(slaveToApp, O_CLOEXEC) < 0) {
            ERROR_EXIT("Error creating pipes");
        }

        curr_id = fork();
        if(curr_id < 0) {
            ERROR_EXIT("Error forking process");
        } else if(curr_id == 0) {
            // Child process (slave)
            for(int i = 0; i < numSlaves; i++) {
                if(i != curr_slave) {
                    close(slaves[i].app_to_slave[0]);
                    close(slaves[i].slave_to_app[1]);
                    close(slaves[i].app_to_slave[1]);
                    close(slaves[i].slave_to_app[0]);
                }
            }
            dup2(appToSlave[0], STDIN_FILENO);
            dup2(slaveToApp[1], STDOUT_FILENO);
            close(appToSlave[0]);
            close(slaveToApp[1]);
            
            execl("./bin/slave", "./slave", NULL);
            ERROR_EXIT("Error executing slave program");
        } else {
            // Parent process
            slaves[curr_slave].pid = curr_id;
            slaves[curr_slave].app_to_slave[1] = appToSlave[1];
            slaves[curr_slave].slave_to_app[0] = slaveToApp[0];
            close(appToSlave[0]);
            close(slaveToApp[1]);
        }
    }

    // Parent process continues here
    int nextToProcess = numSlaves;
    int processed = 0;
    int writtenCount = 0;
    fd_set readFdSet;

    // Initial distribution of files to slaves
    for(int i = 0; i < numSlaves; i++) {
        write(slaves[i].app_to_slave[1], argv[i + 1], strlen(argv[i + 1]));
        write(slaves[i].app_to_slave[1], "\n", 1);
    }

    while(processed < numFiles) {
        FD_ZERO(&readFdSet);
        int maxFd = -1;
        for(int i = 0; i < numSlaves; i++) {
            FD_SET(slaves[i].slave_to_app[0], &readFdSet);
            if(slaves[i].slave_to_app[0] > maxFd) maxFd = slaves[i].slave_to_app[0];
        }

        if(select(maxFd + 1, &readFdSet, NULL, NULL, NULL) < 0) {
            ERROR_EXIT("Error in select");
        }

        for(int i = 0; i < numSlaves; i++) {
            if(FD_ISSET(slaves[i].slave_to_app[0], &readFdSet)) {
                char buffer[MAX_RES_LENGTH];
                ssize_t bytesRead = read(slaves[i].slave_to_app[0], buffer, sizeof(buffer) - 1);
                if(bytesRead > 0) {
                    buffer[bytesRead] = '\0';
                    fprintf(output, "%s", buffer);
                    fflush(output);

                    sem_wait(sem);
                    writtenCount += sprintf(shmAddr + writtenCount, "%s", buffer);
                    sem_post(sem);

                    processed++;

                    if(nextToProcess < numFiles) {
                        write(slaves[i].app_to_slave[1], argv[nextToProcess + 1], strlen(argv[nextToProcess + 1]));
                        write(slaves[i].app_to_slave[1], "\n", 1);
                        nextToProcess++;
                    }
                }
            }
        }
    }

    sem_post(semDone);

    // Cleanup
    fclose(output);
    sem_close(sem);
    sem_close(semDone);
    sem_unlink(SEM_PATH);
    sem_unlink(SEM_DONE_PATH);
    munmap(shmAddr, SHM_DEF_SIZE);
    close(shmFd);
    shm_unlink(SHM_PATH);

    for(int i = 0; i < numSlaves; i++) {
        close(slaves[i].app_to_slave[1]);
        close(slaves[i].slave_to_app[0]);
    }

    free(slaves);

    fprintf(stderr, "All done in app!\n");
    return 0;
}