// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http:\/\/www.viva64.com

#include "lib.h"
#include <unistd.h>


// slave starts and reads from stdin , and afterwards writes to stdout
// in mas

int main(){
    // md5 command
    char command[sizeof("md5sum \"\"") + MAX_FILE_PATH];
    // our file path to append
    char filePath[MAX_FILE_PATH];
    // where we'll store the
    char result[MAX_HASH];

    // we read from stands
    // dont remember why the -1 de PI 
    int n;
    while((n = read(STDIN_FILENO, filePath, sizeof(filePath) -1))){
        if(n < 0 ){
            fprintf(stdout, "An error ocurred with file path");
        }
        // here would go the file validation (is it a dir?),,, optional for late
        //append the file path as part of the command
        snprintf(command,sizeof(command),"md5sum \"%s\"", filePath);
        FILE * md5sum = popen(command,"r");
        if(md5sum == NULL){
            fprintf(stderr, "Could not execute md5sum command: %s", command);
            exit(1);
        }
        if(fgets(result,MAX_HASH,md5sum) == NULL){
            fprintf(stderr,"An error occured while parsing md5sum result \n");
            exit(1);
        }
        else{
            printf("%s\n",result);
        }
    }
    exit(0);
}
