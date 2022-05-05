#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/types.h>
#include <stdbool.h>
#include <stdlib.h>

#define CAPACITY 3
#define PASSENGERS 8

static volatile int passengers_waiting = 0;


static pthread_mutex_t ride = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t   cnd_started  = PTHREAD_COND_INITIALIZER, cnd_finished  = PTHREAD_COND_INITIALIZER, cnd_enough_passengers = PTHREAD_COND_INITIALIZER;


void* passenger_program(void* id){
	while(true){
		int time_to_prepare = 5 + (rand()%5);
		printf("\t\t%d is preparing for a ride, this will take %d seconds\n", *(int*)id, time_to_prepare);
		sleep(time_to_prepare);
		printf("%d has finished preparing\n", *(int*)id);
		
		pthread_mutex_lock(&ride);
		passengers_waiting++;
		printf("Queue for a ride is %d people long\n", passengers_waiting);
		if(passengers_waiting >= CAPACITY){
			pthread_cond_signal( &cnd_enough_passengers );
			}
			
		pthread_cond_wait(&cnd_started, &ride);
		printf("\t\t%d is having a fun rollercoaster ride\n", *(int*)id);
		pthread_cond_wait(&cnd_finished, &ride);
		
		printf("%d has finished their's ride\n", *(int*)id);
		pthread_mutex_unlock(&ride);
		}
   
}

void* rollercoaster_program(void* id){
   
	while(true){
		pthread_mutex_lock(&ride);

		if(passengers_waiting >= CAPACITY){

			for(int i = 0; i < CAPACITY; i++){
				pthread_cond_signal( &cnd_started );
				}

        passengers_waiting -= CAPACITY;

        int ride_time = 15+(rand()%2);
        printf("\t\t\tRollercoaster starts, ride will take %d seconds\n", ride_time);
        pthread_mutex_unlock(&ride);

        sleep(ride_time);

        pthread_mutex_lock(&ride);
        for(int i = 0; i < CAPACITY; i++){
			pthread_cond_signal(&cnd_finished);
			}   

		}

    pthread_mutex_unlock(&ride);
	}
   

}


int main(){
	srand(time(NULL));
	pthread_t rollercoaster_thread, passenger_threads[PASSENGERS];

	// passengers + rollercoaster
	int thread_ids[PASSENGERS+1];
	int thread_count = 0;
	
	thread_ids[thread_count] = thread_count;
	pthread_create(&rollercoaster_thread, NULL, rollercoaster_program, &thread_ids[thread_count]);
	thread_count++;
	
	for(int i = 0; i < PASSENGERS; i++){
		thread_ids[thread_count] = thread_count;
		pthread_create(&passenger_threads[i], NULL, passenger_program, &thread_ids[thread_count]);
		thread_count++;
		}
		
	pthread_join(rollercoaster_thread, NULL);
	return 0;
}
