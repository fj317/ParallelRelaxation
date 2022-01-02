#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

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
    printf("\n");
}

void getRow(double *array, double *row, int rowNum, int size) {
    int startIndex = rowNum * size;
    for (int i = startIndex; i < startIndex + size; i ++) {
        row[i - startIndex] = array[i];
    }

}

void updateRow(double *array, double*row, int rowNum, int size) {
    int startIndex = rowNum * size;
    for (int i = startIndex; i < startIndex + size; i ++) {
        array[i] = row[i - startIndex];
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
    double* row = malloc((unsigned long) dimensions * sizeof(double));
    double *processArray = malloc((unsigned long)(processWorkload + 2) * (unsigned long)dimensions * sizeof(double));
    double *finishedArray = malloc((unsigned long)(processWorkload + 2) * (unsigned long)dimensions * sizeof(double));
    double *array = malloc((unsigned long)dimensions * (unsigned long)dimensions * sizeof(double));
    int recvPrecisionMet;
    int quitLoop = 0;
    int precisionMet;
    int currentValue;
    double result;

    // if not root process
    if (processNumber != 0) {
        // can free array as unused 
        free(array);
    }

    if (processNumber == 0) {
        // if process 0, setup array and partition into equal chunks and send to each process
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
    }

    MPI_Recv(processArray, (processWorkload + 2) * dimensions, MPI_DOUBLE, 0, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    while (quitLoop == 0) {
        result = 0.0;
        currentValue = 0;
        // initialise to true initially
        precisionMet = 1;
        //printf("I'm %d, cur value %f\n", processNumber, processArray[currentValue]);
        for (int i = 0; i < processWorkload + 2; i++) {
            for (int j = 0; j < dimensions; j++) {
                // if first row just copy the number 
                if (currentValue < dimensions) {
                    finishedArray[currentValue] = processArray[currentValue];
                } // if last row just copy number
                else if (currentValue > (processWorkload + 1) * dimensions) {
                    finishedArray[currentValue] = processArray[currentValue];
                } // if first column just copy number
                else if (currentValue % dimensions == 0) {
                    finishedArray[currentValue] = processArray[currentValue];
                } // if last column just copy number
                else if (currentValue % dimensions == 5) {
                    finishedArray[currentValue] = processArray[currentValue];
                } else {
                    // calculate result (average 4 neighbour values)
                    result = (processArray[currentValue-1] + processArray[currentValue+1] + processArray[currentValue-dimensions] + processArray[currentValue+dimensions]) / 4.0;
                    //printf("I'm %d, Curr Value: %f , Result %f --- 1: %f, 2: %f, 3: %f, 4: %f\n", processNumber, processArray[currentValue], result, processArray[currentValue-1], processArray[currentValue+1], processArray[currentValue-dimensions], processArray[currentValue+dimensions]);
                    //printf("I'm %d and got result %f\n", processNumber, result);
                    // if precision isn't met for current value
                    // take absolute value of difference in case it is negative
                    if (fabs(result - processArray[currentValue]) > precision) {
                        // if precision isn't met then change precisiobMet to false
                        // then it will only be true if precision is met for every value
                        precisionMet = 0;
                        // only update finishedArray if there precision isn't met
                        finishedArray[currentValue] = result;
                    } else {
                        //printf("Precision met for value: %f. Precision value: %f\n", processArray[currentValue], fabs(result - processArray[currentValue]));
                        finishedArray[currentValue] = processArray[currentValue];
                    }
                }
                // add 1 to currentValue (iterate to next value)
                currentValue++;
            }
        }
        // use MPI_allreduce to find whether precision is met for all processes
        MPI_Allreduce(&precisionMet, &recvPrecisionMet, 1, MPI_INT, MPI_LAND, MPI_COMM_WORLD);
        if (recvPrecisionMet == 1) {
            // send finishedArray to root node
            // get entire row (use getRow procedure)
            if (processNumber != 0) {
                getRow(finishedArray, row, 1, dimensions * processWorkload);
                MPI_Send(row, dimensions * processWorkload, MPI_DOUBLE, 0, 1, MPI_COMM_WORLD);
            }
            // quit loops
            quitLoop = 1;
        }        
        if (quitLoop == 0) {
            // send first row to prev process, send last row to next process
            // if process 0 only send last row
            if (processNumber == 0) {
                // last row worked on is processWorkload
                getRow(finishedArray, row, processWorkload, dimensions);
                // send row to process 1
                MPI_Send(row, dimensions, MPI_DOUBLE, 1, 1, MPI_COMM_WORLD);
                // recv last row update
                MPI_Recv(row, dimensions, MPI_DOUBLE, 1, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                // update row
                updateRow(finishedArray, row, processWorkload + 1, dimensions);
            } // else if process is last process only send first row to prev process
            else if (processNumber == totalProcesses - 1) {
                // first row is 0
                getRow(finishedArray, row, 1, dimensions);
                // send row to prev process
                MPI_Send(row, dimensions, MPI_DOUBLE, totalProcesses - 2, 1, MPI_COMM_WORLD);
                // recv first row update
                MPI_Recv(row, dimensions, MPI_DOUBLE, totalProcesses - 2, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                // update row
                updateRow(finishedArray, row, 0, dimensions);
            } // otherwise send both rows
            else {
                // send first row:
                getRow(finishedArray, row, 1, dimensions);
                MPI_Send(row, dimensions, MPI_DOUBLE, processNumber - 1, 1, MPI_COMM_WORLD);
                // send last row
                getRow(finishedArray, row, processWorkload, dimensions);
                MPI_Send(row, dimensions, MPI_DOUBLE, processNumber + 1, 1, MPI_COMM_WORLD);
                // update first row
                MPI_Recv(row, dimensions, MPI_DOUBLE, processNumber - 1, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                updateRow(finishedArray, row, 0, dimensions);
                // update last row
                MPI_Recv(row, dimensions, MPI_DOUBLE, processNumber + 1, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                updateRow(finishedArray, row, processWorkload + 1, dimensions);
            }
            // update array pointers so process array is now finished array

            void *tempPtr = processArray;
            processArray = finishedArray;
            finishedArray = tempPtr;
            MPI_Barrier(MPI_COMM_WORLD);
        }
    }
    
    
    // process 0 merges arrays
    if (processNumber == 0) {
        // update first manually
        getRow(finishedArray, row, 1, dimensions * processWorkload);
        updateRow(array, row, 1, dimensions * processWorkload);
        // for each process, receieve their results
        for (int i = 1; i < totalProcesses; i++) {
            MPI_Recv(processArray, (processWorkload + 1) * dimensions, MPI_DOUBLE, i, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            // add received array to finishedArray
            updateRow(array, processArray, (i * processWorkload) + 1, dimensions * processWorkload);
        }
        // output merged array
        printf("\n");
        printArray(array, dimensions, dimensions);
    }

    MPI_Finalize();
    free(processArray);
}