#include "busca_a_estrela.h"

#include "simetria.h"

bool FronteiraAEstrela::adicionar(
    int indice,
    const NoBusca& no
)
{
    EstadoCubo canonico = estadoCanonico(no.estado);

    if (!visitados.insert(canonico).second)
    {
        return false;
    }

    Entrada entrada;
    entrada.indice = indice;
    entrada.custo = no.profundidade + tabela.h(no.estado);
    entrada.ordem = proximaOrdem++;
    fila.push(entrada);

    return true;
}

int FronteiraAEstrela::remover()
{
    int indice = fila.top().indice;
    fila.pop();
    return indice;
}

bool FronteiraAEstrela::vazia() const
{
    return fila.empty();
}

void FronteiraAEstrela::limpar()
{
    while (!fila.empty())
    {
        fila.pop();
    }

    visitados.clear();
    proximaOrdem = 0;
}

ResultadoBusca buscaAEstrela(const EstadoCubo& inicial)
{
    FronteiraAEstrela fronteira;
    return buscarGenerico(inicial, fronteira);
}
