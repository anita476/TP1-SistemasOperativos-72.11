// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http:\/\/www.viva64.com

#include <stdio.h>
#include <stdlib.h>

int main(int argc, char * argv[]){
    if(argc == 0){
        fprintf(stderr, "Please input at least one file path");
        exit(1); 
    }
    exit(0);

}