#ifndef SHARED_MEMORY_H
#define SHARED_MEMORY_H

#include <stdio.h>
#include <stdlib.h>
#include <sys/shm.h>
#include <sys/types.h>

/*************************************************************create shared memory********************************************************************/
int cree_mem_partage(key_t key, size_t size) {
    int shmid = shmget(key, size, IPC_CREAT | 0666);
    if (shmid == -1) {
        perror("Echec de shmget");
        exit(1);
    }
    return shmid;
}

/*************************************************************attach shared memory********************************************************************/
struct tampon * attache_mem_partage(int shmid) {
    struct tampon  *mem = shmat(shmid, NULL, 0);
    if (mem == (struct tampon *)-1) {
        perror("Echec de shmat");
        exit(1);
    }
    return mem;
}

/*************************************************************detach shared memory********************************************************************/
void detache_mem_partage(struct tampon * mem) {
    if (shmdt(mem) == -1) {
        perror("Echec lors du détachement");
        exit(1);
    }
}

/*************************************************************destroy shared memory********************************************************************/
void detruire_mem_partage(int shmid) {
    if (shmctl(shmid, IPC_RMID, NULL) == -1) {
        perror("Echec lors de la destruction de la mémoire partagée");
        exit(1);
    }
}

#endif // SHARED_MEMORY_H

