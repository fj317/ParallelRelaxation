#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include "pthread_barrier.c"

struct threadParameters {
    int arrayDimensions;
    double precision;
    double **array;
    double **newArray;
    int totalThreads;
    int threadNum;
    int precisionCount;
    pthread_mutex_t *precisionMutex;
    pthread_mutex_t *threadNumberMutex;
    pthread_barrier_t *barrier;
};


void randomArrayGen(int size, double **array) {
    double randomValue;
    srand ( (unsigned int)time ( NULL));
    for (int i = 0; i < size; i++) {
        for (int j = 0; j < size; j++) {
            randomValue = (rand() / (double)RAND_MAX);
            array[i][j] = randomValue;
        }
    }
}

void relaxationThread(struct threadParameters *arg) {
    struct threadParameters *info = arg;
    //struct threadParameters *info = arg;
    int localThreadNumber;
    //pthread_mutex_t threadNumberMutex = info->threadNumberMutex;
    pthread_mutex_lock(info->threadNumberMutex);
    localThreadNumber = info->threadNum;
    info->threadNum = info-> threadNum + 1;
    pthread_mutex_unlock(info->threadNumberMutex);
    int totalThreads = info->totalThreads;
    int arrayDimensions = info->arrayDimensions;
    double precision = info->precision;

    int iterateOnce = 0;
    // barrier to ensure threads dont start iteration before all threads are created and thus loop variables wont cause errors
    pthread_barrier_wait(info->barrier);

    while (1) {
        double result = 0.0;
        // average 4 numbers around it
        for (int i = 1 + localThreadNumber; i < arrayDimensions - 1; i+= totalThreads) {
            for (int j = 1; j < arrayDimensions - 1; j++) {
                result = (info->array[i-1][j] + info->array[i+1][j] + info->array[i][j-1] + info->array[i][j+1]) / 4;
                // check if precision is reached for ALL values
                if (info->precisionCount == (arrayDimensions-2)*(arrayDimensions-2)) {
                    // exit loops because precision is met
                    goto breakLoops;
                } else if (result - info->newArray[i][j] < precision && iterateOnce == 1) {
                    // lock so precisionCount can be updated
                    pthread_mutex_lock(info->precisionMutex);
                    // if precision not reached for ALL values, but is reached for this value then add 1 to the precisionCount
                    info->precisionCount++;
                    pthread_mutex_unlock(info->precisionMutex);
                } else {
                    // if precision isn't reached then update the value and reset the precisionCount
                    info->newArray[i][j] = result;
                    // lock so precisionCount can be updated without errors
                    pthread_mutex_lock(info->precisionMutex);
                    info->precisionCount = 0;
                    
                    pthread_mutex_unlock(info->precisionMutex);
                }
            }
        }
        iterateOnce = 1;
        // barrier to ensure threads dont start next iteration before all threads are finished
        breakLoops: pthread_barrier_wait(info->barrier);
        // if the first thread then do the memory copy required
        if (localThreadNumber == 0) {
            memcpy(info->array, info->newArray, sizeof(*info->array) * (unsigned long)arrayDimensions);
        }
        if (info->precisionCount == (arrayDimensions-2)*(arrayDimensions-2)) {
            // kill all thread as finished
            printf("thread %d dead\n", localThreadNumber);
            pthread_exit(NULL);
        }
        // barrier again to stop threads from continuing until thread 1 has finished copying memory
        pthread_barrier_wait(info->barrier);
    }
}

int main(void) {

    printf("---NEW PROGRAM--- \n");
    int dimensions = 50;
    double precision = 0.01;
    int totalThreads = 0;
    int precisionCount = 0;
    double **array = malloc((unsigned long)dimensions * sizeof(double *));
    double **newArray = malloc((unsigned long)dimensions * sizeof(double *));
    for (int i = 0; i < dimensions; i++) {
        array[i] = malloc((unsigned long)dimensions * sizeof(double));
        newArray[i] = malloc((unsigned long)dimensions * sizeof(double));
    }
    randomArrayGen(dimensions, array);
    for (int i = 0; i < dimensions; i++) {
        for (int j = 0; j < dimensions; j++) {
            printf(" %lf ", array[i][j]);
        }
        printf("\n");
    }
    printf("\n\n");

    struct threadParameters *info = malloc(sizeof(struct threadParameters));
    info->array = array;
    info->newArray = newArray;
    info->precision = precision;
    info->arrayDimensions = dimensions;
    // number of threads is the number of rows to be operated on
    totalThreads = dimensions - 2;
    info->totalThreads = totalThreads;
    info->precisionCount = precisionCount;

    pthread_mutex_t precisionMutex;
    pthread_mutex_init(&precisionMutex, NULL);
    info->precisionMutex = &precisionMutex;

    pthread_mutex_t threadNumberMutex;
    pthread_mutex_init(&threadNumberMutex, NULL);
    info->threadNumberMutex = &threadNumberMutex;

    pthread_barrier_t barrier;
    pthread_barrier_init(&barrier, NULL, (unsigned int)totalThreads);
    info->barrier = &barrier;

    // copy values from array into newArray so values can be changed in newArray and won't cause any problems
    memcpy(newArray, array, sizeof(*array) * (unsigned long)dimensions);

    printf("Starting threads\n");

    pthread_t threads[44];
    for (int x = 0; x < totalThreads; x++) {
        pthread_create(&threads[x], NULL, (void*(*)(void*))relaxationThread, (void*)info);
    }

    // join threads together so array will print once all finished
    for (int x = 0; x < 4; x++) {
        pthread_join(threads[x], NULL);
    }

    for (int i = 0; i < dimensions; i++) {
        for (int j = 0; j < dimensions; j++) {
            printf(" %lf ", newArray[i][j]);
        }
        printf("\n");
    }


}