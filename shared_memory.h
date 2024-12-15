#ifndef SHARED_MEMORY_H
#define SHARED_MEMORY_H

#include <stdio.h>
#include <stdlib.h>
#include <sys/shm.h>
#include <sys/types.h>

#define R 3       // Taille de Tfmissions
// Shared memory structure
struct tampon {
    int q;    // Queue pointer (next empty in tampon)
    int cpt;  // Counter for the number of items in the tampon
    struct Tfmissions {
        int camion_id;         // Truck identifier
        int mission_status;    // Mission status (5 if finished)
        int consomation_recente; // Recent consumption
    } tabmission[R]; // Array of missions
};
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
    // Initialize shared memory buffer
    if (mem == (struct tampon *)-1) {
        perror("Echec de shmat");
        exit(1);
    }
    // Initialize shared memory buffer fields
    mem->q = 0; 
    mem->cpt = 0; // No items initially
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
    }else printf("\tla memoire partage detruit avec succes.\n");
}

#endif // SHARED_MEMORY_H

