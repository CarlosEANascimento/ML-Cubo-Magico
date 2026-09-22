#include "simetria.h"

#include <array>
#include <cstdint>
#include <vector>

namespace
{
    struct Vetor
    {
        int x;
        int y;
        int z;
    };

    struct Matriz
    {
        int m[3][3];

        bool operator==(const Matriz& outra) const
        {
            for (int i = 0; i < 3; ++i)
            {
                for (int j = 0; j < 3; ++j)
                {
                    if (m[i][j] != outra.m[i][j])
                    {
                        return false;
                    }
                }
            }

            return true;
        }
    };

    Vetor aplicarMatriz(
        const Matriz& matriz,
        Vetor vetor
    )
    {
        return {
            matriz.m[0][0] * vetor.x +
            matriz.m[0][1] * vetor.y +
            matriz.m[0][2] * vetor.z,

            matriz.m[1][0] * vetor.x +
            matriz.m[1][1] * vetor.y +
            matriz.m[1][2] * vetor.z,

            matriz.m[2][0] * vetor.x +
            matriz.m[2][1] * vetor.y +
            matriz.m[2][2] * vetor.z
        };
    }

    Matriz multiplicar(
        const Matriz& a,
        const Matriz& b
    )
    {
        Matriz resultado{};

        for (int i = 0; i < 3; ++i)
        {
            for (int j = 0; j < 3; ++j)
            {
                for (int k = 0; k < 3; ++k)
                {
                    resultado.m[i][j] +=
                        a.m[i][k] * b.m[k][j];
                }
            }
        }

        return resultado;
    }

    /*
     * As rotações de 90 graus em X e Y geram
     * todas as 24 orientações do cubo.
     *
     * As matrizes são geradas uma única vez.
     */
    const std::vector<Matriz>& matrizesRotacao()
    {
        static const std::vector<Matriz> matrizes = []()
        {
            const Matriz identidade = {{
                {1, 0, 0},
                {0, 1, 0},
                {0, 0, 1}
            }};

            const Matriz rotacaoX = {{
                {1, 0, 0},
                {0, 0, -1},
                {0, 1, 0}
            }};

            const Matriz rotacaoY = {{
                {0, 0, 1},
                {0, 1, 0},
                {-1, 0, 0}
            }};

            std::vector<Matriz> resultado;
            resultado.push_back(identidade);

            for (std::size_t i = 0;
                 i < resultado.size();
                 ++i)
            {
                const Matriz geradores[] = {
                    rotacaoX,
                    rotacaoY
                };

                for (const Matriz& gerador :
                     geradores)
                {
                    Matriz nova =
                        multiplicar(
                            gerador,
                            resultado[i]
                        );

                    bool existe = false;

                    for (const Matriz& atual :
                         resultado)
                    {
                        if (atual == nova)
                        {
                            existe = true;
                            break;
                        }
                    }

                    if (!existe)
                    {
                        resultado.push_back(nova);
                    }
                }
            }

            return resultado;
        }();

        return matrizes;
    }

    /*
     * Mapeamento dos 8 cantos:
     *
     * 0 = esquerda, cima, trás
     * 1 = direita,  cima, trás
     * 2 = esquerda, cima, frente
     * 3 = direita,  cima, frente
     * 4 = esquerda, baixo, trás
     * 5 = direita,  baixo, trás
     * 6 = esquerda, baixo, frente
     * 7 = direita,  baixo, frente
     */
    Vetor coordenadaDoIndice(int indice)
    {
        return {
            (indice & 1) ? 1 : -1,
            (indice & 4) ? -1 : 1,
            (indice & 2) ? 1 : -1
        };
    }

    int indiceDaCoordenada(Vetor coordenada)
    {
        return
            (coordenada.y < 0 ? 4 : 0) +
            (coordenada.z > 0 ? 2 : 0) +
            (coordenada.x > 0 ? 1 : 0);
    }

    enum class Direcao
    {
        LEFT,
        RIGHT,
        TOP,
        BOTTOM,
        FRONT,
        BACK
    };

    Vetor vetorDaDirecao(Direcao direcao)
    {
        switch (direcao)
        {
            case Direcao::LEFT:
                return {-1, 0, 0};

            case Direcao::RIGHT:
                return {1, 0, 0};

            case Direcao::TOP:
                return {0, 1, 0};

            case Direcao::BOTTOM:
                return {0, -1, 0};

            case Direcao::FRONT:
                return {0, 0, 1};

            case Direcao::BACK:
                return {0, 0, -1};
        }

        return {0, 0, 0};
    }

    Direcao direcaoDoVetor(Vetor vetor)
    {
        if (vetor.x == -1)
            return Direcao::LEFT;

        if (vetor.x == 1)
            return Direcao::RIGHT;

        if (vetor.y == 1)
            return Direcao::TOP;

        if (vetor.y == -1)
            return Direcao::BOTTOM;

        if (vetor.z == 1)
            return Direcao::FRONT;

        return Direcao::BACK;
    }

    Cor obterCor(
        const PecaLogica& peca,
        Direcao direcao
    )
    {
        switch (direcao)
        {
            case Direcao::LEFT:
                return peca.left;

            case Direcao::RIGHT:
                return peca.right;

            case Direcao::TOP:
                return peca.top;

            case Direcao::BOTTOM:
                return peca.bottom;

            case Direcao::FRONT:
                return peca.front;

            case Direcao::BACK:
                return peca.back;
        }

        return Cor::CINZA;
    }

    void definirCor(
        PecaLogica& peca,
        Direcao direcao,
        Cor cor
    )
    {
        switch (direcao)
        {
            case Direcao::LEFT:
                peca.left = cor;
                break;

            case Direcao::RIGHT:
                peca.right = cor;
                break;

            case Direcao::TOP:
                peca.top = cor;
                break;

            case Direcao::BOTTOM:
                peca.bottom = cor;
                break;

            case Direcao::FRONT:
                peca.front = cor;
                break;

            case Direcao::BACK:
                peca.back = cor;
                break;
        }
    }

    EstadoCubo aplicarRotacaoGlobal(
        const EstadoCubo& estado,
        const Matriz& matriz
    )
    {
        EstadoCubo novo{};

        const Direcao direcoes[] = {
            Direcao::LEFT,
            Direcao::RIGHT,
            Direcao::TOP,
            Direcao::BOTTOM,
            Direcao::FRONT,
            Direcao::BACK
        };

        for (int indice = 0;
             indice < 8;
             ++indice)
        {
            Vetor posicaoAntiga =
                coordenadaDoIndice(indice);

            Vetor posicaoNova =
                aplicarMatriz(
                    matriz,
                    posicaoAntiga
                );

            int novoIndice =
                indiceDaCoordenada(
                    posicaoNova
                );

            PecaLogica novaPeca{};

            for (Direcao direcao : direcoes)
            {
                Cor cor =
                    obterCor(
                        estado.pecas[indice],
                        direcao
                    );

                Vetor vetorDirecao =
                    vetorDaDirecao(
                        direcao
                    );

                Vetor novoVetorDirecao =
                    aplicarMatriz(
                        matriz,
                        vetorDirecao
                    );

                Direcao novaDirecao =
                    direcaoDoVetor(
                        novoVetorDirecao
                    );

                definirCor(
                    novaPeca,
                    novaDirecao,
                    cor
                );
            }

            novo.pecas[novoIndice] =
                novaPeca;
        }

        return novo;
    }

    std::array<std::uint8_t, 48>
    serializarEstado(
        const EstadoCubo& estado
    )
    {
        std::array<std::uint8_t, 48>
            serial{};

        std::size_t indice = 0;

        for (const PecaLogica& peca :
             estado.pecas)
        {
            const Cor cores[] = {
                peca.left,
                peca.right,
                peca.top,
                peca.bottom,
                peca.front,
                peca.back
            };

            for (Cor cor : cores)
            {
                serial[indice++] =
                    static_cast<std::uint8_t>(
                        cor
                    );
            }
        }

        return serial;
    }
}

std::vector<EstadoCubo> gerarSimetrias(
    const EstadoCubo& estado
)
{
    std::vector<EstadoCubo> resultado;

    const auto& matrizes =
        matrizesRotacao();

    resultado.reserve(
        matrizes.size()
    );

    for (const Matriz& matriz :
         matrizes)
    {
        resultado.push_back(
            aplicarRotacaoGlobal(
                estado,
                matriz
            )
        );
    }

    return resultado;
}

EstadoCubo estadoCanonico(
    const EstadoCubo& estado
)
{
    const auto& matrizes =
        matrizesRotacao();

    EstadoCubo melhor =
        aplicarRotacaoGlobal(
            estado,
            matrizes[0]
        );

    auto melhorSerial =
        serializarEstado(melhor);

    for (std::size_t i = 1;
         i < matrizes.size();
         ++i)
    {
        EstadoCubo candidato =
            aplicarRotacaoGlobal(
                estado,
                matrizes[i]
            );

        auto serialCandidato =
            serializarEstado(
                candidato
            );

        if (serialCandidato <
            melhorSerial)
        {
            melhor =
                candidato;

            melhorSerial =
                serialCandidato;
        }
    }

    return melhor;
}

bool equivalentesPorSimetria(
    const EstadoCubo& a,
    const EstadoCubo& b
)
{
    return
        estadoCanonico(a) ==
        estadoCanonico(b);
}
