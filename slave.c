// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

#include "lib.h"
#include <ctype.h>

// slave starts and reads from stdin, and afterwards writes to stdout
// in master we should change STDIN_FILENO and STDOUT_FILENO 
// that way we can execute slave from master and slave doesnt notice"

int main() {
    // fprintf(stderr, "Slave process started with PID %d\n", getpid());
    // md5 command
    char command[sizeof("md5sum ") + 2 /*for quotation marks*/ + MAX_FILEPATH_LENGTH];
    // our file path to append
    char filePath[MAX_FILEPATH_LENGTH];
    // where we'll store the
    char result[MAX_MD5_LENGTH];

    // Read line by line -> so pselect doesnt hang :)
    setlinebuf(stdout);

    // we read from "stdin" (file descriptor is a mistery)
    // dont remember why the -1 de PI 
    int n;
    // fprintf(stderr, "Slave is going to read from %d\n", STDIN_FILENO);

    while ((n = read(STDIN_FILENO, filePath, sizeof(filePath) - 1))) {
        if (n < 0) {
            fprintf(stdout, "An error ocurred while reading the file path");
            exit(1);
        }
        // here would go the file validation (is it a dir? can i read it ? well formed path?),,, optional for later

        if (filePath[n - 1] == '\n') {
            filePath[n - 1] = 0;
        }
        else {
            filePath[n] = 0;
        }

        // Validate if filePath is not empty
        if (filePath[0] == 0) {
            continue;
        }

        // fprintf(stderr, "Slave %d received filepath: %s\n", getpid(), filePath);

        //append the file path as part of the command
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

        // Remove newline from result if present
        result[strcspn(result, "\n")] = 0;

        char output[MAX_RES_LENGTH];
        int written = snprintf(output, sizeof(output), "slave>> %d\t%s\t%s\n", getpid(), filePath, result);
        
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

    // fprintf(stderr, "Slave %d exiting\n", getpid());

    exit(0);

    // else {
    //     printf("slave>> %d\t%s\t\t\t%s\n", getpid(), filePath, result);
    // }

    // pclose(md5sum);           
}