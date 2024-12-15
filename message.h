#ifndef MESSAGE_H
#define MESSAGE_H

#include <sys/ipc.h>
#include <sys/msg.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

// Define the message queue key


// Function to initialize the message queue
int init_message_queue(key_t key) {
    int msgid;

    // Create the message queue
    msgid = msgget( key, IPC_CREAT | 0666); // Permissions: read/write for everyone
    if (msgid == -1) {
        perror("Error initializing message queue");
        exit(1);
    }

    return msgid;
}

// Function to delete the message queue
void delete_message_queue(key_t key) {
    int msgid = msgget(key, 0666);  // Check if the queue exists

    /*if (msgid == -1) {
        printf("No message queue to delete.\n");
    } else {*/
        if (msgctl(msgid, IPC_RMID, NULL) == -1) {
            perror("Error deleting message queue");
        } else {
            printf("\tFile de message detruit avec succes.\n");
        }
    //}
}
// verifier si la file de message existe deja 
int file_existe(key_t key)
{
	int msgid = msgget(key, 0666); 
	if (msgid != -1) {printf("\n\tfile de message existe!\n");return 1;}//so it already exists
	else return 0;
}






#endif // MESSAGE_H

