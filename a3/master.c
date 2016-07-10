#define _POSIX_C_SOURCE 1

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <sys/wait.h>
#include <sys/types.h>
#include "mapreduce.h"
#include "linkedlist.c"
 
/*
 * Helper function 
 *
 * Check if an argument was entered.
 */
void check_arg(int arg) {
    if (arg == 0) {
        fprintf(stderr, "Usage: mapreduce -d dirname [-m numprocs] [-c numprocs]\n");
        exit(1);
    }
}

/*
 * Helper function 
 *
 * Check if an error occured while closing a file descriptor.
 */
void close_check(int fd) {
    if ((close(fd)) == -1) { 
        perror("close");
        exit(1);
    }
}

/*
 * Helper function 
 *
 * Parent waits for all worker processes in fd_array to finish executing
 * by looping numprocs times.
 */
void wait_workers(int *fd_array, int numprocs) {
    int stillWaiting;
    do {
       stillWaiting = 0;
        for (int w = 0; w < numprocs; w++) {
           if (fd_array[w] > 0) {
              if (waitpid(fd_array[w], NULL, WNOHANG) != 0) {
                 fd_array[w] = 0; // Child is done
              } else {
                 stillWaiting = 1; // Still waiting on this child
              }
           }
           sleep(0);
        }
    } while (stillWaiting);    
}


/*
 * Master process
 */
int main(int argc, char *argv[]) {

    char dirname[MAX_FILENAME];
    int m_numprocs = 2;
    int r_numprocs = 2;
    int d_flag = 0;    // 1 when the user inputed a valid argument for d
    int *all_map_pids = NULL;   // Array of pids for all map_workers
    int *all_re_pids = NULL;    // Array of pids for all map_workers
    Pair pair;
    LLKeyValues *key_values = NULL;
    
    // Use getopt to check and store arguments
    int opt = 0;
    while ((opt = getopt(argc, argv, "r:m:d:")) != -1) {
        switch(opt) {
            case 'd':
                strncpy(dirname, optarg, MAX_FILENAME);
                d_flag = 1;
                break;
            case 'm':
                m_numprocs = strtol(optarg, NULL, 10);
                check_arg(m_numprocs);
                break;
            case 'r':
                r_numprocs = strtol(optarg, NULL, 10);
                check_arg(r_numprocs);
                break;
            default:
                fprintf(stderr, "Usage: mapreduce -d dirname [-m numprocs] [-c numprocs]\n");
                exit(1); 
        }
    }
    
    check_arg(d_flag);

    
    // Create a pipe for ls process
    int ls_fd[2];     // File descriptors for pipes to ls process
    if ((pipe(ls_fd)) == -1) {
        perror("pipe");
        exit(1);
    }

    int ls_pid;    // PID of ls process
    if ((ls_pid = fork()) > 0) {    // Parent process after ls
        
        close_check(ls_fd[1]); // Close the writing end

        // Reset stdin so that it reads from the pipe
        if ((dup2(ls_fd[0], fileno(stdin))) == -1) { 
            perror("dup2");
            exit(1);
        }   

        close_check(ls_fd[0]); // Close the reading end 

        wait(NULL);    // Parent waits for ls process to finish executing
        
        // File descriptors for pipes to map_worker process
        int map_fp_fd[m_numprocs][2];   // from parent (send stuff to child)
        int map_tp_fd[m_numprocs][2];   // to parent (get stuff from child)
        
        for (int g = 0; g < m_numprocs; g++) {
            
            // Create pipe to send stuff to child
            if ((pipe(map_fp_fd[g])) == -1) {
                perror("pipe");
                exit(1);
            }

            // Create pipe to get stuff from child
            if ((pipe(map_tp_fd[g])) == -1) {
                perror("pipe");
                exit(1);
            }
        }

        all_map_pids = malloc(sizeof(int) * m_numprocs);
        int map_pid;  // PID of one map_worker child process
        for (int i = 0; i < m_numprocs; i++) {
            
            if ((map_pid = fork()) > 0)    {    // Parent process
                all_map_pids[i] = map_pid;
                
                // Close read on the "from parent" pipe 
                // (will only write to this pipe - to child)
                close_check(map_fp_fd[i][0]);
                
                // Close write on the "to parent" pipe 
                // (will only read from this pipe - from child)
                close_check(map_tp_fd[i][1]);

                int m = 0; // To identify each map worker
                char path[MAX_FILENAME] = "";
                char file_name[MAX_FILENAME] = "";
                
                // Use scanf to read file names from stdin 
                // until there are no more input files
                while (scanf("%s", file_name) > 0) {
					
                    // Create file path starting at current directory
                    strncpy(path, "./", MAX_FILENAME); 
                    strncpy(path, dirname, MAX_FILENAME);
                    strncat(path, "/", 1);
                    strncat(path, file_name, MAX_FILENAME);
                    strncat(path, "\0", 1);
        
                    // Write input file name to map_worker to the "from parent" pipe
                    if (write(map_fp_fd[m][1], path, MAX_FILENAME) == -1) {
                        perror("write to pipe");
                    }
        
                    m++;
                    // Reached the last map_worker, start at beginning again
                    // to evenly distribute
                    if (m == m_numprocs) {
                        m = 0;
                    }
                }
                
                // Close the writing end of all children processes
                // "from parent" pipe
                close_check(map_fp_fd[i][1]);
                
                // Read pairs from mapworker child
                while (read(map_tp_fd[i][0], &pair, sizeof(Pair)) > 0) {
                    insert_into_keys(&key_values, pair);
                }
                
                // Done reading pairs, close the reading end of all 
                // children processes ("to parent" pipe)
                close_check(map_tp_fd[i][0]);
                
            } else if (map_pid == 0) { // Child process (will run mapworker)
                
                // Close write from parent (will only read from parent)
                close_check(map_fp_fd[i][1]);
                
                // Close read to parent (will only write to parent)
                close_check(map_tp_fd[i][0]);
                
                // Run function, using write to parent and read from parent
                map_worker(map_tp_fd[i][1], map_fp_fd[i][0]);
                
                close_check(map_fp_fd[i][0]); // Close read from parent
                close_check(map_tp_fd[i][1]); // Close write to parent
                exit(0);
                
            } else {
                perror("fork");
                exit(1);
            }
        }
        
        // Parent waits for all map_worker process to finish executing
        wait_workers(all_map_pids, m_numprocs);
        free(all_map_pids);
        
         // File descriptors for pipes to reduce_worker process
        int reduce_fp_fd[r_numprocs][2];   // from parent (send stuff to child)
        for (int h = 0; h < r_numprocs; h++) {
			
            // Create pipe to send stuff to child
            if ((pipe(reduce_fp_fd[h])) == -1) {
                perror("pipe");
                exit(1);
            }
        } 
        
        all_re_pids = malloc(sizeof(int) * r_numprocs);
        int re_pid;  // PID of one reduce_worker child process
        for (int j = 0; j < r_numprocs; j++) {
            if ((re_pid = fork()) > 0) { // Parent process
                all_re_pids[j] = re_pid;

                close_check(reduce_fp_fd[j][0]); // Close read 
                
                if (j == 0) {
                    int r = 0; // Identify each reduce_worker
                    LLKeyValues *curr = key_values; 
                    while (curr != NULL) {
                        if (write(reduce_fp_fd[r][1], &curr, 
                            sizeof(LLKeyValues *)) == -1) {
                            perror("write to pipe");
                        }
                        curr = curr->next;
                        r++;
                        if (r == r_numprocs) {
                            r = 0;
                        }
                    }
                }
				
                close_check(reduce_fp_fd[j][1]); // Finished writing, close write
				
            } else if (re_pid == 0) { // Child process (will run reduce_worker)
			
				FILE *output_file;
				char path[MAX_FILENAME] = "";
				char pid[10];
				int error = 0;
				
				// Create file path starting at current directory
				strcpy(path, "./");
				snprintf(pid, 10, "%d", getpid());   // Convert PID from int to String
				strncat(path, pid, MAX_FILENAME);    // PID as file name
				strncat(path, ".out", MAX_FILENAME);
				strncat(path, "\0", 1);
				
				output_file = fopen(path, "wb"); // Write in binary
				if (!output_file) {
					perror("fopen");
					exit(1);
				} 
        
                close_check(reduce_fp_fd[j][1]); // Close write
				
				// Redirect outfd to file, and infd as read pipe
                reduce_worker(fileno(output_file), reduce_fp_fd[j][0]);
				
                close_check(reduce_fp_fd[j][0]); // Finished reading, close read
				
				// Close file
				error = fclose(output_file);
				if (error != 0) {
					fprintf(stderr, "fclose failed\n");
					exit(1);
				}
				
                exit(0);
				
            } else {
                perror("fork");
                exit(1);
            }
        }

        // Parent waits for all reduce_worker process to finish executing
        wait_workers(all_re_pids, r_numprocs);
        free(all_re_pids);
        free_key_values_list(key_values); // Deallocate memory from the list

    } else if (ls_pid == 0) { // Child process (will run ls)
        
        close_check(ls_fd[0]); // Child won't be reading from pipe
        
        // Reset stdout so that when we write to stdout it goes to the pipe
        if ((dup2(ls_fd[1], fileno(stdout))) == -1) { 
            perror("dup2");
            exit(1);
        }   
        
        close_check(ls_fd[1]); // Close the writing end
        
        // Execute the ls process passing in dirname as an argument
        execl("/bin/ls", "ls", dirname, NULL);
        fprintf(stderr, "Error: exec should not return\n");

    } else {
        perror("fork");
        exit(1);
    }

    return 0;
    
}
