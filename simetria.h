#ifndef SIMETRIA_H
#define SIMETRIA_H

#include "estado.h"

#include <vector>

std::vector<EstadoCubo> gerarSimetrias(
    const EstadoCubo& estado
);

EstadoCubo estadoCanonico(
    const EstadoCubo& estado
);

bool equivalentesPorSimetria(
    const EstadoCubo& a,
    const EstadoCubo& b
);

#endif