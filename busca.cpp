#include "busca.h"

#include <algorithm>

ResultadoBusca buscarGenerico(
    const EstadoCubo& inicial,
    Fronteira& fronteira
)
{
    ResultadoBusca resultado;

    std::vector<NoBusca> nos;

    fronteira.limpar();

    NoBusca raiz;

    raiz.estado = inicial;
    raiz.pai = -1;
    raiz.movimento = Movimento::NENHUM;
    raiz.profundidade = 0;

    if (!fronteira.adicionar(0, raiz))
    {
        return resultado;
    }

    nos.push_back(raiz);

    while (!fronteira.vazia())
    {
        int indiceAtual =
            fronteira.remover();

        NoBusca atual =
            nos[indiceAtual];

        resultado.estadosVisitados++;
        if (estadoFinal(atual.estado))
        {
            resultado.encontrou = true;
            resultado.profundidade =
                atual.profundidade;

            int indice = indiceAtual;

            while (nos[indice].pai != -1)
            {
                resultado.caminho.push_back(
                    nos[indice].movimento
                );

                indice =
                    nos[indice].pai;
            }

            std::reverse(
                resultado.caminho.begin(),
                resultado.caminho.end()
            );

            return resultado;
        }

        auto sucessores =
            gerarSucessores(atual.estado);

        for (const auto& sucessor : sucessores)
        {
            NoBusca novoNo;

            novoNo.estado =
                sucessor.first;

            novoNo.movimento =
                sucessor.second;

            novoNo.pai =
                indiceAtual;

            novoNo.profundidade =
                atual.profundidade + 1;

            int novoIndice =
                static_cast<int>(
                    nos.size()
                );
            if (!fronteira.adicionar(
                    novoIndice,
                    novoNo))
            {
                continue;
            }

            nos.push_back(novoNo);
        }
    }

    return resultado;
}