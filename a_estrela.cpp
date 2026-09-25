#include "a_estrela.h"
#include "simetria.h"

/* ---------------------------------------------------------------
 * FronteiraIDA -- Implementacao dos metodos da interface Fronteira
 * usando uma pilha (std::stack) e limite de custo f = g + h.
 * --------------------------------------------------------------- */

bool FronteiraIDA::adicionar(int indice, const NoBusca &no) {
  int g = no.profundidade;
  int h = tabela.h(no.estado);
  int f = g + h;

  if (f > limiteF) {
    if (f < proximoLimite) {
      proximoLimite = f;
    }
    return false;
  }

  EstadoCubo canonico = estadoCanonico(no.estado);

  auto it = visitados.find(canonico);

  if (it != visitados.end() && it->second <= g) {
    return false;
  }

  visitados[canonico] = g;

  pilha.push(indice);

  return true;
}

int FronteiraIDA::remover() {
  int indice = pilha.top();

  pilha.pop();

  return indice;
}

bool FronteiraIDA::vazia() const { return pilha.empty(); }

void FronteiraIDA::limpar() {
  while (!pilha.empty()) {
    pilha.pop();
  }

  visitados.clear();
}

int FronteiraIDA::obterProximoLimite() const { return proximoLimite; }

/* ---------------------------------------------------------------
 * IDDFS -- Iterative Deepening A* Search
 *
 * Executa buscarGenerico() com FronteiraIDA.
 * --------------------------------------------------------------- */

ResultadoBusca buscaIDAEstrela(const EstadoCubo &inicial) {
  ResultadoBusca acumulado;

  static const TabelaDistancias tabela;

  int limite = tabela.h(inicial);

  while (limite <= PROFUNDIDADE_MAXIMA) {
    FronteiraIDA fronteira(limite, tabela);

    ResultadoBusca parcial = buscarGenerico(inicial, fronteira);

    acumulado.estadosVisitados += parcial.estadosVisitados;

    if (parcial.encontrou) {
      acumulado.encontrou = true;
      acumulado.profundidade = parcial.profundidade;
      acumulado.caminho = parcial.caminho;

      return acumulado;
    }

    int proxLim = fronteira.obterProximoLimite();

    if (proxLim > PROFUNDIDADE_MAXIMA) {
      break;
    }

    limite = proxLim;
  }

  return acumulado;
}
