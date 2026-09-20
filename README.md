Cubo Mágico 2x2x2 com Busca em Largura (BFS)

Projeto acadêmico desenvolvido em C++17 para representar, movimentar e resolver um Cubo Mágico 2x2x2.

O projeto possui duas partes principais:

Cubo visual 3D

OpenGL

GLFW

Dear ImGui

Mecanismo de busca

representação lógica do cubo

geração de movimentos

Busca em Largura (BFS)

tabela de estados visitados

simetrias

reconstrução do caminho da solução

Esta documentação descreve principalmente o trabalho realizado na branch:

busca-largura

1. Visão geral

A ideia do projeto é permitir que o usuário:

movimente o cubo manualmente
        ↓
o programa leia o estado atual
        ↓
um algoritmo de busca procure uma solução
        ↓
o programa mostre a sequência de movimentos

Atualmente, a Busca em Largura (BFS) já consegue:

representar o estado do cubo;

gerar sucessores;

detectar estados já visitados;

evitar estados equivalentes por simetria;

encontrar uma solução;

informar a profundidade da solução;

contar estados visitados;

reconstruir o caminho;

receber o estado atual do cubo 3D;

mostrar o resultado em uma janela ImGui.

2. Objetivo acadêmico

O trabalho exige a implementação de algoritmos de busca para resolver o Cubo Mágico 2x2x2.

Os elementos principais são:

Estado

Função sucessora

Função avaliadora

Estrutura de busca

Estados visitados

Reconstrução da solução

Interface para interação

Um requisito importante é que o laço principal da busca seja genérico, ou seja, ele não deve precisar ser reescrito para cada algoritmo.

A ideia é que BFS, busca em profundidade limitada/iterativa e A* possam utilizar a mesma estrutura principal.

3. Arquitetura geral

O fluxo atual é:

CUBO VISUAL 3D
     │
     │ CubeSection
     ▼
extrairEstadoCubo()
     │
     │ EstadoCubo
     ▼
BUSCA EM LARGURA
     │
     ├── fila FIFO
     ├── tabela hash
     ├── estados visitados
     ├── simetrias
     └── reconstrução do caminho
     │
     ▼
ResultadoBusca
     │
     ├── encontrou?
     ├── profundidade
     ├── estados visitados
     └── caminho
     │
     ▼
INTERFACE IMGUI

4. Estrutura dos principais arquivos

cubo.cpp

Responsável pela parte gráfica e pela interação do usuário.

Contém:

criação do cubo 3D;

desenho das peças;

movimentos animados;

controles do teclado;

interface ImGui;

conversão do cubo visual para o estado da IA;

botão para executar a BFS.

estado.h

Define a estrutura lógica usada pelos algoritmos de busca.

Principais tipos:

enum class Cor;
enum class Movimento;

struct PecaLogica;
struct EstadoCubo;
struct EstadoHash;

Também declara as principais operações sobre o estado:

EstadoCubo criarEstadoResolvido();

bool estadoFinal(
    const EstadoCubo& estado
);

EstadoCubo aplicarMovimento(
    const EstadoCubo& estado,
    Movimento movimento
);

std::vector<std::pair<EstadoCubo, Movimento>>
gerarSucessores(
    const EstadoCubo& estado
);

const char* nomeMovimento(
    Movimento movimento
);

estado.cpp

Implementa a lógica do cubo.

Contém:

estado inicial resolvido;

hash;

função de objetivo;

movimentos;

geração de sucessores.

busca.h

Define as estruturas genéricas utilizadas por todos os algoritmos.

Nó de busca

struct NoBusca
{
    EstadoCubo estado;

    int pai = -1;

    Movimento movimento =
        Movimento::NENHUM;

    int profundidade = 0;
};

Cada nó guarda:

o estado atual;

o índice do nó pai;

o movimento usado para chegar nele;

a profundidade.

Isso permite reconstruir a solução ao final.

Resultado da busca

struct ResultadoBusca
{
    bool encontrou = false;

    std::vector<Movimento> caminho;

    std::size_t estadosVisitados = 0;

    int profundidade = -1;
};

O resultado informa:

se foi encontrada uma solução;

qual sequência de movimentos resolve o cubo;

quantos estados foram visitados;

qual a profundidade da solução.

Interface de fronteira

class Fronteira
{
public:
    virtual ~Fronteira() = default;

    virtual bool adicionar(
        int indice,
        const NoBusca& no
    ) = 0;

    virtual int remover() = 0;

    virtual bool vazia() const = 0;

    virtual void limpar() = 0;
};

Essa abstração permite trocar a estrutura de dados sem alterar o laço principal.

Exemplo:

BFS   → queue
DFS   → stack
A*    → priority_queue

5. Laço genérico de busca

O laço principal está em:

busca.cpp

Fluxo simplificado:

adiciona estado inicial
        ↓
enquanto a fronteira não estiver vazia
        ↓
remove próximo nó
        ↓
conta estado visitado
        ↓
verifica se é solução
        ↓
se for:
    reconstrói caminho
    retorna resultado
        ↓
se não for:
    gera sucessores
        ↓
adiciona sucessores válidos

Uma vantagem importante dessa estrutura é que o algoritmo genérico não precisa saber se está utilizando fila, pilha ou fila de prioridade.

6. Representação do Cubo 2x2x2

O cubo possui 8 peças de canto.

A ordem utilizada no programa é:

0 = esquerda / cima  / trás
1 = direita  / cima  / trás
2 = esquerda / cima  / frente
3 = direita  / cima  / frente

4 = esquerda / baixo / trás
5 = direita  / baixo / trás
6 = esquerda / baixo / frente
7 = direita  / baixo / frente

Cada peça lógica possui:

left
right
top
bottom
front
back

As faces internas recebem:

Cor::CINZA

Exemplo de uma peça:

Peça 0

left   = laranja
top    = branco
back   = azul

right  = cinza
bottom = cinza
front  = cinza

7. Movimentos implementados

Foram implementados os 12 movimentos básicos:

U
U'

D
D'

L
L'

R
R'

F
F'

B
B'

Onde ' representa o movimento inverso.

Exemplo:

R

é desfeito por:

R'

A função:

aplicarMovimento()

gera um novo estado após executar um movimento.

A função:

gerarSucessores()

gera todos os próximos estados possíveis a partir de um estado atual.

8. Busca em Largura — BFS

A BFS utiliza uma fila FIFO:

std::queue<int> fila;

Isso faz com que os estados sejam explorados por nível:

profundidade 0
      ↓
profundidade 1
      ↓
profundidade 2
      ↓
profundidade 3
      ↓
...

Como cada movimento possui custo 1, a primeira solução encontrada pela BFS é uma solução de menor profundidade dentro do espaço de estados considerado.

9. Estados visitados

Para evitar explorar o mesmo estado várias vezes, é utilizado:

std::unordered_set<
    EstadoCubo,
    EstadoHash
> visitados;

O estado é marcado como visitado no momento em que entra na fila.

Isso evita duplicatas dentro da própria fronteira.

10. Hash do estado

Foi implementado:

struct EstadoHash

para permitir que EstadoCubo seja utilizado em:

std::unordered_set

O hash combina as cores das 8 peças.

Importante:

hash igual

não significa obrigatoriamente:

estado igual

Por isso unordered_set também utiliza:

operator==

para confirmar a igualdade real do estado.

11. Simetrias do cubo

O Cubo 2x2x2 pode ser observado em 24 orientações globais diferentes.

Por isso foram criadas:

gerarSimetrias()
estadoCanonico()
equivalentesPorSimetria()

Arquivos:

simetria.h
simetria.cpp

Estado canônico

Para um estado são consideradas suas 24 rotações.

Fluxo:

estado
  ↓
gera 24 orientações
  ↓
serializa cada uma
  ↓
escolhe a menor
  ↓
estado canônico

A BFS usa o estado canônico apenas para verificar visitados.

O estado real continua armazenado nos nós.

Isso é importante porque a solução precisa ser reconstruída utilizando movimentos reais.

12. Resultado da otimização por simetria

Em um dos testes:

R U F

sem redução por simetria foram visitados aproximadamente:

863 estados

com estado canônico:

143 estados

Isso mostra uma grande redução no espaço explorado.

13. Testes realizados

Teste com profundidade 1

Embaralhamento:
R

Resultado:
Profundidade: 1
Estados visitados: 9
Caminho: R'

Validacao da solucao: OK

Teste com profundidade 2

Embaralhamento:
R U

Resultado:
Profundidade: 2
Estados visitados: 30
Caminho: U' R'

Validacao da solucao: OK

Teste com profundidade 3

Embaralhamento:
R U F

Resultado:
Profundidade: 3
Estados visitados: 143
Caminho: F' U' L'

Validacao da solucao: OK

Teste com profundidade 4

Embaralhamento:
R U F L

Resultado:
Profundidade: 4
Estados visitados: 499
Caminho: L' F' U' L'

Validacao da solucao: OK

Teste com profundidade 5

Embaralhamento:
R U F L B

Resultado:
Profundidade: 5
Estados visitados: 2188
Caminho: L' F' U' L' U'

Validacao da solucao: OK

14. Testes automatizados

O arquivo:

teste_bfs.cpp

valida:

movimentos;

movimentos inversos;

geração das 24 simetrias;

estado canônico;

BFS;

profundidade;

reconstrução do caminho;

reaplicação da solução;

estado final.

Saída esperada semelhante a:

Quantidade de simetrias: 24
Teste das simetrias: OK
Teste do estado canonico: OK
Teste dos movimentos: OK
Cubo embaralhado com: R U F L B
Solucao encontrada!
Profundidade: 5
Estados visitados: 2188
Caminho: L' F' U' L' U'
Validacao da solucao: OK

15. Integração com o cubo visual

A parte visual utiliza:

struct CubeSection

Cada peça visual guarda:

seus vértices;

cor da face esquerda;

cor da face direita;

cor da face superior;

cor da face inferior;

cor da face frontal;

cor da face traseira.

Para que a BFS possa usar o cubo visual foi criada:

EstadoCubo extrairEstadoCubo(
    const std::array<CubeSection, 8>& cube
);

Ela faz:

CubeSection
     ↓
converte cores RGB
     ↓
PecaLogica
     ↓
EstadoCubo

16. Teste da conversão visual

Na inicialização do programa:

cubo visual resolvido
        ↓
extrairEstadoCubo()
        ↓
EstadoCubo
        ↓
comparação
        ↓
criarEstadoResolvido()

Quando a conversão está correta, aparece:

[BFS] Conversao do cubo visual: OK

Esse teste já passou corretamente.

17. Interface da BFS

Foi criada uma janela ImGui:

Busca em Largura

com o botão:

Resolver com BFS

Ao clicar:

EstadoCubo atual =
    extrairEstadoCubo(cube);

resultadoBFS =
    buscaEmLargura(atual);

A interface mostra:

Solucao encontrada!

Profundidade: X

Estados visitados: Y

Caminho:
R' U F' ...

18. Como baixar e preparar o projeto

Clone o repositório:

git clone https://github.com/CarlosEANascimento/ML-Cubo-Magico.git

Entre na pasta:

cd ML-Cubo-Magico

Troque para a branch da BFS:

git switch busca-largura

Caso a branch ainda não exista localmente:

git fetch origin
git switch -c busca-largura origin/busca-largura

19. Dependência Dear ImGui

O projeto espera o Dear ImGui em:

external/imgui

Caso essa pasta não exista:

mkdir -p external
git clone https://github.com/ocornut/imgui.git external/imgui

Depois verifique:

ls external/imgui

Devem existir arquivos como:

imgui.cpp
imgui.h
imgui_draw.cpp
imgui_tables.cpp
imgui_widgets.cpp
backends/

20. Comandos principais

Limpar arquivos compilados

make clean

Remove:

cubo
teste_bfs

Compilar o programa gráfico

make

Ao terminar sem erros, será criado:

./cubo

Executar o cubo

./cubo

Compilar e rodar apenas os testes da BFS

make test-bfs

Esse comando não precisa abrir a interface gráfica.

Fluxo recomendado para desenvolvimento

make clean
make
./cubo

Para testar somente a busca:

make clean
make test-bfs

21. Como operar o cubo

O cubo é movimentado utilizando:

LETRA DA FACE + SETA

É importante manter a letra pressionada enquanto aperta a seta.

Movimentos da face direita

R

R + ↑

R'

R + ↓

Movimentos da face esquerda

L

L + ↑

L'

L + ↓

Movimentos da face superior

U

U + ←

U'

U + →

Movimentos da face inferior

D

D + →

D'

D + ←

Movimentos da face frontal

F

F + →

F'

F + ←

Movimentos da face traseira

B

B + ←

B'

B + →

Tabela completa de controles

Movimento

Teclas

L

L + ↑

L'

L + ↓

R

R + ↑

R'

R + ↓

U

U + ←

U'

U + →

D

D + →

D'

D + ←

F

F + →

F'

F + ←

B

B + ←

B'

B + →

22. Como testar a BFS pela interface

Primeiro execute:

./cubo

Ao iniciar, o terminal deve mostrar:

[BFS] Conversao do cubo visual: OK

Depois:

clique na janela do cubo para dar foco;

faça um movimento, por exemplo:

R + ↑

espere a animação terminar;

solte as teclas;

clique no botão:

Resolver com BFS

O resultado esperado para um único movimento R é semelhante a:

Solucao encontrada!

Profundidade: 1

Estados visitados: ...

Caminho:
R'

23. Exemplo de uso completo

1. Abrir o programa

./cubo

2. Movimentar o cubo

R + ↑

3. Esperar a rotação terminar

4. Clicar em:

Resolver com BFS

5. A BFS lê o estado atual

6. A solução aparece na interface

Profundidade: 1
Caminho: R'

24. Navegação da câmera

A câmera pode ser movimentada com o mouse.

Clique e arraste

para girar a visualização do cubo.

O scroll do mouse altera o zoom.

Esses movimentos alteram apenas a câmera.

Eles não alteram o estado lógico do Cubo Mágico.

25. Cuidados durante o uso

Não execute a BFS enquanto uma face ainda estiver girando.

O programa já verifica:

L != 0 ||
R != 0 ||
U != 0 ||
D != 0 ||
F != 0 ||
B != 0

Se houver uma animação em andamento, a interface mostra:

Aguarde o movimento terminar.

26. Mensagem libdecor no Linux

Em alguns ambientes Linux pode aparecer:

libdecor-gtk-WARNING: Failed to initialize GTK
Failed to load plugin 'libdecor-gtk.so'
No plugins found, falling back on no decorations

Essa mensagem está relacionada à decoração da janela.

Ela não indica falha da BFS.

Se a janela abrir normalmente, o programa pode continuar sendo utilizado.

27. Erro: external/imgui/imgui.cpp

Se aparecer:

make: *** Sem regra para processar o alvo
'external/imgui/imgui.cpp'

significa que o ImGui não está presente na pasta esperada.

Execute:

mkdir -p external
git clone https://github.com/ocornut/imgui.git external/imgui

e compile novamente:

make clean
make

28. Estado atual do projeto

Concluído

Representação lógica do cubo

Estado resolvido

Igualdade entre estados

Hash

12 movimentos

Função sucessora

Função avaliadora

Estrutura genérica de busca

BFS

Fila FIFO

Tabela de visitados

Reconstrução do caminho

Contagem de estados visitados

Simetrias

24 rotações globais

Estado canônico

Testes da BFS

Validação automática da solução

Cubo gráfico

Conversão visual → lógica

Validação da conversão

Compilação do executável gráfico

Janela ImGui para BFS

Botão Resolver com BFS

Exibição da profundidade

Exibição dos estados visitados

Exibição do caminho

29. Próximo passo

O próximo passo é fazer o cubo executar automaticamente a sequência retornada pela BFS.

Hoje:

BFS encontra:

R' U F'

mas apenas mostra essa sequência.

O próximo objetivo é:

BFS encontra solução
        ↓
usuário clica em "Executar solução"
        ↓
executa primeiro movimento
        ↓
espera animação terminar
        ↓
executa próximo movimento
        ↓
...
        ↓
cubo visual fica resolvido

Interface desejada:

Busca em Largura

Solucao encontrada!

Profundidade: 5
Estados visitados: 2188

Caminho:
L' F' U' L' U'

[ Executar solucao ]

30. Próximas melhorias possíveis

Depois da execução automática:

botão para embaralhar;

seed fixa para testes reproduzíveis;

reiniciar cubo;

executar solução passo a passo;

medir tempo da busca;

comparar BFS com e sem simetria;

implementar busca limitada;

implementar busca em profundidade iterativa;

implementar A*;

adicionar heurística;

selecionar algoritmo na interface.

31. Observação sobre profundidade

A quantidade de movimentos usados para embaralhar não necessariamente é igual à profundidade ótima.

Exemplo:

R R'

foram executados dois movimentos, porém o cubo voltou ao estado inicial.

Logo:

movimentos executados = 2
profundidade ótima = 0

A BFS não tenta simplesmente desfazer os movimentos feitos pelo usuário.

Ela explora o espaço de estados e encontra uma solução de menor profundidade.

32. Resumo técnico

A implementação atual pode ser resumida como:

Cubo visual
     ↓
Estado lógico
     ↓
Função sucessora
     ↓
BFS
     ↓
Fila FIFO
     ↓
Hash
     ↓
Estados visitados
     ↓
Simetrias
     ↓
Estado canônico
     ↓
Objetivo encontrado
     ↓
Reconstrução pelo pai
     ↓
Sequência de movimentos
     ↓
Interface ImGui

33. Branch

Desenvolvimento da BFS:

busca-largura

Repositório:

CarlosEANascimento/ML-Cubo-Magico

Situação atual

A Busca em Largura está funcional, testada e conectada ao cubo 3D.

O próximo marco é utilizar o caminho retornado pela BFS para animar automaticamente a resolução do Cubo Mágico.