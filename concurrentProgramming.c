#include <stdio.h>       // Standard Input/Output functions
#include <stdlib.h>      // General utilities library (memory allocation, random numbers, etc.)
#include <pthread.h>     // POSIX threads library for multi-threading support
#include <unistd.h>      // Standard symbolic constants and types (e.g., fork, exec, sleep)
#include <sys/types.h>   // Data types used in system calls (e.g., pid_t)
#include <sys/wait.h>    // Wait for process termination functions
#include <sys/times.h>   // Times function for measuring CPU time
#include <sys/time.h>    // Time-related functions (e.g., gettimeofday)
#include <string.h>      // String manipulation functions
#include <semaphore.h>   // POSIX semaphore support for synchronization

#define MAX_PRIMES 1000000

unsigned long primes[MAX_PRIMES]; // Global array for storing prime numbers
int num_primes = 0;               // Counter for the number of primes found
int num_slaves;                   // Number of processes or threads
unsigned long max_prime;          // Maximum prime number to find

sem_t semaphore; // Semaphore for synchronization

// Function to calculate prime numbers within a range for a specific process or thread
unsigned long calculate_primes(int slave_id, int num_slaves, unsigned long max_prime) {
    unsigned long num, i, primes_found = 0, flag;
    num = 3 + 2 * slave_id;

    while (num <= max_prime) {
        flag = 0;
        for (i = 2; i <= num / 2; i++) {
            if (num % i == 0) {
                flag = 1;
                break;
            }
        }
        if (flag == 0 && (num > 1)) {
            primes[num_primes++] = num; // Store prime in the global array
            primes_found++;
        }
        num = num + 2 * num_slaves;
    }

    return primes_found;
}

// Thread function for parallel computation of prime numbers
void* thread_function(void* arg) {
    int thread_id = *(int*)arg;

    // Record the start time for the thread
    struct timeval start_time;
    gettimeofday(&start_time, NULL);

    // Calculate primes using the provided function
    unsigned long primes_found = calculate_primes(thread_id, num_slaves, max_prime);

    // Wait for all threads to reach this point before printing
    sem_wait(&semaphore);

    // Record the end time for the thread
    struct timeval end_time;
    gettimeofday(&end_time, NULL);

    // Calculate the time taken by the thread in milliseconds
    double thread_time_taken = (end_time.tv_sec - start_time.tv_sec) * 1000.0 +
                               (end_time.tv_usec - start_time.tv_usec) / 1000.0;

    // Display the results for the thread
    printf("Thread %d computed %lu prime numbers in %.0f milliseconds\n", thread_id, primes_found, thread_time_taken);

    // Release the semaphore to allow the next thread to print
    sem_post(&semaphore);

    // Allocate memory for the result
    unsigned long* result = (unsigned long*)malloc(sizeof(unsigned long));
    *result = primes_found;

    pthread_exit(result);
}

int main(int argc, char* argv[]) {
    // Check for the correct number of command-line arguments
    if (argc != 4) {
        fprintf(stderr, "Usage: %s <max_prime> <n_slaves> <process/thread>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    char* endptr;

    // Parse command-line arguments
    max_prime = strtol(argv[1], &endptr, 10);
    if (*endptr != '\0') {
        fprintf(stderr, "Invalid number: %s\n", argv[1]);
        exit(EXIT_FAILURE);
    }

    num_slaves = strtol(argv[2], &endptr, 10);
    if (*endptr != '\0') {
        fprintf(stderr, "Invalid number: %s\n", argv[2]);
        exit(EXIT_FAILURE);
    }

    char* method = argv[3];

    // Validate the selected method
    if (strcmp(method, "process") != 0 && strcmp(method, "thread") != 0) {
        fprintf(stderr, "Invalid method. Use 'process' or 'thread'.\n");
        exit(EXIT_FAILURE);
    }

    // Record the start time for the main program
    struct timeval start_time;
    gettimeofday(&start_time, NULL);

    // Initialize the semaphore with a value of 1
    sem_init(&semaphore, 0, 1);

    // Initialize variable to accumulate results
    unsigned long primes_found = 0;

    if (strcmp(method, "process") == 0) {
        pid_t pid;
        // Create child processes
        for (int i = 0; i < num_slaves; i++) {
            pid = fork();
            if (pid == 0) {
                // Record the start time for the process
                struct timeval process_start_time;
                gettimeofday(&process_start_time, NULL);

                // Child process
                primes_found = primes_found + calculate_primes(i, num_slaves, max_prime);

                // Record the end time for the process
                struct timeval process_end_time;
                gettimeofday(&process_end_time, NULL);

                // Calculate the time taken by the process in milliseconds
                double process_time_taken = (process_end_time.tv_sec - process_start_time.tv_sec) * 1000.0 +
                                            (process_end_time.tv_usec - process_start_time.tv_usec) / 1000.0;

                // Display the results for the process
                printf("Process %d computed %lu prime numbers in %.0f milliseconds\n", i, primes_found, process_time_taken);

                // Exit the child process
                exit(EXIT_SUCCESS);
            } else if (pid < 0) {
                fprintf(stderr, "Fork failed.\n");
                exit(EXIT_FAILURE);
            }
        }

        // Wait for all child processes to finish
        for (int i = 0; i < num_slaves; i++) {
            wait(NULL);
        }
    } else if (strcmp(method, "thread") == 0) {
        pthread_t threads[num_slaves];
        int thread_id[num_slaves];

        // Create threads
        for (int i = 0; i < num_slaves; i++) {
            thread_id[i] = i;
            pthread_create(&threads[i], NULL, thread_function, &thread_id[i]);
        }

        // Wait for threads to finish and collect their results
        for (int i = 0; i < num_slaves; i++) {
            unsigned long* result;
            pthread_join(threads[i], (void**)&result);
            primes_found = primes_found + *result;
            free(result); // Free the allocated memory
        }

        // Wait for the last thread to release the semaphore
        sem_wait(&semaphore);
        sem_post(&semaphore);
    }

    // Destroy the semaphore
    sem_destroy(&semaphore);

    // Record the end time for the main program
    struct timeval end_time;
    gettimeofday(&end_time, NULL);

    // Display results and running time
    printf("This machine calculated all prime numbers under %lu using %d slaves in %.0f milliseconds\n", max_prime, num_slaves, (end_time.tv_sec - start_time.tv_sec) * 1000.0 + (end_time.tv_usec - start_time.tv_usec) / 1000.0);

    return 0;
}


