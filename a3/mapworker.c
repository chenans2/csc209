#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include "mapreduce.h"
#include "word_freq.c"

/*
 * Map worker process
 */
void map_worker(int outfd, int infd) {

    FILE *input_file;
    char path[MAX_FILENAME] = "";
    char buffer[READSIZE + 1] = ""; // READSIZE + 1 for a final null-terminator
    int error = 0;
    
    // Read until there are no more files in pipe 
    while (read(infd, path, MAX_FILENAME) > 0) {
        input_file = fopen(path, "r");
        if (!input_file) {
            perror("fopen");
            exit(1);
        }
        
        // Process one file
        while (!feof(input_file)) {
            fread(buffer, READSIZE, 1, input_file);
            strncat(buffer, "\0", 1);
            map(buffer, outfd); // Get (key, value) pairs and send to parent
        }
        
        error = fclose(input_file);
        if (error != 0) {
            fprintf(stderr, "fclose failed\n");
            exit(1);
        }
    }
}
