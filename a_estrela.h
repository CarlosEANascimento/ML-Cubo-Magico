#ifndef A_ESTRELA_H
#define A_ESTRELA_H

#include "estado.h"

#include <cstddef>
#include <iterator>
#include <vector>

struct ResultadoBuscaIDA {
  bool encontrou = false;
  int profundidade = 0;
  std::size_t estadosEspandidos = 0;
  std::vector<Movimento> solucao;
};

ResultadoBuscaIDA buscaIDAEstrela(const EstadoCubo &inicial);
#endif // !BUSCA_IDA_ESTRELA_H
