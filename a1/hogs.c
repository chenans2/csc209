#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

int main(int argc, char *argv[]) {
	
	char user[127];
	int pid = 0;
	float cpu = 0;
	float mem = 0;
	int vsz = 0;
	int rss = 0;
	char tty[99];
	char stat[99];
	char start[99];
	char time[99];
	char command[127];
	
	char *mode;
	int found = 0;
	char *user_select;
	int pid_high = 0;
	float cpu_high = -1;
	float mem_high = -1;
	char command_high[127] = "";
	
	if (argc == 1 || argc > 3 ) {
		printf("Invalid number of arguments.\nUsage: hogs [-c | -m] file...\n");
        return 1;
		
	} else {
		
		// Check whether -c or -m
		if (argc == 3) {
			user_select = argv[2];
			if (strcasecmp("-c", argv[1]) == 0) {
				mode = "-c";
			} else if (strcasecmp("-m", argv[1]) == 0) {
				mode = "-m";
			} else {
				printf("Invalid arguments.\nUsage: hogs [-c | -m] file...\n");
				return 1;
			}
			
		// User did not input [-c | -m] argument, default to -c
		} else {
			mode = "-c";
			user_select = argv[1];
		}
		
		// Read stdin
		while (scanf("%s", user) != EOF) {
			scanf("%d", &pid);
			scanf("%f", &cpu);
			scanf("%f", &mem);
			scanf("%d", &vsz);
			scanf("%d", &rss);
			scanf("%s", tty);
			scanf("%s", stat);
			scanf("%s", start);
			scanf("%s", time);
			scanf("%127[^\n]", command);
			
			// User matches
			if (strcmp(user_select, user) == 0) {
				found = 1;
				
				// Current process has higher cpu
				if ((cpu > cpu_high) && (strcmp("-c", mode) == 0)) {
					pid_high = pid;
					cpu_high = cpu;
					strcpy(command_high, command);
					
				// Current process has higher mem
				} else if ((mem > mem_high) && (strcmp("-m", mode)) == 0) {
					pid_high = pid;
					mem_high = mem;
					strcpy(command_high, command);
					
				// Current process has same cpu
				} else if ((cpu == cpu_high) && (strcmp("-c", mode) == 0)) {
					
					// Check which command comes alphabetically first
					if (strcasecmp(command_high, command) > 0) {
						pid_high = pid;
						cpu_high = cpu;
						strcpy(command_high, command);
					}
				
				// Current process has same mem
				} else if ((mem == mem_high) && (strcmp("-m", mode) == 0)) {
					
					// Check which command comes alphabetically first
					if (strcasecmp(command_high, command) > 0) {
						pid_high = pid;
						mem_high = mem;
						strcpy(command_high, command);
					}
				} else {
					//do nothing
				}
			}
		}
		
		// Print only if the user was found at least once
		if (found == 1) {
			if (strcmp("-c", mode) == 0) {
				printf("%d\t%.1f\t%s\n", pid_high, cpu_high, command_high);
			} else {
				printf("%d\t%.1f\t%s\n", pid_high, mem_high, command_high);
			}
		}
		
		return 0;	
	}
}