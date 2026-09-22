#include "busca_profundidade.h"
#include "simetria.h"

#include <unordered_set>
#include <vector>

/*
 * DLS - Depth-Limited Search (busca em profundidade com limite fixo).
 *
 * Parametros:
 *   estado         - estado atual sendo explorado
 *   profAtual      - profundidade atual no caminho
 *   limite         - profundidade maxima permitida nesta iteracao
 *   caminho        - movimentos feitos ate aqui (pilha de backtracking)
 *   emCaminho      - estados canonicos presentes no caminho atual (evita ciclos)
 *   nosVisitados   - contador global de nos expandidos
 *
 * Retorna true se encontrou solucao (o caminho e preenchido).
 */
static bool dls(
    const EstadoCubo&                               estado,
    int                                             profAtual,
    int                                             limite,
    std::vector<Movimento>&                         caminho,
    std::unordered_set<EstadoCubo, EstadoHash>&     emCaminho,
    std::size_t&                                    nosVisitados
)
{
    nosVisitados++;

    if (estadoFinal(estado))
    {
        return true;
    }

    if (profAtual >= limite)
    {
        return false;
    }

    for (const auto& [filho, movimento] : gerarSucessores(estado))
    {
        EstadoCubo canonico = estadoCanonico(filho);

        // Poda: nao re-visita estados canonicamente equivalentes
        // que ja estao no caminho atual (evita ciclos)
        if (emCaminho.count(canonico))
        {
            continue;
        }

        emCaminho.insert(canonico);
        caminho.push_back(movimento);

        if (dls(filho, profAtual + 1, limite, caminho, emCaminho, nosVisitados))
        {
            return true;
        }

        // Backtrack
        caminho.pop_back();
        emCaminho.erase(canonico);
    }

    return false;
}

/*
 * IDDFS - Iterative Deepening Depth-First Search.
 *
 * Executa DLS com limite crescendo de 0 ate encontrar a solucao.
 * Cada iteracao recomeca do estado inicial com um limite maior.
 *
 * Nota sobre nosVisitados: o contador acumula entre todas as iteracoes,
 * refletindo o custo real total da busca (incluindo as re-exploracoes).
 */
ResultadoBusca buscaEmProfundidade(const EstadoCubo& inicial)
{
    ResultadoBusca resultado;

    EstadoCubo canonicoInicial = estadoCanonico(inicial);

    // Caso especial: estado inicial ja e solucao
    if (estadoFinal(inicial))
    {
        resultado.encontrou   = true;
        resultado.profundidade = 0;
        resultado.estadosVisitados = 1;
        return resultado;
    }

    for (int limite = 1; ; limite++)
    {
        std::vector<Movimento>                      caminho;
        std::unordered_set<EstadoCubo, EstadoHash>  emCaminho;

        // Insere o estado inicial no conjunto do caminho
        emCaminho.insert(canonicoInicial);

        bool encontrou = dls(
            inicial,
            0,
            limite,
            caminho,
            emCaminho,
            resultado.estadosVisitados
        );

        if (encontrou)
        {
            resultado.encontrou   = true;
            resultado.profundidade = static_cast<int>(caminho.size());
            resultado.caminho      = caminho;
            return resultado;
        }
    }

    // Nunca atingido (cubo 2x2 sempre tem solucao)
    return resultado;
}
