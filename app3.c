
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

    // process files 


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
