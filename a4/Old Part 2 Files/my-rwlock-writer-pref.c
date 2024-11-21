#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>

#define SHARED_FILE "shared-file.txt"
#define OUTPUT_FILE "output-writer-pref.txt"
#define SPIN_TIME_MS 1000

sem_t mutex;         // Lock for reader_count
sem_t write_lock;    // Locks writing if file is being red
sem_t reader_block;  // Blocks readers when a writer is waiting
sem_t extra_mutex;   // Extra mutex for managing waiting writers
sem_t loopin_mutex;  // Extra mutex for managing priority and ensuring only one reader thread wastes time cycles
int reader_count = 0; // Keeps track of active readers
int writers_waiting = 0; // Keeps track of waiting writers

void* reader(void* arg) {
    FILE* output_fp = fopen(OUTPUT_FILE, "a");
    if (!output_fp) {
        perror("Failed to open output file");
        exit(EXIT_FAILURE);
    }

    // Work around for Priority Control, Don't want each reader thread to waste cycles
    sem_wait(&loopin_mutex);
    while(writers_waiting>0){
        // printf("Writers Waiting{%d}\n",writers_waiting);
        usleep(SPIN_TIME_MS);
    }
    sem_post(&loopin_mutex);

    // printf("Reader left loop\n");
    sem_wait(&reader_block);

    sem_wait(&mutex);
    reader_count++;
    if (reader_count == 1) {
        sem_wait(&write_lock);
    }

    fprintf(output_fp, "Reading,Number-of-readers-present:%d\n", reader_count);
    fflush(output_fp);

    sem_post(&mutex);

    // printf("Reader passed mutex\n");

    sem_post(&reader_block);

    // Simulate reading by opening the shared file
    FILE* shared_fp = fopen(SHARED_FILE, "r");
    if (!shared_fp) {
        perror("Failed to open shared file");
        exit(EXIT_FAILURE);
    }
    char line[256];
    while (fgets(line, sizeof(line), shared_fp) != NULL) {
        // Simply read to simulate the action
    }
    fclose(shared_fp);

    // Exit critical section to update reader count
    sem_wait(&mutex);
    reader_count--;
    if (reader_count == 0) {
        sem_post(&write_lock); // Last reader unlocks the writer
        // printf("Last reader unlocks the writer\n");
    }
    sem_post(&mutex);

    fclose(output_fp);
    return NULL;
}

void* writer(void* arg) {
    FILE* output_fp = fopen(OUTPUT_FILE, "a");
    if (!output_fp) {
        perror("Failed to open output file");
        exit(EXIT_FAILURE);
    }

    sem_wait(&extra_mutex);
    writers_waiting++;
    sem_post(&extra_mutex);

    // Prevent new readers from entering
    sem_wait(&reader_block);

    // Wait for access to write
    sem_wait(&write_lock);

    // Log writing action and reader count
    fprintf(output_fp, "Writing,Number-of-readers-present:%d\n", reader_count);
    fflush(output_fp);

    // Write "Hello world!" to the shared file
    FILE* shared_fp = fopen(SHARED_FILE, "a");
    if (!shared_fp) {
        perror("Failed to open shared file");
        exit(EXIT_FAILURE);
    }
    fprintf(shared_fp, "Hello world!\n");
    fclose(shared_fp);

    writers_waiting--;

    // Release write lock and allow readers to enter
    sem_post(&write_lock);
    sem_post(&reader_block);

    fclose(output_fp);
    return NULL;
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s number_of_reader number_of_writer\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int num_readers = atoi(argv[1]);
    int num_writers = atoi(argv[2]);

    pthread_t* readers = malloc(num_readers * sizeof(pthread_t));
    pthread_t* writers = malloc(num_writers * sizeof(pthread_t));

    if (readers == NULL || writers == NULL) {
        perror("Failed to allocate memory for threads");
        exit(EXIT_FAILURE);
    }

    // Initialize semaphores
    sem_init(&mutex, 0, 1);
    sem_init(&write_lock, 0, 1);
    sem_init(&reader_block, 0, 1);
    sem_init(&extra_mutex, 0, 1);
    sem_init(&loopin_mutex, 0, 1);

    // Spawn reader threads
    for (int i = 0; i < num_readers; i++) {
        pthread_create(&readers[i], NULL, reader, NULL);
    }

    // Spawn writer threads
    for (int i = 0; i < num_writers; i++) {
        pthread_create(&writers[i], NULL, writer, NULL);
    }

    // Wait for all reader threads to finish
    for (int i = 0; i < num_readers; i++) {
        pthread_join(readers[i], NULL);
    }

    // Wait for all writer threads to finish
    for (int i = 0; i < num_writers; i++) {
        pthread_join(writers[i], NULL);
    }

    // Free allocated memory and destroy semaphores
    free(readers);
    free(writers);
    sem_destroy(&mutex);
    sem_destroy(&write_lock);
    sem_destroy(&reader_block);

    return 0;
}
