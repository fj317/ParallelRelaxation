#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>

// function to randomly generate double values and put them into array
void randomArrayGen(int size, double *array) {
    double randomValue;
    srand(5);
    for (int i = 0; i < size; i++) {
        randomValue = (rand() / (double)RAND_MAX);
        array[i] = randomValue;
    }
}

void printArray(double *array, int sizeOne, int sizeTwo) {
    double *thingy = array;
    for (int m = 0; m < sizeOne; m++) {
        for (int n = 0; n < sizeTwo; n++) {
            printf(" %lf ", *thingy);
            thingy++;
        }
        printf("\n");
    }
}


int main (void) {
    MPI_Init(NULL, NULL);
    int dimensions = 6;
    double precision = 0.01;
    int totalProcesses;
    MPI_Comm_size(MPI_COMM_WORLD, &totalProcesses);
    int precisionCount = 0;
    // processWorkload gives the number of rows that each process will work on
    int processWorkload = (dimensions-2) / totalProcesses;
    int processNumber;

    MPI_Comm_rank(MPI_COMM_WORLD, &processNumber);
    double *processArray = malloc((unsigned long)(processWorkload + 2) * (unsigned long)dimensions * sizeof(double));
    double *finishedArray = malloc((unsigned long)processWorkload * (unsigned long)(dimensions-2) * sizeof(double));

    if (processNumber == 0) {
        // if process 0, setup array and partition into equal chunks and send to each process
        double *array = malloc((unsigned long)dimensions * (unsigned long)dimensions * sizeof(double));
        randomArrayGen(dimensions * dimensions, array);
        printArray(array, dimensions, dimensions);

        // get array that is an equal partition of array for each process
        for (int i = 0; i < totalProcesses; i++) {
            for (int j = 0; j < processWorkload + 2; j++) {
                for (int k = 0; k < dimensions; k++) {
                    processArray[(j * dimensions) + k] = array[(i * processWorkload * dimensions) + (j * dimensions) + k];
                    //printf("processArray: %d. Array: %d\n", (j * dimensions) + k, (i * processWorkload * dimensions) + (j * dimensions) + k);
                }
            }

            // send array to the process so it can begin work
            MPI_Send(processArray, (processWorkload + 2) * dimensions, MPI_DOUBLE, i, 1, MPI_COMM_WORLD);
        }
        //free(array);
    }

    MPI_Recv(processArray, (processWorkload + 2) * dimensions, MPI_DOUBLE, 0, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    double result = 0.0;
    int currentValue = dimensions + 1;
    //printf("I'm %d, cur value %f\n", processNumber, processArray[currentValue]);
    for (int i = 0; i < processWorkload; i++) {
        for (int j = 1; j < dimensions - 1; j++) {
            result = (processArray[currentValue-1] + processArray[currentValue+1] + processArray[currentValue-dimensions] + processArray[currentValue+dimensions]) / 4.0;
            //printf("I'm %d, Curr Value: %f --- 1: %f, 2: %f, 3: %f, 4: %f\n", processNumber, processArray[currentValue], processArray[currentValue-1], processArray[currentValue+1], processArray[currentValue-dimensions], processArray[currentValue+dimensions]);
            printf("I'm %d and got result %f\n", processNumber, result);
            finishedArray[currentValue - (dimensions + 1)] = result;
            currentValue++;
        }
    }


    if (processNumber == 0) {
        // merge all processArray's together
        printf("\n");
        printArray(finishedArray, processWorkload, dimensions-2);
    }

    MPI_Finalize();
    free(processArray);
}