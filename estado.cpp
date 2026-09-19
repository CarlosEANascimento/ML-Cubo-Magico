#include "estado.h"

#include <cstddef>

static void combinarHash(std::size_t& hash, Cor cor)
{
    std::size_t valor =
        static_cast<std::size_t>(cor);

    // Combina o valor atual com o hash anterior
    hash ^= valor
        + 0x9e3779b9
        + (hash << 6)
        + (hash >> 2);
}

std::size_t EstadoHash::operator()(
    const EstadoCubo& estado
) const
{
    std::size_t hash = 0;

    for (const PecaLogica& peca : estado.pecas)
    {
        combinarHash(hash, peca.left);
        combinarHash(hash, peca.right);
        combinarHash(hash, peca.top);
        combinarHash(hash, peca.bottom);
        combinarHash(hash, peca.front);
        combinarHash(hash, peca.back);
    }

    return hash;
}