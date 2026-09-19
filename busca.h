#ifndef BUSCA_H
#define BUSCA_H

#include "estado.h"

#include <cstddef>
#include <vector>

struct NoBusca
{
    EstadoCubo estado;

    int pai = -1;

    Movimento movimento =
        Movimento::NENHUM;

    int profundidade = 0;
};

struct ResultadoBusca
{
    bool encontrou = false;

    std::vector<Movimento> caminho;

    std::size_t estadosVisitados = 0;

    int profundidade = -1;
};

#endif