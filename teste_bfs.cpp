#include <iostream>
#include "simetria.h"
#include "estado.h"
#include "busca_largura.h"
#include "busca_profundidade.h"
#include "busca_a_estrela.h"

int main()
{
    EstadoCubo resolvido =
        criarEstadoResolvido();

    auto simetrias =
    gerarSimetrias(resolvido);

    std::cout
        << "Quantidade de simetrias: "
        << simetrias.size()
        << '\n';

    if (simetrias.size() != 24)
    {
        std::cerr
            << "ERRO: deveriam existir 24 simetrias.\n";

        return 1;
    }

    std::cout
        << "Teste das simetrias: OK\n";

    EstadoCubo canonicoResolvido =
    estadoCanonico(resolvido);

    for (const EstadoCubo& simetria : simetrias)
    {
        if (!(estadoCanonico(simetria) == canonicoResolvido))
        {
            std::cerr
                << "ERRO: simetrias nao geraram o mesmo estado canonico.\n";

            return 1;
        }
    }

    std::cout
        << "Teste do estado canonico: OK\n";
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

    EstadoCubo cubo = resolvido;

    cubo = aplicarMovimento(
        cubo,
        Movimento::R
    );

    cubo = aplicarMovimento(
        cubo,
        Movimento::U
    );

    cubo = aplicarMovimento(
    cubo,
    Movimento::F
    );
    cubo = aplicarMovimento(
    cubo,
    Movimento::L
    );
    cubo = aplicarMovimento(
    cubo,
    Movimento::B
    );

    std::cout
        << "Cubo embaralhado com: R U F L B\n";

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

    if (resultado.profundidade > 5)
    {
        std::cerr
            << "ERRO: a BFS encontrou uma solucao maior "
            << "que o embaralhamento utilizado.\n";

        return 1;
    }

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

    ResultadoBusca resultadoIDS =
        buscaEmProfundidade(cubo);

    if (!resultadoIDS.encontrou)
    {
        std::cerr
            << "ERRO: IDDFS nao encontrou solucao.\n";

        return 1;
    }

    std::cout
        << "IDDFS encontrou solucao em profundidade "
        << resultadoIDS.profundidade
        << " e visitou "
        << resultadoIDS.estadosVisitados
        << " estados.\n";

    ResultadoBusca resultadoAEstrela =
        buscaAEstrela(cubo);

    if (!resultadoAEstrela.encontrou)
    {
        std::cerr
            << "ERRO: A* nao encontrou solucao.\n";

        return 1;
    }

    if (resultadoAEstrela.profundidade != resultado.profundidade)
    {
        std::cerr
            << "ERRO: A* nao encontrou a profundidade otima da BFS.\n";

        return 1;
    }

    std::cout
        << "A* encontrou solucao em profundidade "
        << resultadoAEstrela.profundidade
        << " e visitou "
        << resultadoAEstrela.estadosVisitados
        << " estados.\n";

    return 0;
}