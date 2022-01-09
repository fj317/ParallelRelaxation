#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <sys/time.h>

// function to randomly generate double values and put them into array
void randomArrayGen(int size, double *array) {
    double randomValue;
    // uses fixed seed so same numbers generated each time
    srand(5);
    for (int i = 0; i < size; i++) {
        // only creates numbers 0-1
        randomValue = (rand() / (double)RAND_MAX);
        array[i] = randomValue;
    }
}

// procedure to print out an array
// can be 1D or 2D, uses pointer arithmetic 
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

// gets a row of an array
void getRow(double *array, double *row, int rowNum, int size) {
    int startIndex = rowNum * size;
    for (int i = startIndex; i < startIndex + size; i ++) {
        row[i - startIndex] = array[i];
    }

}

// updates a row in an array 
void updateRow(double *array, double*row, int rowNum, int size) {
    int startIndex = rowNum * size;
    for (int i = startIndex; i < startIndex + size; i ++) {
        array[i] = row[i - startIndex];
    }
}

// gets the contents of an array from a starting row
void getArray(double *array, double*valueArray, int rowNum, int dimensions, int elementNumber) {
    int startIndex = rowNum * dimensions;
    for (int i = startIndex; i < startIndex + elementNumber; i ++) {
        array[i - startIndex] = valueArray[i];
    }
}

// updates the contents of an array from a starting row
void updateArray(double *array, double*valueArray, int rowNum, int dimensions, int elementNumber) {
    int startIndex = rowNum * dimensions;
    for (int i = startIndex; i < startIndex + elementNumber; i ++) {
        array[i] = valueArray[i - startIndex];
    }
}


int main (void) {
    // initialise the MPI library
    MPI_Init(NULL, NULL);
    // used for timings
    struct timeval start, end;
    gettimeofday(&start, NULL);
    // the dimensions of the array
    int dimensions = 1000;
    // the desired precision for the results to fall into
    double precision = 0.01;
    // the total number of processes in use
    int totalProcesses;
    // gets the total number of processes
    MPI_Comm_size(MPI_COMM_WORLD, &totalProcesses);
    // processWorkload gives the number of rows that the process will work on
    int processWorkload;
    // totalProcessWorkload is used by the root node to partition the rows into chunks for every process
    int *totalProcessWorkload = malloc((unsigned long)totalProcesses * sizeof(int));
    // gives the process ID/number
    int processNumber;
    // gets the process number
    MPI_Comm_rank(MPI_COMM_WORLD, &processNumber);
    // recv and send variables for sending and receiving the row updates
    double *recvFirstRow = malloc((unsigned long) dimensions * sizeof(double));
    double *recvSecondRow = malloc((unsigned long) dimensions * sizeof(double));
    double *sendFirstRow = malloc((unsigned long) dimensions * sizeof(double));
    double *sendSecondRow = malloc((unsigned long) dimensions * sizeof(double));
    // required for non-blocking calls
    MPI_Request reqs[4];

    // stores the partitioned array the process works on
    double *processArray;
    // stores the finished array where the results are put into
    double *finishedArray;
    // is used by the root process to create the overall array that is to solve
    double *array = malloc((unsigned long)dimensions * (unsigned long)dimensions * sizeof(double));
    // is used by each process in logical AND to find whether precision is met for every process
    int recvPrecisionMet;
    // is used to know whether to quit the iteration loop or not
    int quitLoop = 0;
    // used to state whether the precision has been met or not
    int precisionMet;
    // the current array index being iterated over
    int currentValue;
    // the result of the averaging
    double result;
    // number of loop iterations done so far, used to reduce communciation when its not needed
    int loopIterations = 0;
    // if root or last node, can free memory as only send and receive 1 row
    if (processNumber == 0 || processNumber == totalProcesses - 1) {
        free(recvSecondRow);
        free(sendSecondRow);
    }

    // if process 0, setup array and partition into equal chunks and send to each process
    if (processNumber == 0) {
        // generate the array
        randomArrayGen(dimensions * dimensions, array);
        printArray(array, dimensions, dimensions);
        // set the workload for each process to the rounded down value of the number of 'working' rows (non-boundary rows) divided by the number of processes
        // i.e. split into even chunks
        for (int i = 0; i < totalProcesses; i++) {
            totalProcessWorkload[i] = (int)floor((dimensions-2) / totalProcesses);
        }
        // if the number of working dimensions doesn't fit evenly for each process
        // add 1 to the workload of each row until every row is being dealt with
        if ((dimensions-2) % totalProcesses != 0) {
            for (int i = 0; i < (dimensions-2) % totalProcesses; i++) {
                totalProcessWorkload[i]++;
            }
        }
        // set the workload of the root process
        processWorkload = totalProcessWorkload[0];
        // send processWorkload to each process
        for (int i = 1; i < totalProcesses; i++) {
            MPI_Send(&totalProcessWorkload[i], 1, MPI_INT, i, 1, MPI_COMM_WORLD);
        }
        // get space for processArray, use workload of root process as size because no other process can have higher workload
        processArray = malloc((unsigned long)(processWorkload + 2) * (unsigned long)dimensions * sizeof(double));
        int startRow = processWorkload;
        // get array that is an equal partition of array for each process
        for (int i = 1; i < totalProcesses; i++) {
            for (int j = 0; j < totalProcessWorkload[i] + 2; j++) {
                for (int k = 0; k < dimensions; k++) {
                    processArray[(j * dimensions) + k] = array[((startRow + j) * dimensions) + k];
                }
            }
            startRow += totalProcessWorkload[i];
            // send array to each process so it can begin work
            MPI_Send(processArray, (totalProcessWorkload[i] + 2) * dimensions, MPI_DOUBLE, i, 1, MPI_COMM_WORLD);
        }
        // get processArray for root node
        startRow = 0;
        for (int j = 0; j < processWorkload + 2; j++) {
            for (int k = 0; k < dimensions; k++) {
                processArray[(j * dimensions) + k] = array[((startRow + j) * dimensions) + k];
            }
        }
    } else {
        // if not root process
        // receive the workload
        MPI_Recv(&processWorkload, 1, MPI_INT, 0, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        // can free array as only used by root node
        free(array);
        free(totalProcessWorkload);
        // make space for the processArray the size of the workload + 2 (for the boundary rows)
        processArray = malloc((unsigned long)(processWorkload + 2) * (unsigned long)dimensions * sizeof(double));
        // receive the process array from the root process
        MPI_Recv(processArray, (processWorkload + 2) * dimensions, MPI_DOUBLE, 0, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    }
    // make space for the finished array (same size as processArray)
    finishedArray = malloc((unsigned long)(processWorkload + 2) * (unsigned long)dimensions * sizeof(double));
    while (quitLoop == 0) {
        result = 0.0;
        // initialise to 0 as starting from first value
        currentValue = 0;
        // initialise to true initially
        precisionMet = 1;
        // for each row
        for (int i = 0; i < processWorkload + 2; i++) {
            // for each dimension
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
                else if (currentValue % dimensions == dimensions - 1) {
                    finishedArray[currentValue] = processArray[currentValue];
                } else {
                    // calculate result (average 4 neighbour values)
                    result = (processArray[currentValue-1] + processArray[currentValue+1] + processArray[currentValue-dimensions] + processArray[currentValue+dimensions]) / 4.0;
                    // if precision isn't met for current value
                    // take absolute value of difference in case it is negative
                    if (fabs(result - processArray[currentValue]) > precision) {
                        // if precision isn't met then change precisiobMet to false
                        // then it will only be true if precision is met for every value
                        precisionMet = 0;
                        // only update finishedArray if there precision isn't met
                        finishedArray[currentValue] = result;
                    } else {
                        // if precision is met just add the original value from processArray
                        finishedArray[currentValue] = processArray[currentValue];
                    }
                }
                // add 1 to currentValue (iterate to next value)
                currentValue++;
            }
        }
        // only call MPI_ALLreduce when there have been more than 10 loop iterations
        // done to decrease overheads in idling when we know precision won't be met for the first 10 loops
        if (loopIterations > 10) {
            // use MPI_allreduce to find whether precision is met for all processes
            MPI_Allreduce(&precisionMet, &recvPrecisionMet, 1, MPI_INT, MPI_LAND, MPI_COMM_WORLD);
            // if precision met for all nodes
            if (recvPrecisionMet == 1) {
                // send finishedArray to root node
                if (processNumber != 0) {
                    // realloc to get more row memory
                    sendFirstRow = (double*)realloc(sendFirstRow, (unsigned long) dimensions * (unsigned long) processWorkload * sizeof(double));
                    // get all working rows from finishedArray 
                    getArray(sendFirstRow, finishedArray, 1, dimensions, dimensions * processWorkload);
                    // send array to root node so it can combine
                    MPI_Send(sendFirstRow, processWorkload * dimensions, MPI_DOUBLE, 0, 1, MPI_COMM_WORLD);
                }
                // quit loops
                quitLoop = 1;
            }       
        } 
        // if more than 1 core being used then communcation it required
        // if not quitting the loop send communication
        if (quitLoop == 0 && totalProcesses > 1) {
            // send first row to prev process, send last row to next process

            // if process 0 only send last row
            if (processNumber == 0) {
                //post non-blocking recv and sends for adjacent process
                MPI_Irecv(recvFirstRow, dimensions, MPI_DOUBLE, 1, 1, MPI_COMM_WORLD, &reqs[0]);
                // last row worked on is processWorkload
                getRow(finishedArray, sendFirstRow, processWorkload, dimensions);
                MPI_Isend(sendFirstRow, dimensions, MPI_DOUBLE, 1, 1, MPI_COMM_WORLD, &reqs[1]);
                // wait until the MPI communication has completed
                MPI_Waitall(2, reqs, MPI_STATUS_IGNORE);
                // update rows in arrays                
                updateRow(finishedArray, recvFirstRow, processWorkload + 1, dimensions);
            } // else if process is last process only send first row to prev process
            else if (processNumber == totalProcesses - 1) {
                //post non-blocking recv and sends for adjacent process
                MPI_Irecv(recvFirstRow, dimensions, MPI_DOUBLE, processNumber - 1, 1, MPI_COMM_WORLD, &reqs[0]);
                // last row worked on is processWorkload
                getRow(finishedArray, sendFirstRow, 1, dimensions);
                MPI_Isend(sendFirstRow, dimensions, MPI_DOUBLE, processNumber - 1, 1, MPI_COMM_WORLD, &reqs[1]);
                // wait until the MPI communication has completed
                MPI_Waitall(2, reqs, MPI_STATUS_IGNORE);
                // update rows in arrays                
                updateRow(finishedArray, recvFirstRow, 0, dimensions);
            } // otherwise send both rows
            else {
                //post non-blocking recv and sends for adjacent processes
                MPI_Irecv(recvFirstRow, dimensions, MPI_DOUBLE, processNumber - 1, 1, MPI_COMM_WORLD, &reqs[0]);
                MPI_Irecv(recvSecondRow, dimensions, MPI_DOUBLE, processNumber + 1, 1, MPI_COMM_WORLD, &reqs[1]);
                // gets the first and last working rows
                getRow(finishedArray, sendFirstRow, 1, dimensions);
                getRow(finishedArray, sendSecondRow, processWorkload, dimensions);
                MPI_Isend(sendFirstRow, dimensions, MPI_DOUBLE, processNumber - 1, 1, MPI_COMM_WORLD, &reqs[2]);
                MPI_Isend(sendSecondRow, dimensions, MPI_DOUBLE, processNumber + 1, 1, MPI_COMM_WORLD, &reqs[3]);
                // wait until the MPI communication has completed
                MPI_Waitall(4, reqs, MPI_STATUS_IGNORE);
                // update rows in arrays
                updateRow(finishedArray, recvFirstRow, 0, dimensions);
                updateRow(finishedArray, recvSecondRow, processWorkload + 1, dimensions);
            }
        }
        // update array pointers so process array is now finished array
        void *tempPtr = processArray;
        processArray = finishedArray;
        finishedArray = tempPtr;
        // add 1 to loopIteration as loop completed now
        loopIterations++;
    }
    // process 0 merges arrays
    if (processNumber == 0) {
        // update first manually
        // realloc row to get more memory
        sendFirstRow = (double*)realloc(sendFirstRow, (unsigned long)dimensions * (unsigned long)processWorkload * sizeof(double));
        // gets the working rows of finishedArray and put it in sendFirstRow
        getArray(sendFirstRow, finishedArray, 1, dimensions, dimensions * processWorkload);
        // update array with sendFirstRow variable
        updateArray(array, sendFirstRow, 1, dimensions, dimensions * processWorkload);
        int currentRow = 1 + processWorkload;
        // for each process, receieve their results
        for (int i = 1; i < totalProcesses; i++) {
            MPI_Recv(processArray, (totalProcessWorkload[i] + 1) * dimensions, MPI_DOUBLE, i, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            // add received array to finishedArray
            updateArray(array, processArray, currentRow, dimensions, dimensions * totalProcessWorkload[i]);
            // add the workload of that process to the currentRow so it knows what row to change
            currentRow += totalProcessWorkload[i];
        }
        // timings
        gettimeofday(&end, NULL);
        double timeDifference = (double) (end.tv_usec - start.tv_usec) / 1000000 + (double) (end.tv_sec - start.tv_sec);
        // output merged array
        printf("\n");
        //printArray(array, dimensions, dimensions);
        printf("Total time taken: %f\n", timeDifference);
        // free array memory
        free(array);
    }
    // finalise MPI communication
    MPI_Finalize();
    // free up memory
    free(processArray);
    free(finishedArray);
    free(sendFirstRow);
    free(recvFirstRow);
    // if not root or last process, free up second recv & send
    if (processNumber != 0 && processNumber != totalProcesses - 1) {
        free(sendSecondRow);
        free(recvSecondRow);
    }
}