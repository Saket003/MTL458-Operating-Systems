#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

#define BUFFER_SIZE 100

unsigned int buffer[BUFFER_SIZE];
int in = 0;   
int out = 0;  
int count = 0; 

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t not_empty = PTHREAD_COND_INITIALIZER;
pthread_cond_t not_full = PTHREAD_COND_INITIALIZER;

void* producer(void* arg) {
    FILE* file = fopen("input-part1.txt", "r");
    if (!file) {
        perror("Failed to open input file");
        exit(EXIT_FAILURE);
    }

    unsigned int item;
    while (fscanf(file, "%u", &item) != EOF) {
        pthread_mutex_lock(&mutex);

        while (count == BUFFER_SIZE) {
            pthread_cond_wait(&not_full, &mutex);
        }

        buffer[in] = item;
        in = (in + 1) % BUFFER_SIZE;
        count++;

        pthread_cond_signal(&not_empty);
        pthread_mutex_unlock(&mutex);
        if (item == 0)
            break;
    }

    fclose(file);
    return NULL;
}

void* consumer(void* arg) {
    FILE* output_file = fopen("output-part1.txt", "w");
    if (!output_file) {
        perror("Failed to open output file");
        exit(EXIT_FAILURE);
    }

    while (1) {
        pthread_mutex_lock(&mutex);

        while (count == 0) {
            pthread_cond_wait(&not_empty, &mutex);
        }

        unsigned int item = buffer[out];
        out = (out + 1) % BUFFER_SIZE;
        count--;

        if (item == 0) {
            pthread_mutex_unlock(&mutex);
            break;
        }

        fprintf(output_file, "Consumed:[%u],Buffer-State:[", item);
        for (int i = 0; i < count; i++) {
            int index = (out + i) % BUFFER_SIZE;
            if(buffer[index] == 0)
                break;
            fprintf(output_file, "%u", buffer[index]);
            if (i < count - 1 && buffer[(out + i + 1) % BUFFER_SIZE] != 0) {
                fprintf(output_file, ",");
            }
        }
        fprintf(output_file, "]\n");

        pthread_cond_signal(&not_full);
        pthread_mutex_unlock(&mutex);
    }

    fclose(output_file);
    return NULL;
}

int main() {
    pthread_t prod_thread, cons_thread;

    pthread_create(&prod_thread, NULL, producer, NULL);
    pthread_create(&cons_thread, NULL, consumer, NULL);

    pthread_join(prod_thread, NULL);
    pthread_join(cons_thread, NULL);

    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&not_empty);
    pthread_cond_destroy(&not_full);

    return 0;
}
