# Busca em Profundidade com Aprofundamento Iterativo (IDDFS)

## O que e?

O **IDDFS** (Iterative Deepening Depth-First Search) combina as vantagens da
busca em profundidade (baixo uso de memoria) com as da busca em largura
(garantia de solucao otima).

Funciona assim:
1. Roda uma DFS com limite de profundidade = 0
2. Se nao encontrou, roda com limite = 1
3. Continua aumentando o limite em 1 a cada iteracao
4. Para quando encontra a solucao

---

## Diferenca para a Busca em Largura (BFS)

| Caracteristica     | BFS                          | IDDFS                        |
|--------------------|------------------------------|------------------------------|
| Completo           | Sim                          | Sim                          |
| Otimo              | Sim                          | Sim                          |
| Memoria            | O(b^d) -- exponencial        | O(b * d) -- linear           |
| Tempo              | O(b^d)                       | O(b^d) ligeiramente maior    |
| Re-explora estados | Nao                          | Sim (a cada nova iteracao)   |
| Implementacao      | Fila (queue)                 | Pilha recursiva (call stack) |

Onde b = fator de ramificacao (12 movimentos) e d = profundidade da solucao.

### Por que o IDDFS re-explora e ainda e viavel?

A maioria dos nos esta nos niveis mais profundos da arvore. O custo de
re-explorar os niveis rassos e pequeno comparado ao nivel atual. Por isso
o overhead e em torno de O(b/(b-1)) -- pouco mais de 1x o custo da BFS.

---

## Uso de Simetria

Assim como a BFS, o IDDFS usa `estadoCanonico()` para identificar estados
equivalentes por rotacao do cubo inteiro. Isso significa que o algoritmo
nao distingue entre dois estados que sao iguais apos girar o cubo.

**Onde e aplicado:** dentro de cada caminho DLS, estados canonicos ja
presentes no caminho atual sao podados (evita ciclos).

---

## Implementacao

### `busca_profundidade.h`
Declara a funcao principal:
```cpp
ResultadoBusca buscaEmProfundidade(const EstadoCubo& inicial);
```

### `busca_profundidade.cpp`

**DLS (Depth-Limited Search):**
```
funcao dls(estado, profAtual, limite, caminho, emCaminho):
    nosVisitados++
    se estadoFinal(estado): retorna verdadeiro
    se profAtual >= limite: retorna falso
    para cada (filho, movimento) em gerarSucessores(estado):
        canonico = estadoCanonico(filho)
        se canonico ja esta em emCaminho: pula (poda)
        insere canonico em emCaminho
        caminho.push(movimento)
        se dls(filho, profAtual+1, limite, caminho, emCaminho): retorna verdadeiro
        caminho.pop()             <-- backtrack
        remove canonico de emCaminho
    retorna falso
```

**IDDFS:**
```
funcao buscaEmProfundidade(inicial):
    para limite = 1, 2, 3, ...:
        se dls(inicial, 0, limite, [], {canonico(inicial)}):
            retorna caminho encontrado
```

---

## Comparacao pratica no Cubo Magico 2x2

Para um embaralhamento de profundidade d:

- **BFS**: encontra a solucao otima com d movimentos.
  Usa muita memoria (armazena todos os estados fronteira).

- **IDDFS**: tambem encontra a solucao otima com d movimentos.
  Usa muito menos memoria (apenas o caminho atual na pilha de chamadas).
  Visita mais nos no total (re-exploracao), mas o numero de movimentos
  da solucao e identico ao da BFS.

### Como comparar no programa:
1. Embaralhe o cubo
2. Clique em "Salvar estado atual"
3. Execute BFS e anote: nos visitados + caminho
4. Execute IDS e anote: nos visitados + caminho
5. O caminho devera ter o mesmo numero de movimentos
6. O IDDFS geralmente visita mais nos que a BFS por causa da re-exploracao

---

## Notacao dos movimentos

| Codigo | Face | Seta  | Descricao            |
|--------|------|-------|----------------------|
| Lu     | L    | Cima  | Left para cima       |
| Ld     | L    | Baixo | Left para baixo      |
| Ru     | R    | Cima  | Right para cima      |
| Rd     | R    | Baixo | Right para baixo     |
| Ul     | U    | Esq   | Up para esquerda     |
| Ur     | U    | Dir   | Up para direita      |
| Dr     | D    | Dir   | Down para direita    |
| Dl     | D    | Esq   | Down para esquerda   |
| Fr     | F    | Dir   | Front para direita   |
| Fl     | F    | Esq   | Front para esquerda  |
| Bl     | B    | Esq   | Back para esquerda   |
| Br     | B    | Dir   | Back para direita    |
