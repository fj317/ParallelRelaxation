#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include "pthread_barrier.c"

int integerDimension = 4;
double precision = 0.01;
double array[4][4] = {{1.0, 1.0, 1.0, 1.0}, {1.0, 0.0, 0.0, 0.0}, {1.0, 0.0, 0.0, 0.0}, {1.0, 0.0, 0.0, 0.0}};
double newArray[4][4];
int threadNumber = 0;
int totalThreads = 0;
pthread_mutex_t m;
pthread_barrier_t barrier;

void relaxationThread() {
    int localThreadNumber;
    pthread_mutex_lock(&m);
    localThreadNumber = threadNumber;
    threadNumber++;
    pthread_mutex_unlock(&m);
    // barrier to ensure threads dont start iteration before all threads are created and thus loop variables wont cause errors
    pthread_barrier_wait(&barrier);

    while (1) {
        double result = 0.0;
        // average 4 numbers around it
        for (int i = 1 + localThreadNumber; i < integerDimension - 1; i+= totalThreads) {
            for (int j = 1; j < integerDimension - 1; j++) {
                result = (array[i-1][j] + array[i+1][j] + array[i][j-1] + array[i][j+1]) / 4;
                // check if precision is reached
                if (result - newArray[i][j] < precision && newArray[i][j] != 0.0)
                {
                    // kill thread as finished
                    pthread_exit(NULL);
                }
                newArray[i][j] = result;
            }
        }
        // barrier to ensure threads dont start next iteration before all threads are finished
        pthread_barrier_wait(&barrier);
        // if the first thread then do the memory copy required
        if (localThreadNumber == 0) {
            memcpy(array, newArray, sizeof(newArray));
        }
        // barrier again to stop threads from continuing until thread 1 has finished copying memory
        pthread_barrier_wait(&barrier);
    }
}

int main(void) {

    printf("---NEW PROGRAM--- \n");

    // copy values from array into newArray so values can be changed in newArray and won't cause any problems
    memcpy(newArray, array, sizeof(array));

    pthread_t threads[44];
    pthread_mutex_init(&m, NULL);
    // number of threads is the number of rows to be operated on
    totalThreads = integerDimension - 2;
    pthread_barrier_init(&barrier, NULL, totalThreads);
    printf("total threads %d\n", totalThreads);
    for (int x = 0; x < totalThreads; x++) {
        //printf("%ld Thread to create\n", x);
        pthread_create(&threads[x], NULL, (void*(*)(void*))relaxationThread, NULL);
        //printf("%ld thread success created\n", x);
    }

    for (int x = 0; x < 4; x++) {
        pthread_join(threads[x], NULL);
    }


    // create new threads

    // while (1) {
    //     double result = 0.0;
    //     // average 4 numbers around it
    //     for (int i = 1; i < integerDimension - 1; i++) {
    //         for (int j = 1; j < integerDimension - 1; j++) {
    //             result = (array[i-1][j] + array[i+1][j] + array[i][j-1] + array[i][j+1]) / 4;
    //             // check if precision is reached
    //             if (result - newArray[i][j] < precision && newArray[i][j] != 0.0)
    //             {
    //                 goto printArray;
    //             }
    //             newArray[i][j] = result;
    //         }
    //     }
    //     // do barrier here to wait for all threads to finish updating values
    //     memcpy(array, newArray, sizeof(newArray));
    //     // do barrier so threads dont start again until memory copy has finished
    // }

    for (int i = 0; i < integerDimension; i++) {
        for (int j = 0; j < integerDimension; j++) {
            printf(" %lf ", newArray[i][j]);
        }
        printf("\n");
    }


}