#ifndef BUSCA_PROFUNDIDADE_H
#define BUSCA_PROFUNDIDADE_H

#include <stack>
#include <unordered_set>

#include "busca.h"

/*
 * Fronteira para Busca em Profundidade Limitada.
 *
 * Usa uma pilha (std::stack) como estrutura de dados,
 * garantindo a ordem LIFO (ultimo a entrar, primeiro a sair).
 *
 * A profundidade maxima e configuravel pelo construtor.
 * Nos alem do limite sao rejeitados em adicionar().
 *
 * Usa simetria (estadoCanonico) para poda de estados
 * ja visitados na iteracao corrente.
 */
class FronteiraDFS : public Fronteira
{
private:
    std::stack<int> pilha;

    std::unordered_set<
        EstadoCubo,
        EstadoHash
    > visitados;

    int limiteProf;

public:
    explicit FronteiraDFS(int limite)
        : limiteProf(limite) {}

    bool adicionar(
        int indice,
        const NoBusca& no
    ) override;

    int remover() override;

    bool vazia() const override;

    void limpar() override;
};


/*
 * Busca em Profundidade com Aprofundamento Iterativo (IDDFS)
 *
 * A cada iteracao aumenta o limite de profundidade em 1,
 * chamando buscarGenerico() com uma nova FronteiraDFS.
 *
 * Usa simetria (estadoCanonico) para poda de estados equivalentes.
 *
 * Propriedades:
 *  - Completo: sim
 *  - Otimo: sim (encontra a solucao de menor profundidade)
 *  - Implementacao: iterativa (sem recursao)
 *  - Usa o laco generico de buscarGenerico()
 */
ResultadoBusca buscaEmProfundidade(
    const EstadoCubo& inicial
);

#endif
