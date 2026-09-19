#include "busca_largura.h"

void FronteiraBFS::adicionar(int indice)
{
    fila.push(indice);
}

int FronteiraBFS::remover()
{
    int indice = fila.front();

    fila.pop();

    return indice;
}

bool FronteiraBFS::vazia() const
{
    return fila.empty();
}

void FronteiraBFS::limpar()
{
    while (!fila.empty())
    {
        fila.pop();
    }
}