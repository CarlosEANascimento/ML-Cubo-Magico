#ifndef A_ESTRELA_H
#define A_ESTRELA_H

#include <stack>
#include <unordered_map>

#include "busca.h"
#include "heuristica.h"

#ifndef PROFUNDIDADE_MAXIMA
#define PROFUNDIDADE_MAXIMA 20
#endif

/*
 * Fronteira para Busca IDA* (A* com Aprofundamento Iterativo).
 *
 * Usa uma pilha (std::stack) como estrutura de dados.
 * O limite agora é baseado no custo f (g + h).
 */
class FronteiraIDA : public Fronteira {
private:
  std::stack<int> pilha;

  // Utiliza o EstadoHash já existente no projeto para resolver
  // os erros de compilação do std::unordered_map
  std::unordered_map<EstadoCubo, int, EstadoHash> visitados;

  int limiteF;
  int proximoLimite;
  const TabelaDistancias &tabela;

public:
  FronteiraIDA(int limite, const TabelaDistancias &tab)
      : limiteF(limite), proximoLimite(PROFUNDIDADE_MAXIMA + 1), tabela(tab) {}

  bool adicionar(int indice, const NoBusca &no) override;

  int remover() override;

  bool vazia() const override;

  void limpar() override;

  int obterProximoLimite() const;
};

/*
 * Busca IDA* (Iterative Deepening A*)
 *
 * A cada iteração aumenta o limite de custo f com base no menor
 * custo descartado na iteração anterior.
 */
ResultadoBusca buscaIDAEstrela(const EstadoCubo &inicial);

#endif
