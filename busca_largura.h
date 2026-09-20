#ifndef BUSCA_LARGURA_H
#define BUSCA_LARGURA_H

#include <queue>
#include <unordered_set>

#include "busca.h"

class FronteiraBFS : public Fronteira
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

    int remover() override;

    bool vazia() const override;

    void limpar() override;
};


ResultadoBusca buscaEmLargura(
    const EstadoCubo& inicial
);

#endif