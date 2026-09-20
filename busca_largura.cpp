#include "busca_largura.h"

bool FronteiraBFS::adicionar(
    int indice,
    const NoBusca& no
)
{

    auto [iterador, inseriu] =
        visitados.insert(no.estado);

    if (!inseriu)
    {
        return false;
    }
    fila.push(indice);

    return true;
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

    visitados.clear();
}
ResultadoBusca buscaEmLargura(
    const EstadoCubo& inicial
)
{
    FronteiraBFS fronteira;

    return buscarGenerico(
        inicial,
        fronteira
    );
}