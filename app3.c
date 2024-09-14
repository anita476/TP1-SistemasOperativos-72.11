
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

    fd_set read_fd_set, fd_backup_read;
    FD_ZERO(&read_fd_set);
    for (int i = 0; i < num_slaves; i++) {
        FD_SET(slaves[i].slave_to_app[READ_END], &read_fd_set);
    }
    fd_backup_read = read_fd_set;

    int next_to_process = num_slaves;
    int processed = 0;
    char buffer[MAX_RES_LENGTH];

    // send initial files
    for (int i = 0; i < num_slaves && i < num_files; i++) {
        send_file_to_slave(&slaves[i], argv[i + 1]);
    }

    while (processed < num_files) {
        read_fd_set = fd_backup_read;
        check_error(select(FD_SETSIZE, &read_fd_set, NULL, NULL, NULL), ERROR, "Failed in select"); 

        for (int i = 0; i < num_slaves && processed < num_files; i++) {
            if (FD_ISSET(slaves[i].slave_to_app[READ_END], &read_fd_set)) {
                process_slave_output(i, buffer, sizeof(buffer));
                processed++;
                if (next_to_process < num_files) {
                    send_file_to_slave(&slaves[i], argv[next_to_process + 1]);
                    next_to_process++;
                }
            }
        }
    }

    cleanup_resources(shm, output, slaves, num_slaves);
    
    fclose(output); // returns EOF apparently, idk how to check error

    fprintf(stderr, "All done in app!\n");

    return 0;
}

int calculate_num_slaves(int num_files) {
    int num_slaves;

    // If there are more than 30 files, use 10% of files as children
    if (num_files > MAX_SLAVES) {
        num_slaves = num_files / 10;
    }

    else if (num_files <= MIN_SLAVES) {
        num_slaves = num_files;
    }

    else {
        num_slaves = num_files / 2;
    }

    return num_slaves;

}
void wait_for_view() {
    printf("%s\n", SHM_PATH);
    fprintf(stderr, "Waiting for view to connect\n");
    sleep(2);
    fflush(stdout);
}

void initialize_resources(int num_slaves, SlaveProcessInfo *slaves, SharedMemoryContext *shm) {
    for (int i = 0; i < num_slaves; i++) {
        check_error(pipe(slaves[i].app_to_slave), ERROR, "Failed to pipe"); 
        check_error(pipe(slaves[i].slave_to_app), ERROR, "Failed to pipe"); 
    }
    &shm = create_resources(num_files);
}

void create_slave_processes(num_slaves, slaves) {
    for (int i = 0; i < num_slaves; i++) {
        pid_t pid = fork();
        if (pid < 0) {
            fprintf(stderr, "Fork failed");
            exit(EXIT_FAILURE);
        } else if (pid == 0) {
            for (int j = 0; j < num_slaves; j++) {
                if (j != i) {
                    close(slaves[j].app_to_slave[WRITE_END]);
                    close(slaves[j].app_to_slave[READ_END]);
                    close(slaves[j].slave_to_app[WRITE_END]);
                    close(slaves[j].slave_to_app[READ_END]);
                }
            }
            dup2(slaves[i].app_to_slave[READ_END], STDIN_FILENO);
            dup2(slaves[i].slave_to_app[WRITE_END], STDOUT_FILENO);
            close(slaves[i].app_to_slave[READ_END]);
            close(slaves[i].slave_to_app[WRITE_END]);
            
            char *const argv[] = {"./slave", NULL};
            execv("./bin/slave", argv);
            ERROR_EXIT("Failed to execute slave program");
        } else {
            // Parent process
            slaves[i].pid = pid;
            close(slaves[i].app_to_slave[READ_END]);
            close(slaves[i].slave_to_app[WRITE_END]);
        }
    }
}


void process_slave_output(int slave_index, char *buffer, size_t buffer_size) {
    SlaveProcessInfo *slave = &slaves[slave_index];
    ssize_t bytes_read = read(slave->slave_to_app[READ_END], buffer, buffer_size - 1);
    if (bytes_read > 0) {
        buffer[bytes_read] = '\0';
        fprintf(stderr, "Read from slave %d: %s", slave->pid, buffer);

        // Write to output file
        check_error(fprintf(output, "%s", buffer) < 0, "Failed to write to output file");
        fflush(output);

        // Write to shared memory
        sem_wait(shm->sync_semaphore);
        int written = sprintf(shm->shm_addr + shm->current_position, "%s", buffer);
        shm->current_position += written;
        sem_post(shm->sync_semaphore);
    } else if (bytes_read == 0) {
        fprintf(stderr, "Slave %d has closed its pipe\n", slave->pid);
    } else {
        fprintf(stderr, "Error reading from slave %d: %s\n", slave->pid, strerror(errno));
    }
}