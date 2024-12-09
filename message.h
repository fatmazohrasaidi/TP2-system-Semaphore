#ifndef MESSAGE_H
#define MESSAGE_H

#include <sys/ipc.h>
#include <sys/msg.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

// Define the message queue key
#define MSG_KEY 1234

// Function to initialize the message queue
int init_message_queue() {
    int msgid;

    // Create the message queue
    msgid = msgget(ftok("message.h", MSG_KEY), IPC_CREAT | 0666); // Permissions: read/write for everyone
    if (msgid == -1) {
        perror("Error initializing message queue");
        exit(1);
    }

    printf("Message queue created with ID: %d\n", msgid);
    return msgid;
}

// Function to delete the message queue
void delete_message_queue(int msgid) {
    if (msgctl(msgid, IPC_RMID, NULL) == -1) {
        perror("Error deleting message queue");
        exit(1);
    }

    printf("Message queue with ID %d deleted successfully.\n", msgid);
}

#endif // MESSAGE_H

