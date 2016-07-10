#include "friends.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/*
 * Create a new user with the given name.  Insert it at the tail of the list
 * of users whose head is pointed to by *user_ptr_add.
 *
 * Return:
 *   - 0 if successful
 *   - 1 if a user by this name already exists in this list
 *   - 2 if the given name cannot fit in the 'name' array
 *       (don't forget about the null terminator)
 */
int create_user(const char *name, User **user_ptr_add) {
    
    // check if name is valid length (MAX_NAME 32) 
    if (strlen(name) >= 32) {
        return 2;
    }
    
    User *curr = *user_ptr_add;    // dereference first node of list of users
    User *prev = NULL;
    // create the user and set initial values
    User *new_user = malloc(sizeof(User));
    // check the return value from malloc and terminate the program 
    // with a non-zero status if malloc fails
    if (new_user == NULL) {
        return -1;
    }
    strncpy(new_user->name, name, MAX_NAME);
    strcpy(new_user->profile_pic, "");
    new_user->first_post = NULL;
    // friend list initially has all NULL users (prevent errors later on)
    for (int i = 0; i < 10 ; i++) {
        new_user->friends[i] = NULL;
    }
    new_user->next = NULL;
    
    // the user_list is empty
    if (curr == NULL) {
        *user_ptr_add = new_user;
    } else {
        // traverse linked list until the last node
        while (curr != NULL) {
            // check if user by this name already exists
            if (strcmp(curr->name, name) == 0) {
                return 1;
            }
            prev = curr;
            curr = curr->next;
        }
        prev->next = new_user;
    }
    return 0;
}


/*
 * Return a pointer to the user with this name in
 * the list starting with head. Return NULL if no such user exists.
 *
 * NOTE: You'll likely need to cast a (const User *) to a (User *)
 * to satisfy the prototype without warnings.
 */
User *find_user(const char *name, const User *head) {
    while (head != NULL) {
        if (strcmp(head->name, name) == 0) {
            return (User *)head;
        }
        head = head->next;
    }
    return NULL;
}


/*
 * Print the usernames of all users in the list starting at curr.
 * Names should be printed to standard output, one per line.
 */
void list_users(const User *curr) {
    // pointer to first node of the list
    printf("User List\n");
    while (curr != NULL) {
        printf("  %s\n", curr->name);
        curr = curr->next;
    }
}


/*
 * Change the filename for the profile pic of the given user.
 *
 * Return:
 *   - 0 on success.
 *   - 1 if the file does not exist.
 *   - 2 if the filename is too long.
 */
int update_pic(User *user, const char *filename) {
    
    // check if filename is valid length (MAX_NAME 32)
    if (strlen(filename) >= 32) {
        return 2;
    }
    // check if file exists
    FILE *pic_file = fopen(filename, "r");
    if (pic_file != NULL) {
        strncpy(user->profile_pic, filename, MAX_NAME);
        fclose(pic_file);
        return 0;
    } else { 
        // file doesn't exist
        return 1;
    }
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
    } else if (strcmp(name1, name2) == 0) {
        return 3;
    } else {
        // traverse through array of friends to check if already friends
        // only need to check one user
        int a = 0;
        while (user1->friends[a]->name != NULL) {
            if (strcmp(user1->friends[a]->name, name2) == 0) {
                return 1;
            }
            a++;
        }
        
        // check if one already has MAX_FRIENDS friends
        if (user1->friends[9]->name != NULL || 
            user2->friends[9]->name != NULL) {
            return 2;
        }
            
        // add both users to each other's friends list
        int j = 0;
        while (user1->friends[j]->name != NULL) {
            j++;
        }
        user1->friends[j] = user2;
        
        int k = 0;
        while (user2->friends[k]->name != NULL) {
            k++;
        }
        user2->friends[k] = user1;

        return 0;
    }
}


/*
 * Print a user profile.
 * For an example of the required output format, see the example output
 * linked from the handout.
 * Return:
 *   - 0 on success.
 *   - 1 if the user is NULL.
 */
int print_user(const User *user) {
    if (user == NULL) {
        return 1;
    } else {
        FILE *pic_file;
        int c = 0;
        // check if user set a profile pic, read if yes
        if (strcmp(user->profile_pic, "") != 0) {
            pic_file = fopen(user->profile_pic, "r");
            // check if pic_file is still accessable
            if (pic_file != NULL) {
                while(1) {
                    c = fgetc(pic_file);
                    if (feof(pic_file)) {
                        break;
                    }
                    printf("%c", c);
                }
                fclose(pic_file);
                printf("\n\n");
            }
        }
        
        printf("Name: %s\n\n------------------------------------------\n"
                "Friends: ", user->name);
        // for each element in friend list array, print the name
        int i = 0;
         while (user->friends[i]->name != NULL && i < 10) {
            printf("\n%s", user->friends[i]->name);
            i++; 
        }
        
        printf("\n------------------------------------------\nPosts: ");
        // for each post in linked list, print the author, date and contents
        int first_post = 1;
        Post *curr_post = user->first_post;
        while (curr_post != NULL) {
            if (first_post != 1) {
                printf("\n\n===\n");
            }
            printf("\nFrom: %s\nDate: %s\n%s", curr_post->author, 
            asctime(localtime(curr_post->date)), curr_post->contents);
            curr_post = curr_post->next;
            first_post = 0;
        }
        
        printf("\n------------------------------------------\n");
        return 0;
    }
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
    // check if either User pointer is NULL
    if (author == NULL || target == NULL) {
        return 2;
    } else {
        // traverse through array of friends to check if friends
        // only need to check one User (i.e. author)
        int i = 0;
        while (author->friends[i]->name != NULL) {
            if (strcmp(author->friends[i]->name, target->name) == 0) {
                 // confirmed friends, create the post and return 0
                Post *new_post = malloc(sizeof(Post));
                // check the return value from malloc and terminate the program 
                // with a non-zero status if malloc fails
                if (new_post == NULL) {
                    return -1;
                }
                
                strncpy(new_post->author, author->name, MAX_NAME);
                new_post->contents = contents;
                new_post->date = malloc(sizeof(time_t));
                if (new_post->date == NULL) {
                    return -1;
                }
                
                time(new_post->date); // Get the system time
                new_post->next = target->first_post;
                target->first_post = new_post;
                return 0;
            }
            i++;
        } 
        return 1;
    }
}


/*
 * From the list pointed to by *user_ptr_del, delete the user
 * with the given name.
 * Remove the deleted user from any lists of friends.
 *
 * Return:
 *   - 0 on success.
 *   - 1 if a user with this name does not exist.
 */
int delete_user(const char *name, User **user_ptr_del) {
    User *curr = *user_ptr_del;    // dereference first node of list of users
    User *prev = NULL;
    while (curr != NULL) {
        // find the user to delete
        if (strcmp(curr->name, name) == 0) {
            // remove the user to delete from any lists of friends
            int i = 0;
            while (curr->friends[i] != NULL) {
                // find friend
                User *old_friend = find_user(curr->friends[i]->name, 
                *user_ptr_del);
                int j = 0;
                // interate through their friend list to find user to delete
                while (old_friend->friends[j] != NULL) {
                    if (strcmp(old_friend->friends[j]->name, name) == 0) {
                        // replace with the friend right after in list
                        old_friend->friends[j] = old_friend->friends[j + 1];
                    }
                    j++;
                }
                i++;
            }
            
            // delete the user's posts and deallocate the memory on heap
            Post *curr_post = curr->first_post;
            Post *prev_post = NULL;
            while (curr_post != NULL) {
                prev_post = curr_post;
                curr_post = curr_post->next;
                free(prev_post->date);
                free(prev_post->contents);
                free(prev_post);
            }
            
            // change pointer to point to next user
            if (prev == NULL) {
                *user_ptr_del = curr->next;
            } else {
                prev->next = curr->next; 
            }
            
            free(curr);    // deallocate the memory on heap for this user
            return 0;
        }
        prev = curr;
        curr = curr->next;
    }
    return 1;
}
