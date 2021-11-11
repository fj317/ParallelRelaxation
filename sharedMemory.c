#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>

int randomArrayGen(int size, double array[][size]) {
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
    return 0;
}

int main(void) {

    printf("---NEW PROGRAM--- \n");
    int integerDimension = 4;
    double precision = 0.001;
    //double array[4][4] = {{1.0, 1.0, 1.0, 1.0}, {1.0, 0.0, 0.0, 0.0}, {1.0, 0.0, 0.0, 0.0}, {1.0, 0.0, 0.0, 0.0}};
    double (*array)[integerDimension] = malloc(sizeof(double[integerDimension][integerDimension]));
    randomArrayGen(integerDimension, array);
    double (*newArray)[integerDimension] = malloc(sizeof(double[integerDimension][integerDimension]));;

    for (int i = 0; i < integerDimension; i++) {
        for (int j = 0; j < integerDimension; j++) {
            printf(" %lf ", array[i][j]);
        }
        printf("\n");
    }
    printf("\n\n\n");


    // copy boundary values from array into newarray
    memcpy(newArray, array, sizeof(double[integerDimension][integerDimension]));
    int precisionCount = 0;
    int iterateOnce = 0;

    while (1) {
        double result = 0.0;
        // average 4 numbers around it
        for (int i = 1; i < integerDimension - 1; i++) {
            //printf("i is %d\n", i);
            for (int j = 1; j < integerDimension - 1; j++) {
                result = (array[i-1][j] + array[i+1][j] + array[i][j-1] + array[i][j+1]) / 4;
                //printf("Result for i: %d, j: %d is result %f\n", i, j, result);
                // if precision met for every value
                if (precisionCount == (integerDimension-2)*(integerDimension-2)) {
                    goto printArray;
                // check if precision is reached
                } else if (result - newArray[i][j] < precision && iterateOnce == 1)
                {
                    precisionCount++;

                } else {
                    newArray[i][j] = result;
                    precisionCount = 0;
                }
            }
        }
        iterateOnce = 1;
        memcpy(array, newArray, sizeof(double[integerDimension][integerDimension]));
    //     for (int i = 0; i < integerDimension; i++) {
    //     for (int j = 0; j < integerDimension; j++) {
    //         printf(" %lf ", newArray[i][j]);
    //     }
    //     printf("\n");
    // }
    // printf("\n\n");
    }

    printArray: for (int i = 0; i < integerDimension; i++) {
        for (int j = 0; j < integerDimension; j++) {
            printf(" %lf ", newArray[i][j]);
        }
        printf("\n");
    }


}