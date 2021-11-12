#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include "pthread_barrier.c"

//double **array;
//double **newArray;
double array[4][4] = {{1.0, 1.0, 1.0, 1.0}, {1.0, 0.0, 0.0, 0.0}, {1.0, 0.0, 0.0, 0.0}, {1.0, 0.0, 0.0, 0.0}};
double newArray[4][4];
//double newArray[initialArraySize][initialArraySize];
//double array[initialArraySize][initialArraySize];
//double (*array)[integerDimension] = malloc(sizeof(double[integerDimension][integerDimension]));
//randomArrayGen(integerDimension, array);
//double (*newArray)[integerDimension] = malloc(sizeof(double[integerDimension][integerDimension]));;
int precisionCount = 0;
pthread_mutex_t precisionMutex;

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


void randomArrayGen(int size, double array[][size]) {
    double randomValue;
    srand ( time ( NULL));
    for (int i = 0; i < size; i++) {
        for (int j = 0; j < size; j++) {
            if (i == 0) {
                array[i][j] = 1;
            }
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


    printf("Hello I'm thead %d\n", localThreadNumber);
    int iterateOnce = 0;
    // barrier to ensure threads dont start iteration before all threads are created and thus loop variables wont cause errors
    pthread_barrier_wait(info->barrier);

    while (1) {
        double result = 0.0;
        // average 4 numbers around it
        for (int i = 1 + localThreadNumber; i < arrayDimensions - 1; i+= totalThreads) {
            for (int j = 1; j < arrayDimensions - 1; j++) {
                result = (array[i-1][j] + array[i+1][j] + array[i][j-1] + array[i][j+1]) / 4;
                // check if precision is reached for ALL values
                if (precisionCount == (arrayDimensions-2)*(arrayDimensions-2)) {
                    // kill thread as finished
                    pthread_exit(NULL);
                } else if (result - newArray[i][j] < precision && iterateOnce == 1) {
                    // if precision not reached for ALL values, but is reached for this value then add 1 to the precisionCount
                    precisionCount++;
                } else {
                    // if precision isn't reached then update the value and reset the precisionCount
                    newArray[i][j] = result;
                    precisionCount = 0;
                }
            }
        }
        iterateOnce = 1;
        // barrier to ensure threads dont start next iteration before all threads are finished
        pthread_barrier_wait(info->barrier);
        // if the first thread then do the memory copy required
        if (localThreadNumber == 0) {
            memcpy(array, newArray, sizeof(double) * arrayDimensions * arrayDimensions);
        }
        // barrier again to stop threads from continuing until thread 1 has finished copying memory
        pthread_barrier_wait(info->barrier);
    }
}

int main(void) {

    printf("---NEW PROGRAM--- \n");
    int dimensions = 4;
    double precision = 0.001;
    int totalThreads = 0;
    int precisionCount = 0;

    struct threadParameters *info = malloc(sizeof(struct threadParameters));
    info->array = malloc(dimensions * dimensions * sizeof(double));
    info->newArray = malloc(dimensions * dimensions * sizeof(double));
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
    pthread_barrier_init(&barrier, NULL, totalThreads);
    info->barrier = &barrier;

    // copy values from array into newArray so values can be changed in newArray and won't cause any problems
    memcpy(newArray, array, sizeof(double) * dimensions * dimensions);

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