// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

#include "lib.h"

void check_error(int condition, const char *message) {
    if (condition) {
        perror(message);
        fprintf(stderr, "Error code: %d\n", errno);
        exit(EXIT_FAILURE);
    }
}

SharedMemoryContext *create_resources(int num_files) {
    SharedMemoryContext *shm = malloc(sizeof(SharedMemoryContext));
    check_error(shm == NULL, "Failed to allocate memory for shared memory");

    // Preventive unlinks 
    shm_unlink(SHM_PATH);
    sem_unlink(SEM_SYNC_PATH);
    sem_unlink(SEM_DONE_PATH);

    shm->shm_fd = shm_open(SHM_PATH, O_CREAT | O_RDWR | O_EXCL, S_IRUSR | S_IWUSR);
    check_error(shm->shm_fd == ERROR, "Failed to create shared memory"); 

    check_error(ftruncate(shm->shm_fd, SHM_DEF_SIZE) == ERROR, "Failed to truncate shared memory");

    shm->shm_addr = mmap(NULL, SHM_DEF_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm->shm_fd, 0);
    check_error(shm->shm_addr == MAP_FAILED, "Failed to map shared memory");

    shm->buffer_size = num_files * MAX_RES_LENGTH;
    
    shm->sync_semaphore = sem_open(SEM_SYNC_PATH, O_CREAT | O_EXCL, S_IRUSR | S_IWUSR, 1); 
    check_error(shm->sync_semaphore == SEM_FAILED, "Failed to create sync semaphore");

    shm->done_semaphore = sem_open(SEM_DONE_PATH, O_CREAT | O_EXCL, S_IRUSR | S_IWUSR, 0); 
    check_error(shm->done_semaphore == SEM_FAILED, "Failed to create done semaphore");

    return shm;
}

SharedMemoryContext *open_resources(const char *shm_path) {

    SharedMemoryContext *shm_data = malloc(sizeof(SharedMemoryContext));
    check_error(shm_data == NULL, "Failed to allocate memory for shm in view");

    shm_data->shm_fd = shm_open(shm_path, O_RDONLY, S_IRUSR | S_IWUSR);
    check_error(shm_data->shm_fd == ERROR, "Failed to open shared memory in view");

    shm_data->shm_addr = mmap(NULL, SHM_DEF_SIZE, PROT_READ, MAP_SHARED, shm_data->shm_fd, 0);
    check_error(shm_data->shm_addr == MAP_FAILED, "Failed to map shared memory in view\n");


    shm_data->sync_semaphore = sem_open(SEM_SYNC_PATH, 0);
    check_error(shm_data->sync_semaphore == SEM_FAILED, "Failed to open sync semaphore in view");
    int sem_value;
    sem_getvalue(shm_data->sync_semaphore, &sem_value);


    shm_data->done_semaphore = sem_open(SEM_DONE_PATH, 0);
    check_error(shm_data->done_semaphore == SEM_FAILED, "Failed to open done sempahore in view");
     sem_getvalue(shm_data->sync_semaphore, &sem_value);

    return shm_data;
}

void close_resources(SharedMemoryContext *shm) {
    check_error(sem_close(shm->sync_semaphore) == ERROR, "Failed to close sync semaphore");
    check_error(sem_close(shm->done_semaphore) == ERROR, "Failed to close done semaphore");
    check_error(munmap(shm->shm_addr, SHM_DEF_SIZE) == ERROR, "Failed to unmap mem");
    check_error(close(shm->shm_fd) == ERROR, "Failed to close shared memory fd");
}

void destroy_resources(SharedMemoryContext *shm) {
    close_resources(shm);
    check_error(sem_unlink(SEM_SYNC_PATH) == ERROR, "Failed to unlink sync sempahore");
    check_error(sem_unlink(SEM_DONE_PATH) == ERROR, "Failed to unlink sync sempahore");
    check_error(shm_unlink(SHM_PATH) == ERROR, "Failed to unlink sync sempahore");
    free(shm);
} 
