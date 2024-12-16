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
#define MSG_KEY 123

#define M 10      // Nombre de poubelles
#define N 5       // Nombre de camions
#define CP 300    // Capacité de carburant
#define MIN 70    // Seuil minimal de carburant
#define C 2      // Consommation par unité de distance
#define MAX 12  // Nombre maximal de missions
#define MAX_DISTANCE 10 // Distance max entre les points

/****************************		DECLARATION DES STRUCTURE	***************************************/

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
/************************************************************REMPLIR**************************************************/
void remplir_distance(int dist_decharge[M], int dist_poubelles[M][M]) {
    FILE *file_decharge = fopen("dist_decharge.txt", "r");
    FILE *file_poubelles = fopen("dist_poubelles.txt", "r");

    if (!file_decharge || !file_poubelles) {
        perror("Error opening file");
        exit(1);
    }

    // Fill dist_decharge array
    for (int j = 0; j < M; j++) {
        if (fscanf(file_decharge, "%d", &dist_decharge[j]) != 1) {
            printf("Error: Not enough data in dist_decharge.txt.\n");
            fclose(file_decharge);
            fclose(file_poubelles);
            exit(1);
        }
    }

    // Fill dist_poubelles 2D array
    for (int j = 0; j < M; j++) {
        for (int i = 0; i < M; i++) {
            if (fscanf(file_poubelles, "%d", &dist_poubelles[j][i]) != 1) {
                printf("Error: Not enough data in dist_poubelles.txt.\n");
                fclose(file_decharge);
                fclose(file_poubelles);
                exit(1);
            }
        }
    }

    fclose(file_decharge);
    fclose(file_poubelles);
}
/**************************************************	DEPOSER	********************************************/
void Deposer(struct tampon *buffer, struct Tfmissions temp) {
    int index = buffer->q % R; // Find next empty slot
    buffer->tabmission[index].camion_id = temp.camion_id;
    buffer->tabmission[index].mission_status = temp.mission_status; // Dummy value for mission status
    buffer->tabmission[index].consomation_recente = temp.consomation_recente ; // Dummy value for consumption

    buffer->q = (buffer->q + 1) % R; // Update queue pointer
   
}
/**************************************************	PRELEVER	********************************************/
void Prelever(struct tampon *buffer, int *camion_id, int *mission_status, int *consomation_recente, int *t) {

    *camion_id = buffer->tabmission[*t].camion_id;
    *mission_status = buffer->tabmission[*t].mission_status;
    *consomation_recente = buffer->tabmission[*t].consomation_recente;

    *t= (*t + 1) % R;
}
/**************************************************	CONTROLEUR	********************************************/
void controller(int semid, struct tampon *buffer,int msgid) {
int last_camion = -1; // Track the last camion assigned a mission
struct Message message;
int mission_status,t=0;
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
 int recoie=0;
 int camion_id, mission_status, consomation_recente;
    
        P(semid, 2); // MUTEXCPT semaphore (exclusive access to cpt)

        if ( (buffer->cpt) != 0) 
        {
        		V(semid, 2); // MUTEXCPT
            		Prelever(buffer, &camion_id, &mission_status, &consomation_recente,&t); // Take fin mission from buffer
                	printf("[Controleur] recoie fin de mission(camion_id=%d, mission_status=%d, consomation_recente=%d)\n", camion_id, mission_status, consomation_recente);
                	recoie=1;
	    		P(semid, 2); // MUTEXCPT semaphore (exclusive access to cpt)
            		buffer->cpt--; //printf("[Controleur] decrement cpt=%d\n",buffer->cpt);
            		V(semid, 2); // Release MUTEXCPT
            		V(semid, 0);//release nv?        
        } else 
        {
            V(semid, 2); // Release MUTEXCPT   
            //printf("[Controleur] No fin de mission from tampon.\n");
        }    
       //----------------------mise a jour la structure info camion 
       
        if (recoie==1)//si le controleur recoie une mission, mise a jour la structure info camion  
        {
           recoie = 0;
        	/*etape 1: changer le carburant et la consomation recente */
            if (mission_status==3) {
            		tab[camion_id].carburant_actuel=CP;
            		tab[camion_id].consomation_recente=0;
            }
            else 
            {
            		tab[camion_id].consomation_recente=  consomation_recente;    
            		tab[camion_id].carburant_actuel -= tab[camion_id].consomation_recente;
            		//printf("[controleur] pour le camion=%d consomation_recente=%d , carburant_actuel=%d\n",camion_id,tab[camion_id].consomation_recente, tab[camion_id].carburant_actuel  );
            }
            
            /*etape 2: changer l'etat de camion*/
            if (mission_status !=5) tab[camion_id].etatc=4;
            else 
            {
               tab[camion_id].etatc=5;
            	cpt_camion--; 
            	printf("\n[Controleur] ******cpt_camion=%d*********\n",cpt_camion);// fin de mission
            }       
        }   
    //-----------------------------programmer mission
        int i,count;

	message.fa.mission=0;
	for (i = (last_camion + 1) % N, count = 0; count < N; i = (i + 1) % N, count++) {
    		if (tab[i].etatc == 4) 
    		{
        		printf("[controleur] carburant actuel=%d de camion %d\n", tab[i].carburant_actuel, i);
			if (tab[i].carburant_actuel <= MIN) 
			{
            			message.fa.camion_id = i;
            			message.fa.mission = 3; // refuel
            			tab[i].etatc = 3;
        		} 
        		else 
        			if (tab[i].carburant_actuel <= CP / 3 && tab[i].consomation_recente !=0 ) 
        			{//si le camion a fait une mission et carburant actuel<=CP alors repos
            				message.fa.camion_id = i;
            				message.fa.mission = 2; // rest
            				tab[i].etatc = 2;
        			}			 
        			else 
        			{
            				for (int j = 0; j < M; j++) 
            				{
                				if (etatp[j] == 2) 
                				{
                					int sauv=j;
              						for (int k = j + 1; k < M; k++) 
              						{
                        					if (etatp[k] == 2) 
                        					{
                        					etatp[sauv] = 3;//a la in apres trouve
                            					etatp[k] = 3;
                            					message.fa.camion_id =i;
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
      message.mtype = message.fa.camion_id + 1; // Set type to camion_id + 1
      msgsnd(msgid, &message, sizeof(struct Faffect), 0);
      printf("[Controleur] envoyer une mission %d au camion %d\n",message.fa.mission , message.fa.camion_id);
      //printf("[Controleur] la dist idP1=%d idP2=%d\n",message.fa.idP1,message.fa.idP2);
      }
      
    

      sleep(2);
    } while (cpt_camion != 0);
    printf("\n[CONTREULEUR] DONE!!!\n");
    exit(0);
}

/***************************************************CAMION*********************************************/
void camion(int i,int semid, struct tampon *buffer,int msgid) {
	struct Message message;
	int dist_poubelles[M][M];
	int dist_decharge[M];

    int mission_count = MAX;
    struct Tfmissions temp;
    
    remplir_distance(dist_decharge, dist_poubelles);
    do {
    //----------------------recevoir misssion
    msgrcv(msgid, &message, sizeof(struct Faffect), i + 1, 0);
      printf("{camion %d] received a mission!(mission=%d)\n",i,message.fa.mission);
        switch (message.fa.mission) {
            case 1:
                sleep(2);
                temp.mission_status = 1;
                temp.consomation_recente = C * (dist_decharge[message.fa.idP1] + dist_poubelles[message.fa.idP1][message.fa.idP2]);
                printf("[Camion %d] doing mission %d ....\n",i,message.fa.mission);
                printf("[Camion %d] dis dech=%d dist poub[%d][%d]=%d\n",i,dist_decharge[message.fa.idP1],message.fa.idP1,message.fa.idP2, dist_poubelles[message.fa.idP1][message.fa.idP2]  );
                mission_count--;
                if ( mission_count == 0 ) {temp.mission_status = 5;}
                printf("[Camion %d] mission count=%d\n",i,mission_count);
   
                break;

            case 2:
                sleep(3);
                printf("[Camion %d] doing mission %d ....\n",i,message.fa.mission);
                temp.mission_status = 2;
                temp.consomation_recente =0;
                break;

            case 3:
                sleep(4);
                printf("[Camion %d] doing mission %d ....\n",i,message.fa.mission);
                temp.mission_status = 3;

                break;
        }
        temp.camion_id =i;
        
	//------------------------- camion va deposer
	P(semid, 0); // NV semaphore, si il y a place dans le tampon	
        P(semid, 1); // MUTEXP semaphore (exclusive access to shared buffer)
	Deposer(buffer,temp); // Add fin mission to buffer
        	printf("[Camion %d] Added fin mission to tampon(camion_id=%d)\n", i, i);
        V(semid, 1);// Release MUTEXP
        P(semid, 2); // MUTEXCPT semaphore (exclusive access to cpt)
        buffer->cpt++;
        V(semid, 2); // Release MUTEXCPT
       
	sleep(2);
    } while (mission_count != 0);
    printf("[camion %d] IS DONE!\n",i);
    exit(0);
}



//-----------------------------------------------------------------------------------------------------
void new_line()
{
for (int k=0; k<30;k++)
	{
	printf("----");
	}
}
void affiche_parent()
{
new_line();
printf("\n\t\t\t\tPROCESSUS PARENT:\n");
new_line();
	printf("\n");
}
//-----------------------------------------------------------------------------------------------------
void affiche_fils()
{
new_line();
printf("\n\t\t\t\tPROCESSUS CONTROLEUR ET PROCESSUS CAMIONS:\n");
new_line();
	printf("\n");
}

//-----------------------------------------------------------------------------------------------------
void cree_STRUCTURE(int *shmid, int *semid, int *msgid, struct tampon **buffer) {
    key_t key = CLE;
	affiche_parent();	
    // Delete existing message queue if it exists
    if (file_existe(MSG_KEY) == 1) {
        delete_message_queue(MSG_KEY);
    }

    // 1. Create shared memory
    *shmid = cree_mem_partage(key, sizeof(struct tampon));

    // 2. Attach shared memory
    *buffer = attache_mem_partage(*shmid);

    // 3. Create message queue
    *msgid = init_message_queue(MSG_KEY);

    // 4. Initialize semaphores
    int initValues[] = {R, 1, 1}; // NV = R, MUTEXP = 1, MUTEXCPT = 1
    *semid = sem_create(SEM_KEY, 3, initValues);
    
    // Print success message
    printf("\n\tLES STRUCTURES CREE AVEC SUCCEE:\n");
    printf("\t-----------------------------------\n");
    printf("\tle id d esemble de semaphore: %d\n", *semid);
    printf("\tle id de memoire partage est: %d\n", *shmid);
    printf("\tle id de file de message: %d\n", *msgid);
}
//-----------------------------------------------------------------------------------------------------
void Detruire_STRUCTURE(int msgid, struct tampon *buffer, int shmid, int semid)
{
	delete_message_queue(MSG_KEY) ;
        detache_mem_partage(buffer);
        detruire_mem_partage(shmid) ;
        sem_delete(semid);
}

/************************************************************MAIN****************************************************/
int main() {
   
     int shmid, semid;
    int msgid;
    struct tampon *buffer;
    
    //--------------------------------------cree les structures
    cree_STRUCTURE(&shmid, &semid, &msgid, &buffer);
    
    //--------------------------------------cree les camions et le contreuleur
    affiche_fils();
    int pid;
    pid = fork();
    
    if (pid == 0) {
        controller(semid, buffer, msgid);
        
    }
        for (int i = 0; i < N; i++) {
            int truck_pid = fork();
            
            if (truck_pid == 0) 
            {
                camion(i,semid, buffer, msgid);               
            }
        }
            
        // Wait for the controller + truck process to complete
        for (int i = 0; i < N+1; i++) {
            wait(NULL);  // Wait for each truck process to finish
        }
        affiche_parent();
        Detruire_STRUCTURE(msgid, buffer, shmid, semid);
        
        exit(1);
        
    return 0;
}
