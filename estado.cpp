#include "estado.h"

#include <cstddef>

static void combinarHash(std::size_t& hash, Cor cor)
{
    std::size_t valor = static_cast<std::size_t>(cor);

    hash ^= valor
        + 0x9e3779b9
        + (hash << 6)
        + (hash >> 2);
}

std::size_t EstadoHash::operator()(const EstadoCubo& estado) const
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


EstadoCubo criarEstadoResolvido()
{
    using C = Cor;

    EstadoCubo estado{};

    estado.pecas[0] = {
        C::LARANJA, C::CINZA,
        C::BRANCO, C::CINZA,
        C::CINZA, C::AZUL
    };

    estado.pecas[1] = {
        C::CINZA, C::VERMELHO,
        C::BRANCO, C::CINZA,
        C::CINZA, C::AZUL
    };

    estado.pecas[2] = {
        C::LARANJA, C::CINZA,
        C::BRANCO, C::CINZA,
        C::VERDE, C::CINZA
    };

    estado.pecas[3] = {
        C::CINZA, C::VERMELHO,
        C::BRANCO, C::CINZA,
        C::VERDE, C::CINZA
    };

    estado.pecas[4] = {
        C::LARANJA, C::CINZA,
        C::CINZA, C::AMARELO,
        C::CINZA, C::AZUL
    };

    estado.pecas[5] = {
        C::CINZA, C::VERMELHO,
        C::CINZA, C::AMARELO,
        C::CINZA, C::AZUL
    };

    estado.pecas[6] = {
        C::LARANJA, C::CINZA,
        C::CINZA, C::AMARELO,
        C::VERDE, C::CINZA
    };

    estado.pecas[7] = {
        C::CINZA, C::VERMELHO,
        C::CINZA, C::AMARELO,
        C::VERDE, C::CINZA
    };

    return estado;
}

bool estadoFinal(const EstadoCubo& estado)
{
    return estado == criarEstadoResolvido();
}


static PecaLogica rotateColorsL(const PecaLogica& peca)
{
    PecaLogica nova = peca;

    nova.front  = peca.top;
    nova.bottom = peca.front;
    nova.back   = peca.bottom;
    nova.top    = peca.back;

    return nova;
}

static PecaLogica rotateColorsLPrime(const PecaLogica& peca)
{
    PecaLogica nova = peca;

    nova.back   = peca.top;
    nova.bottom = peca.back;
    nova.front  = peca.bottom;
    nova.top    = peca.front;

    return nova;
}

static PecaLogica rotateColorsR(const PecaLogica& peca)
{
    return rotateColorsLPrime(peca);
}

static PecaLogica rotateColorsRPrime(const PecaLogica& peca)
{
    return rotateColorsL(peca);
}

static PecaLogica rotateColorsU(const PecaLogica& peca)
{
    PecaLogica nova = peca;

    nova.right = peca.back;
    nova.front = peca.right;
    nova.left  = peca.front;
    nova.back  = peca.left;

    return nova;
}

static PecaLogica rotateColorsUPrime(const PecaLogica& peca)
{
    PecaLogica nova = peca;

    nova.left  = peca.back;
    nova.front = peca.left;
    nova.right = peca.front;
    nova.back  = peca.right;

    return nova;
}

static PecaLogica rotateColorsD(const PecaLogica& peca)
{
    return rotateColorsUPrime(peca);
}

static PecaLogica rotateColorsDPrime(const PecaLogica& peca)
{
    return rotateColorsU(peca);
}

static PecaLogica rotateColorsF(const PecaLogica& peca)
{
    PecaLogica nova = peca;

    nova.right  = peca.top;
    nova.bottom = peca.right;
    nova.left   = peca.bottom;
    nova.top    = peca.left;

    return nova;
}

static PecaLogica rotateColorsFPrime(const PecaLogica& peca)
{
    PecaLogica nova = peca;

    nova.left   = peca.top;
    nova.bottom = peca.left;
    nova.right  = peca.bottom;
    nova.top    = peca.right;

    return nova;
}

static PecaLogica rotateColorsB(const PecaLogica& peca)
{
    return rotateColorsFPrime(peca);
}

static PecaLogica rotateColorsBPrime(const PecaLogica& peca)
{
    return rotateColorsF(peca);
}

EstadoCubo aplicarMovimento(
    const EstadoCubo& estado,
    Movimento movimento
)
{
    EstadoCubo novo = estado;
    const auto& antigo = estado.pecas;

    switch (movimento)
    {
        case Movimento::L:
            novo.pecas[0] = rotateColorsL(antigo[4]);
            novo.pecas[4] = rotateColorsL(antigo[6]);
            novo.pecas[6] = rotateColorsL(antigo[2]);
            novo.pecas[2] = rotateColorsL(antigo[0]);
            break;

        case Movimento::L_PRIME:
            novo.pecas[0] = rotateColorsLPrime(antigo[2]);
            novo.pecas[2] = rotateColorsLPrime(antigo[6]);
            novo.pecas[6] = rotateColorsLPrime(antigo[4]);
            novo.pecas[4] = rotateColorsLPrime(antigo[0]);
            break;

        case Movimento::R:
            novo.pecas[5] = rotateColorsR(antigo[1]);
            novo.pecas[7] = rotateColorsR(antigo[5]);
            novo.pecas[3] = rotateColorsR(antigo[7]);
            novo.pecas[1] = rotateColorsR(antigo[3]);
            break;

        case Movimento::R_PRIME:
            novo.pecas[3] = rotateColorsRPrime(antigo[1]);
            novo.pecas[7] = rotateColorsRPrime(antigo[3]);
            novo.pecas[5] = rotateColorsRPrime(antigo[7]);
            novo.pecas[1] = rotateColorsRPrime(antigo[5]);
            break;

        case Movimento::U:
            novo.pecas[1] = rotateColorsU(antigo[0]);
            novo.pecas[3] = rotateColorsU(antigo[1]);
            novo.pecas[2] = rotateColorsU(antigo[3]);
            novo.pecas[0] = rotateColorsU(antigo[2]);
            break;

        case Movimento::U_PRIME:
            novo.pecas[2] = rotateColorsUPrime(antigo[0]);
            novo.pecas[3] = rotateColorsUPrime(antigo[2]);
            novo.pecas[1] = rotateColorsUPrime(antigo[3]);
            novo.pecas[0] = rotateColorsUPrime(antigo[1]);
            break;

        case Movimento::D:
            novo.pecas[7] = rotateColorsD(antigo[6]);
            novo.pecas[5] = rotateColorsD(antigo[7]);
            novo.pecas[4] = rotateColorsD(antigo[5]);
            novo.pecas[6] = rotateColorsD(antigo[4]);
            break;

        case Movimento::D_PRIME:
            novo.pecas[4] = rotateColorsDPrime(antigo[6]);
            novo.pecas[5] = rotateColorsDPrime(antigo[4]);
            novo.pecas[7] = rotateColorsDPrime(antigo[5]);
            novo.pecas[6] = rotateColorsDPrime(antigo[7]);
            break;

        case Movimento::F:
            novo.pecas[3] = rotateColorsF(antigo[2]);
            novo.pecas[7] = rotateColorsF(antigo[3]);
            novo.pecas[6] = rotateColorsF(antigo[7]);
            novo.pecas[2] = rotateColorsF(antigo[6]);
            break;

        case Movimento::F_PRIME:
            novo.pecas[6] = rotateColorsFPrime(antigo[2]);
            novo.pecas[7] = rotateColorsFPrime(antigo[6]);
            novo.pecas[3] = rotateColorsFPrime(antigo[7]);
            novo.pecas[2] = rotateColorsFPrime(antigo[3]);
            break;

        case Movimento::B:
            novo.pecas[4] = rotateColorsB(antigo[0]);
            novo.pecas[5] = rotateColorsB(antigo[4]);
            novo.pecas[1] = rotateColorsB(antigo[5]);
            novo.pecas[0] = rotateColorsB(antigo[1]);
            break;

        case Movimento::B_PRIME:
            novo.pecas[1] = rotateColorsBPrime(antigo[0]);
            novo.pecas[5] = rotateColorsBPrime(antigo[1]);
            novo.pecas[4] = rotateColorsBPrime(antigo[5]);
            novo.pecas[0] = rotateColorsBPrime(antigo[4]);
            break;

        case Movimento::NENHUM:
            break;
    }

    return novo;
}

std::vector<std::pair<EstadoCubo, Movimento>>
gerarSucessores(const EstadoCubo& estado)
{
    static const Movimento movimentos[] = {
        Movimento::U,
        Movimento::U_PRIME,
        Movimento::D,
        Movimento::D_PRIME,
        Movimento::L,
        Movimento::L_PRIME,
        Movimento::R,
        Movimento::R_PRIME,
        Movimento::F,
        Movimento::F_PRIME,
        Movimento::B,
        Movimento::B_PRIME
    };

    std::vector<std::pair<EstadoCubo, Movimento>> resultado;
    resultado.reserve(12);

    for (Movimento movimento : movimentos)
    {
        resultado.push_back({
            aplicarMovimento(estado, movimento),
            movimento
        });
    }

    return resultado;
}


const char* nomeMovimento(Movimento movimento)
{
    switch (movimento)
    {
        case Movimento::U:       return "U";
        case Movimento::U_PRIME: return "U'";
        case Movimento::D:       return "D";
        case Movimento::D_PRIME: return "D'";
        case Movimento::L:       return "L";
        case Movimento::L_PRIME: return "L'";
        case Movimento::R:       return "R";
        case Movimento::R_PRIME: return "R'";
        case Movimento::F:       return "F";
        case Movimento::F_PRIME: return "F'";
        case Movimento::B:       return "B";
        case Movimento::B_PRIME: return "B'";
        default:                  return "-";
    }
}
