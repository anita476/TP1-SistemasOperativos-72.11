// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http:\/\/www.viva64.com

#include "lib.h"

// slave starts and reads from stdin , and afterwards writes to stdout
// in master we should change STDIN_FILENO and STDOUT_FILENO 
// that way we can eexecute slave from master and slave doesnt notice"

int main() {
    // md5 command
    char command[sizeof("md5sum ") + 2 /*for quotation marks*/ + MAX_FILE_PATH];
    // our file path to append
    char filePath[MAX_FILE_PATH];
    // where we'll store the
    char result[MAX_HASH];

    // we read from "stdin" (file descriptor is a mistery)
    // dont remember why the -1 de PI 
    int n;
    while((n = read(STDIN_FILENO, filePath, sizeof(filePath) -1))) {
        if(n < 0) {
            fprintf(stdout, "An error ocurred with file path");
        }

        // here would go the file validation (is it a dir?),,, optional for later

        // overwrite \n because md5sum doesnt accept it :c
        if(filePath[n-1] == '\n') {
            filePath[n-1] = 0;
        }
        else {
            filePath[n] = 0;
        }

        //append the file path as part of the command
        snprintf(command,sizeof(command),"md5sum \"%s\" ", filePath);

        FILE * md5sum = popen(command,"r");
        if(md5sum == NULL) {
            exit(1);
        }
        if(fgets(result,MAX_HASH,md5sum) == NULL) {
            pclose(md5sum);
            exit(1);
        }
        else {
            printf("%s\n",result);
        }
        pclose(md5sum);        
    }
    exit(0);
}
