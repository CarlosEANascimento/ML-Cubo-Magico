#include "busca_largura.h"
#include "simetria.h"

bool FronteiraBFS::adicionar(
    int indice,
    const NoBusca& no
)
{

    EstadoCubo canonico =
        estadoCanonico(no.estado);

    bool inseriu =
        visitados.insert(canonico).second;
        
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