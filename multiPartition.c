#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <errno.h>
#include <sys/time.h>
#include <stdlib.h>
#include <stdbool.h>
#include <limits.h>

#include "chrono.c"

#define DEBUG 1

#define NTIMES 1 // Numero de vezes que as buscas serao feitas
#define MAX_THREADS 8 // Numero maximo de threads
#define MAX_TOTAL_ELEMENTS (int)16e6 // Tam max do vetor de input
#define NP 5 // Tamanho do vetor p

long long InputG[NTIMES * MAX_TOTAL_ELEMENTS];
long long OutputG[NTIMES * MAX_TOTAL_ELEMENTS];
long long PG[NTIMES * NP];
int PosG[NTIMES * NP];

int nThreads;       // numero efetivo de threads
int nTotalElements; // numero total de elementos

int compare(const void *a, const void *b) {
    const long long int_a = *(const long long *)a;
    const long long int_b = *(const long long *)b;

    if (int_a < int_b)
        return -1;
    else if (int_a > int_b)
        return 1;
    else
        return 0;
}

long long geraAleatorioLL() {
    int a = rand();  // Returns a pseudo-random integer
                //    between 0 and RAND_MAX.
    int b = rand();  // same as above
    long long v = (long long)a * 100 + b;
    return v%100;
}

void verifica_particoes( long long *Input, int n, long long *P, int np, long long *Output, int *Pos ) {
    if (DEBUG) {
        printf("Pos = ");
        for (int i = 0; i < np; i++) printf("%d ", Pos[i]);
        
        printf("\nOut = ");
        for (int i = 0; i < n; i++) printf("%lld ", Output[i]);
        printf("\n");
    }

    for (int i = 0; i < np; i++) {
        long long lim_inf = i > 0 ? P[i-1] : 0; 
        long long lim_sup = P[i]; 


        int start = Pos[i];
        int end = i < np - 1 ? Pos[i+1] : n;

        // Nao tem nenhum numero no intervalo
        if (start == end) continue;

        // j E [start, end) => Output[j] E [lim_inf, lim_sup)
        for(int j = start; j < end; j++) {
            if ((Output[j] < lim_inf) || (Output[j] >= lim_sup)) {
                printf("===> particionamento COM ERROS\n");
                printf("%lld na particao de [%lld, %lld)\n\n", Output[i], lim_inf, lim_sup);
                return;
            }
        }
    }
    printf("===> particionamento CORRETO\n\n");
    return;
}


void multi_partition( long long *Input, int n, long long *P, int np, long long *Output, int *Pos ) {
    printf("=== Multi Partition ===\n\n");
    return;
}
        
int main(int argc, char *argv[]) {
    chronometer_t chrono_time;

    if (argc != 3) {
        printf("usage: %s <nTotalElements> <nThreads>\n", argv[0]);
        return 0;
    } else {
        nThreads = atoi(argv[2]);

        if (nThreads == 0) {
            printf("usage: %s <nTotalElements> <nThreads>\n", argv[0]);
            printf("<nThreads> can't be 0\n");
            return 0;
        } if (nThreads > MAX_THREADS) {
            printf("usage: %s <nTotalElements> <nThreads>\n", argv[0]);
            printf("<nThreads> must be less than %d\n", MAX_THREADS);
            return 0;
        }

        nTotalElements = atoi(argv[1]);
        if (nTotalElements > MAX_TOTAL_ELEMENTS) {
            printf("usage: %s <nTotalElements> <nThreads>\n", argv[0]);
            printf("<nTotalElements> must be up to %d\n", MAX_TOTAL_ELEMENTS);
            return 0;
        }
    }

    printf("Will use %d threads to separate %d items on %d partitions\n\n", nThreads, nTotalElements, NP);

    srand((unsigned int)1);

    printf("Initializing Input vector...\n\n");

    for (int i = 0; i < nTotalElements; i++) {
        long long num = geraAleatorioLL();
        for (int j = 0; j < NTIMES; j++) {
            InputG[i + (j*nTotalElements)] = num;
        }
    }

    // Popular PG com NP elementos aleatorios
    for (int i = 0; i < NP - 1; i++) {
        PG[i] = geraAleatorioLL();
    }
    PG[NP - 1] = LLONG_MAX;

    // Ordenar
    qsort(PG, NP, sizeof(long long), compare);

    // Criar copias concatenadas para evitar efeitos de cache
    for (int i = 1; i < NTIMES; i++) {
        for (int j = 0; j < NP; j++) {
            PG[(i*NP) + j] = PG[j];
        }
    }

    if (DEBUG) {
        printf("InputG: ");
        for (int i = 0; i < NTIMES * nTotalElements; i++) printf("%lld ", InputG[i]);
        printf("\n\nPG: ");
        for (int i = 0; i < NTIMES * NP; i++) printf("%lld ", PG[i]);
        printf("\n\n");
    }

    chrono_reset(&chrono_time);
    chrono_start(&chrono_time);

    int p_start = 0, io_start = 0;
    long long *Input, *P, *Output;
    int *Pos;

    for (int i = 0; i < NTIMES; i++) {
        Input = &InputG[io_start];
        Output = &OutputG[io_start];
        P = &PG[p_start];
        Pos = &PosG[p_start];

                
        multi_partition(Input, nTotalElements, P, NP, Output, Pos );
        verifica_particoes(Input, nTotalElements, P, NP, Output, Pos );

        io_start += nTotalElements;
        p_start += NP;
    }

    // COMO CALCULAR ESSSE TEMPO?
    // AQUI ELE ESTA CALCULANDO O TEMPO TOTAL PARA RODAR NTIMES
    // TEM QUE REPORTAR O TEMPO PARA CADA UMA OU A MEDIA??  
    // calcular e imprimir a VAZAO (numero de operacoes/s)
    chrono_stop(&chrono_time);


    double total_time_in_seconds = (double)chrono_gettotal(&chrono_time) /
                                   ((double)1000 * 1000 * 1000);
    printf("total_time_in_seconds: %lf s\n", total_time_in_seconds);

    return 0; 
}
