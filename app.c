// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

#include "lib.h"

#define OUTPUT_FILE "output.txt"

void initialize_resources(int num_slaves, SlaveProcessInfo *slaves, SharedMemoryContext *shm);
void cleanup_resources(SharedMemoryContext *shm, SlaveProcessInfo *slaves, int num_slaves);
void create_slave_processes(int num_slaves, SlaveProcessInfo *slaves);
int send_file_to_slave(SlaveProcessInfo *slave, const char *filename);
int calculate_num_slaves(int num_files);
void wait_for_view();


int main(int argc, char * argv[]) {
    
    signal(SIGPIPE, SIG_IGN);
    signal(SIGPIPE, SIG_IGN);

    if(argc < 2) {
        fprintf(stderr, "Usage: %s <file1> <file2>...\n", argv[0]);
        exit(EXIT_FAILURE); 
    }

    int num_files = argc - 1;
    int num_slaves = calculate_num_slaves(num_files);

    // PVS warns about "output" being null, but it is already handled
    FILE * output = fopen(OUTPUT_FILE, "w");
    check_error(output == NULL, "Failed to create output file");
 
    check_error(fprintf(output, HEADER) < 0, "Failed to write header to output file");

    SlaveProcessInfo *slaves = malloc(sizeof(SlaveProcessInfo) * num_slaves);

    // PVS warns about "slaves" being null, but it is already handled
    check_error(slaves == NULL, "Failed to allocate memory for slaves");

    setvbuf(stdout, NULL, _IONBF, 0);

    // Initialize shared memory & semaphore
    SharedMemoryContext *shm = create_resources(num_files);
    
    wait_for_view();

    // Initialize slaves
    for (int i = 0; i < num_slaves; i++) {
        check_error(pipe(slaves[i].app_to_slave) == ERROR, "Failed to pipe"); 
        check_error(pipe(slaves[i].slave_to_app) == ERROR, "Failed to pipe"); 
    }    

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
    int written = 0;

    // Send initial files
    // Send initial files
    for (int i = 0; i < num_slaves && i < num_files; i++) {
        send_file_to_slave(&slaves[i], argv[i + 1]);
    }

    while (processed < num_files) {
        read_fd_set = fd_backup_read;
        check_error(select(FD_SETSIZE, &read_fd_set, NULL, NULL, NULL) < 0, "Failed in select"); 

        for (int i = 0; i < num_slaves && processed < num_files; i++) {
            if (FD_ISSET(slaves[i].slave_to_app[READ_END], &read_fd_set)) {

                SlaveProcessInfo *slave = &slaves[i];
                ssize_t bytes_read = read(slave->slave_to_app[READ_END], buffer, sizeof(buffer) - 1);

                if (bytes_read > 0) {
                    buffer[bytes_read] = '\0';

                    check_error(fprintf(output, "%s", buffer) < 0, "Failed to write to output file");
                    fflush(output);

                    // add error chcks on semaphores
                    sem_wait(shm->sync_semaphore);
                    written += sprintf(shm->shm_addr + written, "%s", buffer);
                    sem_post(shm->sync_semaphore);
                    sem_post(shm->done_semaphore);
                } 
                
                else if (bytes_read == 0) {
                    fprintf(stderr, "Slave %d has closed its pipe\n", slave->pid);
                } else {
                    fprintf(stderr, "Error reading from slave %d: %s\n", slave->pid, strerror(errno));
                }

                processed++;
                if (next_to_process < num_files) {
                    send_file_to_slave(&slaves[i], argv[next_to_process + 1]);
                    next_to_process++;
                }
            }
        }
    }

    sem_post(shm->done_semaphore);

    cleanup_resources(shm, slaves, num_slaves);
    
    check_error(fclose(output) != 0, "Failed to close output file"); 

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

    else { // between min and max 
        num_slaves = num_files / 2;
    }

    return num_slaves;
}

void wait_for_view() {
    printf("%s\n", SHM_PATH);
    sleep(2);
    fflush(stdout);
}

void create_slave_processes(int num_slaves, SlaveProcessInfo *slaves) {
    for (int i = 0; i < num_slaves; i++) {
        pid_t pid = fork();
        
        if (pid < 0) {
            fprintf(stderr, "Fork failed");
            exit(EXIT_FAILURE);
        } 
        
        else if (pid == 0) {
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
            fprintf(stderr, "Failed to execute slave program");
        } 
        
        else {
            // Parent process
            slaves[i].pid = pid;
            close(slaves[i].app_to_slave[READ_END]);
            close(slaves[i].slave_to_app[WRITE_END]);
        }
    }
}


int send_file_to_slave(SlaveProcessInfo *slave, const char *filename) {
    ssize_t bytes_written = write(slave->app_to_slave[WRITE_END], filename, strlen(filename));

    check_error(bytes_written == ERROR, "Failed to write to slave"); 
    
    if (write(slave->app_to_slave[WRITE_END], "\n", 1) != 1) {
        perror("Failed to write to slave");
        exit(EXIT_FAILURE);
    } 

    return 0;
}

void cleanup_resources(SharedMemoryContext *shm, SlaveProcessInfo *slaves, int num_slaves) {
    destroy_resources(shm);

    for(int i = 0; i < num_slaves; i++) {
		close(slaves[i].app_to_slave[WRITE_END]);
		close(slaves[i].slave_to_app[READ_END]);
	}

    free(slaves);
}