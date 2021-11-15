#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <math.h>

void randomArrayGen(int size, double array[][size]) {
    double randomValue;
    srand ( (unsigned int)time ( NULL));
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

void putValuesIntoArray(int size, double **array, double valueArray[][size]) {
    for (int i = 0; i < size; i++) {
        for (int j = 0; j < size; j++) {
            array[i][j] = valueArray[i][j];
        }
    }
}

void copyArray(int size, double **emptyArray, double **dataArray) {
    for (int i = 0; i < size; i++) {
        for (int j = 0; j < size; j++) {
            emptyArray[i][j] = dataArray[i][j];
        }
    }
}

int main(void) {

    printf("---NEW PROGRAM--- \n");
    int integerDimension = 5;
    double precision = 0.01;
    double testArray[5][5] = {{1.0, 1.0, 1.0, 1.0, 1.0}, {1.0, 0.3, 0.7, 0.8, 0.12}, {1.0, 0.5, 0.15, 0.23, 0.76}, {1.0, 0.2, 0.0, 0.97, 0.41}, {1.0, 0.5, 0.0, 0.25, 0.8}};
    //double array[4][4] = {{1.0, 1.0, 1.0, 1.0}, {1.0, 0.0, 0.0, 0.0}, {1.0, 0.0, 0.0, 0.0}, {1.0, 0.0, 0.0, 0.0}};
    //randomArrayGen(integerDimension, array);
    double **array = malloc((unsigned long)integerDimension * sizeof(double *));
    double **newArray = malloc((unsigned long)integerDimension * sizeof(double *));
    for (int i = 0; i < integerDimension; i++) {
        array[i] = malloc((unsigned long)integerDimension * sizeof(double));
        newArray[i] = malloc((unsigned long)integerDimension * sizeof(double));
    }

    putValuesIntoArray(integerDimension, array, testArray);

    for (int i = 0; i < integerDimension; i++) {
        for (int j = 0; j < integerDimension; j++) {
            printf(" %lf ", array[i][j]);
        }
        printf("\n");
    }
    printf("\n\n");


    // copy boundary values from array into newarray
    copyArray(integerDimension, newArray, array);
    int precisionCount = 0;

    while (1) {
        double result = 0.0;
        // average 4 numbers around it
        for (int i = 1; i < integerDimension - 1; i++) {
            //printf("i is %d\n", i);
            for (int j = 1; j < integerDimension - 1; j++) {
                result = (array[i-1][j] + array[i+1][j] + array[i][j-1] + array[i][j+1]) / 4;
                //printf("Working on i: %d, j:%d. Result is %lf\n", i, j, result);
                //printf("above: %lf, below %lf, left %lf, right %lf\n", array[i-1][j], array[i+1][j], array[i][j-1], array[i][j+1]);
                // if precision met for every value
                if (precisionCount >= (integerDimension-2)*(integerDimension-2)) {
                    goto printArray;
                // check if precision is reached
                } else if (fabs(result - newArray[i][j]) < precision)
                {
                    precisionCount++;

                } else {
                    newArray[i][j] = result;
                    precisionCount = 0;
                }
            }
        }
        copyArray(integerDimension, array, newArray);
        for (int i = 0; i < integerDimension; i++) {
            for (int j = 0; j < integerDimension; j++) {
                printf(" %lf ", array[i][j]);
            }
        printf("\n");
        }
        printf("\n\n");
    }

    printArray: for (int i = 0; i < integerDimension; i++) {
        for (int j = 0; j < integerDimension; j++) {
            printf(" %lf ", newArray[i][j]);
        }
        printf("\n");
    }
}