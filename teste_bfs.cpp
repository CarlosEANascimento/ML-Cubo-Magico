#include <iostream>

#include "estado.h"
#include "busca_largura.h"

int main()
{
    EstadoCubo resolvido =
        criarEstadoResolvido();
    struct ParMovimentos
    {
        Movimento movimento;
        Movimento inverso;
    };

    ParMovimentos pares[] = {
        {Movimento::U, Movimento::U_PRIME},
        {Movimento::D, Movimento::D_PRIME},
        {Movimento::L, Movimento::L_PRIME},
        {Movimento::R, Movimento::R_PRIME},
        {Movimento::F, Movimento::F_PRIME},
        {Movimento::B, Movimento::B_PRIME}
    };

    for (const auto& par : pares)
    {
        EstadoCubo teste =
            aplicarMovimento(
                resolvido,
                par.movimento
            );

        teste =
            aplicarMovimento(
                teste,
                par.inverso
            );

        if (!estadoFinal(teste))
        {
            std::cerr
                << "ERRO: movimento "
                << nomeMovimento(par.movimento)
                << " e seu inverso falharam.\n";

            return 1;
        }
    }

    std::cout
        << "Teste dos movimentos: OK\n";

    EstadoCubo cubo =
        aplicarMovimento(
            resolvido,
            Movimento::R
        );

    std::cout
        << "Cubo embaralhado com: R\n";

    ResultadoBusca resultado =
        buscaEmLargura(cubo);

    if (!resultado.encontrou)
    {
        std::cerr
            << "ERRO: BFS nao encontrou solucao.\n";

        return 1;
    }

    std::cout
        << "Solucao encontrada!\n";

    std::cout
        << "Profundidade: "
        << resultado.profundidade
        << '\n';

    std::cout
        << "Estados visitados: "
        << resultado.estadosVisitados
        << '\n';

    std::cout
        << "Caminho:";

    for (Movimento movimento :
         resultado.caminho)
    {
        std::cout
            << ' '
            << nomeMovimento(movimento);
    }

    std::cout << '\n';
    EstadoCubo verificacao = cubo;

    for (Movimento movimento :
         resultado.caminho)
    {
        verificacao =
            aplicarMovimento(
                verificacao,
                movimento
            );
    }

    if (!estadoFinal(verificacao))
    {
        std::cerr
            << "ERRO: caminho retornado nao resolve o cubo.\n";

        return 1;
    }

    std::cout
        << "Validacao da solucao: OK\n";

    return 0;
}