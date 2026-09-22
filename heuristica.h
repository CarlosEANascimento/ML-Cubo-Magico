#ifndef HEURISTICA_H
#define HEURISTICA_H

#include "estado.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

/*
 * CODIFICAÇÃO EXATA DO ESTADO
 *
 * Cada estado diferente gera uma chave diferente.
 * Cada canto vira 5 bits (8 cantos x 5 = 40 bits):
 *
 *   bits 4..2 = qual peça está na posição (0..7)
 *   bits 1..0 = torção da peça (0, 1 ou 2)
 *
 * A peça é identificada pelas cores que carrega:
 *
 *   bit 0: 0 = laranja, 1 = vermelho
 *   bit 1: 0 = azul,    1 = verde
 *   bit 2: 0 = branco,  1 = amarelo
 *
 * A torção é o eixo do canto onde está a cor branca/amarela:
 *
 *   0 = Y (top/bottom), 1 = X (left/right), 2 = Z (front/back)
 */
std::uint64_t codificarEstado(const EstadoCubo& estado);

/*
 * Chave da classe de simetria: codifica o estado canônico, então os
 * 24 giros globais do mesmo cubo dão a mesma chave.
 */
std::uint64_t chaveCanonica(const EstadoCubo& estado);


/*
 * TABELA DE DISTÂNCIAS (lookup table)
 *
 * Para cada classe de simetria, guarda a distância exata (em movimentos
 * de 90 graus) até o resolvido. Cada entrada é um uint64 empacotado,
 * (chave << 8) | h; o vetor fica ordenado e é consultado por busca
 * binária. O pior caso do 2x2x2 com movimentos de 90 graus é 14.
 */
constexpr int PROFUNDIDADE_MAXIMA = 14;

constexpr const char* ARQUIVO_TABELA = "tabela_h.bin";

/*
 * Gera a tabela por BFS a partir do resolvido, sobre estados canônicos.
 * Todo movimento tem inverso, então a distância "até" o resolvido é a
 * mesma "a partir" dele. estadosPorProfundidade[d] recebe quantas
 * classes existem à distância d.
 */
std::vector<std::uint64_t> gerarTabela(
    std::vector<std::size_t>& estadosPorProfundidade
);

bool salvarTabela(
    const std::string& caminho,
    const std::vector<std::uint64_t>& tabela
);

/*
 * Retorna false se o arquivo não existir, tiver tamanho inválido
 * ou estiver fora de ordem.
 */
bool carregarTabela(
    const std::string& caminho,
    std::vector<std::uint64_t>& tabela
);


/*
 * FUNÇÃO HEURÍSTICA h(n)
 *
 * Distância exata do estado até o resolvido, consultada pela chave
 * canônica: os 24 giros do mesmo cubo têm o mesmo h, já que o
 * estadoFinal aceita qualquer orientação do resolvido.
 *
 * Por ser a distância real, e não uma estimativa, h é:
 *
 *   - admissível:  h(n) <= custo real até o objetivo;
 *   - consistente: |h(n) - h(n')| <= 1 para todo sucessor n' de n.
 *
 * Um estado fora da tabela só existe se o cubo for inválido; nesse
 * caso h = PROFUNDIDADE_MAXIMA + 1.
 */
class TabelaDistancias
{
public:
    /*
     * Carrega ARQUIVO_TABELA; se ele não existir ou for inválido,
     * gera a tabela (cerca de 1 minuto) e grava o arquivo.
     */
    TabelaDistancias();

    int h(const EstadoCubo& estado) const;

private:
    std::vector<std::uint64_t> entradas;
};

#endif
