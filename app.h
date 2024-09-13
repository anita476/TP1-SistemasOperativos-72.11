#include <sys/select.h>
#include <math.h>

void distribute_files_to_slaves(SlaveProcess *slaves, int numSlaves, int numFiles, char *files[], SharedMemoryStruct *shmStruct, FILE *outputFile);
void close_all_resources(SharedMemoryStruct *shmStruct, FILE *output, SlaveProcess *slaves, int numSlaves);
ssize_t wait_for_ready(SlaveProcess *slaves, int numSlaves, int *readySlaves);
int send_file_to_slave(SlaveProcess *slave, const char *filename);
int calculate_num_slaves(int numFiles, int *filesPerSlave);
pid_t create_slave_process(int *readFd, int *writeFd);
void wait_for_view();

SharedMemoryStruct * create_shared_memory_and_semaphore(int numFiles);
