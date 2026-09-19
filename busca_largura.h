#ifndef BUSCA_LARGURA_H
#define BUSCA_LARGURA_H

#include <cstddef>
#include <vector>

#include "estado.h"

struct ResultadoBFS {
    bool encontrou = false;

    std::vector<Movimento> caminho;

    std::size_t estadosVisitados = 0;

    int profundidade = -1;
};

ResultadoBFS buscaEmLargura(
    const EstadoCubo& inicial
);

#endif