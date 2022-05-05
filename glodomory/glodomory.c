#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>
#include <limits.h>

#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/types.h>
#include <sys/ipc.h>

#define PHILOSOPHERS_NUM 5
#define SPAGHETTI_WEIGHT 100

// Semafory
static struct sembuf buf;

void sem_signal(int semid, int semnum){
   buf.sem_num = semnum;  // numer semafora
   buf.sem_op = 1;  // operacja semafora
   buf.sem_flg = 0;  // flaga operacji
   if (semop(semid, &buf, 1) == -1){
      perror("sem up");
      exit(1);
   }
}

void sem_wait(int semid, int semnum){
   buf.sem_num = semnum;
   buf.sem_op = -1;
   buf.sem_flg = 0;
   if (semop(semid, &buf, 1) == -1){
      perror("sem down");
      exit(1);
   }
}


// Filozofowie inicjalizacja
struct Philosopher{
    int id;
    int total_spaghetti_weight;  // "importance" of a philosopher (more spaghetti => less important)
    bool ready_to_eat;
};

typedef struct Philosopher Philosopher;  // to avoid struct Philosopher

void bubbleSort(Philosopher sortedQ[]){
    int total_philosophers = PHILOSOPHERS_NUM;  // because #define are immutable
    bool swapped = false;
    do{
        swapped = false;
        for(int i = 0; i < total_philosophers - 1; i++){

            if(sortedQ[i].total_spaghetti_weight > sortedQ[i + 1].total_spaghetti_weight){
                Philosopher temp = sortedQ[i];
                sortedQ[i] = sortedQ[i + 1];
                sortedQ[i + 1] = temp;  // "bubbling" upwards
                swapped = true;  // flag to check whether further sorting is needed
            }   
        }
        total_philosophers--;

    }while(swapped == true);
}


void enqueue(Philosopher sortedQ[], Philosopher newP ){
    bool empty_found = false;
    for(int i = 0; i < PHILOSOPHERS_NUM; i++){
		// -1 means an empty space
        if(sortedQ[i].id == -1){
            sortedQ[i] = newP;  // place philosopher in the queue
            empty_found = true;
            break;
        }
    }
    if(empty_found == false){
        perror("no empty place has been found");
    }

    bubbleSort(sortedQ);  // sort the queue

}

Philosopher dequeue(Philosopher sortedQ[]){
    if(sortedQ[0].id == -1){
        perror("queue is empty");
    }
    Philosopher firstP = sortedQ[0];  // MSP = MostSignificantPhilosopher out


    for(int i = 0; i < PHILOSOPHERS_NUM-1; i++){
        sortedQ[i] = sortedQ[i+1];  // moving the queue
    }

    Philosopher nullP = {-1, 2147483647, false};
    sortedQ[PHILOSOPHERS_NUM-1] = nullP;  // add absurd weights to make this artificial Philosopher unmovable

    return firstP;
    
}


void philosopher_routine(int id, int arbitrator_permission_sem, int philosophers_sem, int forkk_status_sem, Philosopher philosophers[], bool forkks[]){
    srand(getpid());  // get a seed from pid (which will be unique)

    const int left_forkk = id;
    const int right_forkk = (id + 1) % PHILOSOPHERS_NUM;  // PHILOSOPHERS_NUM = FORKKS_NUM, so % ensures that a right forkk is given
    int sleepTime = 1; // how long certain activity takes place

	// main phil loop
    while(true){

        // thinking
        sleepTime = rand() % 15;
        printf("%i: began thinking for: %is\n", id, sleepTime);
        sleep(sleepTime);
        

        // ready to eat
        sem_wait(philosophers_sem, id);  // sem wait
        philosophers[id].ready_to_eat = true;  // change the state
        sem_signal(philosophers_sem, id);  // sem lowered

        // arbitrator wait 
        printf("%i: began waiting for permission \n", id);
        sem_wait(arbitrator_permission_sem, id);  // wait for an arbitrator, when allowed -> start eating

        // eating
        sleepTime = rand() % 15;
        printf("%i: began eating for: %is\n", id, sleepTime);  // nom nom
        sleep(sleepTime);

        // finsihed eating
        sem_wait(forkk_status_sem, 0);  // wait for fork semaphore
        forkks[left_forkk] = true;  // make both forks usable
        forkks[right_forkk] = true;
        sem_signal(forkk_status_sem, 0);

		// add soaghetti weight to total consumed
        sem_wait(philosophers_sem, id);
        philosophers[id].total_spaghetti_weight += SPAGHETTI_WEIGHT;  // add consumed spaghetti
        sem_signal(philosophers_sem, id);
        

        printf("%i: ate: %i\n", id, philosophers[id].total_spaghetti_weight);
    }    
}

// show certain queue
void printQ(int forkk_num, Philosopher* queue){
    printf("queue of forkk number: %i: ", forkk_num);
    int i = 0;
    int philosopherID = queue[i].id;
    while(philosopherID != -1){
        printf("Philosopher %i(%i), ", philosopherID, queue[i].total_spaghetti_weight);
        i++;
        philosopherID = queue[i].id;
    }
    printf("\n");
}

void arbitrator_routine(int arbitrator_permission_sem, int philosophers_sem, int forkk_status_sem, Philosopher philosophers[], bool forkks[]){

    Philosopher* forkk_queues[PHILOSOPHERS_NUM];  // assign a queue to every fork

    for(int i = 0 ; i < PHILOSOPHERS_NUM; i++){
        forkk_queues[i] = (Philosopher*)malloc(PHILOSOPHERS_NUM * sizeof(Philosopher));  // dynamic allocation of memory

        for(int j = 0 ; j < PHILOSOPHERS_NUM; j++){  // fill with ridiculous "NULL" Philosophers
            forkk_queues[i][j].id = -1;
            forkk_queues[i][j].total_spaghetti_weight = 2147483647;
            forkk_queues[i][j].ready_to_eat = false;
        }
        
    }

	// main arbitrator loop
    while(true){

        // checking if someone wants to eat, enquing
        for(int id = 0; id < PHILOSOPHERS_NUM; id++){  // check every philosopher

            sem_wait(philosophers_sem, id);  // wait for philosophers semaphore

            if(philosophers[id].ready_to_eat){  // if philosopher is ready to eat, enqueue them
                const int left_forkk = id;
                const int right_forkk = (id + 1) % PHILOSOPHERS_NUM;

                enqueue(forkk_queues[left_forkk], philosophers[id]);
                enqueue(forkk_queues[right_forkk], philosophers[id]); 

                philosophers[id].ready_to_eat = false;
                printf("Philosopher %i enqued\n", id);
                printQ(left_forkk, forkk_queues[left_forkk]);
                printQ(right_forkk, forkk_queues[right_forkk]);
				// enqueue both forks and go on
            }

            sem_signal(philosophers_sem, id);  // release the philosopher
        }

        // check the queues
        for(int id = 0; id < PHILOSOPHERS_NUM; id++){
            const int left_forkk = id;
            const int right_forkk = (id + 1) % PHILOSOPHERS_NUM;

            sem_wait(forkk_status_sem, 0);
            // only 1st one in the queue with the lowest total spaghetti gets to eat (to prevent process starvation)
            if(forkk_queues[left_forkk][0].id == id && forkk_queues[right_forkk][0].id == id && forkks[left_forkk] && forkks[right_forkk]){
                dequeue(forkk_queues[left_forkk]);
                dequeue(forkk_queues[right_forkk]);

                forkks[left_forkk] = false;  // set both forks - in use
                forkks[right_forkk] = false;

                sem_signal(arbitrator_permission_sem, id);
                printf("Philosopher: %i allowed to eat by an arbitrator\n", id);
            }

            sem_signal(forkk_status_sem, 0);  // release the semaphore
        }

    }

}


int main(){
	printf("glodomory started\n");
	// 0660 group and owner, read, write rights
    int philosophers_shmid = shmget(IPC_PRIVATE, PHILOSOPHERS_NUM*sizeof(Philosopher), IPC_CREAT|0660);  // get id of shared memory
    Philosopher* philosophers = (Philosopher*)shmat(philosophers_shmid, NULL, 0);  // add shared memory identified by shmid to process

    int forkks_shmid = shmget(IPC_PRIVATE, PHILOSOPHERS_NUM*sizeof(bool), IPC_CREAT|0660);  // get id of shared memory connected with forks
    bool* forkks = (bool*)shmat(forkks_shmid, NULL, 0);

    int philosophers_sem = semget(IPC_PRIVATE, PHILOSOPHERS_NUM, IPC_CREAT|0660);  // get phils semaphore

    int forkk_status_sem = semget(IPC_PRIVATE, 1, IPC_CREAT|0660);  // get semaphore index
    semctl(forkk_status_sem, 0, SETVAL, 1);  // set value of semaphore 0 to 1

    int arbitrator_permission_sem = semget(IPC_PRIVATE, PHILOSOPHERS_NUM, IPC_CREAT|0660);

    
    
    for(int i = 0; i < PHILOSOPHERS_NUM; i++){  // initiate philosophers list
        Philosopher phil = {i, 0, false};  // not ready to eat
        philosophers[i] = phil;
        forkks[i] = true;  // forks is "free"
        
        semctl(arbitrator_permission_sem, i, SETVAL, 0);
        semctl(philosophers_sem, i, SETVAL, 1);
    }

	// initiate the phil routine for all philosophers
    for(int i = 0; i < PHILOSOPHERS_NUM; i++){

        pid_t pid = fork();
        if(pid == 0){  // go on if pid returned 0
            philosopher_routine(i, arbitrator_permission_sem, philosophers_sem, forkk_status_sem, philosophers, forkks);
            exit(0);
        }
    }

    // initiate the arbitrator routine
    pid_t pid = fork();
    if(pid == 0){
        arbitrator_routine(arbitrator_permission_sem, philosophers_sem, forkk_status_sem, philosophers, forkks);
        exit(0);
    }

    return 0;
}


