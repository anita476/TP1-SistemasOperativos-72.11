// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http:\/\/www.viva64.com

#include "lib.h"

int main(int argc, char * argv[]) {
    if(argc == 0) {
        fprintf(stderr, "Usage: ./md5 path\n");
        exit(1); 
    }
    exit(0);
}