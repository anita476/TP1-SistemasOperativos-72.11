SharedMemoryStruct * create_shared_memory_and_semaphore(int numFiles);
pid_t create_slave_process(int *readFd, int *writeFd);
int send_file_to_slave(SlaveProcess *slave, const char *filename);
ssize_t wait_for_ready(SlaveProcess *slaves, int numSlaves, int *readySlaves);
int calculate_num_slaves(int numFiles);
void distribute_files_to_slaves(SlaveProcess *slaves, int numSlaves, int numFiles, char *files[], SharedMemoryStruct *shmStruct, FILE *outputFile);
void wait_for_view();
void close_all_resources(SharedMemoryStruct *shmStruct, FILE *output, SlaveProcess *slaves, int numSlaves);