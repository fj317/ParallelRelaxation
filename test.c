#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <stdio.h>

int main(void) {
    double randomValue;
    srand ( time ( NULL));
    randomValue = (double)rand()/RAND_MAX * 2.0-1.0;
    printf("random %lf\n", randomValue);

    //double div = RAND_MAX / 1;
    randomValue = (rand() / (double)RAND_MAX);
    printf("random %lf", randomValue);


    return 0;
}
