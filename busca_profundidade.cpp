#include "busca_profundidade.h"
#include "simetria.h"


/* ---------------------------------------------------------------
 * FronteiraDFS -- Implementacao dos metodos da interface Fronteira
 * usando uma pilha (std::stack) e limite de profundidade.
 * --------------------------------------------------------------- */

bool FronteiraDFS::adicionar(
    int indice,
    const NoBusca& no
)
{
    // Poda por profundidade: rejeita nos alem do limite
    if (no.profundidade > limiteProf)
    {
        return false;
    }

    // Poda por simetria: estados canonicamente equivalentes
    // ja visitados nesta iteracao sao descartados
    EstadoCubo canonico =
        estadoCanonico(no.estado);

    bool inseriu =
        visitados.insert(canonico).second;

    if (!inseriu)
    {
        return false;
    }

    pilha.push(indice);

    return true;
}

int FronteiraDFS::remover()
{
    int indice = pilha.top();

    pilha.pop();

    return indice;
}

bool FronteiraDFS::vazia() const
{
    return pilha.empty();
}

void FronteiraDFS::limpar()
{
    while (!pilha.empty())
    {
        pilha.pop();
    }

    visitados.clear();
}


/* ---------------------------------------------------------------
 * IDDFS -- Iterative Deepening Depth-First Search
 *
 * Executa buscarGenerico() com FronteiraDFS, aumentando o limite
 * de profundidade a cada iteracao ate encontrar a solucao.
 *
 * O contador de estadosVisitados acumula entre todas as iteracoes,
 * refletindo o custo real total da busca.
 * --------------------------------------------------------------- */

ResultadoBusca buscaEmProfundidade(
    const EstadoCubo& inicial
)
{
    ResultadoBusca acumulado;

    for (int limite = 0; ; limite++)
    {
        FronteiraDFS fronteira(limite);

        ResultadoBusca parcial =
            buscarGenerico(inicial, fronteira);

        acumulado.estadosVisitados +=
            parcial.estadosVisitados;

        if (parcial.encontrou)
        {
            acumulado.encontrou    = true;
            acumulado.profundidade = parcial.profundidade;
            acumulado.caminho      = parcial.caminho;
            return acumulado;
        }
    }

    // Nunca atingido (cubo 2x2 sempre tem solucao)
    return acumulado;
}
