#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

#ifdef __linux__
#include <sched.h>
#include <pthread.h>
int pinToCore(int coreId)
{
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(coreId, &cpuset);
    return pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
}
#elif defined(__APPLE__)
//cant really bind to core :(
#include <pthread.h>
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
//stop hopping between clusters (p0 and p1) --> only use shared l2 cache????


#endif

double runMatrixMul(int iterations, int n);

int main(void)
{
    srand(time(NULL));
    pinToCore(0);
    double time = runMatrixMul(100, 512);
    printf("time elapsed: %lf", time);
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
