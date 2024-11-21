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

#define DEBUG 0

#define NTIMES (long long)10 // Numero de vezes que as buscas serao feitas
#define MAX_THREADS 8 // Numero maximo de threads
#define MAX_TOTAL_ELEMENTS (long long)16e6 // Tam max do vetor de input
#define NP 100000 // Tamanho do vetor p

long long InputG[NTIMES * MAX_TOTAL_ELEMENTS];
long long OutputG[NTIMES * MAX_TOTAL_ELEMENTS];
long long PG[NTIMES * NP];
int PosG[NTIMES * NP];

int nThreads;       // numero efetivo de threads
int nTotalElements; // numero total de elementos

bool initialized = false;

typedef struct {
    int start, end;
    long long *Input, *P;
    int *partition_size, *partition_of;
    int np, myIndex;
} thread_args_t;

pthread_t Thread[MAX_THREADS];
int Thread_id[MAX_THREADS];
pthread_barrier_t bsearch_barrier;
thread_args_t thread_args[MAX_THREADS];

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

// retorna indice do primeiro elemento estritamente MAIOR que x
int upper_bound(long long x, long long *v, int n) {
    int first = 0, last = n - 1, ans = -1; 
    while (first <= last) {
        int m = first + (last - first) / 2;

        if (v[m] > x) {
            ans = m;
            last = m - 1;
        } else {
            first = m + 1;
        }
    }

    return ans;
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
        printf("\n\n");
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
                printf("%lld na particao de [%lld, %lld)\n\n", Output[j], lim_inf, lim_sup);
                return;
            }
        }
    }
    printf("===> particionamento CORRETO\n\n");
    return;
}

void *find_partitions(void *ptr) {
    thread_args_t args = *(thread_args_t*)ptr;

    while (true) {
        pthread_barrier_wait(&bsearch_barrier);       

        for (int i = args.start; i <= args.end; i++) {
            int partition = upper_bound(args.Input[i], args.P, args.np);
            args.partition_size[partition]++;
            args.partition_of[i] = partition;
        }

        pthread_barrier_wait(&bsearch_barrier);       

        if (args.myIndex == 0)
            return NULL;
    }

}


void multi_partition( long long *Input, int n, long long *P, int np, long long *Output, int *Pos ) {
    int partition_of[n];

    if (!initialized) {

        // initialize thread args
        int chunk_size = (n + nThreads - 1) / nThreads;
        for (int i = 0; i < nThreads; i++) {
            thread_args[i].myIndex = i;
            thread_args[i].Input = Input;
            thread_args[i].P = P;
            thread_args[i].np = np;
            thread_args[i].start = i * chunk_size;
            thread_args[i].end = (i + 1) * chunk_size - 1;
            thread_args[i].partition_of = partition_of; 
            thread_args[i].partition_size = (int*) malloc(np * sizeof(int)); 

            if (thread_args[i].end >= n) {
                thread_args[i].end = n-1;
            }
            
            if (DEBUG)
                printf("Thread %d vai processar de %d até %d\n", i, thread_args[i].start, thread_args[i].end);
        }

        // intialize threads
        if (pthread_barrier_init(&bsearch_barrier, NULL, nThreads) != 0) {
            perror("pthread_barrier_init");
            exit(EXIT_FAILURE);
        }

        // cria todas as outra threads trabalhadoras
        for (int i = 1; i < nThreads; i++) {
            if (pthread_create(&Thread[i], NULL, find_partitions, &thread_args[i]) != 0) {
                perror("pthread_create");
                exit(EXIT_FAILURE);
            }
        }
        initialized = true;
    }

    for (int i = 0; i < nThreads; i++) {
        memset(thread_args[i].partition_size, 0, np*sizeof(int)); 
    }

    find_partitions(&thread_args[0]);

    // calcular tamanho total das particoes
    int partition_size[np];
    memset(partition_size, 0, (np * sizeof(int)));
    for (int i = 0; i < nThreads; i++)
        for (int j = 0; j < np; j++)
            partition_size[j] += thread_args[i].partition_size[j];

    // calcular indice de inicio de cada particao em Output
    int start_index[np]; 
    memset(start_index, 0, (np * sizeof(int)));
    for (int i = 1; i < np; i++) 
        start_index[i] += start_index[i - 1] + partition_size[i - 1];

    // os indices de inicio sao o vetor Pos 
    memcpy(Pos, start_index, sizeof(start_index));

    // montar output
    for (int i = 0; i < n; i++) {
        int partition = partition_of[i];
        Output[start_index[partition]] = Input[i];
        start_index[partition]++;
    } 

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
            printf("<nTotalElements> must be up to %lld\n", MAX_TOTAL_ELEMENTS);
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
        printf("Input: ");
        for (int i = 0; i < nTotalElements; i++) printf("%lld ", InputG[i]);
        printf("\n\nP: ");
        for (int i = 0; i < NP; i++) printf("%lld ", PG[i]);
        printf("\n\n");
    }

    chrono_reset(&chrono_time);
    chrono_start(&chrono_time);

    int p_start = 0, io_start = 0;
    long long *Input, *P, *Output;
    int *Pos;

    for (int i = 0; i < NTIMES; i++) {
        printf("\n\n==== TIME %d ====\n\n", i);
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
