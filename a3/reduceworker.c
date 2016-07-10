#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include "mapreduce.h"

/*
 * Reduce worker process
 */
void reduce_worker(int outfd, int infd) {
    
	LLKeyValues *curr = NULL; 
    Pair new_pair;
	
    // Read until there are no more pairs in pipe 
    while (read(infd, &curr, sizeof(LLKeyValues *)) > 0) {
        new_pair = reduce(curr->key, curr->head_value);
		write(outfd, &new_pair, sizeof(Pair));
        //fwrite(new_pair.key, MAX_KEY, 1, output_file);
        //fwrite(new_pair.value, MAX_VALUE, 1, output_file);
    }

}
