# ParallelRelaxation
Using C to perform relaxation technique in parallel to solve differential equations.

The three files all implement the same algorithm to solve differential equations by using a method known as relaxation technique. This is done by having an array of values and repeatedly replacing a value with the average of its four neighbours; excepting boundary values, which remain at fixed values. This is repeated until all values settle down to within a given precision.

## sequentialProgram.c
This program implements the algorithm described above sequentially. It takes **integerDimension** (the number of dimensions/size of the array) and **precision** (the required precision to reach) as input parameters. 

## sharedMemoryParallel.c
This version uses the POSIX thread (pthread) library and assumes a shared memory system is being used. It recreates the algorithm in parallel by partitioning the array into rows and getting each thread to calculate all row values before moving to the next. This continues until the desired precision has been met for every value within the array. Like the previous program, it takes **integerDimension** (the number of dimensions/size of the array) and **precision** (the required precision to reach) as input parameters. Additionally, it takes **totalThreads** (the total number of threads to use) as a parameter. The program uses locks to ensure race conditions are not made when updating shared variables, and barriers to ensure synchronisation before starting each iteration.

Unexpectedly, using multiple threads does not achieve speedup in this program. I hypothesise that this is due major overheads in the code (perhaps the use of barriers, or the way the array is partitioned) however further work is required to track down this overhead.

## distributedMemoryParallel.c
This version uses the MPI library and assumes a distributed memory system. It follows a similar approach as the shared memory version. The root process partitions the array into chunks and then communicates to the other processes/nodes their partitions. At the end of each iteration, the nodes communicate with the neighbouring nodes to send updated rows that can be used to calculate the next iteration of values.

This program achieves good speedup with larger sized arrays 


The programs can be compiled with your prefered C compiler. The POSIX library and MPI librarys are required to run their respective programs.
