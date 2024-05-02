#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <signal.h>

#define KRED  "\x1B[31m"
#define KGRN  "\x1B[32m"
#define KBLU  "\x1B[34m"
#define KNRM  "\x1B[0m"

#define N 10 // Number of mCounter threads
#define BUFFER_SIZE 5 // Size of the buffer

int counter = 0; // Counter for incoming messages
int buffer[BUFFER_SIZE]; // Buffer for storing counts
int bufferHead = 0; // Index pointing to the head of the buffer
int bufferTail = 0; // Index pointing to the tail of the buffer

sem_t counterMutex; // Semaphore for accessing the counter
sem_t bufferMutex; // Semaphore for accessing the buffer
sem_t emptySlots; // Semaphore to track empty slots in the buffer
sem_t fullSlots; // Semaphore to track filled slots in the buffer


void enqueue(int data) {
    buffer[bufferTail] = data; 
    bufferTail = (bufferTail + 1) % BUFFER_SIZE; 
}

void dequeue() {
    bufferHead = (bufferHead + 1) % BUFFER_SIZE; 
}

void* mCounter(void* arg) {
    int threadId = *(int *)arg; // Extract the integer id from the void pointer argument arg.
    srand(time(NULL)); // Sends the random number generator with the current time to ensure that the sequence of random numbers generated is different in each program run.

    while (1) {
        sleep(rand() % 6); // Sleep for a random time to simulate message arrival

        printf("%sCounter thread %d: received a message\n",KBLU , threadId);
        
        printf("%sCounter thread %d: waiting to write\n",KBLU , threadId);
        
        sem_wait(&counterMutex); //locking access to the shared counter
        
        counter++;
        
        printf("%sCounter thread %d: now adding to counter, counter value=%d\n",KBLU , threadId, counter);
        
        sem_post(&counterMutex); //allowing other threads to access the shared counter
    }

    return NULL;
}

void* mMonitor(void* arg) {
    while (1) {
        sleep(rand() % 6 + 3); // Wait for time interval T1
        
        printf("%sMonitor thread: waiting to read counter\n",KGRN);
        
        sem_wait(&counterMutex);  //locking access to the shared counter
        
        int count = counter; // Save the current value of the shared counter
        
        counter = 0;
        
        printf("%sMonitor thread: reading a count value of %d\n", KGRN , count);
        
        sem_post(&counterMutex); // Allowing other threads to access the shared counter
        
        int sem_value; 
        
        sem_getvalue(&emptySlots,&sem_value); // Retrieve the current value of the emptySlots semaphore (number of empty places).
        
        if(sem_value == 0) // Check if the buffer is full 
        {
	   printf("%sMonitor thread: Buffer full!!\n",KRED);
	   
        }

        sem_wait(&emptySlots); //If the buffer is not full,this operation decrements the semaphore value, allowing the mMonitor thread to proceed.
        sem_wait(&bufferMutex); //locking access to the shared buffer.
        
	int temp = bufferTail; // Save the current value of bufferTail.

        enqueue(count); // Enqueue the saved counter value into the buffer.
        
        printf("%sMonitor thread: writing to buffer at position %d\n", KGRN , temp);
        
        sem_post(&bufferMutex); // Allowing other threads to access the shared buffer
        sem_post(&fullSlots); // Increment the fullSlots semaphore to signal that the buffer is no longer empty.
    }

    return NULL;
}

void* mCollector(void* arg) {
    while (1) {
    	sleep(rand() % 6 + 6); // Sleep for a random time 
    	
        int sem_value;
        
	sem_getvalue(&fullSlots,&sem_value); // number of busy places ?
	
        if(sem_value == 0) // Check if the buffer is empty
        {
            printf("%sCollector thread: nothing is in the buffer!\n", KRED );

        }
        sem_wait(&fullSlots); // If the buffer is not empty, this operation decrements the semaphore value, allowing the mCollector thread to proceed
        sem_wait(&bufferMutex); // Locking access to the shared buffer
        

        
	int temp = bufferHead; // Save the current value of bufferFront
	
        dequeue(); // Dequeue the first element entered the buffer
        
        printf("%sCollector thread: reading from the buffer at position %d\n", KRED ,temp);
        
        sem_post(&bufferMutex); // Allowing other threads to access the shared buffer
        sem_post(&emptySlots); // Increment the bufferEmpty semaphore to signal that the buffer is no longer full.
    }

    return NULL;
}


void intHandler(int dummy) {
	// set the noramal color back
    printf("%sExit\n", KNRM);
	// Destroy the semaphore 
	sem_destroy(&counterMutex); 
	sem_destroy(&bufferMutex); 
	sem_destroy(&emptySlots); 
	sem_destroy(&fullSlots);
	exit(0);
}


int main() {

    signal(SIGINT, intHandler); // Set up a signal handler for the interrupt signal (Ctrl+C).
    
    sem_init(&counterMutex, 0, 1);
    sem_init(&bufferMutex, 0, 1);
    sem_init(&emptySlots, 0, BUFFER_SIZE);
    sem_init(&fullSlots, 0, 0);

    pthread_t mCounterThreads[N];
    pthread_t mMonitorThread;
    pthread_t mCollectorThread;

    // Create mCounter threads
    for (int i = 0; i < N; i++) {
        int* threadId = malloc(sizeof(int));
        *threadId = i + 1;
        pthread_create(&mCounterThreads[i], NULL, mCounter, threadId);
    }

    //pthread_create(&mMonitorThread, NULL, mMonitor, NULL);
    pthread_create(&mCollectorThread, NULL, mCollector, NULL);

    //pthread_join(mMonitorThread, NULL);
    pthread_join(mCollectorThread, NULL);
    
    // Join threads (wait for them to finish)
    for (int i = 0; i < N; i++) {
        pthread_cancel(mCounterThreads[i]);
    }

    sem_destroy(&counterMutex);
    sem_destroy(&bufferMutex);
    sem_destroy(&emptySlots);
    sem_destroy(&fullSlots);

    return 0;
}
