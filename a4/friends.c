#include "friends.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/*
 * Create a new user with the given name.  Insert it at the tail of the list 
 * of users whose head is pointed to by *user_ptr_add.
 *
 * Return:
 *   - 0 on success.
 *   - 1 if a user by this name already exists in this list.
 *   - 2 if the given name cannot fit in the 'name' array
 *       (don't forget about the null terminator).
 */
int create_user(const char *name, User **user_ptr_add) {
    if (strlen(name) >= MAX_NAME) {
        return 2;
    }

    User *new_user = malloc(sizeof(User));
    if (new_user == NULL) {
        perror("malloc");
        exit(1);
    }
    strncpy(new_user->name, name, MAX_NAME); // name has max length MAX_NAME - 1

    for (int i = 0; i < MAX_NAME; i++) {
        new_user->profile_pic[i] = '\0';
    }

    new_user->first_post = NULL;
    new_user->next = NULL;
    for (int i = 0; i < MAX_FRIENDS; i++) {
        new_user->friends[i] = NULL;
    }

    // Add user to list
    User *prev = NULL;
    User *curr = *user_ptr_add;
    while (curr != NULL && strcmp(curr->name, name) != 0) {
        prev = curr;
        curr = curr->next;
    }

    if (*user_ptr_add == NULL) {       // bug fixed 03/04/2016. Now correct on repeat of 1st name
        *user_ptr_add = new_user;
        return 0;
    } else if (curr != NULL) {
        free(new_user);
        return 1;
    } else {
        prev->next = new_user;
        return 0;
    }
}


/* 
 * Return a pointer to the user with this name in
 * the list starting with head. Return NULL if no such user exists.
 *
 * NOTE: You'll likely need to cast a (const User *) to a (User *)
 * to satisfy the prototype without warnings.
 */
User *find_user(const char *name, const User *head) {
/*    const User *curr = head;
    while (curr != NULL && strcmp(name, curr->name) != 0) {
        curr = curr->next;
    }

    return (User *)curr;
*/
    while (head != NULL && strcmp(name, head->name) != 0) {
        head = head->next;
    }

    return (User *)head;
}


/*
 * Print the usernames of all users in the list starting at curr.
 * Names should be printed to standard output, one per line.
 */
char *list_users(const User *curr) {
    const User *head = curr; // save the head of the linked list
    int total_length = 0;
    
    // loop to find total_length
    while (curr != NULL) {
        total_length += (strlen(curr->name) + strlen("\r\n"));
        curr = curr->next;
    }
    total_length += strlen("\0");
    
    // allocate the space
    char *buf = malloc(sizeof(char) * (total_length));    
    if (buf == NULL) {
        perror("malloc");
        exit(1);
    }

    curr = head;
    snprintf(buf, sizeof(curr->name), "%s\r\n", curr->name);
    // loop again to put everything into buf
    curr = curr->next;

    while (curr != NULL) {
        snprintf(buf + strlen(buf), sizeof(curr->name), "%s\r\n", curr->name);
        curr = curr->next;
    }
    
    return buf;
}



/* 
 * Make two users friends with each other.  This is symmetric - a pointer to 
 * each user must be stored in the 'friends' array of the other.
 *
 * New friends must be added in the first empty spot in the 'friends' array.
 *
 * Return:
 *   - 0 on success.
 *   - 1 if the two users are already friends.
 *   - 2 if the users are not already friends, but at least one already has
 *     MAX_FRIENDS friends.
 *   - 3 if the same user is passed in twice.
 *   - 4 if at least one user does not exist.
 *
 * Do not modify either user if the result is a failure.
 * NOTE: If multiple errors apply, return the *largest* error code that applies.
 */
int make_friends(const char *name1, const char *name2, User *head) {
    User *user1 = find_user(name1, head);
    User *user2 = find_user(name2, head);

    if (user1 == NULL || user2 == NULL) {
        return 4;
    } else if (user1 == user2) { // Same user
        return 3;
    }

    int i, j;
    for (i = 0; i < MAX_FRIENDS; i++) {
        if (user1->friends[i] == NULL) { // Empty spot
            break;
        } else if (user1->friends[i] == user2) { // Already friends.
            return 1;
        }
    }

    for (j = 0; j < MAX_FRIENDS; j++) {
        if (user2->friends[j] == NULL) { // Empty spot
            break;
        } 
    }

    if (i == MAX_FRIENDS || j == MAX_FRIENDS) { // Too many friends.
        return 2;
    }

    user1->friends[i] = user2;
    user2->friends[j] = user1;
    return 0;
}


/* 
 * Print a user profile.
 * For an example of the required output format, see the example output
 * linked from the handout.
 * Return:
 *   - 0 on success.
 *   - 1 if the user is NULL.
 */
char *print_user(const User *user) {
    if (user == NULL) {
        return NULL;
    }
    
    int total_length = 0;
    
    // find total_length
    total_length += (strlen("Name: \r\n\r\n") + 1);
    total_length += (strlen(user->name) + 1);
    total_length += (strlen("------------------------------------------\r\n") + 1);
    
    total_length += (strlen("Friends:\r\n") + 1);
    for (int i = 0; i < MAX_FRIENDS && user->friends[i] != NULL; i++) {
        total_length += (strlen("\r\n") + 1);
        total_length += (strlen(user->friends[i]->name) + 1);
    }
    total_length += (strlen("------------------------------------------\r\n") + 1);
    
    total_length += (strlen("Posts:\r\n") + 1);
    const Post *curr = user->first_post;
    while (curr != NULL) {
        total_length += (strlen("From: \r\n") + 1);
		total_length += (strlen(curr->author) + 1);
		total_length += (strlen("Date: \r\n") + 1);
		total_length += (strlen(asctime(localtime(curr->date))) + 1);
		total_length += (strlen("\r\n") + 1);
		total_length += (strlen(curr->contents) + 1);
        curr = curr->next;
        if (curr != NULL) {
            total_length += (strlen("\r\n===\r\n\r\n") + 1);
        }
    }
    total_length += (strlen("------------------------------------------\r\n") + 1);
    
    // allocate the space
    char *buf = malloc(sizeof(char) * (total_length));    
    if (buf == NULL) {
        perror("malloc");
        exit(1);
    }
    
    // put name into buf
    snprintf(buf, sizeof(user->name), "Name: %s\r\n\r\n", user->name);
    snprintf(buf + strlen(buf), sizeof("------------------------------------------\r\n"), 
    "------------------------------------------\r\n");
    
    // put friend list into buf
    snprintf(buf + strlen(buf), sizeof("Friends:\r\n"), "Friends:\r\n");
    for (int j = 0; j < MAX_FRIENDS && user->friends[j] != NULL; j++) {
        snprintf(buf + strlen(buf), sizeof(user->friends[j]->name), 
        "%s\r\n", user->friends[j]->name);
    }
    snprintf(buf + strlen(buf), sizeof("------------------------------------------\r\n"), 
    "------------------------------------------\r\n");
    
    // put post list into buf
    snprintf(buf + strlen(buf), sizeof("Posts:\r\n"), "Posts:\r\n");
    const Post *curr2 = user->first_post;
    while (curr2 != NULL) {
		snprintf(buf + strlen(buf), sizeof(curr2->author), "From: %s\r\n", curr2->author);
		sprintf(buf + strlen(buf), "Date: %s\r\n", asctime(localtime(curr2->date)));
		sprintf(buf + strlen(buf), "%s\r\n", curr2->contents);
        curr2 = curr2->next;
        if (curr2 != NULL) {
            snprintf(buf + strlen(buf), sizeof("\r\n===\r\n\r\n"), "\r\n===\r\n\r\n");
        }
    }
    // 44
    snprintf(buf + strlen(buf), sizeof("------------------------------------------\r\n"), 
    "------------------------------------------\r\n");
    
    return buf;
}


/*
 * Make a new post from 'author' to the 'target' user,
 * containing the given contents, IF the users are friends.
 *
 * Insert the new post at the *front* of the user's list of posts.
 *
 * Use the 'time' function to store the current time.
 *
 * 'contents' is a pointer to heap-allocated memory - you do not need
 * to allocate more memory to store the contents of the post.
 *
 * Return:
 *   - 0 on success
 *   - 1 if users exist but are not friends
 *   - 2 if either User pointer is NULL
 */
int make_post(const User *author, User *target, char *contents) {
    if (target == NULL || author == NULL) {
        return 2;
    }

    int friends = 0;
    for (int i = 0; i < MAX_FRIENDS && target->friends[i] != NULL; i++) {
        if (strcmp(target->friends[i]->name, author->name) == 0) {
            friends = 1;
            break;
        }
    }

    if (friends == 0) {
        return 1;
    }

    // Create post
    Post *new_post = malloc(sizeof(Post));
    if (new_post == NULL) {
        perror("malloc");
        exit(1);
    }
    strncpy(new_post->author, author->name, MAX_NAME);
    new_post->contents = contents;
    new_post->date = malloc(sizeof(time_t));
    if (new_post->date == NULL) {
        perror("malloc");
        exit(1);
    }
    time(new_post->date);
    new_post->next = target->first_post;
    target->first_post = new_post;

    return 0;
}

