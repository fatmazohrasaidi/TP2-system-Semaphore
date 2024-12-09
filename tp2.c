#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <sys/msg.h>
#include <sys/wait.h>
#include <semaphore.h>
#include "djikstra.h"
#include "shared_memory.h"
#include "message.h"
#include <sys/ipc.h>

// Define CLE and SEM_KEY
#define CLE 1234       // Example value for the key
#define SEM_KEY 5678   // Example value for the semaphore key

#define M 10      // Nombre de poubelles
#define N 5       // Nombre de camions
#define R 3       // Taille de Tfmissions
#define CP 300    // Capacité de carburant
#define MIN 70    // Seuil minimal de carburant
#define C 4       // Consommation par unité de distance
#define MAX 12    // Nombre maximal de missions
#define MAX_DISTANCE 10 // Distance max entre les points


int dist_poubelles[M][M];
int dist_decharge[M];
int nv = R;
// Function declarations

void remplir_distance();

/****************************		DECLARATION DES STRUCTURE	***************************************/
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

// Structure for Faffect message (Controller -> Truck)
struct Faffect {
    int camion_id;      // Truck identifier
    int mission;        // Mission type (1 = Collect, 2 = Rest, 3 = Fuel)
    int idP1, idP2;     // IDs of trash bins for collection
};

// Message structure for System V message queue
struct Message {
    long mtype;            // Message type (must be long)
    struct Faffect fa;     // Payload (Faffect structure)
};


// deposer dans le tampon /*le camion va deposer when mission finished*/
void Deposer(struct tampon *buffer, int camion_id, struct Tfmissions temp) {
    int index = buffer->q % R; // Find next empty slot

    buffer->tabmission[index].camion_id = temp.camion_id;
    buffer->tabmission[index].mission_status = temp.mission_status; // Dummy value for mission status
    buffer->tabmission[index].consomation_recente = temp.consomation_recente ; // Dummy value for consumption

    buffer->q = (buffer->q + 1) % R; // Update queue pointer
}

// prelever from le tampon /*le contreuleur va prelever*/
void Prelever(struct tampon *buffer, int *camion_id, int *mission_status, int *consomation_recente) {
    int index = (buffer->q - buffer->cpt + R) % R; // Calculate oldest entry

    *camion_id = buffer->tabmission[index].camion_id;
    *mission_status = buffer->tabmission[index].mission_status;
    *consomation_recente = buffer->tabmission[index].consomation_recente;

    
}
/***********************************************************************/
/**************************************************	CONTROLEUR	********************************************/
//apres avoir deposer dans le tampon on doit verifier si il est plein ou non mais 
//le truc est qu'on veut pas qu'il bloque and so we use cpt :entier := 0; partage entre les processus et on le protege par des mutex
// on utlise un mutexcpt memoire partage q (qui gere le mouvements de tampon)
void controller(int semid, struct tampon *buffer,int msgid) {
int last_camion = -1; // Track the last camion assigned a mission
struct Message message;
int mission_status;
    message.mtype = 1; // Message type (can be used for prioritization)
//--------------------------------initialisation
    int etatp[M];   
    struct info_camions {
        int etatc;
        int consomation_recente;
        int carburant_actuel;
        int idP1, idP2;
    } tab[N];
     for (int i = 0; i < N; i++) {
        tab[i].etatc = 4;
        tab[i].consomation_recente = 0;
        tab[i].carburant_actuel = CP;
    }
    int cpt_camion = N;
 do {
    //-----------------------------changer etatp
    
    for (int i = 0; i < M; i++) {
        etatp[i] = (rand() % 2) + 1;
    }
    
    //-----------------------------prelever from le tampon
 
    
        P(semid, 2); // MUTEXCPT semaphore (exclusive access to cpt)

        if (buffer->cpt > 0) {
            // Critical section: read and remove from buffer
            int camion_id, mission_status, consomation_recente;
            V(semid, 2); // Release MUTEXCPT
            Prelever(buffer, &camion_id, &mission_status, &consomation_recente); // Take mission from buffer
	    P(semid, 2); // MUTEXCPT semaphore (exclusive access to cpt)
            printf("[Controleur] recoie fin de mission(camion_id=%d, mission_status=%d, consomation_recente=%d)\n",
                   camion_id, mission_status, consomation_recente);
            
            if (mission_status==3) {
            		tab[camion_id].carburant_actuel=CP;
            }
            else {
            		tab[camion_id].consomation_recente=  consomation_recente;    
            		tab[camion_id].carburant_actuel -= tab[camion_id].consomation_recente;
            }
            
            if (mission_status !=5) tab[camion_id].etatc=4;
            else tab[camion_id].etatc=5;
            
            // Decrement item count (cpt) 
            buffer->cpt--; //printf("[Controleur] decrement cpt=%d\n",buffer->cpt);

          
           V(semid, 2); // Release MUTEXCPT
        } else {
            V(semid, 2); // Release MUTEXCPT
            printf("[Controleur] No fin de mission from tampon.\n");
        }

        
    //-----------------------------programmer mission
        
	message.fa.mission=0;
for (int i = (last_camion + 1) % N, count = 0; count < N; i = (i + 1) % N, count++) {
    if (tab[i].etatc == 4) {
        printf("[controleur] carburant actuel=%d de camion %d\n", tab[i].carburant_actuel, i);

        if (tab[i].carburant_actuel <= MIN) {
            message.fa.camion_id = i;
            message.fa.mission = 3; // refuel
            tab[i].etatc = 3;
        } else if (tab[i].carburant_actuel <= CP / 3 && tab[i].consomation_recente !=0 ) {//si le camion a fait une mission et carburanr actuel<=CP alors repos
            message.fa.camion_id = i;
            message.fa.mission = 2; // rest
            tab[i].etatc = 2;
        } else {
            for (int j = 0; j < M; j++) {
                if (etatp[j] == 2) {
                    etatp[j] = 3;
                    for (int k = j + 1; k < M; k++) {
                        if (etatp[k] == 2) {
                            etatp[k] = 3;
                            message.fa.camion_id = i;
                            message.fa.mission = 1; // collect
                            tab[i].etatc = 1;
                            message.fa.idP1 = j;
                            message.fa.idP2 = k;
                            break;
                        }
                    }
                    break;
                }
            }
        }

        last_camion = i; // Update the last camion assigned
        break;           // Exit after assigning one mission
    }
}

      //-----------------------------send mission (msg)
      if (message.fa.mission !=0){
      msgsnd(msgid, &message, sizeof(struct Faffect), 0);
      printf("[Controleur] envoyer une mission %d au camion %d\n",message.fa.mission , message.fa.camion_id);
      //printf("[Controleur] la dist idP1=%d idP2=%d\n",message.fa.idP1,message.fa.idP2);
      }else printf("\ncant send mission!\n");
      
    
      
      if (mission_status == 5) cpt_camion--; // fin de mission
      sleep(2);
    } while (cpt_camion != 0);
    
    exit(0);
}

/***************************************************CAMION*********************************************/
void camion(int i,int semid, struct tampon *buffer,int msgid) {
struct Message message;
    message.mtype = 1; // Message type (can be used for prioritization)

    int mission_count = MAX;
    struct Tfmissions temp;
    
    remplir_distance();
    do {
    //----------------------recevoir misssion
    msgrcv(msgid, &message, sizeof(struct Faffect), 1, 0);
    //printf("[camion %d] received a message.\n",i);
    // Check if the message is for the correct camion_id
      if (message.fa.camion_id ==i)
      {
        switch (message.fa.mission) {
            case 1:
                sleep(2);
                temp.mission_status = 1;
                temp.consomation_recente = C * (dist_decharge[message.fa.idP1] + dist_poubelles[message.fa.idP1][message.fa.idP2]);
                printf("[Camion %d] dis dech=%d dist poub[%d][%d]=%d\n",i,dist_decharge[message.fa.idP1],message.fa.idP1,message.fa.idP2, dist_poubelles[message.fa.idP1][message.fa.idP2]  );
                
                mission_count--;
                if ( mission_count == 5 ) {temp.mission_status = 5;}
                printf("[Camion %d] mission count=%d\n",i,mission_count);
   
                break;

            case 2:
                sleep(3);
                temp.mission_status = 2;
                temp.consomation_recente =0;
                break;

            case 3:
                sleep(4);
                temp.mission_status = 3;

                break;
        }
        temp.camion_id =i;
        
        
        P(semid, 0); // NV semaphore, si il y a place dans le tampon

        P(semid, 1); // MUTEXP semaphore (exclusive access to shared buffer)

        // Critical section: write into buffer
        Deposer(buffer, i,temp); // Add mission to buffer
        printf("[Camion %d] Added fin mission to tampon(camion_id=%d)\n", i, i);

        // Release MUTEXP
        V(semid, 1);

        // Update item count (cpt)
        P(semid, 2); // MUTEXCPT semaphore (exclusive access to cpt)
        buffer->cpt++;
        //printf("[Camion %d] Incremented item count cpt=%d\n",i,buffer->cpt);
        V(semid, 2); // Release MUTEXCPT

       
    }
    else {
         // Message is not for this truck; requeue it
         //printf("[camion %d] Message not for this truck, requeuing.\n", i);
         msgsnd(msgid, &message, sizeof(struct Faffect), 0);
	}
    } while (mission_count != 0);
    printf("[camion %d] IS DONE!\n",i);
    exit(0);
}


void lock_file(FILE *file) {
    int fd = fileno(file);
    struct flock lock;
    lock.l_type = F_RDLCK; // Read lock
    lock.l_whence = SEEK_SET;
    lock.l_start = 0;
    lock.l_len = 0; // Lock the whole file
    if (fcntl(fd, F_SETLKW, &lock) == -1) {
        perror("Error locking file");
        exit(1);
    }
}

void unlock_file(FILE *file) {
    int fd = fileno(file);
    struct flock lock;
    lock.l_type = F_UNLCK; // Unlock
    lock.l_whence = SEEK_SET;
    lock.l_start = 0;
    lock.l_len = 0;
    if (fcntl(fd, F_SETLK, &lock) == -1) {
        perror("Error unlocking file");
        exit(1);
    }
}

/************************************************************REMPLIR**************************************************/
void remplir_distance() {
    FILE *file_decharge = fopen("dist_decharge.txt", "r");
    FILE *file_poubelles = fopen("dist_poubelles.txt", "r");

    if (!file_decharge || !file_poubelles) {
        perror("Error opening file");
        exit(1);
    }

    for (int j = 0; j < M; j++) {
        if (fscanf(file_decharge, "%d", &dist_decharge[j]) != 1) {
            printf("Error: Not enough data in decharge.txt.\n");
            fclose(file_decharge);
            fclose(file_poubelles);
            exit(1);
        }
    }
    int i,j;
    lock_file(file_poubelles); // Lock before reading
    for (j = 0; j < M; j++) {
        for (i = 0; i < M; i++) {
            if (fscanf(file_poubelles, "%d", &dist_poubelles[j][i]) != 1) {
                printf("Error: Not enough data in poubelles.txt.\n");
                fclose(file_decharge);
                fclose(file_poubelles);
                exit(1);
            }
        }
    }
    
    unlock_file(file_poubelles); // Unlock after reading
   

    fclose(file_decharge);
    fclose(file_poubelles);
}

/************************************************************MAIN****************************************************/
int main() {
   
     int shmid, semid;
    key_t key = CLE;
    //--------------------------------------cree les structures
    // Create shared memory
    shmid = cree_mem_partage(key, sizeof(struct tampon)) ;

    // Attach shared memory
    struct tampon *buffer = attache_mem_partage(shmid);

    // Create a message queue
    int msgid =init_message_queue();



    // Initialize shared memory buffer
    buffer->q = 0; // Start with the first slot
    buffer->cpt = 0; // No items initially

    // Create semaphores
    int initValues[] = {R, 1, 1}; // NV = R, MUTEXP = 1, MUTEXCPT = 1
    semid = sem_create(SEM_KEY, 3, initValues);
    
    
    
    //--------------------------------------cree les camions et le contreuleur
    // Simulate controller sending tasks and trucks processing them
    int pid;
    
    // Fork to create a new process for the controller
    pid = fork();
    
    if (pid == 0) {
        // Child process: Controller behavior
        controller(semid, buffer, msgid);
        
    }
        // Parent process: Trucks behavior
        for (int i = 0; i < N; i++) {
            int truck_pid = fork();
            
            if (truck_pid == 0) 
            {
                // Child process: Truck behavior
                camion(i,semid, buffer, msgid);
                
            }
        }
            
        // Wait for the controller + truck process to complete
        for (int i = 0; i < N+1; i++) {
            wait(NULL);  // Wait for each truck process to finish
        }
        exit(1);
    return 0;
}
