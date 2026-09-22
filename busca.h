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

class Fronteira
{
public:

    virtual ~Fronteira() = default;


    virtual bool adicionar(
        int indice,
        const NoBusca& no
    ) = 0;


    virtual int remover() = 0;


    virtual bool vazia() const = 0;


    virtual void limpar() = 0;
};

ResultadoBusca buscarGenerico(
    const EstadoCubo& inicial,
    Fronteira& fronteira
);


#endif