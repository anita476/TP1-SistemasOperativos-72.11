// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http:\/\/www.viva64.com

#include "lib.h"
#include "app.h"

#define OUTPUT_FILE "output.txt"

int main(int argc, char * argv[]) {
    
    signal(SIGPIPE,SIG_IGN);

    // Check if the amount of arguments is valid
    if(argc < 2) {
        fprintf(stderr, "Usage: %s <file1> <file2>...\n", argv[0]);
        exit(EXIT_FAILURE); 
    }

    int num_files = argc - 1;
    int num_slaves = calculate_num_slaves(num_files);

    FILE * output = fopen(OUTPUT_FILE, "w");
    check_error(output, NULL, "Failed to create output file");
 
    fprintf(output, HEADER);

    SlaveProcessInfo *slaves = malloc(sizeof(SlaveProcessInfo) * num_slaves);
    check_error(slaves, NULL, "Failed to allocate memory for slaves");

    setvbuf(stdout, NULL, _IONBF, 0);

    // initilize shared memory & semaphore

    SharedMemoryContext *shm = malloc(sizeof(SlaveProcessInfo));
    check_error(slaves, NULL, "Failed to allocate memory for shared memory");

    initialize_resources(num_slaves, slaves, shm);
    
    wait_for_view();

    create_slave_processes(num_slaves, slaves);
    process_files(num_slaves, slaves, argv + 1, num_files, &shm_ctx);



    // for (int i = 0; i < num_slaves; i++) {
    //     // create_slave_process(&slaves[i]);
    //     check_error(send_file_to_slave(&slaves[i], argv[i + 1]), ERROR, "Failed to send file %s to slave %d", argv[i + 1], slaves[i].pid);
    // }

    int next_to_process = num_slaves; 
    int processed = 0; 
    int written_bytes = 0;
    fd_set read_fd_set; 
    int max_fd = -1;

    for (int i = 0; i < num_slaves; i++) {
        if (slaves[i].read_fd > maxFd) {
            maxFd = slaves[i].read_fd;
        }
    }

    while (processed < num_files) {
        FD_ZERO(&read_fdSet);
        for (int i = 0; i < num_slaves; i++) {
            FD_SET(slaves[i].read_fd, &read_fdSet);
        }

        int selected_fd = select(maxFd + 1, &read_fdSet, NULL, NULL, NULL);
        if (selected_fd == ERROR) {
            perror("Error in select");
            break;
        }

        for (int i = 0; i < num_slaves; i++) {
            // Process data from slave
            char buffer[MAX_RES_LENGTH];
            ssize_t bytes_read = read(slave->read_fd, buffer, MAX_RES_LENGTH - 1);
            if (bytes_read > 0) {
                buffer[bytes_read] = '\0'; 
        
                check_error(write_to_output(output_file, buffer), ERROR, "Failed to write to output file");

                fflush(output_file);

                sem_wait(shm->sync_semaphore); 

                int written = sprintf(shm->shared_mem_ptr + shm->current_position, "%s", data)
                
                sem_post(shm->sync_semaphore);

            }
            
            processed++; 
            if (next_to_process < num_files) {
                check_error(send_file_to_slave(&slaves[i], argv[next_to_process + 1], ERROR, "Failed to send file to slave %d\n", slaves[i].pid)); 
                nextToProcess++;
            }
        }
    }

    // Fix ME 
    distribute_files_to_slaves(slaves, num_slaves, num_files, argv, shm, output);

    cleanup_resources(shm, output, slaves, num_slaves);
    
    fclose(output); // returns EOF apparently, idk how to check error

    close_slaves(slaves); 
    free(slaves);

    fprintf(stderr, "All done in app!\n");

    return 0;
}

// FixMe: find a better logic for the number of files sent at once 

void create_slave_process(SlaveProcessInfo *slave) {
    // Initiate pipes - check  -1 return -> cant fork :C
    if(pipe(appToSlave) < 0 || pipe(slaveToApp) < 0) {
        ERROR_EXIT("Error creating pipes"); 
    }
    // fprintf(stderr,"APP to SLAVE pipe: readEnd: %d, writeEnd: %d\n",appToSlave[0],appToSlave[1]);
    // fprintf(stderr, "SLAVE to APP pipe: readEnd: %d, writeEnd: %d\n",slaveToApp[0],slaveToApp[1]);

    FILE *app_to_slave_write = fdopen(appToSlave[WRITE_END], "w");
    FILE *slave_to_app_read = fdopen(slaveToApp[READ_END], "r");

    if (app_to_slave_write == NULL || slave_to_app_read == NULL) {
        ERROR_EXIT("Error creating FILE streams");
    }

    // Set pipes to unbuffered mode
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

        slave->read_fd = slaveToApp[READ_END];
        slave->write_fd = appToSlave[WRITE_END];
        slave->pid = pid; 
    }

    return;
}

void close_slaves(SlaveProcessInfo *slaves) {
    for (int i = 0; i < num_slaves; i++) {
        fprintf(stderr, "Closing pipes for slave %d (read_fd: %d, write_fd: %d)\n", 
                slaves[i].pid, slaves[i].read_fd, slaves[i].write_fd);
        close(slaves[i].read_fd);
        close(slaves[i].write_fd);
    }

}


// !!! FixMe REVISAR!!!
// aca no se si hacer que devuelva -1, o que haga un exit directamente desde esta funcion (como lo hicimos para las otras funcioens) -> las funciones modulares no deberían hacer exit mejor
// better name: write in pipe for slave? 
int send_file_to_slave(SlaveProcessInfo *slave, const char *filename) {
    ssize_t bytes_written = write(slave->write_fd, filename, strlen(filename));
    // fprintf(stderr, "Writing file %s to slave w pid %d from fd write: %d\n", filename, slave->pid, slave->write_fd);
    
    if (bytes_written < 0 || write(slave->write_fd, "\n", 1) != 1) {
        fprintf(stderr, "Error writing to slave");
        return ERROR; 
    }

    return 0;
}




// ssize_t poll_ready_slaves(SlaveProcessInfo *slaves, int num_slaves, int *ready_slaves) {
//     fd_set read_fdSet;
//     FD_ZERO(&read_fdSet);

//     int maxFd = -1;

//     // Finds the highest fd & add each slave's file descriptor 
//     for (int i = 0; i < num_slaves; i++) {
//         FD_SET(slaves[i].read_fd, &read_fdSet);
//         if (slaves[i].read_fd > maxFd) {
//             maxFd = slaves[i].read_fd;
//         }
//     }

//     int selectRes = select(maxFd + 1, &read_fdSet, NULL, NULL, NULL);
//     check_error(selectRes, ERROR, "Error selecting ready slaves");

//     int num_ready = 0;

//     // checks which slaves are ready and adds them to ready_slaves
//     for (int i = 0; i < num_slaves && num_ready < selectRes; i++) {
//         // Checks if the file descriptor is part of the set
//         if (FD_ISSET(slaves[i].read_fd, &read_fdSet)) {
//             ready_slaves[num_ready++] = i;
//         }
//     }

//     return num_ready;
// }

// void distribute_files_to_slaves(SlaveProcessInfo *slaves, int num_slaves, int num_files, char *files[], SharedMemoryContext *shm, FILE * outputFile) {
//     int nextToProcess = num_slaves;
//     int processed = 0;
//     int writtenCount = 0;

//     while (processed < num_files) {
//         int ready_slaves[num_slaves];
//         int num_ready = poll_ready_slaves(slaves, num_slaves, ready_slaves);

//         for (int i = 0; i < num_ready; i++) {
//             int whichSlave = ready_slaves[i];
//             SlaveProcessInfo *slave = &slaves[whichSlave];

//             char buffer[MAX_RES_LENGTH];
//             ssize_t bytesRead = read(slave->read_fd, buffer, sizeof(buffer) - 1);

//             if (bytesRead > 0) {
//                 fprintf(stderr, "Bytes read: %ld\n", bytesRead);
//                 buffer[bytesRead] = '\0';
//                 fprintf(stderr, "Read from slave %d: %s", slave->pid, buffer);

//                 // Write to output file
//                 if (fprintf(outputFile, "%s", buffer) < 0) {
//                     ERROR_EXIT("");
//                 }

//                 fflush(outputFile);
//                 fprintf(stderr, "Waiting on semaphore...\n");

//                 if (sem_wait(shm->sync_semaphore) != 0) {
//                     ERROR_EXIT("Error waiting on semaphore");
//                 }

//                 fprintf(stderr, "Semaphore acquired, about to write...\n");

//                 writtenCount += sprintf(shm->shm_addr + writtenCount, 
//                                             "%s", buffer);

//                 fprintf(stderr, "Data written to shared memory\n");

//                 sem_post(shm->sync_semaphore);
//                 fprintf(stderr, "Posted to sync_semaphore for view to read\n");

//                 processed++;

//                 if (nextToProcess < num_files) {
//                     // no se si debe ser un + 1 o no 
//                     if (send_file_to_slave(slave, files[nextToProcess]) < 0) {
//                         fprintf(stderr, "Error sending file to slave %d\n", slave->pid);
//                         exit(ERROR);
//                     }
//                     nextToProcess++;
//                 }

//             } 
            
//             else if (bytesRead == 0) {
//                 fprintf(stderr, "Slave %d has closed its pipe\n", slave->pid);
//             } 
            
//             else {
//                 fprintf(stderr, "Error reading from slave %d: %s\n", slave->pid, strerror(errno));
//             }
//         }
//     }

//     // Signal that we're done processing
//     fprintf(stderr, "Done processing, posting to done_semaphore\n");
//     sem_post(shm->done_semaphore);

//     fprintf(stderr, "Done posting\n");
//     fprintf(stderr, "Content of shared memory:\n%s\n", shm->shm_addr);
// }

