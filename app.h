#include <sys/select.h>

int calculate_num_slaves(int num_files);

void create_slave_process(SlaveProcessInfo *slave);

void close_slaves(SlaveProcessInfo *slaves);

int send_file_to_slave(SlaveProcessInfo *slave, const char *filename);

void wait_for_view();

ssize_t poll_ready_slaves(SlaveProcessInfo *slaves, int num_slaves, int *ready_slaves);
