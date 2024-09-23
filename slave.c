// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

#include "lib.h"
#include <ctype.h>

int main() {
    char command[sizeof("md5sum ") + 2 /*for quotation marks*/ + MAX_FILEPATH_LENGTH];
    char filePath[MAX_FILEPATH_LENGTH];
    char result[MAX_MD5_LENGTH];

    // Read line by line so pselect doesn't hang
    setlinebuf(stdout);

    int n;
    while ((n = read(STDIN_FILENO, filePath, sizeof(filePath) - 1))) {
        if (n < 0) {
            fprintf(stdout, "An error ocurred while reading the file path");
            exit(1);
        }

        if (filePath[n - 1] == '\n') {
            filePath[n - 1] = 0;
        }

        else {
            filePath[n] = 0;
        }

        if (filePath[0] == 0) {
            continue;
        }

        // Append the file path as part of the command
        snprintf(command, sizeof(command), "md5sum \"%s\" ", filePath);

        FILE * md5sum = popen(command, "r");

        if (md5sum == NULL) {
            fprintf(stderr, "Slave %d: Failed to execute md5sum command\n", getpid());
            continue;
        }

        if (fgets(result, MAX_MD5_LENGTH, md5sum) == NULL) {
            fprintf(stderr, "Slave %d: Failed to read md5sum result\n", getpid());
            pclose(md5sum);
            continue;
        }

        char output[MAX_RES_LENGTH];
        int written = snprintf(output, sizeof(output), "%d\t%s\t%s\n", getpid(), filePath, result);
        
        if (written < 0 || written >= sizeof(output)) {
            fprintf(stderr, "Slave %d: Error formatting output\n", getpid());
            continue;
        }

        if (write(STDOUT_FILENO, output, written) != written) {
            fprintf(stderr, "Slave %d: Error writing to stdout: %s\n", getpid(), strerror(errno));
        } 

        pclose(md5sum);        
    }

    if (ferror(stdin)) {
        fprintf(stderr, "Slave %d: Error reading from stdin: %s\n", getpid(), strerror(errno));
    }

    exit(0);        
}