#ifndef BUSCA_LARGURA_H
#define BUSCA_LARGURA_H

#include <queue>
#include <unordered_set>

#include "busca.h"

class FronteiraBFS
{
private:
 std::queue<int> fila;

    std::unordered_set<
        EstadoCubo,
        EstadoHash
    > visitados;

public:
  bool adicionar(
        int indice,
        const NoBusca& no
    ) override;

    int remover();

    bool vazia() const;

    void limpar();
};


ResultadoBusca buscaEmLargura(
    const EstadoCubo& inicial
);

#endif