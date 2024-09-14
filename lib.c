#include "lib.h"

void check_error(int return_value, int check_value, const char *message) {
    if (return_value == check_value) {
        perror(message); 
        fprintf(stderr, "Error code: %d\n", errno);
        exit(EXIT_FAILURE);
    }
}

SharedMemoryContext *create_resources(int num_files) {
    SharedMemoryContext *shm = malloc(sizeof(SharedMemoryContext));
    check_error(shm, NULL, "Failed to allocate memory for shared memory");

    // preventive unlinks 
    shm_unlink(SHM_PATH);
    sem_unlink(SEM_SYNC_PATH);
    sem_unlink(SEM_DONE_PATH);

    shm->fd = shm_open(SHM_PATH, O_CREAT | O_RDWR | O_EXCL, S_IRUSR | S_IWUSR);
    check_error(shm->fd, ERROR, "Failed to create shared memory"); 

    check_error(ftrucante(shm->fd, SHM_DEF_SIZE), ERROR, "Failed to truncate shared memory");

    shm->shm_addr = mmap(NULL,SHM_DEF_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm->fd, 0);
    check_error(shm->shm_addr, MAP_FAILED, "Failed to map shared memory");

    shm->buffer_size = num_files * MAX_RES_LENGTH;

    // idk if this should be here
    strncpy(shm->sync_sem_name, SEM_PATH, NAME_SIZE - 1);
    shm->sync_sem_name[NAME_SIZE - 1] = '\0'; 

    strncpy(shm->done_semaphoreName, SEM_DONE_PATH, NAME_SIZE - 1);
    shm->done_semaphoreName[NAME_SIZE - 1] = '\0'; 

    shm->sync_semaphore = sem_open(shm->sync_sem_name, O_CREAT | O_EXCL, S_IRUSR | S_IWUSR, 1); 
    check_error("Failed to create sync semaphore");

    shm->done_semaphore = sem_open(shm->done_semaphoreName, O_CREAT | O_EXCL, S_IRUSR | S_IWUSR, 0); 
    check_error("Failed to create done semaphore");

    return shm;
}

// no se si deberia pasarle el sem_syn_path o directamente ponerlo yo usando los valores DEFINIDOS 
SharedMemoryContext *open_resources(const char *shm_path) {
    // do we use buffer size? 
    shm_data->fd = shm_open(shm_path, O_RDONLY, S_IRUSR | S_IWUSR);
    check_error(shm_data->fd, "Failed to open shared memory in view");

    shm_data->shm_addr = mmap(NULL, SHM_DEF_SIZE, PROT_READ, MAP_SHARED, shm_data->fd, 0);
    check_error("Failed to map shared memory in view\n");

    shm_data->sync_semaphore = sem_open(SEM_SYNC_PATH, 0);
    check_error(shm_data->sync_semaphore, SEM_FAILED, "Failed to open sync sempahore in view");

    shm_data->done_semaphore = sem_open(SEM_DONE_PATH, 0);
    check_error(shm_data->done_semaphore, SEM_FAILED, "Failed to open done sempahore in view");

}

void close_resources(SharedMemoryContext *shm) {
    check_error(sem_close(shm->sync_semaphore), ERROR, "Failed to close sync semaphore");
    check_error(sem_close(shm->done_semaphore), ERROR, "Failed to close done semaphore");
    check_error(munmap(shm->shm_addr, SHM_DEF_SIZE), MAP_FAILED, "Failed to unmap shared memory");
    check_error(close(shm->shm_fd), ERROR, "Failed to close shared memory fd");
}

void cleanup_resources(SharedMemoryContext *shm, SlaveProcessInfo *slaves) {
    close_resources(shm);
    check_error(sem_unlink(shm->sync_sem_name), ERROR, "Failed to unlink sync sempahore");
    check_error(sem_unlink(shm->done_sem_name), ERROR, "Failed to unlink sync sempahore");
    check_error(shm_unlink(SHM_PATH), ERROR, "Failed to unlink sync sempahore");
    free(shm);
} 
