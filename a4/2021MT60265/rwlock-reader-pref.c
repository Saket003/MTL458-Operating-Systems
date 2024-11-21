#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <stdlib.h>

#define SHARED_FILE "shared-file.txt"
#define OUTPUT_FILE "output-reader-pref.txt"

sem_t read_lock;
sem_t write_lock;
int reader_count = 0;

void *reader(void *arg){
    FILE* output_fp = fopen(OUTPUT_FILE, "a");
    if (!output_fp) {
        perror("Failed to open output file");
        exit(EXIT_FAILURE);
    }

    sem_wait(&read_lock);
    reader_count++;
    if (reader_count == 1)  sem_wait(&write_lock);
    // Shifted here to ensure correct ordering of outputs
    fprintf(output_fp, "Reading,Number-of-readers-present:[%d]\n", reader_count);
    fflush(output_fp);

    sem_post(&read_lock);

    FILE* shared_fp = fopen(SHARED_FILE, "r");
    if (!shared_fp) {
        perror("Failed to open shared file");
        exit(EXIT_FAILURE);
    }
    char ch[4];
    while (fgets(ch, sizeof(ch), shared_fp) != NULL) {
        // Kuch toh hua he
    }
    fclose(shared_fp);

    sem_wait(&read_lock);
    reader_count--;
    if (reader_count == 0) {
        sem_post(&write_lock);
    }
    sem_post(&read_lock);

    fclose(output_fp);
    return NULL;
}

void *writer(void *arg) {
    FILE* output_fp = fopen(OUTPUT_FILE, "a");
    if (!output_fp) {
        perror("Failed to open output file");
        exit(EXIT_FAILURE);
    }

    sem_wait(&write_lock);

    fprintf(output_fp, "Writing,Number-of-readers-present:[%d]\n", reader_count);
    fflush(output_fp);

    FILE* shared_fp = fopen(SHARED_FILE, "a");
    if (!shared_fp) {
        perror("Failed to open shared file");
        exit(EXIT_FAILURE);
    }
    fprintf(shared_fp, "Hello world!\n");
    fclose(shared_fp);

    sem_post(&write_lock);

    fclose(output_fp);
    return NULL;
}

int main(int argc, char **argv) {
    sem_init(&read_lock, 0, 1);
    sem_init(&write_lock, 0, 1);
    
    //Do not change the code below to spawn threads
    if(argc != 3) return 1;
    int n = atoi(argv[1]);
    int m = atoi(argv[2]);
    pthread_t readers[n], writers[m];

    // Create reader and writer threads
    for (int i = 0; i < n; i++) pthread_create(&readers[i], NULL, reader, NULL);        
    for (int i = 0; i < m; i++) pthread_create(&writers[i], NULL, writer, NULL);

    // Wait for all threads to complete
    for (int i = 0; i < n; i++) pthread_join(readers[i], NULL);
    for (int i = 0; i < m; i++) pthread_join(writers[i], NULL);

    return 0;
}