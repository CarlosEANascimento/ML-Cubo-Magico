#include "heuristica.h"
#include "simetria.h"

#include <algorithm>
#include <fstream>
#include <iostream>
#include <unordered_set>

static bool corDoEixoY(Cor cor)
{
    return cor == Cor::BRANCO || cor == Cor::AMARELO;
}

/*
 * Índice do canto (mesma ordem de estado.h):
 *
 *   bit 0: 0 = esquerda, 1 = direita
 *   bit 1: 0 = trás,     1 = frente
 *   bit 2: 0 = cima,     1 = baixo
 */
static std::uint64_t codificarPeca(const PecaLogica& peca, int posicao)
{
    Cor x = (posicao & 1) ? peca.right  : peca.left;
    Cor z = (posicao & 2) ? peca.front  : peca.back;
    Cor y = (posicao & 4) ? peca.bottom : peca.top;

    int id = 0;
    int torcao = 2;

    if (corDoEixoY(y))
    {
        torcao = 0;
    }
    else if (corDoEixoY(x))
    {
        torcao = 1;
    }

    for (Cor cor : {x, y, z})
    {
        if (cor == Cor::VERMELHO) id |= 1;
        if (cor == Cor::VERDE)    id |= 2;
        if (cor == Cor::AMARELO)  id |= 4;
    }

    return static_cast<std::uint64_t>((id << 2) | torcao);
}

std::uint64_t codificarEstado(const EstadoCubo& estado)
{
    std::uint64_t chave = 0;

    for (int posicao = 0; posicao < 8; ++posicao)
    {
        chave = (chave << 5) | codificarPeca(estado.pecas[posicao], posicao);
    }

    return chave;
}

std::uint64_t chaveCanonica(const EstadoCubo& estado)
{
    return codificarEstado(estadoCanonico(estado));
}

std::vector<std::uint64_t> gerarTabela(
    std::vector<std::size_t>& estadosPorProfundidade
)
{
    std::vector<std::uint64_t> tabela;
    std::unordered_set<std::uint64_t> visitadas;

    estadosPorProfundidade.assign(PROFUNDIDADE_MAXIMA + 1, 0);

    EstadoCubo inicial = estadoCanonico(criarEstadoResolvido());
    std::uint64_t chaveInicial = codificarEstado(inicial);

    visitadas.insert(chaveInicial);
    tabela.push_back(chaveInicial << 8);
    estadosPorProfundidade[0] = 1;

    std::vector<EstadoCubo> nivel = {inicial};

    for (int profundidade = 1;
         profundidade <= PROFUNDIDADE_MAXIMA && !nivel.empty();
         ++profundidade)
    {
        std::vector<EstadoCubo> proximo;

        for (const EstadoCubo& estado : nivel)
        {
            for (const auto& sucessor : gerarSucessores(estado))
            {
                EstadoCubo canonico = estadoCanonico(sucessor.first);
                std::uint64_t chave = codificarEstado(canonico);

                if (!visitadas.insert(chave).second)
                {
                    continue;
                }

                tabela.push_back(
                    (chave << 8) |
                    static_cast<std::uint64_t>(profundidade)
                );

                estadosPorProfundidade[profundidade]++;
                proximo.push_back(canonico);
            }
        }

        nivel = std::move(proximo);
    }

    std::sort(tabela.begin(), tabela.end());

    return tabela;
}

/*
 * Formato do arquivo: só o vetor ordenado, gravado direto (8 bytes por
 * entrada). A quantidade de entradas é o tamanho do arquivo dividido por 8.
 */
bool salvarTabela(
    const std::string& caminho,
    const std::vector<std::uint64_t>& tabela
)
{
    std::ofstream arquivo(caminho, std::ios::binary | std::ios::trunc);

    if (!arquivo)
    {
        return false;
    }

    arquivo.write(
        reinterpret_cast<const char*>(tabela.data()),
        static_cast<std::streamsize>(tabela.size() * sizeof(std::uint64_t))
    );

    return static_cast<bool>(arquivo);
}

bool carregarTabela(
    const std::string& caminho,
    std::vector<std::uint64_t>& tabela
)
{
    std::ifstream arquivo(caminho, std::ios::binary);

    if (!arquivo)
    {
        return false;
    }

    arquivo.seekg(0, std::ios::end);
    std::streamoff tamanho = arquivo.tellg();
    arquivo.seekg(0);

    if (tamanho <= 0 || tamanho % sizeof(std::uint64_t) != 0)
    {
        return false;
    }

    std::vector<std::uint64_t> lida(
        static_cast<std::size_t>(tamanho) / sizeof(std::uint64_t)
    );

    arquivo.read(reinterpret_cast<char*>(lida.data()), tamanho);

    if (!arquivo || !std::is_sorted(lida.begin(), lida.end()))
    {
        return false;
    }

    tabela = std::move(lida);

    return true;
}

TabelaDistancias::TabelaDistancias()
{
    if (carregarTabela(ARQUIVO_TABELA, entradas))
    {
        return;
    }

    std::cerr << "Tabela '" << ARQUIVO_TABELA
              << "' ausente ou invalida. Gerando (cerca de 1 minuto)...\n";

    std::vector<std::size_t> contagem;

    entradas = gerarTabela(contagem);

    if (!salvarTabela(ARQUIVO_TABELA, entradas))
    {
        std::cerr << "Aviso: nao foi possivel gravar '" << ARQUIVO_TABELA
                  << "'. A tabela ficara so em memoria.\n";
    }
}

int TabelaDistancias::h(const EstadoCubo& estado) const
{
    std::uint64_t chave = chaveCanonica(estado);

    auto it = std::lower_bound(entradas.begin(), entradas.end(), chave << 8);

    if (it != entradas.end() && (*it >> 8) == chave)
    {
        return static_cast<int>(*it & 0xFF);
    }

    return PROFUNDIDADE_MAXIMA + 1;
}