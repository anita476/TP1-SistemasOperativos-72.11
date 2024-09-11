void create_shared_memory_and_semaphore(int *shmFd, SharedMemoryStruct **shmAddr, const char *shmName, sem_t **semaphore, const char *semName);
ssize_t wait_for_ready(SlaveProcess *slaves, int numSlaves, int *readySlaves);
pid_t create_slave_process(int *readFd, int *writeFd);
int send_file_to_slave(SlaveProcess *slave, const char *filename);
int calculate_num_slaves(int numFiles);
void wait_for_view();
void distribute_files_to_slaves(SlaveProcess *slaves, int numSlaves, int numFiles, char *files[], sem_t *semaphore, SharedMemoryStruct *shmAddr);
void close_all_resources(SharedMemoryStruct *shmAddr, int shmFd, sem_t *semaphore, FILE *output);