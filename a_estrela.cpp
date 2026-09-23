#include "a_estrela.h"
#include "heuristica.h"

#include <algorithm>
#include <utility>
#include <vector>

namespace {
Movimento movimentoInverso(Movimento movimento) {
  switch (movimento) {
  case Movimento::U:
    return Movimento::U_PRIME;
  case Movimento::U_PRIME:
    return Movimento::U;
  case Movimento::D:
    return Movimento::D_PRIME;
  case Movimento::D_PRIME:
    return Movimento::D;
  case Movimento::L:
    return Movimento::L_PRIME;
  case Movimento::L_PRIME:
    return Movimento::L;
  case Movimento::R:
    return Movimento::R_PRIME;
  case Movimento::R_PRIME:
    return Movimento::R;
  case Movimento::F:
    return Movimento::F_PRIME;
  case Movimento::F_PRIME:
    return Movimento::F;
  case Movimento::B:
    return Movimento::B_PRIME;
  case Movimento::B_PRIME:
    return Movimento::B;
  default:
    return Movimento::NENHUM;
  }
}
struct NoPilha {
  EstadoCubo estado;
  int g;
  Movimento ultimoMovimento;
  std::vector<Movimento> caminho;
};
} // namespace

ResultadoBuscaIDA buscaIDAEstrela(const EstadoCubo &inicial) {
  static const TabelaDistancias tabela;

  ResultadoBuscaIDA resultado;

  int limite = tabela.h(inicial);

  while (limite <= PROFUNDIDADE_MAXIMA) {
    int proximoLimite = PROFUNDIDADE_MAXIMA + 1;

    std::vector<NoPilha> pilha;
    pilha.push_back({inicial, 0, Movimento::NENHUM, {}});

    while (!pilha.empty()) {
      NoPilha no = std::move(pilha.back());
      pilha.pop_back();

      resultado.estadosEspandidos++;

      int h = tabela.h(no.estado);
      int f = no.g + h;

      if (f > limite) {
        proximoLimite = std::min(proximoLimite, f);
        continue;
      }

      if (estadoFinal(no.estado)) {
        resultado.encontrou = true;
        resultado.profundidade = no.g;
        resultado.solucao = std::move(no.caminho);
        return resultado;
      }

      for (const auto &sucessor : gerarSucessores(no.estado)) {
        if (sucessor.second == movimentoInverso(no.ultimoMovimento)) {
          continue;
        }

        NoPilha proximo;
        proximo.estado = sucessor.first;
        proximo.g = no.g + 1;
        proximo.ultimoMovimento = sucessor.second;
        proximo.caminho = no.caminho;
        proximo.caminho.push_back(sucessor.second);

        pilha.push_back(std::move(proximo));
      }
    }

    if (proximoLimite > PROFUNDIDADE_MAXIMA) {
      break;
    }

    limite = proximoLimite;
  }
  return resultado;
}
