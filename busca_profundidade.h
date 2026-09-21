#ifndef BUSCA_PROFUNDIDADE_H
#define BUSCA_PROFUNDIDADE_H

#include "busca.h"

/*
 * Busca em Profundidade com Aprofundamento Iterativo (IDDFS)
 *
 * A cada iteracao aumenta o limite de profundidade em 1.
 * Usa simetria (estadoCanonico) para poda de estados equivalentes
 * dentro de cada caminho explorado.
 *
 * Propriedades:
 *  - Completo: sim
 *  - Otimo: sim (encontra a solucao de menor profundidade)
 *  - Memoria: O(b * d) -- muito menor que BFS O(b^d)
 *  - Tempo: ligeiramente maior que BFS (re-explora estados)
 */
ResultadoBusca buscaEmProfundidade(
    const EstadoCubo& inicial
);

#endif
