#include <stdio.h>

void verifica_particoes( long long *Input, int n, long long *P, int np, long long *Output, int *Pos ) {
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
                printf("\n===> particionamento COM ERROS\n");
                printf("%lld na particao de [%lld, %lld)\n\n", Output[j], lim_inf, lim_sup);
                return;
            }
        }
    }
    printf("\n===> particionamento CORRETO\n\n");
    return;
}