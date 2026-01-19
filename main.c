#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <pthread.h>
#include <unistd.h>

#ifdef __linux__
#include <sched.h>
#include <D_GNU_SOURCE>

int pinToCore(int coreId)
{
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(coreId, &cpuset);
    return pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
}
#elif defined(__APPLE__)
//cant really bind to core :(

#include <pthread/qos.h>
#include <mach/mach.h>

void pinToCore(int coreId)
{
    int ret = pthread_set_qos_class_self_np(QOS_CLASS_USER_INTERACTIVE, 0);
    if (ret != 0)
    {
        printf("pthread_set_qos_class_self_np failed\n");
    }
    else
    {
        printf("pthread_set_qos_class_self_np success\n");
    }
}
#endif

typedef struct
{
    int coreId;
    double *matrixA, *matrixB, *matrixC;
    int n; //size
    double compute;
    int iterations;
} threadData;


double runMatrixMul(int iterations, int n);
void initRandom(double* M, int iterations);
void* multiMatrixMul(void* arg);

int main(int argc, char** argv)
{
    int nthreads = 1;
    int n = 256;
    int it = 10;

    int opt;
    while ((opt = getopt(argc, argv, "t:n:i:")) != -1)
    {
        switch (opt)
        {
            case 't':
                nthreads = atoi(optarg);
                break;
            case 'n':
                n = atoi(optarg);
                break;
            case 'i':
                it = atoi(optarg);
                break;
        }
    }



    srand(time(NULL));
    if (nthreads == 1)
    {
        pinToCore(0);
        double time = runMatrixMul(it, n);
        printf("time elapsed: %lf", time);
    }
    else
    {
        pthread_t threads[nthreads];
        threadData threadDatas[nthreads];
        for (int i = 0; i < nthreads; i++)
        {
            threadDatas[i].coreId = i;
            threadDatas[i].n = n;
            threadDatas[i].matrixA = malloc(n * n * sizeof(double));
            threadDatas[i].matrixB = malloc(n * n * sizeof(double));
            threadDatas[i].matrixC = malloc(n * n * sizeof(double));
            initRandom(threadDatas[i].matrixA, n);
            initRandom(threadDatas[i].matrixB, n);
            threadDatas[i].iterations = it;

            pthread_create(&threads[i], NULL, multiMatrixMul, &threadDatas[i]);
        }
        double totalCompute = 0.0;
        for (int i = 0; i < nthreads; i++)
        {
            pthread_join(threads[i], NULL);
            totalCompute += threadDatas[i].compute;
            free(threadDatas[i].matrixA);
            free(threadDatas[i].matrixB);
            free(threadDatas[i].matrixC);
        }

        printf("total compute %.3f \n", totalCompute);
    }

    return 0;
}

double runMatrixMul(int iterations, int n)
{
    clock_t t = clock();

    double *matrixA = malloc(n * n * sizeof(double));
    double *matrixB = malloc(n * n * sizeof(double));
    double *matrixC = malloc(n * n * sizeof(double));

    for (int i = 0; i < iterations; i++)
    {

        //randomizing + clearing matrices
        for (int j = 0; j < n; j++)
        {
            for (int k = 0; k < n; k++)
            {
                matrixA[j * (k + 1) ] = (double) rand() / (double) RAND_MAX;
                matrixB[j * (k + 1) ] = (double) rand() / (double) RAND_MAX;
                matrixC[j * (k + 1) ] = 0;
            }
        }


        for (int j = 0; j < n; j++ )
        {
            for (int k = 0; k < n; k++)
            {
                double sum = 0;
                for (int l = 0; l < n; l++)
                {
                    sum += matrixA[j * n + l] * matrixB[l * n + k];
                }
                matrixC[j * n + k] = sum;
            }
        }
        // for (int j = 0; j < n; j++)
        // {
        //     for (int k = 0; k < n; k++)
        //     {
        //         printf("%lf ", matrixC[k * n + j]);
        //     }
        //     printf("\n");
        // }
        // printf("\n \n");
    }

    free(matrixA);
    free(matrixB);
    free(matrixC);

    t = clock() - t;
    return ((double)t)/CLOCKS_PER_SEC;
}


void* multiMatrixMul(void* arg)
{
    threadData* data = (threadData*)arg;
    pinToCore(data->coreId);
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);
    //calculation
    for (int z = 0; z < data->iterations; z++)
    {
        for (int j = 0; j < data->n; j++ )
        {
            for (int k = 0; k < data->n; k++)
            {
                double sum = 0;
                for (int l = 0; l < data->n; l++)
                {
                    sum += data->matrixA[j * data->n + l] * data->matrixB[l * data->n + k];
                }
                data->matrixC[j * data->n + k] = sum;
            }
        }
    }
    clock_gettime(CLOCK_MONOTONIC, &end);
    double sec = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1000000000.0;
    data->compute = (2.0 * data-> n * data->n * data->n * data->iterations) / (sec * 1000000.0);
    return NULL;
}

void initRandom(double* M, int iterations)
{
    for (int i = 0; i < iterations; i++)
    {
        for (int j = 0; j < iterations; j++)
        {
            M[i * (1 + j)] = rand() / (double) RAND_MAX;
        }
    }
}