#ifndef BUSCA_A_ESTRELA_H
#define BUSCA_A_ESTRELA_H

#include <cstddef>
#include <queue>
#include <unordered_set>
#include <vector>

#include "busca.h"
#include "heuristica.h"

class FronteiraAEstrela : public Fronteira
{
private:
    struct Entrada
    {
        int indice;
        int custo;
        std::size_t ordem;
    };

    struct Comparador
    {
        bool operator()(const Entrada& esquerda, const Entrada& direita) const
        {
            if (esquerda.custo != direita.custo)
            {
                return esquerda.custo > direita.custo;
            }

            return esquerda.ordem > direita.ordem;
        }
    };

    std::priority_queue<Entrada, std::vector<Entrada>, Comparador> fila;
    std::unordered_set<EstadoCubo, EstadoHash> visitados;
    TabelaDistancias tabela;
    std::size_t proximaOrdem = 0;

public:
    bool adicionar(int indice, const NoBusca& no) override;
    int remover() override;
    bool vazia() const override;
    void limpar() override;
};

ResultadoBusca buscaAEstrela(const EstadoCubo& inicial);

#endif
