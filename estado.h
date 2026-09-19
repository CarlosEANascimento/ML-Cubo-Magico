#ifndef ESTADO_H
#define ESTADO_H

#include <array>
#include <cstddef>
#include <cstdint>
#include <utility>
#include <vector>

enum class Cor : std::uint8_t {
    CINZA = 0,
    BRANCO,
    AMARELO,
    VERDE,
    AZUL,
    VERMELHO,
    LARANJA
};

enum class Movimento : std::uint8_t {
    NENHUM = 0,

    U,
    U_PRIME,

    D,
    D_PRIME,

    L,
    L_PRIME,

    R,
    R_PRIME,

    F,
    F_PRIME,

    B,
    B_PRIME
};

/*
 * Representação lógica de um dos 8 cantos.
 *
 * Não possui coordenadas OpenGL.
 * Guarda somente as informações necessárias para a IA.
 */
struct PecaLogica {
    Cor left   = Cor::CINZA;
    Cor right  = Cor::CINZA;
    Cor top    = Cor::CINZA;
    Cor bottom = Cor::CINZA;
    Cor front  = Cor::CINZA;
    Cor back   = Cor::CINZA;

    bool operator==(const PecaLogica& outra) const
    {
        return left   == outra.left   &&
               right  == outra.right  &&
               top    == outra.top    &&
               bottom == outra.bottom &&
               front  == outra.front  &&
               back   == outra.back;
    }
};


/*
 * ESTADO do problema.
 *
 * O cubo 2x2x2 possui 8 peças de canto.
 */
struct EstadoCubo {
    std::array<PecaLogica, 8> pecas;

    bool operator==(const EstadoCubo& outro) const
    {
        return pecas == outro.pecas;
    }
};


/*
 * Hash comum.
 *
 * Primeiro faremos a BFS funcionar com ele.
 * Depois vamos substituir/complementar pela chave
 * canônica baseada nas 24 simetrias do cubo.
 */
struct EstadoHash {
    std::size_t operator()(const EstadoCubo& estado) const;
};


/*
 * Cria o cubo completamente resolvido.
 */
EstadoCubo criarEstadoResolvido();


/*
 * FUNÇÃO AVALIADORA
 *
 * Retorna true quando o cubo está resolvido.
 */
bool estadoFinal(const EstadoCubo& estado);


/*
 * Aplica um movimento sem animação.
 *
 * Esta função será usada pelas IAs.
 */
EstadoCubo aplicarMovimento(
    const EstadoCubo& estado,
    Movimento movimento
);


/*
 * FUNÇÃO SUCESSORA
 *
 * Retorna todos os estados possíveis a partir
 * de um determinado estado.
 */
std::vector<std::pair<EstadoCubo, Movimento>>
gerarSucessores(const EstadoCubo& estado);


/*
 * Apenas para imprimir a solução de forma legível.
 */
const char* nomeMovimento(Movimento movimento);

#endif