# Cubo Mágico 2x2x2 com Busca em IA

Projeto acadêmico desenvolvido em **C++17** para representar, movimentar e resolver um **Cubo Mágico 2x2x2**.

O projeto possui duas partes principais:

1. **Cubo visual 3D**
   - OpenGL
   - GLFW
   - Dear ImGui

2. **Mecanismo de busca**
   - representação lógica do cubo
   - geração de movimentos
        - Busca em Largura (BFS)
        - Busca em Profundidade Iterativa (IDDFS)
        - Busca A*
        - heurística por tabela de distâncias exatas
   - tabela de estados visitados
   - simetrias
   - reconstrução do caminho da solução

Esta documentação descreve o estado atual do trabalho na branch:

```text
busca-largura
```

---

# 1. Visão geral

A ideia do projeto é permitir que o usuário:

```text
movimente o cubo manualmente
        ↓
o programa leia o estado atual
        ↓
um algoritmo de busca procure uma solução
        ↓
o programa mostre a sequência de movimentos
```

Atualmente, o programa consegue:

- representar o estado do cubo;
- gerar sucessores;
- detectar estados já visitados;
- evitar estados equivalentes por simetria;
- encontrar uma solução por BFS, IDDFS ou A*;
- informar a profundidade da solução;
- contar estados visitados;
- reconstruir o caminho;
- receber o estado atual do cubo 3D;
- mostrar o resultado em uma janela ImGui;
- repetir um embaralhamento usando uma seed escolhida pelo usuário.

---

# 2. Objetivo acadêmico

O trabalho exige a implementação de algoritmos de busca para resolver o Cubo Mágico 2x2x2.

Os elementos principais são:

- **Estado**
- **Função sucessora**
- **Função avaliadora**
- **Estrutura de busca**
- **Estados visitados**
- **Reconstrução da solução**
- **Interface para interação**

Um requisito importante é que o **laço principal da busca seja genérico**, ou seja, ele não deve precisar ser reescrito para cada algoritmo.

A ideia é que BFS, busca em profundidade limitada/iterativa e A* possam utilizar a mesma estrutura principal.

---

# 3. Arquitetura geral

O fluxo atual é:

```text
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
```

---

# 4. Estrutura dos principais arquivos

## `cubo.cpp`

Responsável pela parte gráfica e pela interação do usuário.

Contém:

- criação do cubo 3D;
- desenho das peças;
- movimentos animados;
- controles do teclado;
- interface ImGui;
- conversão do cubo visual para o estado da IA;
- botão para executar a BFS.

---

## `estado.h`

Define a estrutura lógica usada pelos algoritmos de busca.

Principais tipos:

```cpp
enum class Cor;
enum class Movimento;

struct PecaLogica;
struct EstadoCubo;
struct EstadoHash;
```

Também declara as principais operações sobre o estado:

```cpp
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
```

---

## `estado.cpp`

Implementa a lógica do cubo.

Contém:

- estado inicial resolvido;
- hash;
- função de objetivo;
- movimentos;
- geração de sucessores.

---

## `busca.h`

Define as estruturas genéricas utilizadas por todos os algoritmos.

### Nó de busca

```cpp
struct NoBusca
{
    EstadoCubo estado;

    int pai = -1;

    Movimento movimento =
        Movimento::NENHUM;

    int profundidade = 0;
};
```

Cada nó guarda:

- o estado atual;
- o índice do nó pai;
- o movimento usado para chegar nele;
- a profundidade.

Isso permite reconstruir a solução ao final.

---

## Resultado da busca

```cpp
struct ResultadoBusca
{
    bool encontrou = false;

    std::vector<Movimento> caminho;

    std::size_t estadosVisitados = 0;

    int profundidade = -1;
};
```

O resultado informa:

- se foi encontrada uma solução;
- qual sequência de movimentos resolve o cubo;
- quantos estados foram visitados;
- qual a profundidade da solução.

---

## Interface de fronteira

```cpp
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
```

Essa abstração permite trocar a estrutura de dados sem alterar o laço principal.

Exemplo:

```text
BFS   → queue
DFS   → stack
A*    → priority_queue
```

---

# 5. Laço genérico de busca

O laço principal está em:

```text
busca.cpp
```

Fluxo simplificado:

```text
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
```

Uma vantagem importante dessa estrutura é que o algoritmo genérico não precisa saber se está utilizando fila, pilha ou fila de prioridade.

---

# 6. Busca em Profundidade Iterativa — IDDFS

A IDDFS utiliza uma pilha (`std::stack`) e executa a busca em várias
iterações, aumentando o limite de profundidade a cada tentativa. A mesma
função `buscarGenerico()` é reutilizada; apenas a estrutura de fronteira e o
limite mudam.

A implementação é iterativa, sem recursão, completa e encontra uma solução de
menor profundidade para movimentos de custo unitário.

# 7. Busca A*

A busca A* utiliza `std::priority_queue` e a mesma função `buscarGenerico()`.
Cada estado recebe a prioridade:

```text
f(n) = g(n) + h(n)
```

`g(n)` é a profundidade do nó e `h(n)` é a distância exata até um estado
resolvido, consultada em `tabela_h.bin`. A tabela é gerada por BFS a partir do
estado resolvido quando ainda não existe e depois reutilizada.

Como a heurística representa a distância real, ela é admissível e consistente.
Por isso o A* mantém a solução ótima e normalmente visita menos estados que a
BFS.

# 8. Reprodutibilidade

O painel de embaralhamento possui o campo `Seed`. O mesmo valor de seed e a
mesma quantidade de movimentos produzem o mesmo embaralhamento, permitindo
comparar BFS, IDDFS e A* sobre exatamente o mesmo estado inicial.

# 9. Representação do Cubo 2x2x2

O cubo possui 8 peças de canto.

A ordem utilizada no programa é:

```text
0 = esquerda / cima  / trás
1 = direita  / cima  / trás
2 = esquerda / cima  / frente
3 = direita  / cima  / frente

4 = esquerda / baixo / trás
5 = direita  / baixo / trás
6 = esquerda / baixo / frente
7 = direita  / baixo / frente
```

Cada peça lógica possui:

```cpp
left
right
top
bottom
front
back
```

As faces internas recebem:

```cpp
Cor::CINZA
```

Exemplo de uma peça:

```text
Peça 0

left   = laranja
top    = branco
back   = azul

right  = cinza
bottom = cinza
front  = cinza
```

---

# 10. Movimentos implementados

Foram implementados os 12 movimentos básicos:

```text
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
```

Onde `'` representa o movimento inverso.

Exemplo:

```text
R
```

é desfeito por:

```text
R'
```

A função:

```cpp
aplicarMovimento()
```

gera um novo estado após executar um movimento.

A função:

```cpp
gerarSucessores()
```

gera todos os próximos estados possíveis a partir de um estado atual.

---

# 8. Busca em Largura — BFS

A BFS utiliza uma fila FIFO:

```cpp
std::queue<int> fila;
```

Isso faz com que os estados sejam explorados por nível:

```text
profundidade 0
      ↓
profundidade 1
      ↓
profundidade 2
      ↓
profundidade 3
      ↓
...
```

Como cada movimento possui custo 1, a primeira solução encontrada pela BFS é uma solução de menor profundidade dentro do espaço de estados considerado.

---

# 9. Estados visitados

Para evitar explorar o mesmo estado várias vezes, é utilizado:

```cpp
std::unordered_set<
    EstadoCubo,
    EstadoHash
> visitados;
```

O estado é marcado como visitado no momento em que entra na fila.

Isso evita duplicatas dentro da própria fronteira.

---

# 10. Hash do estado

Foi implementado:

```cpp
struct EstadoHash
```

para permitir que `EstadoCubo` seja utilizado em:

```cpp
std::unordered_set
```

O hash combina as cores das 8 peças.

Importante:

```text
hash igual
```

não significa obrigatoriamente:

```text
estado igual
```

Por isso `unordered_set` também utiliza:

```cpp
operator==
```

para confirmar a igualdade real do estado.

---

# 11. Simetrias do cubo

O Cubo 2x2x2 pode ser observado em 24 orientações globais diferentes.

Por isso foram criadas:

```cpp
gerarSimetrias()
estadoCanonico()
equivalentesPorSimetria()
```

Arquivos:

```text
simetria.h
simetria.cpp
```

---

## Estado canônico

Para um estado são consideradas suas 24 rotações.

Fluxo:

```text
estado
  ↓
gera 24 orientações
  ↓
serializa cada uma
  ↓
escolhe a menor
  ↓
estado canônico
```

A BFS usa o estado canônico apenas para verificar visitados.

O estado real continua armazenado nos nós.

Isso é importante porque a solução precisa ser reconstruída utilizando movimentos reais.

---

# 12. Resultado da otimização por simetria

Em um dos testes:

```text
R U F
```

sem redução por simetria foram visitados aproximadamente:

```text
863 estados
```

com estado canônico:

```text
143 estados
```

Isso mostra uma grande redução no espaço explorado.

---

# 13. Testes realizados

## Teste com profundidade 1

```text
Embaralhamento:
R

Resultado:
Profundidade: 1
Estados visitados: 9
Caminho: R'

Validacao da solucao: OK
```

---

## Teste com profundidade 2

```text
Embaralhamento:
R U

Resultado:
Profundidade: 2
Estados visitados: 30
Caminho: U' R'

Validacao da solucao: OK
```

---

## Teste com profundidade 3

```text
Embaralhamento:
R U F

Resultado:
Profundidade: 3
Estados visitados: 143
Caminho: F' U' L'

Validacao da solucao: OK
```

---

## Teste com profundidade 4

```text
Embaralhamento:
R U F L

Resultado:
Profundidade: 4
Estados visitados: 499
Caminho: L' F' U' L'

Validacao da solucao: OK
```

---

## Teste com profundidade 5

```text
Embaralhamento:
R U F L B

Resultado:
Profundidade: 5
Estados visitados: 2188
Caminho: L' F' U' L' U'

Validacao da solucao: OK
```

---

# 14. Testes automatizados

O arquivo:

```text
teste_bfs.cpp
```

valida:

- movimentos;
- movimentos inversos;
- geração das 24 simetrias;
- estado canônico;
- BFS;
- profundidade;
- reconstrução do caminho;
- reaplicação da solução;
- estado final.

Saída esperada semelhante a:

```text
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
```

---

# 15. Integração com o cubo visual

A parte visual utiliza:

```cpp
struct CubeSection
```

Cada peça visual guarda:

- seus vértices;
- cor da face esquerda;
- cor da face direita;
- cor da face superior;
- cor da face inferior;
- cor da face frontal;
- cor da face traseira.

Para que a BFS possa usar o cubo visual foi criada:

```cpp
EstadoCubo extrairEstadoCubo(
    const std::array<CubeSection, 8>& cube
);
```

Ela faz:

```text
CubeSection
     ↓
converte cores RGB
     ↓
PecaLogica
     ↓
EstadoCubo
```

---

# 16. Teste da conversão visual

Na inicialização do programa:

```text
cubo visual resolvido
        ↓
extrairEstadoCubo()
        ↓
EstadoCubo
        ↓
comparação
        ↓
criarEstadoResolvido()
```

Quando a conversão está correta, aparece:

```text
[BFS] Conversao do cubo visual: OK
```

Esse teste já passou corretamente.

---

# 17. Interface da BFS

Foi criada uma janela ImGui:

```text
Busca em Largura
```

com o botão:

```text
Resolver com BFS
```

Ao clicar:

```cpp
EstadoCubo atual =
    extrairEstadoCubo(cube);

resultadoBFS =
    buscaEmLargura(atual);
```

A interface mostra:

```text
Solucao encontrada!

Profundidade: X

Estados visitados: Y

Caminho:
R' U F' ...
```

---

# 18. Como baixar e preparar o projeto

Clone o repositório:

```bash
git clone https://github.com/CarlosEANascimento/ML-Cubo-Magico.git
```

Entre na pasta:

```bash
cd ML-Cubo-Magico
```

Troque para a branch da BFS:

```bash
git switch busca-largura
```

Caso a branch ainda não exista localmente:

```bash
git fetch origin
git switch -c busca-largura origin/busca-largura
```

---

# 19. Dependência Dear ImGui

O projeto espera o Dear ImGui em:

```text
external/imgui
```

Caso essa pasta não exista:

```bash
mkdir -p external
git clone https://github.com/ocornut/imgui.git external/imgui
```

Depois verifique:

```bash
ls external/imgui
```

Devem existir arquivos como:

```text
imgui.cpp
imgui.h
imgui_draw.cpp
imgui_tables.cpp
imgui_widgets.cpp
backends/
```

---

# 20. Comandos principais

## Limpar arquivos compilados

```bash
make clean
```

Remove:

```text
cubo
teste_bfs
```

---

## Compilar o programa gráfico

```bash
make
```

Ao terminar sem erros, será criado:

```text
./cubo
```

---

## Executar o cubo

```bash
./cubo
```

---

## Compilar e rodar apenas os testes da BFS

```bash
make test-bfs
```

Esse comando não precisa abrir a interface gráfica.

---

## Fluxo recomendado para desenvolvimento

```bash
make clean
make
./cubo
```

Para testar somente a busca:

```bash
make clean
make test-bfs
```

---

# 21. Como operar o cubo

O cubo é movimentado utilizando:

```text
LETRA DA FACE + SETA
```

É importante manter a letra pressionada enquanto aperta a seta.

---

## Movimentos da face direita

### R

```text
R + ↑
```

### R'

```text
R + ↓
```

---

## Movimentos da face esquerda

### L

```text
L + ↑
```

### L'

```text
L + ↓
```

---

## Movimentos da face superior

### U

```text
U + ←
```

### U'

```text
U + →
```

---

## Movimentos da face inferior

### D

```text
D + →
```

### D'

```text
D + ←
```

---

## Movimentos da face frontal

### F

```text
F + →
```

### F'

```text
F + ←
```

---

## Movimentos da face traseira

### B

```text
B + ←
```

### B'

```text
B + →
```

---

## Tabela completa de controles

| Movimento | Teclas |
|---|---|
| `L` | `L + ↑` |
| `L'` | `L + ↓` |
| `R` | `R + ↑` |
| `R'` | `R + ↓` |
| `U` | `U + ←` |
| `U'` | `U + →` |
| `D` | `D + →` |
| `D'` | `D + ←` |
| `F` | `F + →` |
| `F'` | `F + ←` |
| `B` | `B + ←` |
| `B'` | `B + →` |

---

# 22. Como testar a BFS pela interface

Primeiro execute:

```bash
./cubo
```

Ao iniciar, o terminal deve mostrar:

```text
[BFS] Conversao do cubo visual: OK
```

Depois:

1. clique na janela do cubo para dar foco;
2. faça um movimento, por exemplo:

```text
R + ↑
```

3. espere a animação terminar;
4. solte as teclas;
5. clique no botão:

```text
Resolver com BFS
```

O resultado esperado para um único movimento `R` é semelhante a:

```text
Solucao encontrada!

Profundidade: 1

Estados visitados: ...

Caminho:
R'
```

---

# 23. Exemplo de uso completo

```text
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
```

---

# 24. Navegação da câmera

A câmera pode ser movimentada com o mouse.

```text
Clique e arraste
```

para girar a visualização do cubo.

O scroll do mouse altera o zoom.

Esses movimentos alteram apenas a câmera.

Eles não alteram o estado lógico do Cubo Mágico.

---

# 25. Cuidados durante o uso

Não execute a BFS enquanto uma face ainda estiver girando.

O programa já verifica:

```cpp
L != 0 ||
R != 0 ||
U != 0 ||
D != 0 ||
F != 0 ||
B != 0
```

Se houver uma animação em andamento, a interface mostra:

```text
Aguarde o movimento terminar.
```

---

# 26. Mensagem `libdecor` no Linux

Em alguns ambientes Linux pode aparecer:

```text
libdecor-gtk-WARNING: Failed to initialize GTK
Failed to load plugin 'libdecor-gtk.so'
No plugins found, falling back on no decorations
```

Essa mensagem está relacionada à decoração da janela.

Ela não indica falha da BFS.

Se a janela abrir normalmente, o programa pode continuar sendo utilizado.

---

# 27. Erro: `external/imgui/imgui.cpp`

Se aparecer:

```text
make: *** Sem regra para processar o alvo
'external/imgui/imgui.cpp'
```

significa que o ImGui não está presente na pasta esperada.

Execute:

```bash
mkdir -p external
git clone https://github.com/ocornut/imgui.git external/imgui
```

e compile novamente:

```bash
make clean
make
```

---

# 28. Estado atual do projeto

## Concluído

- [x] Representação lógica do cubo
- [x] Estado resolvido
- [x] Igualdade entre estados
- [x] Hash
- [x] 12 movimentos
- [x] Função sucessora
- [x] Função avaliadora
- [x] Estrutura genérica de busca
- [x] BFS
- [x] Fila FIFO
- [x] Tabela de visitados
- [x] Reconstrução do caminho
- [x] Contagem de estados visitados
- [x] Simetrias
- [x] 24 rotações globais
- [x] Estado canônico
- [x] Testes da BFS
- [x] Validação automática da solução
- [x] Cubo gráfico
- [x] Conversão visual → lógica
- [x] Validação da conversão
- [x] Compilação do executável gráfico
- [x] Janela ImGui para BFS
- [x] Botão `Resolver com BFS`
- [x] Exibição da profundidade
- [x] Exibição dos estados visitados
- [x] Exibição do caminho

---

# 29. Próximo passo

O próximo passo é fazer o cubo executar automaticamente a sequência retornada pela BFS.

Hoje:

```text
BFS encontra:

R' U F'
```

mas apenas mostra essa sequência.

O próximo objetivo é:

```text
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
```

Interface desejada:

```text
Busca em Largura

Solucao encontrada!

Profundidade: 5
Estados visitados: 2188

Caminho:
L' F' U' L' U'

[ Executar solucao ]
```

---

# 30. Próximas melhorias possíveis

Depois da execução automática:

- botão para embaralhar;
- seed fixa para testes reproduzíveis;
- reiniciar cubo;
- executar solução passo a passo;
- medir tempo da busca;
- comparar BFS com e sem simetria;
- implementar busca limitada;
- implementar busca em profundidade iterativa;
- implementar A*;
- adicionar heurística;
- selecionar algoritmo na interface.

---

# 31. Observação sobre profundidade

A quantidade de movimentos usados para embaralhar não necessariamente é igual à profundidade ótima.

Exemplo:

```text
R R'
```

foram executados dois movimentos, porém o cubo voltou ao estado inicial.

Logo:

```text
movimentos executados = 2
profundidade ótima = 0
```

A BFS não tenta simplesmente desfazer os movimentos feitos pelo usuário.

Ela explora o espaço de estados e encontra uma solução de menor profundidade.

---

# 32. Resumo técnico

A implementação atual pode ser resumida como:

```text
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
```

---

# 33. Branch

Desenvolvimento da BFS:

```text
busca-largura
```

Repositório:

```text
CarlosEANascimento/ML-Cubo-Magico
```

---

## Situação atual

A Busca em Largura está funcional, testada e conectada ao cubo 3D.

O próximo marco é utilizar o caminho retornado pela BFS para animar automaticamente a resolução do Cubo Mágico.
