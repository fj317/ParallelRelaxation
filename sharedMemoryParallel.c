#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <math.h>

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


// function to randomly generate double values and put them into array
void randomArrayGen(int size, double **array) {
    double randomValue;
    // use the time as seed to ensure numbers are less predictable
    srand ( (unsigned int)time ( NULL));
    for (int i = 0; i < size; i++) {
        for (int j = 0; j < size; j++) {
            randomValue = (rand() / (double)RAND_MAX);
            array[i][j] = randomValue;
        }
    }
}

// function to put values from one array  into an array
void putValuesIntoArray(int size, double **array, double valueArray[][size]) {
    for (int i = 0; i < size; i++) {
        for (int j = 0; j < size; j++) {
            array[i][j] = valueArray[i][j];
        }
    }
}

// function to copy one array to another array
void copyArray(int size, double **emptyArray, double **dataArray) {
    for (int i = 0; i < size; i++) {
        for (int j = 0; j < size; j++) {
            emptyArray[i][j] = dataArray[i][j];
        }
    }
}

// thread specific function
void relaxationThread(struct threadParameters *arg) {
    // getting variables from parameter
    struct threadParameters *info = arg;
    // mutex to ensure each thread can assign itself a threadNumber without errors occuring
    pthread_mutex_lock(info->threadNumberMutex);
    int localThreadNumber = info->threadNum;
    info->threadNum = info-> threadNum + 1;
    pthread_mutex_unlock(info->threadNumberMutex);
    int totalThreads = info->totalThreads;
    int arrayDimensions = info->arrayDimensions; 


    while (1) {
        // barrier to ensure threads don't start iteration before all threads are synchronised (so all threads have thread number & copyArray function has finished)
        pthread_barrier_wait(info->barrier);
        double result = 0.0;
        // loop through array 1 row at a time
        // iterate by the number of threads 
        for (int i = 1 + localThreadNumber; i < arrayDimensions - 1; i+= totalThreads) {
            for (int j = 1; j < arrayDimensions - 1; j++) {
                // average 4 numbers around the curretn array value
                result = (info->array[i-1][j] + info->array[i+1][j] + info->array[i][j-1] + info->array[i][j+1]) / 4;
                // check if precision is reached for ALL values
                if (info->precisionCount >= (arrayDimensions-2)*(arrayDimensions-2)) {
                    // exit loops because precision is met
                    goto breakLoops;
                // check if precision is reached for the specific value
                // take absolute value of difference incase it is negative
                } else if (fabs(result - info->newArray[i][j]) < info->precision) {
                    // lock so precisionCount can be updated
                    pthread_mutex_lock(info->precisionMutex);
                    // add 1 to precisionCount as precision met for this value
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
        // barrier to ensure all threads have finished looping through array before copying the array values
        breakLoops: pthread_barrier_wait(info->barrier);
        // check if precision met, if so then end the thread
        if (info->precisionCount >= (arrayDimensions-2)*(arrayDimensions-2)) {
            // kill all thread as finished
            pthread_exit(NULL);
        }
        // if the first thread then do the memory copy required
        if (localThreadNumber == 0) {
            copyArray(arrayDimensions, info->array, info->newArray);
        }
    }
}

int main(void) {
    // variable declaration
    int dimensions = 4;
    double precision = 0.01;
    int totalThreads = 2;
    int precisionCount = 0;
    int threadNum = 0;
    pthread_t *threads = malloc(sizeof(pthread_t) * (unsigned long) dimensions);
    // mallocing space for arrays
    double **array = malloc((unsigned long)dimensions * sizeof(double *));
    double **newArray = malloc((unsigned long)dimensions * sizeof(double *));
    for (int i = 0; i < dimensions; i++) {
        array[i] = malloc((unsigned long)dimensions * sizeof(double));
        newArray[i] = malloc((unsigned long)dimensions * sizeof(double));
    }
    double testingArray[4][4] = {{1.0, 1.0, 1.0, 1.0}, {1.0, 0.0, 0.0, 0.0}, {1.0, 0.0, 0.0, 0.0}, {1.0, 0.0, 0.0, 0.0}};

    // if random array generation is required, uncomment the line below and comment the after it (or vice versa)
    //randomArrayGen(dimensions, array);
    putValuesIntoArray(dimensions, array, testingArray);

    // copy values from array into newArray so values can be changed in newArray and won't cause any problems
    copyArray(dimensions, newArray, array);

    printf("Starting array:\n");
    for (int i = 0; i < dimensions; i++) {
        for (int j = 0; j < dimensions; j++) {
            printf(" %lf ", array[i][j]);
        }
        printf("\n");
    }
    printf("\n");

    // creating struct so arguments can be give to thread
    struct threadParameters *info = malloc(sizeof(struct threadParameters));
    // initialising variables into struct
    info->array = array;
    info->newArray = newArray;
    info->precision = precision;
    info->arrayDimensions = dimensions;
    info->threadNum = threadNum;
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

    printf("Starting threads...\n\n");

    // creating the threads
    for (int x = 0; x < totalThreads; x++) {
        pthread_create(&threads[x], NULL, (void*)relaxationThread, (void*)info);
    }

    // join threads together so array will print once all finished
    for (int x = 0; x < totalThreads; x++) {
        pthread_join(threads[x], NULL);
    }

    // destroy mutexes and barriers to free up resources
    pthread_mutex_destroy(&precisionMutex);
    pthread_mutex_destroy(&threadNumberMutex);
    pthread_barrier_destroy(&barrier);

    // print final array
    printf("Finished array:\n");
    for (int i = 0; i < dimensions; i++) {
        for (int j = 0; j < dimensions; j++) {
            printf(" %lf ", newArray[i][j]);
        }
        printf("\n");
    }
    // free arrays and struct to free up resources
    free(array);
    free(newArray);
    free(info);
}