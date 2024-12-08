#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/wait.h>
#include <unistd.h>

int semid;

union semun {
    int val;
    struct semid_ds *buf;
    ushort *array;
};

/********************** CREATE NAMED SEMAPHORES ********************/
int sem_create(key_t cle, int nbSemaphore, int initValues[]) {
    union semun arg_ctl;

    // Create the semaphore set
    semid = semget(ftok("dijkstra.h", cle), nbSemaphore, IPC_CREAT | IPC_EXCL | 0666);
    if (semid == -1) {
        semid = semget(ftok("dijkstra.h", cle), nbSemaphore, 0666); // Get existing semaphore set
        if (semid == -1) {
            perror("Erreur semget()");
            exit(1);
        }
    }

    // Initialize each semaphore in the set
    for (int i = 0; i < nbSemaphore; i++) {
        arg_ctl.val = initValues[i];
        if (semctl(semid, i, SETVAL, arg_ctl) == -1) {
            perror("Erreur initialisation semaphore");
            exit(1);
        }
    }

    return semid; // Return the semaphore set ID
}

/********************** P() ********************/
void P(int semid, int sem_num) {
    struct sembuf sempar;
    sempar.sem_num = sem_num;
    sempar.sem_op = -1;
    sempar.sem_flg = SEM_UNDO;
    if (semop(semid, &sempar, 1) == -1)
        perror("Erreur operation P");
}

/********************** V() ********************/
void V(int semid, int sem_num) {
    struct sembuf sempar;
    sempar.sem_num = sem_num;
    sempar.sem_op = 1;
    sempar.sem_flg = SEM_UNDO;
    if (semop(semid, &sempar, 1) == -1)
        perror("Erreur operation V");
}

/********************** SEM_DELETE() ********************/
void sem_delete(int semid) {
    if (semctl(semid, 0, IPC_RMID, 0) == -1)
        perror("Erreur dans destruction semaphore");
}

