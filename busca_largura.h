#ifndef BUSCA_LARGURA_H
#define BUSCA_LARGURA_H

#include <queue>

#include "busca.h"

class FronteiraBFS
{
private:
    std::queue<int> fila;

public:
    void adicionar(int indice);

    int remover();

    bool vazia() const;

    void limpar();
};

#endif