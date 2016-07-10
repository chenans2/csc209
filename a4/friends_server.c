#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "friends.h"
#include <sys/socket.h>
#include <sys/select.h>

#define MAX_NAME 32 
#define INPUT_BUFFER_SIZE 256
#define INPUT_ARG_MAX_NUM 12
#define DELIM " \n"

#ifndef PORT
    #define PORT 53692
#endif

/* 
 * Client struct to support multiple file descriptors
 * Taken from sample server
 */
struct client {
    int fd;
    char name[MAX_NAME];
    struct in_addr ipaddr;
    struct client *next;
    int user_flag;
} *top = NULL;

static User *user_list = NULL;
static User *user = NULL;
static int listenfd;
static void addclient(int fd, struct in_addr addr);
static void removeclient(int fd);

/* 
 * Print a formatted error message to stderr.
 */
void error(char *msg) {
    fprintf(stderr, "Error: %s\n", msg);
}

/* 
 * Read and process commands, taken from friendme.c and modified slightly
 * Return:  -1 for quit command
 *          0 otherwise
 */
int process_args(int cmd_argc, char **cmd_argv, int fd, int user_flag) {
    char message[INPUT_BUFFER_SIZE];

    if (cmd_argc <= 0) {
        return 0;
    } else if (strcmp(cmd_argv[0], "quit") == 0 && cmd_argc == 1) {
        return -1;
    } else if (user_flag == 1) {
        // truncate user name
        if (strlen(cmd_argv[0]) > 31) {
            cmd_argv[0][30] = '\n';
            cmd_argv[0][31] = '\0';
        }
        if (create_user(cmd_argv[0], &user_list) == 1) {    // create user and put in user_list
            write(fd, "Welcome back.\r\nGo ahead and enter user commands>\r\n",
                sizeof("Welcome back.\r\nGo ahead and enter user commands>\r\n"));
        } else {
            write(fd, "Welcome.\r\nGo ahead and enter user commands>\r\n",
                sizeof("Welcome.\r\nGo ahead and enter user commands>\r\n"));
        }
    } else if (strcmp(cmd_argv[0], "list_users") == 0 && cmd_argc == 1) {
        char *list_buf = list_users(user_list);
        write(fd, list_buf, strlen(list_buf));
        free(list_buf);
    } else if (strcmp(cmd_argv[0], "make_friends") == 0 && cmd_argc == 2) {
        switch (make_friends(cmd_argv[1], user->name, user_list)) {
            case 0:
                strcpy(message, "You are now friends with ");
                strcat(message, cmd_argv[1]);
                strcat(message, ".\r\n");
                write(fd, message, strlen(message));
                
                struct client *curr = top;
                while (curr != NULL) {
                    if (strcmp(curr->name, cmd_argv[1]) == 0) {
                        strcpy(message, "You have been friended by ");
                        strcat(message, user->name);
                        strcat(message, ".\r\n");
                        write(curr->fd, message, strlen(message));
                    }
                    curr = curr->next;
                }
                break;
            case 1:
                write(fd, "You are already friends\r\n", 
                    sizeof("You are already friends\r\n"));
                break;
            case 2:
                write(fd, "At least one user you entered has the max number of friends\r\n", 
                    sizeof("At least one user you entered has the max number of friends\r\n"));
                break;
            case 3:
                write(fd, "You can't friend yourself\r\n", 
                    sizeof("You can't friend yourself\r\n"));
                break;
            case 4:
                write(fd, "The user you entered does not exist\r\n",
                    sizeof("The user you entered does not exist\r\n"));
                break;
        }
    } else if (strcmp(cmd_argv[0], "post") == 0 && cmd_argc >= 3) {
        // first determine how long a string we need
        int space_needed = 0;
        for (int i = 3; i < cmd_argc; i++) {
            space_needed += strlen(cmd_argv[i]) + 1;
        }

        // allocate the space
        char *contents = malloc(space_needed);
        if (contents == NULL) {
            perror("malloc");
            exit(1);
        }

        // copy in the bits to make a single string
        strcpy(contents, cmd_argv[2]);
        for (int i = 3; i < cmd_argc; i++) {
            strcat(contents, " ");
            strcat(contents, cmd_argv[i]);
        }

        User *author = find_user(user->name, user_list);
        User *target = find_user(cmd_argv[1], user_list);
        switch (make_post(author, target, contents)) {
            case 0:
                strcpy(message, "From ");
                struct client *curr = top;
                while (curr != NULL) {
                    if (strcmp(curr->name, cmd_argv[1]) == 0) {
                        strcat(message, user->name);
                        strcat(message, ": ");
                        strcat(message, contents);
                        strcat(message, "\r\n");
                        write(curr->fd, message, strlen(message));
                    }
                    curr = curr->next;
                }
                break;
            case 1:
                write(fd, "You can only post to your friends\r\n", 
                    sizeof("You can only post to your friends\r\n"));
                break;
            case 2:
                write(fd, "The user you want to post to does not exist\r\n",
                    sizeof("The user you want to post to does not exist\r\n"));
                break;
        }
    } else if (strcmp(cmd_argv[0], "profile") == 0 && cmd_argc == 2) {
        User *curr = find_user(cmd_argv[1], user_list);
        char *profile_buf = NULL;
        profile_buf = print_user(curr);
        if (profile_buf == NULL) {
            write(fd, "User not found\r\n", sizeof("Useaa   afterr not found\r\n"));
        } else {
            write(fd, profile_buf, strlen(profile_buf));
        }
        free(profile_buf);
    } else {
        write(fd, "Incorrect syntax\r\n", sizeof("Incorrect syntax\r\n"));
    }
    return 0;
}


/*
 * Tokenize the string stored in cmd.
 * Return the number of tokens, and store the tokens in cmd_argv.
 */
int tokenize(char *cmd, char **cmd_argv) {
    int cmd_argc = 0;
    char *next_token = strtok(cmd, DELIM);    
    while (next_token != NULL) {
        if (cmd_argc >= INPUT_ARG_MAX_NUM - 1) {
            error("Too many arguments!");
            cmd_argc = 0;
            break;
        }
        cmd_argv[cmd_argc] = next_token;
        cmd_argc++;
        next_token = strtok(NULL, DELIM);
    }
    return cmd_argc;
}


/*
 * Set up the server.
 * Taken from lab11 bufserver.c
 */
void setup(void) {
    int on = 1, status;
    struct sockaddr_in self;

    if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket");
        exit(1);
    }

    // Make sure we can reuse the port immediately after the
    // server terminates.
    status = setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, (const char *) &on, sizeof(on));
    if(status == -1) {
        perror("setsockopt -- REUSEADDR");
    }

    self.sin_family = AF_INET;
    self.sin_addr.s_addr = INADDR_ANY;
    self.sin_port = htons(PORT);
    memset(&self.sin_zero, 0, sizeof(self.sin_zero)); // Initialize sin_zero to 0

    //printf("Listening on %d\n", PORT);

    if (bind(listenfd, (struct sockaddr *)&self, sizeof(self)) == -1) {
        perror("bind"); // probably means port is in use
        exit(1);
    }

    if (listen(listenfd, 5) == -1) {
        perror("listen");
        exit(1);
    }
}

/* 
 * Search the first inbuf characters of buf for a network newline ("\r\n").
 * Return the location of the '\r' if the network newline is found,
 * or -1 otherwise.
 */
int find_network_newline(const char *buf, int inbuf) {
    for (int i = 0; i < inbuf - 1; i++) {
        if (buf[i] == '\r' && buf[i + 1] == '\n') {
            return i; // return the location of '\r' if found
        }
    }
    return -1; 
}

/* 
 * Accept connection, and add client.
 */
void newconnection() {
    int fd;
    struct sockaddr_in peer;
    socklen_t socklen = sizeof(peer);

    if ((fd = accept(listenfd, (struct sockaddr *)&peer, &socklen)) < 0) {
        perror("accept");
    } else {
        //printf("New connection on port %d\n", ntohs(peer.sin_port));
        addclient(fd, peer.sin_addr);
        // prompt client for user name
        if (write(fd, "What is your user name?\r\n", 
                    sizeof("What is your user name?\r\n")) == -1) {
            perror("write to pipe");
        }
    }
}

/* 
 * Read input
 */
void whatsup(struct client *p) {
    int nbytes;
    int inbuf;      // number of bytes currently in buffer
    int room;       // room left in buffer
    char *after;    // pointer to position after the (valid) data in buf
    int where;      // location of network newline
    char buf[INPUT_BUFFER_SIZE];
    int flag = 0;
 
    // Receive messages
    inbuf = 0;          // buffer is empty; has no bytes
    room = sizeof(buf); // room == capacity of the whole buffer
    after = buf;        // start writing at beginning of buf

    while ((nbytes = read(p->fd, after, room)) > 0) {
        flag = 0;

        // update inbuf with nbytes (the number of bytes just read)
        inbuf += nbytes; 

        // call find_network_newline, store result in variable "where"
        where = find_network_newline(buf, inbuf);

        if (where >= 0) { // we have a full line
            buf[where] = '\0';

            char *cmd_argv[INPUT_ARG_MAX_NUM];
            int cmd_argc = tokenize(buf, cmd_argv);
            user = find_user(p->name, user_list);

            if (cmd_argc > 0 && process_args(cmd_argc, cmd_argv, 
                p->fd, p->user_flag) == -1) {
                flag = 1;
                break; // can only reach if quit command was entered
            }

            if (p->user_flag == 1) {
                user = find_user(buf, user_list);
                strncpy(p->name, user->name, MAX_NAME);
                p->user_flag = 0;
            }

            break;

            // memmove(destination, source, number_of_bytes)
            inbuf -= (where + 2);
            if (inbuf > 0){
                memmove(buf, &buf[where + 2], inbuf); // moves buf to beginning
            }

        }
        // update room and after, in preparation for the next read
        room = sizeof(buf) - inbuf; 
        after = &buf[inbuf];
    }
    if (flag == 1){
        close(p->fd);
        removeclient(p->fd);
    }
}

/* 
 * Taken from sample server by Alan J Rosenthal.
 */
static void addclient(int fd, struct in_addr addr) {
    struct client *p = malloc(sizeof(struct client));

    if (!p) {
        fprintf(stderr, "out of memory!\n");
        exit(1);
    }

    //printf("Adding client %s\n", inet_ntoa(addr));
    fflush(stdout);
    p->fd = fd;
    p->user_flag = 1;
    p->ipaddr = addr;
    p->next = top;
    top = p;
}

/* 
 * Taken from sample server by Alan J Rosenthal.
 */
static void removeclient(int fd) {
    struct client **p;
    for (p = &top; *p && (*p)->fd != fd; p = &(*p)->next);
    if (*p) {
        struct client *t = (*p)->next;
        //printf("Removing client %s\n", inet_ntoa((*p)->ipaddr));
        fflush(stdout);
        free(*p);
        *p = t;
    } else {
        fprintf(stderr, "Trying to remove fd %d, but I don't know about it\n", fd);
        fflush(stderr);
    }
}

/* 
 * Majority taken from sample server by Alan J Rosenthal.
 */
int main() {
    struct client *p;
    extern void setup(), newconnection(), whatsup(struct client *p);

    setup();

    // the only way the server exits is by being killed
    while (1) {
        fd_set fdlist;  // set of active sockets
        int maxfd = listenfd;
        FD_ZERO(&fdlist); // initializes the file descriptor set set to be the empty set
        FD_SET(listenfd, &fdlist); // adds listenfd to the file descriptor set fdlist 

        for (p = top; p; p = p->next) {
            FD_SET(p->fd, &fdlist);
            if (p->fd > maxfd) {
                maxfd = p->fd;
            }
        }

        if (select(maxfd + 1, &fdlist, NULL, NULL, NULL) < 0) {
            perror("select");
        } else {
            // loops through file descriptors
            for (p = top; p; p = p->next) {
                if (FD_ISSET(p->fd, &fdlist)) {
                    break;
                }
            }
            if (p) {
                whatsup(p);
            }
            if (FD_ISSET(listenfd, &fdlist)) {
                newconnection();
            }
        }
    }
    return 0;
}