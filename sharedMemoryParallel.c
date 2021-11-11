#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include "pthread_barrier.c"

int integerDimension = 4;
double precision = 0.001;
double array[4][4] = {{1.0, 1.0, 1.0, 1.0}, {1.0, 0.0, 0.0, 0.0}, {1.0, 0.0, 0.0, 0.0}, {1.0, 0.0, 0.0, 0.0}};
double newArray[4][4];
int threadNumber = 0;
int totalThreads = 0;
int precisionCount = 0;
pthread_mutex_t threadNumberMutex;
pthread_mutex_t precisionMutex;
pthread_barrier_t barrier;

void relaxationThread() {
    int localThreadNumber;
    int iterateOnce = 0;
    pthread_mutex_lock(&threadNumberMutex);
    localThreadNumber = threadNumber;
    threadNumber++;
    pthread_mutex_unlock(&threadNumberMutex);
    // barrier to ensure threads dont start iteration before all threads are created and thus loop variables wont cause errors
    pthread_barrier_wait(&barrier);

    while (1) {
        double result = 0.0;
        // average 4 numbers around it
        for (int i = 1 + localThreadNumber; i < integerDimension - 1; i+= totalThreads) {
            for (int j = 1; j < integerDimension - 1; j++) {
                result = (array[i-1][j] + array[i+1][j] + array[i][j-1] + array[i][j+1]) / 4;
                // check if precision is reached for ALL values
                if (precisionCount == (integerDimension-2)*(integerDimension-2))
                {
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
    pthread_mutex_init(&threadNumberMutex, NULL);
    pthread_mutex_init(&precisionMutex, NULL);

    // number of threads is the number of rows to be operated on
    totalThreads = integerDimension - 2;
    pthread_barrier_init(&barrier, NULL, totalThreads);
    for (int x = 0; x < totalThreads; x++) {
        pthread_create(&threads[x], NULL, (void*(*)(void*))relaxationThread, NULL);
    }

    // join threads together so array will print once all finished
    for (int x = 0; x < 4; x++) {
        pthread_join(threads[x], NULL);
    }

    for (int i = 0; i < integerDimension; i++) {
        for (int j = 0; j < integerDimension; j++) {
            printf(" %lf ", newArray[i][j]);
        }
        printf("\n");
    }


}