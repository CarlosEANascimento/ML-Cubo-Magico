#include <GLFW/glfw3.h>
#include <iostream>
#include <array>
#include <functional>
#include <cmath>
#include <utility>
#include <algorithm>
#include <cstdint>
#include <memory>
#include <queue>
#include <random>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"

// constantes
const float N = 1.0f;
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGTH = 600;

constexpr float PI = 3.14159265f;

const float raio1 = N;
const float raio2 = std::sqrt(std::pow(raio1, 2.0f) + std::pow(raio1, 2.0f));

int L = 0;	// 0 para, -1 L', 1 L
int R = 0;	// equivalentes
int U = 0;
int D = 0;
int F = 0;
int B = 0;

float theta = 0.0f;
float amountOfRotation = 0.0f;

// structs
struct CamState {
	float rotX = 25.0f;
	float rotY = 25.0f;
	float zoom = -4.0f;
	double lastMousePosX = 0.0;
	double lastMousePosY = 0.0;
	bool isDragging = false;
};

struct Coord3d {
	float x = 0.0f;
	float y = 0.0f;
	float z = 0.0f;
};

struct Color {
	float r = 0.0f;
	float g = 0.0f;
	float b = 0.0f;
};

struct CubeSection {
	Coord3d v0;
	Coord3d v1;
	Coord3d v2;
	Coord3d v3;
	Coord3d v4;
	Coord3d v5;
	Coord3d v6;
	Coord3d v7;

	Color left;
	Color right;
	Color top;
	Color bottom;
	Color front;
	Color back;
};

// =========================
// Modelo logico + IA 2x2x2
// =========================
// Estado: 24 adesivos, 4 por face, na ordem U, D, R, L, F, B.
// Este estado e propositalmente separado da geometria 3D: a IA trabalha
// sobre uma representacao simples, serializavel e facil de explicar.
struct CubeState {
	std::array<char, 24> stickers{};
};

struct StickerRef {
	int x;
	int y;
	int z;
	int nx;
	int ny;
	int nz;
};

enum class FrontierMode {
	FIFO,      // Busca em Largura: fila
	LIFO,      // Busca em Profundidade Limitada: pilha
	PRIORITY   // A*: fila de prioridade
};

struct SearchStats {
	bool solved = false;
	int visited = 0;
	int depth = 0;
	std::vector<std::string> solution;
	std::string message;
};

struct SearchNode {
	CubeState state;
	int parent = -1;
	std::string move;
	int depth = 0;
};

struct SearchConfig {
	FrontierMode frontierMode = FrontierMode::FIFO;
	bool useDepthLimit = false;
	int depthLimit = 0;
	int heuristicWeight = 0;
	std::string name;
};

struct Frontier {
	virtual ~Frontier() = default;
	virtual void add(int nodeIndex, int priority) = 0;
	virtual int removeNext() = 0;
	virtual bool empty() const = 0;
};

struct QueueFrontier : Frontier {
	std::queue<int> data;
	void add(int nodeIndex, int) override { data.push(nodeIndex); }
	int removeNext() override {
		int nodeIndex = data.front();
		data.pop();
		return nodeIndex;
	}
	bool empty() const override { return data.empty(); }
};

struct StackFrontier : Frontier {
	std::vector<int> data;
	void add(int nodeIndex, int) override { data.push_back(nodeIndex); }
	int removeNext() override {
		int nodeIndex = data.back();
		data.pop_back();
		return nodeIndex;
	}
	bool empty() const override { return data.empty(); }
};

struct PriorityFrontier : Frontier {
	struct Item {
		int priority;
		int order;
		int nodeIndex;
	};

	struct Compare {
		bool operator()(const Item &a, const Item &b) const {
			if (a.priority != b.priority) return a.priority > b.priority;
			return a.order > b.order;
		}
	};

	std::priority_queue<Item, std::vector<Item>, Compare> data;
	int order = 0;

	void add(int nodeIndex, int priority) override {
		data.push({priority, order++, nodeIndex});
	}

	int removeNext() override {
		int nodeIndex = data.top().nodeIndex;
		data.pop();
		return nodeIndex;
	}

	bool empty() const override { return data.empty(); }
};

const std::array<std::string, 12> ALL_MOVES = {
	"L", "L'", "R", "R'", "U", "U'", "D", "D'", "F", "F'", "B", "B'"
};

// Cada indice aponta para a coordenada do cubinho e a normal daquela face.
// A ordem coincide com a rede mostrada na interface:
// U: 0..3, D: 4..7, R: 8..11, L: 12..15, F: 16..19, B: 20..23.
const std::array<StickerRef, 24> STICKER_REFS = {{
	{ 1,  1, -1,  0,  1,  0}, { 1,  1,  1,  0,  1,  0},
	{-1,  1, -1,  0,  1,  0}, {-1,  1,  1,  0,  1,  0},
	{ 1, -1,  1,  0, -1,  0}, { 1, -1, -1,  0, -1,  0},
	{-1, -1,  1,  0, -1,  0}, {-1, -1, -1,  0, -1,  0},
	{ 1,  1,  1,  1,  0,  0}, { 1,  1, -1,  1,  0,  0},
	{ 1, -1,  1,  1,  0,  0}, { 1, -1, -1,  1,  0,  0},
	{-1,  1, -1, -1,  0,  0}, {-1,  1,  1, -1,  0,  0},
	{-1, -1, -1, -1,  0,  0}, {-1, -1,  1, -1,  0,  0},
	{-1,  1,  1,  0,  0,  1}, { 1,  1,  1,  0,  0,  1},
	{-1, -1,  1,  0,  0,  1}, { 1, -1,  1,  0,  0,  1},
	{ 1,  1, -1,  0,  0, -1}, {-1,  1, -1,  0,  0, -1},
	{ 1, -1, -1,  0,  0, -1}, {-1, -1, -1,  0,  0, -1},
}};

CubeState makeSolvedState() {
	CubeState state;
	const std::array<char, 6> faces = {'W', 'Y', 'R', 'O', 'G', 'B'};
	for (int face = 0; face < 6; ++face) {
		for (int i = 0; i < 4; ++i) state.stickers[face * 4 + i] = faces[face];
	}
	return state;
}

std::string serializeState(const CubeState &state) {
	return std::string(state.stickers.begin(), state.stickers.end());
}

bool isGoalState(const CubeState &state) {
	return state.stickers == makeSolvedState().stickers;
}

int evaluateState(const CubeState &state) {
	int misplaced = 0;
	CubeState solved = makeSolvedState();
	for (int i = 0; i < 24; ++i) {
		if (state.stickers[i] != solved.stickers[i]) ++misplaced;
	}

	// Heuristica escolhida para A*: uma rotacao altera no maximo 8 adesivos.
	// Portanto ceil(adesivos_fora_do_lugar / 8) e um limite inferior simples.
	return (misplaced + 7) / 8;
}

bool sameState(const CubeState &a, const CubeState &b) {
	return a.stickers == b.stickers;
}

bool isInverseMove(const std::string &a, const std::string &b) {
	if (a.empty() || b.empty()) return false;
	if (a[0] != b[0]) return false;
	return a.size() != b.size();
}

bool isSameFaceMove(const std::string &a, const std::string &b) {
	return !a.empty() && !b.empty() && a[0] == b[0];
}

StickerRef rotateStickerRef(StickerRef ref, char axis, int dir) {
	auto rotateOnce = [&](StickerRef &s) {
		if (axis == 'X') {
			int y = s.y, z = s.z, ny = s.ny, nz = s.nz;
			s.y = -z; s.z = y;
			s.ny = -nz; s.nz = ny;
		} else if (axis == 'Y') {
			int x = s.x, z = s.z, nx = s.nx, nz = s.nz;
			s.x = -z; s.z = x;
			s.nx = -nz; s.nz = nx;
		} else {
			int x = s.x, y = s.y, nx = s.nx, ny = s.ny;
			s.x = y; s.y = -x;
			s.nx = ny; s.ny = -nx;
		}
	};

	int turns = dir > 0 ? 1 : 3;
	for (int i = 0; i < turns; ++i) rotateOnce(ref);
	return ref;
}

int stickerIndexFor(const StickerRef &ref) {
	for (int i = 0; i < static_cast<int>(STICKER_REFS.size()); ++i) {
		const auto &candidate = STICKER_REFS[i];
		if (candidate.x == ref.x && candidate.y == ref.y && candidate.z == ref.z &&
			candidate.nx == ref.nx && candidate.ny == ref.ny && candidate.nz == ref.nz) {
			return i;
		}
	}
	return -1;
}

bool isStickerInLayer(const StickerRef &ref, char axis, int layer) {
	if (axis == 'X') return ref.x == layer;
	if (axis == 'Y') return ref.y == layer;
	return ref.z == layer;
}

void moveInfo(const std::string &move, char &axis, int &layer, int &dir) {
	char face = move[0];
	bool prime = move.size() > 1;

	if (face == 'R') { axis = 'X'; layer =  1; dir = -1; }
	if (face == 'L') { axis = 'X'; layer = -1; dir =  1; }
	if (face == 'U') { axis = 'Y'; layer =  1; dir =  1; }
	if (face == 'D') { axis = 'Y'; layer = -1; dir = -1; }
	if (face == 'F') { axis = 'Z'; layer =  1; dir =  1; }
	if (face == 'B') { axis = 'Z'; layer = -1; dir = -1; }

	if (prime) dir *= -1;
}

CubeState applyMoveToState(const CubeState &state, const std::string &move) {
	char axis = 'X';
	int layer = 1;
	int dir = 1;
	moveInfo(move, axis, layer, dir);

	CubeState next = state;
	for (int i = 0; i < 24; ++i) {
		StickerRef ref = STICKER_REFS[i];
		if (!isStickerInLayer(ref, axis, layer)) continue;

		StickerRef rotated = rotateStickerRef(ref, axis, dir);
		int destination = stickerIndexFor(rotated);
		if (destination >= 0) {
			next.stickers[destination] = state.stickers[i];
		}
	}
	return next;
}

std::vector<std::pair<std::string, CubeState>> successors(const SearchNode &node) {
	std::vector<std::pair<std::string, CubeState>> nextStates;
	nextStates.reserve(12);

	for (const auto &move : ALL_MOVES) {
		if (isInverseMove(node.move, move)) continue;
		nextStates.push_back({move, applyMoveToState(node.state, move)});
	}

	return nextStates;
}

std::vector<std::string> reconstructSolution(const std::vector<SearchNode> &nodes, int nodeIndex) {
	std::vector<std::string> solution;
	while (nodeIndex >= 0 && nodes[nodeIndex].parent >= 0) {
		solution.push_back(nodes[nodeIndex].move);
		nodeIndex = nodes[nodeIndex].parent;
	}
	std::reverse(solution.begin(), solution.end());
	return solution;
}

std::unique_ptr<Frontier> makeFrontier(FrontierMode mode) {
	if (mode == FrontierMode::FIFO) return std::make_unique<QueueFrontier>();
	if (mode == FrontierMode::LIFO) return std::make_unique<StackFrontier>();
	return std::make_unique<PriorityFrontier>();
}

int calculatePriority(const CubeState &state, int depth, const SearchConfig &config) {
	if (config.frontierMode != FrontierMode::PRIORITY) return depth;

	// Parametro que transforma o mesmo laco em A*:
	// f(n) = g(n) + peso * h(n). Para A*, peso = 1.
	return depth + config.heuristicWeight * evaluateState(state);
}

SearchStats solveSearch(const CubeState &start, const SearchConfig &config, int maxVisited) {
	std::unique_ptr<Frontier> frontier = makeFrontier(config.frontierMode);
	std::vector<SearchNode> nodes;
	std::unordered_map<std::string, int> bestDepth;
	SearchStats stats;

	nodes.push_back({start, -1, "", 0});
	frontier->add(0, calculatePriority(start, 0, config));
	bestDepth[serializeState(start)] = 0;

	// Laco exigido no enunciado:
	// 1. Adicionar estado na estrutura
	// 2. Enquanto a estrutura nao estiver vazia:
	//    2.1 Remover proximo estado da estrutura
	//    2.2 Avaliar estado
	//    2.3 Adicionar estados seguintes na estrutura
	// A estrutura e a prioridade mudam por parametro, sem alterar este laco.
	while (!frontier->empty()) {
		int currentIndex = frontier->removeNext();
		SearchNode current = nodes[currentIndex];
		stats.visited++;

		if (isGoalState(current.state)) {
			stats.solved = true;
			stats.depth = current.depth;
			stats.solution = reconstructSolution(nodes, currentIndex);
			stats.message = "Solucao encontrada.";
			return stats;
		}

		if (stats.visited >= maxVisited) {
			stats.message = "Limite de estados visitados atingido.";
			return stats;
		}

		if (config.useDepthLimit && current.depth >= config.depthLimit) continue;

		for (const auto &entry : successors(current)) {
			const std::string &move = entry.first;
			const CubeState &childState = entry.second;
			std::string key = serializeState(childState);
			int childDepth = current.depth + 1;

			auto found = bestDepth.find(key);
			if (found != bestDepth.end() && found->second <= childDepth) continue;

			bestDepth[key] = childDepth;
			nodes.push_back({childState, currentIndex, move, childDepth});
			int childIndex = static_cast<int>(nodes.size()) - 1;
			int priority = calculatePriority(childState, childDepth, config);
			frontier->add(childIndex, priority);
		}
	}

	stats.message = "Sem solucao.";
	return stats;
}

SearchStats solveBfs(const CubeState &start, int maxVisited) {
	SearchConfig config;
	config.frontierMode = FrontierMode::FIFO;
	config.name = "Busca em Largura";
	return solveSearch(start, config, maxVisited);
}

SearchStats solveDepthLimited(const CubeState &start, int depthLimit, int maxVisited) {
	SearchConfig config;
	config.frontierMode = FrontierMode::LIFO;
	config.useDepthLimit = true;
	config.depthLimit = depthLimit;
	config.name = "Busca em Profundidade Limitada";
	return solveSearch(start, config, maxVisited);
}

SearchStats solveIterativeDeepening(const CubeState &start, int maxDepth, int maxVisited) {
	SearchStats total;
	for (int limit = 0; limit <= maxDepth; ++limit) {
		SearchStats attempt = solveDepthLimited(start, limit, maxVisited);
		total.visited += attempt.visited;

		if (attempt.solved) {
			attempt.visited = total.visited;
			attempt.message = "Solucao encontrada com limite " + std::to_string(limit) + ".";
			return attempt;
		}
	}
	total.message = "Sem solucao ate o limite informado.";
	return total;
}

SearchStats solveAStar(const CubeState &start, int maxVisited) {
	SearchConfig config;
	config.frontierMode = FrontierMode::PRIORITY;
	config.heuristicWeight = 1;
	config.name = "A*";
	return solveSearch(start, config, maxVisited);
}

CubeState scrambleState(uint32_t seed, int movesCount, std::vector<std::string> &movesUsed) {
	CubeState state = makeSolvedState();
	std::mt19937 rng(seed);
	std::uniform_int_distribution<int> dist(0, static_cast<int>(ALL_MOVES.size()) - 1);
	std::string previous;

	movesUsed.clear();
	for (int i = 0; i < movesCount; ++i) {
		std::string move;
		do {
			move = ALL_MOVES[dist(rng)];
		} while (isInverseMove(previous, move) || isSameFaceMove(previous, move));

		state = applyMoveToState(state, move);
		movesUsed.push_back(move);
		previous = move;
	}
	return state;
}

std::string joinMoves(const std::vector<std::string> &moves) {
	if (moves.empty()) return "(nenhum)";
	std::ostringstream out;
	for (size_t i = 0; i < moves.size(); ++i) {
		if (i) out << ' ';
		out << moves[i];
	}
	return out.str();
}

Color colorForSticker(char sticker) {
	if (sticker == 'W') return {0.85f, 0.85f, 0.85f};
	if (sticker == 'Y') return {1.0f, 1.0f, 0.0f};
	if (sticker == 'R') return {1.0f, 0.0f, 0.0f};
	if (sticker == 'O') return {1.0f, 0.5f, 0.0f};
	if (sticker == 'G') return {0.0f, 1.0f, 0.0f};
	if (sticker == 'B') return {0.0f, 0.0f, 1.0f};
	return {0.15f, 0.15f, 0.15f};
}

void applyStateToCube(std::array<CubeSection, 8> &cube, const CubeState &state) {
	Color gray = {0.15f, 0.15f, 0.15f};
	for (auto &piece : cube) {
		piece.left = gray;
		piece.right = gray;
		piece.top = gray;
		piece.bottom = gray;
		piece.front = gray;
		piece.back = gray;
	}

	cube[1].top = colorForSticker(state.stickers[0]);
	cube[3].top = colorForSticker(state.stickers[1]);
	cube[0].top = colorForSticker(state.stickers[2]);
	cube[2].top = colorForSticker(state.stickers[3]);

	cube[7].bottom = colorForSticker(state.stickers[4]);
	cube[5].bottom = colorForSticker(state.stickers[5]);
	cube[6].bottom = colorForSticker(state.stickers[6]);
	cube[4].bottom = colorForSticker(state.stickers[7]);

	cube[3].right = colorForSticker(state.stickers[8]);
	cube[1].right = colorForSticker(state.stickers[9]);
	cube[7].right = colorForSticker(state.stickers[10]);
	cube[5].right = colorForSticker(state.stickers[11]);

	cube[0].left = colorForSticker(state.stickers[12]);
	cube[2].left = colorForSticker(state.stickers[13]);
	cube[4].left = colorForSticker(state.stickers[14]);
	cube[6].left = colorForSticker(state.stickers[15]);

	cube[2].front = colorForSticker(state.stickers[16]);
	cube[3].front = colorForSticker(state.stickers[17]);
	cube[6].front = colorForSticker(state.stickers[18]);
	cube[7].front = colorForSticker(state.stickers[19]);

	cube[1].back = colorForSticker(state.stickers[20]);
	cube[0].back = colorForSticker(state.stickers[21]);
	cube[5].back = colorForSticker(state.stickers[22]);
	cube[4].back = colorForSticker(state.stickers[23]);
}

// callbacks
void framebufferSizeCallback(GLFWwindow* window, int width, int height) {
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
		glfwSetWindowShouldClose(window, true);
	}
}

void cursosPosCallback(GLFWwindow* window, double xPos, double yPos) {
    if (ImGui::GetIO().WantCaptureMouse) return;

    float rotationSpeed = 0.5f;
    auto* cam = static_cast<CamState*>(glfwGetWindowUserPointer(window));

    // o if cam vai para true se o ponteiro cam aponta para um endereço válido
    // static_cast converte 1 tipo num outro
    if (cam && cam->isDragging) {
        cam->rotX += static_cast<float>(xPos - cam->lastMousePosX) * rotationSpeed;
        cam->rotY += static_cast<float>(yPos - cam->lastMousePosY) * rotationSpeed;
        cam->lastMousePosX = xPos;
        cam->lastMousePosY = yPos;
    }
}

void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
	// se os cliques forem para interação com os componentes da GUI os cliques são ignorados
	if (ImGui::GetIO().WantCaptureMouse) return;

	auto* cam = static_cast<CamState*>(glfwGetWindowUserPointer(window));
	if (!cam) return;

	if (button == GLFW_MOUSE_BUTTON_LEFT) {
		if (action == GLFW_PRESS) {
			cam->isDragging = true;
			glfwGetCursorPos(window, &cam->lastMousePosX, &cam->lastMousePosY);
		} else if (action == GLFW_RELEASE) {
			cam->isDragging = false;
		}
	}
}

void scrollCallback (GLFWwindow* window, double xOffset, double yOffset) {
	if (ImGui::GetIO().WantCaptureMouse) return;

	auto* cam = static_cast<CamState*>(glfwGetWindowUserPointer(window));
	if (!cam) return;

	cam->zoom += static_cast<float>(yOffset) * 0.1f;

	if(cam->zoom > -3.0f) cam->zoom = -3.0f;
	if(cam->zoom < -10.0f) cam->zoom = -10.0f;
}

// utilitários

// IA fez -> basicamente projeta 1 ponto do plano 3d renderizado, no plano 2d da janela
ImVec2 worldToScreen(float x, float y, float z) {
    float modelview[16], projection[16];
    int viewport[4];

    glGetFloatv(GL_MODELVIEW_MATRIX, modelview);
    glGetFloatv(GL_PROJECTION_MATRIX, projection);
    glGetIntegerv(GL_VIEWPORT, viewport);

    // 1. Aplica a matriz ModelView
    float eyeX = modelview[0]*x + modelview[4]*y + modelview[8]*z  + modelview[12];
    float eyeY = modelview[1]*x + modelview[5]*y + modelview[9]*z  + modelview[13];
    float eyeZ = modelview[2]*x + modelview[6]*y + modelview[10]*z + modelview[14];
    float eyeW = modelview[3]*x + modelview[7]*y + modelview[11]*z + modelview[15];

    // 2. Aplica a matriz Projection (Clip space)
    float clipX = projection[0]*eyeX + projection[4]*eyeY + projection[8]*eyeZ  + projection[12]*eyeW;
    float clipY = projection[1]*eyeX + projection[5]*eyeY + projection[9]*eyeZ  + projection[13]*eyeW;
    float clipW = projection[3]*eyeX + projection[7]*eyeY + projection[11]*eyeZ + projection[15]*eyeW;

    if (clipW <= 0.001f) return ImVec2(-1000, -1000); // Fora/atrás da câmera

    // 3. Normaliza (NDC: -1 a 1) e mapeia para a janela em pixels
    float ndcX = clipX / clipW;
    float ndcY = clipY / clipW;

    float screenX = viewport[0] + (ndcX + 1.0f) * 0.5f * viewport[2];
    // Inverte o eixo Y pois no ImGui Y=0 fica no topo da janela
    float screenY = viewport[1] + (1.0f - (ndcY + 1.0f) * 0.5f) * viewport[3];

    return ImVec2(screenX, screenY);
}

std::array<Coord3d, 8> calcCubeVertex(Coord3d center, float size) {
	Coord3d vertex[8];

	vertex[0] = {center.x - size / 2, center.y + size / 2, center.z - size / 2};
	vertex[1] = {center.x + size / 2, center.y + size / 2, center.z - size / 2};
	vertex[2] = {center.x - size / 2, center.y + size / 2, center.z + size / 2};
	vertex[3] = {center.x + size / 2, center.y + size / 2, center.z + size / 2};
	vertex[4] = {center.x - size / 2, center.y - size / 2, center.z - size / 2};
	vertex[5] = {center.x + size / 2, center.y - size / 2, center.z - size / 2};
	vertex[6] = {center.x - size / 2, center.y - size / 2, center.z + size / 2};
	vertex[7] = {center.x + size / 2, center.y - size / 2, center.z + size / 2};

	return {
		vertex[0], vertex[1], vertex[2], vertex[3], vertex[4], vertex[5], vertex[6], vertex[7]
	};
}

// movimentos do cubo

// interações com o teclado
void processInput (GLFWwindow* window) {
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
		glfwSetWindowShouldClose(window, true);
	}
}

void rubiksInteractions (GLFWwindow* window, std::array<CubeSection, 8> &cube) {
	bool isBusy = (L != 0 || R != 0 || U != 0 || D != 0 || F != 0 || B != 0);
	if (isBusy) return;

    if (glfwGetKey(window, GLFW_KEY_L) == GLFW_PRESS) {
        if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) {
			L = 1;
        } else if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) {
			L = -1;
        }
    } else if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS) {
        if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) {
			R = 1;
        } else if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) {
			R = -1;
        }
    } else if (glfwGetKey(window, GLFW_KEY_U) == GLFW_PRESS) {
        if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) {
			U = 1;
        } else if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) {
			U = -1;
        }
    } else if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
        if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) {
			D = 1;
        } else if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) {
			D = -1;
        }
    } else if (glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS) {
        if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) {
			F = 1;
        } else if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) {
			F = -1;
        }
    } else if (glfwGetKey(window, GLFW_KEY_B) == GLFW_PRESS) {
        if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) {
			B = 1;
        } else if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) {
			B = -1;
        }
    }
}

// renders
void drawCartesianPlan () {
	glLineWidth(1.0f);
    glBegin(GL_LINES);

        //eixo x
        glColor3f(1.0f, 0.5f, 0.5f);
        glVertex3d(0.0f - N * 1.5, 0.0f, 0.0f);
        glVertex3d(0.0f + N * 1.5, 0.0f, 0.0f);

        // eixo y
        glColor3f(0.5f, 1.0f, 0.5f);
        glVertex3d(0.0f, 0.0f - N * 1.5, 0.0f);
        glVertex3d(0.0f, 0.0f + N * 1.5, 0.0f);

        // eixo z
        glColor3f(0.5f, 0.5f, 1.0f);
        glVertex3d(0.0f, 0.0f, 0.0f - N * 1.5);
        glVertex3d(0.0f, 0.0f, 0.0f + N * 1.5);

    glEnd();
}

void drawCartesianPlanLabels () {
	float L = N * 1.75;
	ImVec2 pPosX = worldToScreen( L,  0,  0);
	ImVec2 pNegX = worldToScreen(-L,  0,  0);
	ImVec2 pPosY = worldToScreen( 0,  L,  0);
	ImVec2 pNegY = worldToScreen( 0, -L,  0);
	ImVec2 pPosZ = worldToScreen( 0,  0,  L);
	ImVec2 pNegZ = worldToScreen( 0,  0, -L);

	ImDrawList* draw = ImGui::GetForegroundDrawList();

	draw->AddText(pPosX, ImColor(255, 128, 128), "+X (R)");
	draw->AddText(pNegX, ImColor(255, 128, 128), "-X (L)");
	draw->AddText(pPosY, ImColor(128, 255, 128), "+Y (U)");
	draw->AddText(pNegY, ImColor(128, 255, 128), "-Y (D)");
	draw->AddText(pPosZ, ImColor(128, 128, 255), "+Z (F)");
	draw->AddText(pNegZ, ImColor(128, 128, 255), "-Z (B)");
}

void drawCubeSection (CubeSection cube) {
	Coord3d v0 = cube.v0;
	Coord3d v1 = cube.v1;
	Coord3d v2 = cube.v2;
	Coord3d v3 = cube.v3;
	Coord3d v4 = cube.v4;
	Coord3d v5 = cube.v5;
	Coord3d v6 = cube.v6;
	Coord3d v7 = cube.v7;

	glBegin(GL_QUADS);
	// left
	glColor3f(cube.left.r, cube.left.g, cube.left.b);
		glVertex3f(v4.x, v4.y, v4.z);
		glVertex3f(v6.x, v6.y, v6.z);
		glVertex3f(v2.x, v2.y, v2.z);
		glVertex3f(v0.x, v0.y, v0.z);

	// right
	glColor3f(cube.right.r, cube.right.g, cube.right.b);
		glVertex3f(v5.x, v5.y, v5.z);
		glVertex3f(v7.x, v7.y, v7.z);
		glVertex3f(v3.x, v3.y, v3.z);
		glVertex3f(v1.x, v1.y, v1.z);

	// top
	glColor3f(cube.top.r, cube.top.g, cube.top.b);
		glVertex3f(v0.x, v0.y, v0.z);
		glVertex3f(v2.x, v2.y, v2.z);
		glVertex3f(v3.x, v3.y, v3.z);
		glVertex3f(v1.x, v1.y, v1.z);

	// bot
	glColor3f(cube.bottom.r, cube.bottom.g, cube.bottom.b);
		glVertex3f(v4.x, v4.y, v4.z);
		glVertex3f(v5.x, v5.y, v5.z);
		glVertex3f(v7.x, v7.y, v7.z);
		glVertex3f(v6.x, v6.y, v6.z);

	// front 
	glColor3f(cube.front.r, cube.front.g, cube.front.b);
		glVertex3f(v2.x, v2.y, v2.z);
		glVertex3f(v6.x, v6.y, v6.z);
		glVertex3f(v7.x, v7.y, v7.z);
		glVertex3f(v3.x, v3.y, v3.z);

	// back
	glColor3f(cube.back.r, cube.back.g, cube.back.b);
		glVertex3f(v0.x, v0.y, v0.z);
		glVertex3f(v4.x, v4.y, v4.z);
		glVertex3f(v5.x, v5.y, v5.z);
		glVertex3f(v1.x, v1.y, v1.z);
	glEnd();

	// contorno
	glLineWidth(3.0f);
	glBegin(GL_LINES);
	glColor3f(1.0f, 1.0f, 1.0f);
		//1
		glVertex3f(v0.x, v0.y, v0.z);
		glVertex3f(v2.x, v2.y, v2.z);

		glVertex3f(v2.x, v2.y, v2.z);
		glVertex3f(v6.x, v6.y, v6.z);

		glVertex3f(v6.x, v6.y, v6.z);
		glVertex3f(v4.x, v4.y, v4.z);

		glVertex3f(v4.x, v4.y, v4.z);
		glVertex3f(v0.x, v0.y, v0.z);

		//2
		glVertex3f(v0.x, v0.y, v0.z);
		glVertex3f(v1.x, v1.y, v1.z);

		glVertex3f(v1.x, v1.y, v1.z);
		glVertex3f(v5.x, v5.y, v5.z);

		glVertex3f(v5.x, v5.y, v5.z);
		glVertex3f(v4.x, v4.y, v4.z);

		glVertex3f(v4.x, v4.y, v4.z);
		glVertex3f(v0.x, v0.y, v0.z);

		//3
		glVertex3f(v1.x, v1.y, v1.z);
		glVertex3f(v5.x, v5.y, v5.z);

		glVertex3f(v5.x, v5.y, v5.z);
		glVertex3f(v7.x, v7.y, v7.z);

		glVertex3f(v7.x, v7.y, v7.z);
		glVertex3f(v3.x, v3.y, v3.z);

		glVertex3f(v3.x, v3.y, v3.z);
		glVertex3f(v1.x, v1.y, v1.z);

		//4
		glVertex3f(v3.x, v3.y, v3.z);
		glVertex3f(v7.x, v7.y, v7.z);

		glVertex3f(v7.x, v7.y, v7.z);
		glVertex3f(v6.x, v6.y, v6.z);

		glVertex3f(v6.x, v6.y, v6.z);
		glVertex3f(v2.x, v2.y, v2.z);

		glVertex3f(v2.x, v2.y, v2.z);
		glVertex3f(v3.x, v3.y, v3.z);
	glEnd();
}

void drawRubiks (std::array<CubeSection, 8> &cubes) {
	drawCubeSection(cubes[0]);
	drawCubeSection(cubes[1]);
	drawCubeSection(cubes[2]);
	drawCubeSection(cubes[3]);
	drawCubeSection(cubes[4]);
	drawCubeSection(cubes[5]);
	drawCubeSection(cubes[6]);
	drawCubeSection(cubes[7]);
}

void drawInfosGUI (
	std::array<CubeSection*, 4> uSection,
	std::array<CubeSection*, 4> dSection,
	std::array<CubeSection*, 4> rSection,
	std::array<CubeSection*, 4> lSection,
	std::array<CubeSection*, 4> fSection,
	std::array<CubeSection*, 4> bSection
) {
	ImGui::SetNextWindowPos(ImVec2(20, 20), ImGuiCond_Always);
	ImGui::Begin("TextoFixo", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_AlwaysAutoResize);
	ImGui::Text("Cubo Mágico (Arraste com o Mouse)");
	ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);

	ImGui::Text("Use a janela 'Jogo e IA' para mover, embaralhar e resolver.");

	ImGui::Text("UP FACES");
	ImGui::TextColored(ImVec4(uSection[3]->top.r, uSection[3]->top.g, uSection[3]->top.b, 1.0f),"[O]");
	ImGui::SameLine();
	ImGui::TextColored(ImVec4(uSection[2]->top.r, uSection[2]->top.g, uSection[2]->top.b, 1.0f),"[O]");
	ImGui::TextColored(ImVec4(uSection[0]->top.r, uSection[0]->top.g, uSection[0]->top.b, 1.0f),"[O]");
	ImGui::SameLine();
	ImGui::TextColored(ImVec4(uSection[1]->top.r, uSection[1]->top.g, uSection[1]->top.b, 1.0f),"[O]");

	ImGui::Text("DOWN FACES");
	ImGui::TextColored(ImVec4(dSection[2]->bottom.r, dSection[2]->bottom.g, dSection[2]->bottom.b, 1.0f),"[O]");
	ImGui::SameLine();
	ImGui::TextColored(ImVec4(dSection[3]->bottom.r, dSection[3]->bottom.g, dSection[3]->bottom.b, 1.0f),"[O]");
	ImGui::TextColored(ImVec4(dSection[1]->bottom.r, dSection[1]->bottom.g, dSection[1]->bottom.b, 1.0f),"[O]");
	ImGui::SameLine();
	ImGui::TextColored(ImVec4(dSection[0]->bottom.r, dSection[0]->bottom.g, dSection[0]->bottom.b, 1.0f),"[O]");
	
	ImGui::Text("RIGHT FACES");
	ImGui::TextColored(ImVec4(rSection[1]->right.r, rSection[1]->right.g, rSection[1]->right.b, 1.0f),"[O]");
	ImGui::SameLine();
	ImGui::TextColored(ImVec4(rSection[0]->right.r, rSection[0]->right.g, rSection[0]->right.b, 1.0f),"[O]");
	ImGui::TextColored(ImVec4(rSection[2]->right.r, rSection[2]->right.g, rSection[2]->right.b, 1.0f),"[O]");
	ImGui::SameLine();
	ImGui::TextColored(ImVec4(rSection[3]->right.r, rSection[3]->right.g, rSection[3]->right.b, 1.0f),"[O]");

	ImGui::Text("LEFT FACES");
	ImGui::TextColored(ImVec4(lSection[0]->left.r, lSection[0]->left.g, lSection[0]->left.b, 1.0f),"[O]");
	ImGui::SameLine();
	ImGui::TextColored(ImVec4(lSection[1]->left.r, lSection[1]->left.g, lSection[1]->left.b, 1.0f),"[O]");
	ImGui::TextColored(ImVec4(lSection[3]->left.r, lSection[3]->left.g, lSection[3]->left.b, 1.0f),"[O]");
	ImGui::SameLine();
	ImGui::TextColored(ImVec4(lSection[2]->left.r, lSection[2]->left.g, lSection[2]->left.b, 1.0f),"[O]");

	ImGui::Text("FRONT FACES");
	ImGui::TextColored(ImVec4(fSection[1]->front.r, fSection[1]->front.g, fSection[1]->front.b, 1.0f),"[O]");
	ImGui::SameLine();
	ImGui::TextColored(ImVec4(fSection[0]->front.r, fSection[0]->front.g, fSection[0]->front.b, 1.0f),"[O]");
	ImGui::TextColored(ImVec4(fSection[2]->front.r, fSection[2]->front.g, fSection[2]->front.b, 1.0f),"[O]");
	ImGui::SameLine();
	ImGui::TextColored(ImVec4(fSection[3]->front.r, fSection[3]->front.g, fSection[3]->front.b, 1.0f),"[O]");

	ImGui::Text("BACK FACES");
	ImGui::TextColored(ImVec4(bSection[0]->back.r, bSection[0]->back.g, bSection[0]->back.b, 1.0f),"[O]");
	ImGui::SameLine();
	ImGui::TextColored(ImVec4(bSection[1]->back.r, bSection[1]->back.g, bSection[1]->back.b, 1.0f),"[O]");
	ImGui::TextColored(ImVec4(bSection[3]->back.r, bSection[3]->back.g, bSection[3]->back.b, 1.0f),"[O]");
	ImGui::SameLine();
	ImGui::TextColored(ImVec4(bSection[2]->back.r, bSection[2]->back.g, bSection[2]->back.b, 1.0f),"[O]");
	ImGui::End();
}

void drawAiControlGUI(
	CubeState &state,
	std::array<CubeSection, 8> &cube,
	int &seed,
	int &scrambleLength,
	int &depthLimit,
	int &maxVisited,
	std::vector<std::string> &scrambleMoves,
	SearchStats &lastStats,
	std::vector<std::string> &pendingSolution,
	int &pendingSolutionIndex
) {
	ImGui::SetNextWindowPos(ImVec2(560, 20), ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowSize(ImVec2(330, 560), ImGuiCond_FirstUseEver);
	ImGui::Begin("Jogo e IA");

	ImGui::TextWrapped("Estado: 24 adesivos (U,D,R,L,F,B). Sucessores: 12 movimentos.");
	ImGui::TextWrapped("Mesmo laco de busca: BFS usa fila, IDDFS usa pilha+limite, A* usa prioridade g+h.");
	ImGui::Separator();

	ImGui::Text("Jogar");
	auto moveButton = [&](const char *label, const std::string &move) {
		if (ImGui::Button(label, ImVec2(70, 0))) {
			state = applyMoveToState(state, move);
			applyStateToCube(cube, state);
			pendingSolution.clear();
			pendingSolutionIndex = 0;
			lastStats = {};
			lastStats.message = "Movimento aplicado: " + move;
		}
	};

	moveButton("L", "L"); ImGui::SameLine();
	moveButton("L'", "L'"); ImGui::SameLine();
	moveButton("R", "R"); ImGui::SameLine();
	moveButton("R'", "R'");
	moveButton("U", "U"); ImGui::SameLine();
	moveButton("U'", "U'"); ImGui::SameLine();
	moveButton("D", "D"); ImGui::SameLine();
	moveButton("D'", "D'");
	moveButton("F", "F"); ImGui::SameLine();
	moveButton("F'", "F'"); ImGui::SameLine();
	moveButton("B", "B"); ImGui::SameLine();
	moveButton("B'", "B'");

	if (ImGui::Button("Resetar cubo", ImVec2(-1, 0))) {
		state = makeSolvedState();
		applyStateToCube(cube, state);
		scrambleMoves.clear();
		pendingSolution.clear();
		pendingSolutionIndex = 0;
		lastStats = {};
		lastStats.message = "Cubo resetado.";
	}

	ImGui::Separator();
	ImGui::Text("Embaralhamento reproduzivel");
	ImGui::InputInt("Seed", &seed);
	ImGui::SliderInt("Qtd. movimentos", &scrambleLength, 1, 12);
	if (ImGui::Button("Embaralhar com seed", ImVec2(-1, 0))) {
		state = scrambleState(static_cast<uint32_t>(seed), scrambleLength, scrambleMoves);
		applyStateToCube(cube, state);
		pendingSolution.clear();
		pendingSolutionIndex = 0;
		lastStats = {};
		lastStats.message = "Cubo embaralhado.";
	}
	ImGui::TextWrapped("Scramble: %s", joinMoves(scrambleMoves).c_str());

	ImGui::Separator();
	ImGui::Text("Solucionar por IA");
	ImGui::InputInt("Max. visitados", &maxVisited);
	if (maxVisited < 1000) maxVisited = 1000;
	ImGui::SliderInt("Limite IDDFS", &depthLimit, 1, 14);

	auto runSolver = [&](const char *name, const std::function<SearchStats()> &solver) {
		lastStats = solver();
		pendingSolution = lastStats.solution;
		pendingSolutionIndex = 0;
		if (lastStats.solved) {
			lastStats.message = std::string(name) + ": solucao encontrada.";
		} else {
			lastStats.message = std::string(name) + ": " + lastStats.message;
		}
	};

	if (ImGui::Button("Busca em Largura", ImVec2(-1, 0))) {
		runSolver("BFS", [&]() { return solveBfs(state, maxVisited); });
	}
	if (ImGui::Button("Profundidade Iterativa", ImVec2(-1, 0))) {
		runSolver("IDDFS", [&]() { return solveIterativeDeepening(state, depthLimit, maxVisited); });
	}
	if (ImGui::Button("A* (prioridade g+h)", ImVec2(-1, 0))) {
		runSolver("A*", [&]() { return solveAStar(state, maxVisited); });
	}

	ImGui::Separator();
	ImGui::Text("Resultado");
	ImGui::Text("Visitados: %d", lastStats.visited);
	ImGui::Text("Profundidade: %d", lastStats.depth);
	ImGui::Text("h(estado atual): %d", evaluateState(state));
	ImGui::TextWrapped("%s", lastStats.message.c_str());
	ImGui::TextWrapped("Solucao: %s", joinMoves(pendingSolution).c_str());

	if (!pendingSolution.empty()) {
		ImGui::Text("Passo: %d / %d", pendingSolutionIndex, static_cast<int>(pendingSolution.size()));
		if (ImGui::Button("Aplicar proximo passo", ImVec2(-1, 0)) &&
			pendingSolutionIndex < static_cast<int>(pendingSolution.size())) {
			state = applyMoveToState(state, pendingSolution[pendingSolutionIndex]);
			applyStateToCube(cube, state);
			pendingSolutionIndex++;
		}
		if (ImGui::Button("Aplicar solucao toda", ImVec2(-1, 0))) {
			while (pendingSolutionIndex < static_cast<int>(pendingSolution.size())) {
				state = applyMoveToState(state, pendingSolution[pendingSolutionIndex]);
				pendingSolutionIndex++;
			}
			applyStateToCube(cube, state);
		}
	}

	ImGui::End();
}

void rotateX (Coord3d &v, float angle) {
	float cosA = std::cos(angle);
	float sinA = std::sin(angle);
	float previousY = v.y;
	float previousZ = v.z;

	v.y = previousY * cosA - previousZ * sinA;
	v.z = previousY * sinA + previousZ * cosA;
}

void rotateSectionX (CubeSection &cube, float angle) {
	rotateX(cube.v0, angle);
	rotateX(cube.v1, angle);
	rotateX(cube.v2, angle);
	rotateX(cube.v3, angle);
	rotateX(cube.v4, angle);
	rotateX(cube.v5, angle);
	rotateX(cube.v6, angle);
	rotateX(cube.v7, angle);
}

void rotateY (Coord3d &v, float angle) {
	float cosA = std::cos(angle);
	float sinA = std::sin(angle);
	float previousX = v.x;
	float previousZ = v.z;
	v.x = previousX * cosA - previousZ * sinA;
	v.z = previousX * sinA + previousZ * cosA;
}

void rotateSectionY (CubeSection &cube, float angle) {
	rotateY(cube.v0, angle);
	rotateY(cube.v1, angle);
	rotateY(cube.v2, angle);
	rotateY(cube.v3, angle);
	rotateY(cube.v4, angle);
	rotateY(cube.v5, angle);
	rotateY(cube.v6, angle);
	rotateY(cube.v7, angle);
}

void rotateZ (Coord3d &v, float angle) {
	float cosA = std::cos(angle);
	float sinA = std::sin(angle);
	float previousX = v.x;
	float previousY = v.y;
	v.x = previousX * cosA + previousY * sinA;
	v.y = -previousX * sinA + previousY * cosA;
}

void rotateSectionZ (CubeSection &cube, float angle) {
	rotateZ(cube.v0, angle);
	rotateZ(cube.v1, angle);
	rotateZ(cube.v2, angle);
	rotateZ(cube.v3, angle);
	rotateZ(cube.v4, angle);
	rotateZ(cube.v5, angle);
	rotateZ(cube.v6, angle);
	rotateZ(cube.v7, angle);
}

// rotaciona as cores internas de uma peça segundo o movimento L (horário)
CubeSection rotateColorsL (const CubeSection &piece) {
	CubeSection p = piece;
	p.front  = piece.top;   
	p.bottom = piece.front; 
	p.back   = piece.bottom;
	p.top    = piece.back;  
	return p;
}

// rotaciona as cores internas de uma peça segundo o movimento L' (anti-horário)
CubeSection rotateColorsLPrime (const CubeSection &piece) {
	CubeSection p = piece;
	p.back   = piece.top;   
	p.bottom = piece.back;  
	p.front  = piece.bottom;
	p.top    = piece.front; 
	return p;
}

// R é o inverso de L no eixo X
CubeSection rotateColorsR (const CubeSection &piece) { return rotateColorsLPrime(piece); }
CubeSection rotateColorsRPrime (const CubeSection &piece) { return rotateColorsL(piece); }

// rotaciona as cores internas segundo o movimento U (horário)
CubeSection rotateColorsU (const CubeSection &piece) {
	CubeSection p = piece;
	p.right = piece.back; 
	p.front = piece.right;
	p.left  = piece.front;
	p.back  = piece.left; 
	return p;
}

// rotaciona as cores internas segundo o movimento U' (anti-horário)
CubeSection rotateColorsUPrime (const CubeSection &piece) {
	CubeSection p = piece;
	p.left  = piece.back; 
	p.front = piece.left; 
	p.right = piece.front;
	p.back  = piece.right;
	return p;
}

// D é o inverso de U no eixo Y
CubeSection rotateColorsD (const CubeSection &piece) { return rotateColorsUPrime(piece); }
CubeSection rotateColorsDPrime (const CubeSection &piece) { return rotateColorsU(piece); }

// rotaciona as cores internas segundo o movimento F (horário)
CubeSection rotateColorsF (const CubeSection &piece) {
	CubeSection p = piece;
	p.right  = piece.top;   
	p.bottom = piece.right; 
	p.left   = piece.bottom;
	p.top    = piece.left;  
	return p;
}

// rotaciona as cores internas segundo o movimento F' (anti-horário)
CubeSection rotateColorsFPrime (const CubeSection &piece) {
	CubeSection p = piece;
	p.left   = piece.top;   
	p.bottom = piece.left;  
	p.right  = piece.bottom;
	p.top    = piece.right; 
	return p;
}

// B é o inverso de F no eixo Z
CubeSection rotateColorsB (const CubeSection &piece) { return rotateColorsFPrime(piece); }
CubeSection rotateColorsBPrime (const CubeSection &piece) { return rotateColorsF(piece); }

// restaura os vértices canônicos (sem distorção) do slot correspondente
void resetSlotVertices (CubeSection &sec, int slotIndex) {
	Coord3d origin = {0.0f, 0.0f, 0.0f};
	auto centers = calcCubeVertex(origin, N);
	auto v = calcCubeVertex(centers[slotIndex], N);
	sec.v0 = v[0];
	sec.v1 = v[1];
	sec.v2 = v[2];
	sec.v3 = v[3];
	sec.v4 = v[4];
	sec.v5 = v[5];
	sec.v6 = v[6];
	sec.v7 = v[7];
}

void moveRubiks (
	std::array<CubeSection*, 4> uSection,
	std::array<CubeSection*, 4> dSection,
	std::array<CubeSection*, 4> rSection,
	std::array<CubeSection*, 4> lSection,
	std::array<CubeSection*, 4> fSection,
	std::array<CubeSection*, 4> bSection
	) {

	float speed = 0.05f;

	if (L != 0) {
		float step = speed * L;

		for (auto* cube:lSection) {
			rotateSectionX(*cube, step);
		}

		amountOfRotation += std::abs(step);

		if (amountOfRotation >= (PI/2.0f)) {
			if (L == 1) {
				CubeSection old0 = *lSection[0];
				CubeSection old2 = *lSection[1];
				CubeSection old6 = *lSection[2];
				CubeSection old4 = *lSection[3];

				*lSection[0] = rotateColorsL(old4);
				*lSection[3] = rotateColorsL(old6);
				*lSection[2] = rotateColorsL(old2);
				*lSection[1] = rotateColorsL(old0);
			} else if (L == -1) {
				CubeSection old0 = *lSection[0];
				CubeSection old2 = *lSection[1];
				CubeSection old6 = *lSection[2];
				CubeSection old4 = *lSection[3];

				*lSection[0] = rotateColorsLPrime(old2);
				*lSection[1] = rotateColorsLPrime(old6);
				*lSection[2] = rotateColorsLPrime(old4);
				*lSection[3] = rotateColorsLPrime(old0);
			}

			resetSlotVertices(*lSection[0], 0);
			resetSlotVertices(*lSection[1], 2);
			resetSlotVertices(*lSection[2], 6);
			resetSlotVertices(*lSection[3], 4);

			L = 0;
			amountOfRotation = 0.0f;
		}
	} else if (R != 0) {
		float step = speed * R;
		for (auto* cube:rSection) {
			rotateSectionX(*cube, -step);
		}
		amountOfRotation += std::abs(step);
		if (amountOfRotation >= (PI/2.0f)) {
			if (R == 1) {
				CubeSection old1 = *rSection[0];
				CubeSection old3 = *rSection[1];
				CubeSection old7 = *rSection[2];
				CubeSection old5 = *rSection[3];

				*rSection[3] = rotateColorsR(old1);
				*rSection[2] = rotateColorsR(old5);
				*rSection[1] = rotateColorsR(old7);
				*rSection[0] = rotateColorsR(old3);
			} else if (R == -1) {
				CubeSection old1 = *rSection[0];
				CubeSection old3 = *rSection[1];
				CubeSection old7 = *rSection[2];
				CubeSection old5 = *rSection[3];

				*rSection[1] = rotateColorsRPrime(old1);
				*rSection[2] = rotateColorsRPrime(old3);
				*rSection[3] = rotateColorsRPrime(old7);
				*rSection[0] = rotateColorsRPrime(old5);
			}
			resetSlotVertices(*rSection[0], 1);
			resetSlotVertices(*rSection[1], 3);
			resetSlotVertices(*rSection[2], 7);
			resetSlotVertices(*rSection[3], 5);
			R = 0;
			amountOfRotation = 0.0f;
		}
	} else if (U != 0) {
		float step = speed * U;
		for (auto* cube:uSection) {
			rotateSectionY(*cube, step);
		}
		amountOfRotation += std::abs(step);
		if (amountOfRotation >= (PI/2.0f)) {
			if (U == 1) {
				CubeSection old0 = *uSection[0];
				CubeSection old2 = *uSection[1];
				CubeSection old3 = *uSection[2];
				CubeSection old1 = *uSection[3];

				*uSection[3] = rotateColorsU(old0);
				*uSection[2] = rotateColorsU(old1);
				*uSection[1] = rotateColorsU(old3);
				*uSection[0] = rotateColorsU(old2);
			} else if (U == -1) {
				CubeSection old0 = *uSection[0];
				CubeSection old2 = *uSection[1];
				CubeSection old3 = *uSection[2];
				CubeSection old1 = *uSection[3];

				*uSection[1] = rotateColorsUPrime(old0);
				*uSection[2] = rotateColorsUPrime(old2);
				*uSection[3] = rotateColorsUPrime(old3);
				*uSection[0] = rotateColorsUPrime(old1);
			}
			resetSlotVertices(*uSection[0], 0);
			resetSlotVertices(*uSection[1], 2);
			resetSlotVertices(*uSection[2], 3);
			resetSlotVertices(*uSection[3], 1);
			U = 0;
			amountOfRotation = 0.0f;
		}
	} else if (D != 0) {
		float step = speed * D;
		for (auto* cube:dSection) {
			rotateSectionY(*cube, -step);
		}
		amountOfRotation += std::abs(step);
		if (amountOfRotation >= (PI/2.0f)) {
			if (D == 1) {
				CubeSection old4 = *dSection[0];
				CubeSection old6 = *dSection[1];
				CubeSection old7 = *dSection[2];
				CubeSection old5 = *dSection[3];

				*dSection[2] = rotateColorsD(old6);
				*dSection[3] = rotateColorsD(old7);
				*dSection[0] = rotateColorsD(old5);
				*dSection[1] = rotateColorsD(old4);
			} else if (D == -1) {
				CubeSection old4 = *dSection[0];
				CubeSection old6 = *dSection[1];
				CubeSection old7 = *dSection[2];
				CubeSection old5 = *dSection[3];

				*dSection[0] = rotateColorsDPrime(old6);
				*dSection[3] = rotateColorsDPrime(old4);
				*dSection[2] = rotateColorsDPrime(old5);
				*dSection[1] = rotateColorsDPrime(old7);
			}
			resetSlotVertices(*dSection[0], 4);
			resetSlotVertices(*dSection[1], 6);
			resetSlotVertices(*dSection[2], 7);
			resetSlotVertices(*dSection[3], 5);
			D = 0;
			amountOfRotation = 0.0f;
		}
	} else if (F != 0) {
		float step = speed * F;
		for (auto* cube:fSection) {
			rotateSectionZ(*cube, step);
		}
		amountOfRotation += std::abs(step);
		if (amountOfRotation >= (PI/2.0f)) {
			if (F == 1) {
				CubeSection old3 = *fSection[0];
				CubeSection old2 = *fSection[1];
				CubeSection old6 = *fSection[2];
				CubeSection old7 = *fSection[3];

				*fSection[0] = rotateColorsF(old2);
				*fSection[3] = rotateColorsF(old3);
				*fSection[2] = rotateColorsF(old7);
				*fSection[1] = rotateColorsF(old6);
			} else if (F == -1) {
				CubeSection old3 = *fSection[0];
				CubeSection old2 = *fSection[1];
				CubeSection old6 = *fSection[2];
				CubeSection old7 = *fSection[3];

				*fSection[2] = rotateColorsFPrime(old2);
				*fSection[3] = rotateColorsFPrime(old6);
				*fSection[0] = rotateColorsFPrime(old7);
				*fSection[1] = rotateColorsFPrime(old3);
			}
			resetSlotVertices(*fSection[0], 3);
			resetSlotVertices(*fSection[1], 2);
			resetSlotVertices(*fSection[2], 6);
			resetSlotVertices(*fSection[3], 7);
			F = 0;
			amountOfRotation = 0.0f;
		}
	} else if (B != 0) {
		float step = speed * B;
		for (auto* cube:bSection) {
			rotateSectionZ(*cube, -step);
		}
		amountOfRotation += std::abs(step);
		if (amountOfRotation >= (PI/2.0f)) {
			if (B == 1) {
				CubeSection old1 = *bSection[0];
				CubeSection old0 = *bSection[1];
				CubeSection old4 = *bSection[2];
				CubeSection old5 = *bSection[3];

				*bSection[2] = rotateColorsB(old0);
				*bSection[3] = rotateColorsB(old4);
				*bSection[0] = rotateColorsB(old5);
				*bSection[1] = rotateColorsB(old1);
			} else if (B == -1) {
				CubeSection old1 = *bSection[0];
				CubeSection old0 = *bSection[1];
				CubeSection old4 = *bSection[2];
				CubeSection old5 = *bSection[3];

				*bSection[0] = rotateColorsBPrime(old0);
				*bSection[3] = rotateColorsBPrime(old1);
				*bSection[2] = rotateColorsBPrime(old5);
				*bSection[1] = rotateColorsBPrime(old4);
			}
			resetSlotVertices(*bSection[0], 1);
			resetSlotVertices(*bSection[1], 0);
			resetSlotVertices(*bSection[2], 4);
			resetSlotVertices(*bSection[3], 5);
			B = 0;
			amountOfRotation = 0.0f;
		}
	}
}

int main() {
	CamState cam;
	Coord3d cubeOrigem;
	std::array<CubeSection, 8> cube;
	
	std::array<CubeSection*, 4> uSection = {&cube[0], &cube[2], &cube[3], &cube[1]};
	std::array<CubeSection*, 4> dSection = {&cube[4], &cube[6], &cube[7], &cube[5]};

	std::array<CubeSection*, 4> rSection = {&cube[1], &cube[3], &cube[7], &cube[5]};
	std::array<CubeSection*, 4> lSection = {&cube[0], &cube[2], &cube[6], &cube[4]};

	std::array<CubeSection*, 4> fSection = {&cube[3], &cube[2], &cube[6], &cube[7]};
	std::array<CubeSection*, 4> bSection = {&cube[1], &cube[0], &cube[4], &cube[5]};

	CubeState currentState = makeSolvedState();
	SearchStats lastStats;
	lastStats.message = "Pronto para jogar ou resolver.";
	std::vector<std::string> scrambleMoves;
	std::vector<std::string> pendingSolution;
	int pendingSolutionIndex = 0;
	int seed = 2026;
	int scrambleLength = 4;
	int depthLimit = 10;
	int maxVisited = 500000;

	// cores
	Color red = {1.0f, 0.0f, 0.0f};
	Color green = {0.0f, 1.0f, 0.0f};
	Color blue = {0.0f, 0.0f, 1.0f};
	Color white = {0.8f, 0.8f, 0.8f};
	Color yellow = {1.0f, 1.0f, 0.0f};
	Color orange = {1.0f, 0.5f, 0.0f};
	Color gray = {0.15f, 0.15f, 0.15f};

	// inicializando e montando o cubo

	// isso aqui me dá o centroide de cada um dos pequenos cubos
	auto sectionsCenter = calcCubeVertex(cubeOrigem, N);
	auto cubeVertexes = calcCubeVertex(sectionsCenter[0], N);

	cube[0] = {
		cubeVertexes[0],
		cubeVertexes[1],
		cubeVertexes[2],
		cubeVertexes[3],
		cubeVertexes[4],
		cubeVertexes[5],
		cubeVertexes[6],
		cubeVertexes[7],
		orange, gray, white, gray, gray, blue};

	cubeVertexes = calcCubeVertex(sectionsCenter[1], N);
	
	cube[1] = {
		cubeVertexes[0],
		cubeVertexes[1],
		cubeVertexes[2],
		cubeVertexes[3],
		cubeVertexes[4],
		cubeVertexes[5],
		cubeVertexes[6],
		cubeVertexes[7],
		gray, red, white, gray, gray, blue};

	cubeVertexes = calcCubeVertex(sectionsCenter[2], N);
	
	cube[2] = {
		cubeVertexes[0],
		cubeVertexes[1],
		cubeVertexes[2],
		cubeVertexes[3],
		cubeVertexes[4],
		cubeVertexes[5],
		cubeVertexes[6],
		cubeVertexes[7],
		orange, gray, white, gray, green, gray};

	cubeVertexes = calcCubeVertex(sectionsCenter[3], N);
	
	cube[3] = {
		cubeVertexes[0],
		cubeVertexes[1],
		cubeVertexes[2],
		cubeVertexes[3],
		cubeVertexes[4],
		cubeVertexes[5],
		cubeVertexes[6],
		cubeVertexes[7],
		gray, red, white, gray, green, gray};

	cubeVertexes = calcCubeVertex(sectionsCenter[4], N);

	cube[4] = {
		cubeVertexes[0],
		cubeVertexes[1],
		cubeVertexes[2],
		cubeVertexes[3],
		cubeVertexes[4],
		cubeVertexes[5],
		cubeVertexes[6],
		cubeVertexes[7],
		orange, gray, gray, yellow, gray, blue};

	cubeVertexes = calcCubeVertex(sectionsCenter[5], N);

	cube[5] = {
		cubeVertexes[0],
		cubeVertexes[1],
		cubeVertexes[2],
		cubeVertexes[3],
		cubeVertexes[4],
		cubeVertexes[5],
		cubeVertexes[6],
		cubeVertexes[7],
		gray, red, gray, yellow, gray, blue};

	cubeVertexes = calcCubeVertex(sectionsCenter[6], N);

	cube[6] = {
		cubeVertexes[0],
		cubeVertexes[1],
		cubeVertexes[2],
		cubeVertexes[3],
		cubeVertexes[4],
		cubeVertexes[5],
		cubeVertexes[6],
		cubeVertexes[7],
		orange, gray, gray, yellow, green, gray};

	cubeVertexes = calcCubeVertex(sectionsCenter[7], N);

	cube[7] = {
		cubeVertexes[0],
		cubeVertexes[1],
		cubeVertexes[2],
		cubeVertexes[3],
		cubeVertexes[4],
		cubeVertexes[5],
		cubeVertexes[6],
		cubeVertexes[7],
		gray, red, gray, yellow, green, gray};

	applyStateToCube(cube, currentState);

	if (!glfwInit()) {
		std::cerr << "Falha ao inicializar o GLFW" << std::endl;
		return -1;
	}

	// setando o GLFW (biblioteca gráfica)
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);

	// criando a instância da janela e os callbacks
	GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGTH, "Cubo Mágico", NULL, NULL);

	if (!window) {
		std::cerr << "Falha ao inicializar a janela" << std::endl;
		glfwTerminate();
		return -1;
	}

	glfwMakeContextCurrent(window);
	glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);
	glfwSetCursorPosCallback(window, cursosPosCallback);
	glfwSetMouseButtonCallback(window, mouseButtonCallback);
	glfwSetScrollCallback(window, scrollCallback);

	glEnable(GL_DEPTH_TEST);

	// faz com que os valores da cam sejam atualizados juntos dos eventuais callbacks
	glfwSetWindowUserPointer(window, &cam);

	// inicializando o ImGui
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGui::StyleColorsDark();
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init("#version 120");

	// loop de renderização
	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();

		// mexer com os inputs
		processInput(window);

		// cria o frame onde o GUI vai ficar
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		// limpa o buffer do open gl
		glClearColor(0.12f, 0.12f, 0.12f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// cam
		glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        glFrustum(-1.0, 1.0, -0.75, 0.75, 1.5, 20.0);

        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
        glTranslatef(0.0f, 0.0f, cam.zoom);
        glRotatef(cam.rotY, 1.0f, 0.0f, 0.0f);
        glRotatef(cam.rotX, 0.0f, 1.0f, 0.0f);

		// renderizações 3D
		drawCartesianPlan();

		drawRubiks(cube);

		drawCartesianPlanLabels();
		drawInfosGUI(uSection, dSection, rSection, lSection, fSection, bSection);
		drawAiControlGUI(
			currentState,
			cube,
			seed,
			scrambleLength,
			depthLimit,
			maxVisited,
			scrambleMoves,
			lastStats,
			pendingSolution,
			pendingSolutionIndex
		);

		// renderizações label & hud

		// exibe o frame
		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
		glfwSwapBuffers(window);
	}

	//  finaliza tudo
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	glfwDestroyWindow(window);
	glfwTerminate();
	return 0;
}
