#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>

#define LOWER_NUM 1
#define UPPER_NUM 10000
#define BUFFER_SIZE 100
#define MAX_COUNT 10000

int buffer[BUFFER_SIZE];
int count = 0;
int in = 0;
int out = 0;
int produced_numbers = 0;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t can_produce = PTHREAD_COND_INITIALIZER;
pthread_cond_t can_consume = PTHREAD_COND_INITIALIZER;

void* producer(void *arg) {
    FILE *all_file = fopen("all.txt", "w");
    if (all_file == NULL) {
        perror("Failed to open file");
        exit(1);
    }
    
    for (int i = 0; i < MAX_COUNT; i++) {
        int num = rand() % (UPPER_NUM - LOWER_NUM + 1) + LOWER_NUM;
        
        pthread_mutex_lock(&mutex);
        while (count == BUFFER_SIZE) {
            pthread_cond_wait(&can_produce, &mutex);
        }
        
        buffer[in] = num;
        in = (in + 1) % BUFFER_SIZE;
        count++;
        produced_numbers++;
        
        fprintf(all_file, "%d\n", num);
        pthread_cond_signal(&can_consume);
        pthread_mutex_unlock(&mutex);
    }
    
    fclose(all_file);
    return NULL;
}

void* consumer(void *arg) {
    intptr_t is_even = (intptr_t)arg;
    FILE *file = fopen(is_even ? "even.txt" : "odd.txt", "w");
    if (file == NULL) {
        perror("Failed to open file");
        exit(1);
    }
    
    while (1) {
        pthread_mutex_lock(&mutex);
        while (count == 0) {
            if (produced_numbers >= MAX_COUNT) {
                pthread_mutex_unlock(&mutex);
                fclose(file);
                return NULL;
            }
            pthread_cond_wait(&can_consume, &mutex);
        }
        
        // Adjusted for stack behavior: removal happens from the 'in' side, which is the top of the stack.
        int num = buffer[(in - 1 + BUFFER_SIZE) % BUFFER_SIZE]; // Look at the most recently added item.
        
        if ((is_even && num % 2 == 0) || (!is_even && num % 2 != 0)) {
            fprintf(file, "%d\n", num);
            in = (in - 1 + BUFFER_SIZE) % BUFFER_SIZE; // Adjust 'in' to remove the item.
            count--;
            pthread_cond_signal(&can_produce);
        }
        pthread_mutex_unlock(&mutex);
    }
}


int main() {
    pthread_t producer_thread, consumer_thread_even, consumer_thread_odd;
    
    srand(time(NULL));
    pthread_create(&producer_thread, NULL, producer, NULL);
    pthread_create(&consumer_thread_even, NULL, consumer, (void *)(intptr_t)1);
    pthread_create(&consumer_thread_odd, NULL, consumer, (void *)(intptr_t)0);

    
    pthread_join(producer_thread, NULL);
    pthread_join(consumer_thread_even, NULL);
    pthread_join(consumer_thread_odd, NULL);
    
    return 0;
}
