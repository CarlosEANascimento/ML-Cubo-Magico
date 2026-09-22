#include <chrono>
#include <iostream>
#include <random>
#include <unordered_set>
#include <vector>

#include "busca_largura.h"
#include "estado.h"
#include "heuristica.h"
#include "simetria.h"

/*
 * Constrói tabela_h.bin se faltar e depois prova que ela está certa.
 * Uso: ./tabela (uns 2 a 3 minutos). Roda até o fim mesmo se algo já
 * tiver falhado antes, então o resultado sempre sai completo.
 *
 * Geração: BFS a partir do resolvido, conferindo a quantidade de estados
 * por profundidade contra a contagem conhecida do 2x2x2 antes de gravar.
 *
 * Verificação por amostragem:
 *   -  a tabela tem exatamente 3674160 entradas
 *   A. simetria: h(resolvido) = 0, e os 24 giros de 300 estados
 *      embaralhados caem sempre na mesma chave e no mesmo h
 *   B. h bate com a profundidade que a BFS do grupo encontra, em 12
 *      embaralhamentos (referência independente da tabela)
 *
 * Verificação exaustiva, por uma BFS que percorre TODOS os 3674160
 * estados canônicos e as arestas entre eles:
 *   C. total de estados = 8! * 3^7 / 24 (uma colisão de chave o reduziria)
 *   D. nenhum estado ou sucessor fica de fora da tabela
 *   E. h == 0 exatamente nos estados finais
 *   F. h(s) <= h(s') + 1 em toda aresta (consistência)
 *   G. todo estado não final tem um sucessor com h uma unidade menor
 *
 * E, F e G juntas provam que h é a distância real: E fixa o zero, F limita
 * por cima e G, descendo pelos vizinhos, limita por baixo. Ou seja, h é
 * admissível, consistente e exato.
 */

constexpr std::size_t TOTAL_ESPERADO = 3674160;

// Ferramentas de impressao

static void linha(const char* nome, std::size_t valor, std::size_t esperado)
{
    std::cout << "  " << nome << ": " << valor
              << "   (esperado " << esperado << ")"
              << (valor == esperado ? "" : "   <-- FALHOU")
              << '\n';
}

static void situacao(const char* nome, bool ok)
{
    std::cout << "  " << nome << ": " << (ok ? "OK" : "FALHOU") << '\n';
}

// Construir a tabela (gera se precisar)
// Classes de simetria por distância até o resolvido.
static const std::size_t ESPERADO_POR_PROFUNDIDADE[PROFUNDIDADE_MAXIMA + 1] = {
    1, 6, 27, 120, 534, 2256, 8969, 33058,
    114149, 360508, 930588, 1350852, 782536, 90280, 276
};

// Garante que tabela_h.bin existe e é válido; gera e grava se precisar.
static bool prepararArquivo()
{
    std::vector<std::uint64_t> lida;

    if (carregarTabela(ARQUIVO_TABELA, lida))
    {
        std::cout << "Tabela '" << ARQUIVO_TABELA << "' ja existe ("
                  << lida.size() << " entradas).\n";
        return true;
    }

    std::cout << "Gerando tabela ate profundidade "
              << PROFUNDIDADE_MAXIMA << "...\n";

    auto inicio = std::chrono::steady_clock::now();

    std::vector<std::size_t> contagem;
    std::vector<std::uint64_t> tabela = gerarTabela(contagem);

    double segundos = std::chrono::duration<double>(
        std::chrono::steady_clock::now() - inicio
    ).count();

    bool ok = true;

    std::cout << "\nProf.    Estados    Esperado\n";

    for (int d = 0; d <= PROFUNDIDADE_MAXIMA; ++d)
    {
        bool bate = contagem[d] == ESPERADO_POR_PROFUNDIDADE[d];

        std::cout << "  " << d
                  << "\t" << contagem[d]
                  << "\t" << ESPERADO_POR_PROFUNDIDADE[d]
                  << (bate ? "" : "   <-- DIFERE")
                  << '\n';

        if (!bate)
        {
            ok = false;
        }
    }

    std::cout << "\nTotal de entradas: " << tabela.size() << '\n';
    std::cout << "Tempo de geracao: " << segundos << " s\n";

    if (!ok)
    {
        std::cerr << "ERRO: contagem por profundidade diferente da esperada. "
                  << "Arquivo NAO gravado.\n";
        return false;
    }

    if (!salvarTabela(ARQUIVO_TABELA, tabela))
    {
        std::cerr << "ERRO: nao foi possivel gravar " << ARQUIVO_TABELA << ".\n";
        return false;
    }

    std::cout << "Contagem por profundidade: OK\n";
    std::cout << "Tabela gravada em " << ARQUIVO_TABELA << " ("
              << tabela.size() * sizeof(std::uint64_t)
              << " bytes)\n";

    return true;
}

// Verificar a tabela (tamanho, A e B, depois C a G)
static EstadoCubo embaralhar(
    EstadoCubo estado,
    int movimentos,
    std::mt19937& gerador
)
{
    for (int i = 0; i < movimentos; ++i)
    {
        auto sucessores = gerarSucessores(estado);
        estado = sucessores[gerador() % sucessores.size()].first;
    }

    return estado;
}

// Percorre TODOS os estados canônicos alcançáveis a partir do resolvido
// e consulta a tabela em cada estado e em cada sucessor.
static bool verificar(const TabelaDistancias& tabela, std::size_t tamanhoCarregado)
{
    linha("-  tamanho da tabela           ", tamanhoCarregado, TOTAL_ESPERADO);
    bool tamanhoOk = tamanhoCarregado == TOTAL_ESPERADO;

    std::mt19937 gerador(12345);
    EstadoCubo resolvido = criarEstadoResolvido();

    // A. simetria (o resolvido entra na lista; se h(resolvido) = 0, seus
    // 24 giros tambem tem h = 0)
    std::vector<EstadoCubo> estados = {resolvido};

    for (int i = 0; i < 300; ++i)
    {
        estados.push_back(embaralhar(resolvido, gerador() % 31, gerador));
    }

    bool simetriaOk = tabela.h(resolvido) == 0;

    for (const EstadoCubo& estado : estados)
    {
        std::uint64_t chave = chaveCanonica(estado);
        int h = tabela.h(estado);

        for (const EstadoCubo& giro : gerarSimetrias(estado))
        {
            if (chaveCanonica(giro) != chave || tabela.h(giro) != h)
            {
                simetriaOk = false;
            }
        }
    }

    situacao("A. 24 giros dao a mesma chave e o mesmo h ", simetriaOk);

    // B. h contra a BFS
    bool bfsOk = true;

    for (int i = 0; i < 12; ++i)
    {
        EstadoCubo estado = embaralhar(resolvido, 1 + gerador() % 7, gerador);
        int h = tabela.h(estado);
        ResultadoBusca bfs = buscaEmLargura(estado);

        if (!bfs.encontrou || bfs.profundidade != h)
        {
            bfsOk = false;
            std::cerr << "    BFS = " << bfs.profundidade
                      << ", h = " << h << '\n';
        }
    }

    situacao("B. h igual a profundidade da BFS (12 casos)", bfsOk);

    // C a G: varredura exaustiva
    std::cout << "\nVerificando '" << ARQUIVO_TABELA
              << "' (cerca de 2 minutos)...\n";

    auto inicio = std::chrono::steady_clock::now();

    const int AUSENTE = PROFUNDIDADE_MAXIMA + 1;

    std::unordered_set<std::uint64_t> visitadas;
    visitadas.reserve(TOTAL_ESPERADO + TOTAL_ESPERADO / 8);

    EstadoCubo raiz = estadoCanonico(resolvido);
    visitadas.insert(codificarEstado(raiz));

    std::vector<EstadoCubo> nivel = {raiz};

    std::size_t ausentes = 0;
    std::size_t finalErrado = 0;
    std::size_t naoConsistentes = 0;
    std::size_t semDescida = 0;
    std::size_t arestas = 0;

    while (!nivel.empty())
    {
        std::vector<EstadoCubo> proximo;

        for (const EstadoCubo& estado : nivel)
        {
            int h = tabela.h(estado);
            bool final = estadoFinal(estado);

            if (h == AUSENTE) ++ausentes;
            if (final != (h == 0)) ++finalErrado;

            bool temDescida = false;

            for (const auto& sucessor : gerarSucessores(estado))
            {
                ++arestas;

                int hSucessor = tabela.h(sucessor.first);

                if (hSucessor == AUSENTE) ++ausentes;
                if (h > hSucessor + 1) ++naoConsistentes;
                if (hSucessor == h - 1) temDescida = true;

                EstadoCubo canonico = estadoCanonico(sucessor.first);

                if (visitadas.insert(codificarEstado(canonico)).second)
                {
                    proximo.push_back(canonico);
                }
            }

            if (!final && !temDescida) ++semDescida;
        }

        nivel = std::move(proximo);
    }

    double segundos = std::chrono::duration<double>(
        std::chrono::steady_clock::now() - inicio
    ).count();

    std::cout << "\nArestas conferidas: " << arestas << '\n';
    std::cout << "Tempo: " << segundos << " s\n\n";

    linha("C. estados distintos            ", visitadas.size(), TOTAL_ESPERADO);
    linha("D. estados/sucessores ausentes  ", ausentes, 0);
    linha("E. estadoFinal != (h == 0)      ", finalErrado, 0);
    linha("F. arestas com h(s) > h(s')+1   ", naoConsistentes, 0);
    linha("G. nao finais sem sucessor h-1  ", semDescida, 0);

    return
        tamanhoOk &&
        simetriaOk &&
        bfsOk &&
        visitadas.size() == TOTAL_ESPERADO &&
        ausentes == 0 &&
        finalErrado == 0 &&
        naoConsistentes == 0 &&
        semDescida == 0;
}

int main()
{
    if (!prepararArquivo())
    {
        return 1;
    }

    // So para o tamanho; TabelaDistancias abaixo faz sua propria leitura.
    std::vector<std::uint64_t> lida;
    carregarTabela(ARQUIVO_TABELA, lida);

    TabelaDistancias tabela;

    if (!verificar(tabela, lida.size()))
    {
        std::cerr << "\nFALHOU: a tabela nao e confiavel.\n";
        return 1;
    }

    std::cout << "\nTabela verificada: sem colisoes, admissivel, "
              << "consistente e exata.\n";
    return 0;
}
