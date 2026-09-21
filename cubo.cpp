#include <GLFW/glfw3.h>
#include <iostream>
#include <array>
#include <functional>
#include <cmath>
#include <utility>

#include <queue>
#include <cstdlib>
#include <ctime>
#include <string>
#include <sstream>
#include <thread>
#include <atomic>

#include "external/imgui/imgui.h"
#include "external/imgui/backends/imgui_impl_glfw.h"
#include "external/imgui/backends/imgui_impl_opengl3.h"

#include "busca_largura.h"
#include "estado.h"

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

// fila de movimentos para o embaralhamento
struct Move { int face; int dir; }; // face: 0=L,1=R,2=U,3=D,4=F,5=B  dir: 1 ou -1
std::queue<Move> shuffleQueue;
int shuffleMoveCount = 10;   // quantidade de movimentos escolhida pelo usuário
bool isShuffling = false;
float rotationSpeed = 0.05f;  // velocidade de rotação (rad/frame)
bool resetRequested = false;  // sinaliza que o cubo deve ser reinicializado

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

// ===================== ESTADO SALVO & SOLVER =====================
// Salva o estado visual (cores) do cubo para uso pelos algoritmos
struct CubeColorState {
    Color left, right, top, bottom, front, back;
};
struct CubeColorState8 {
    std::array<CubeColorState, 8> pieces;
    bool valid = false;
};

CubeColorState8 savedCubeState;  // estado salvo pelo usuário

// Fila de movimentos para a solução animada
std::queue<Move> solutionQueue;
bool isPlayingSolution = false;

// Resultado da busca em largura
struct SolverResult {
    bool ran          = false;
    bool found        = false;
    int  nodesVisited = 0;
    int  moveCount    = 0;
    std::string path;
};
SolverResult solverResult;
std::atomic<bool> solverRunning{false};
// =================================================================

// Converte Color (OpenGL float) → Cor (enum lógico)
static Cor colorToCor(const Color& c) {
    float r = c.r, g = c.g, b = c.b;
    if (r < 0.2f && g < 0.2f && b < 0.2f) return Cor::CINZA;
    if (r > 0.5f && g > 0.5f && b > 0.5f) return Cor::BRANCO;
    if (r > 0.5f && g > 0.5f && b < 0.3f) return Cor::AMARELO;
    if (r < 0.3f && g > 0.5f && b < 0.3f) return Cor::VERDE;
    if (r < 0.3f && g < 0.3f && b > 0.5f) return Cor::AZUL;
    if (r > 0.5f && g < 0.3f && b < 0.3f) return Cor::VERMELHO;
    if (r > 0.5f && g > 0.2f && g < 0.7f && b < 0.2f) return Cor::LARANJA;
    return Cor::CINZA;
}

// Converte o array de CubeSection (visual) → EstadoCubo (lógico para IA)
static EstadoCubo cubeToEstado(const std::array<CubeSection, 8>& cube) {
    EstadoCubo estado;
    for (int i = 0; i < 8; i++) {
        estado.pecas[i].left   = colorToCor(cube[i].left);
        estado.pecas[i].right  = colorToCor(cube[i].right);
        estado.pecas[i].top    = colorToCor(cube[i].top);
        estado.pecas[i].bottom = colorToCor(cube[i].bottom);
        estado.pecas[i].front  = colorToCor(cube[i].front);
        estado.pecas[i].back   = colorToCor(cube[i].back);
    }
    return estado;
}

// Converte Movimento → Move (para enfileirar na fila de animação)
static Move movimentoToMove(Movimento m) {
    switch (m) {
        case Movimento::L:       return {0,  1};
        case Movimento::L_PRIME: return {0, -1};
        case Movimento::R:       return {1,  1};
        case Movimento::R_PRIME: return {1, -1};
        case Movimento::U:       return {2,  1};
        case Movimento::U_PRIME: return {2, -1};
        case Movimento::D:       return {3,  1};
        case Movimento::D_PRIME: return {3, -1};
        case Movimento::F:       return {4,  1};
        case Movimento::F_PRIME: return {4, -1};
        case Movimento::B:       return {5,  1};
        case Movimento::B_PRIME: return {5, -1};
        default:                 return {0,  0};
    }
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
        if (glfwGetKey(window, GLFW_KEY_UP)) {
			L = 1;
        } else if (glfwGetKey(window, GLFW_KEY_DOWN)) {
			L = -1;
        }
    } else if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS) {
        if (glfwGetKey(window, GLFW_KEY_UP)) {
			R = 1;
        } else if (glfwGetKey(window, GLFW_KEY_DOWN)) {
			R = -1;
        }
    } else if (glfwGetKey(window, GLFW_KEY_U) == GLFW_PRESS) {
        if (glfwGetKey(window, GLFW_KEY_LEFT)) {
			U = 1;
        } else if (glfwGetKey(window, GLFW_KEY_RIGHT)) {
			U = -1;
        }
    } else if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
        if (glfwGetKey(window, GLFW_KEY_RIGHT)) {
			D = 1;
        } else if (glfwGetKey(window, GLFW_KEY_LEFT)) {
			D = -1;
        }
    } else if (glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS) {
        if (glfwGetKey(window, GLFW_KEY_RIGHT)) {
			F = 1;
        } else if (glfwGetKey(window, GLFW_KEY_LEFT)) {
			F = -1;
        }
    } else if (glfwGetKey(window, GLFW_KEY_B) == GLFW_PRESS) {
        if (glfwGetKey(window, GLFW_KEY_LEFT)) {
			B = 1;
        } else if (glfwGetKey(window, GLFW_KEY_RIGHT)) {
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

	ImGui::Text("Para mover R & L: L + seta UP | DOWN; R + seta UP | DOWN");
	ImGui::Text("Para mover U & D: U + seta RIGHT | LEFT; D + seta RIGHT | LEFT");
	ImGui::Text("Para mover F & B: F + seta RIGHT | LEFT; B + seta RIGHT | LEFT");

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


// GERAR EMBARALHAMENTO
// gera N movimentos aleatórios e os coloca na fila de embaralhamento
void shuffleRubiks(int numMoves) {
	// limpa qualquer embaralhamento anterior
	while (!shuffleQueue.empty()) shuffleQueue.pop();

	int lastFace = -1; // evita repetir a mesma face consecutivamente
	for (int i = 0; i < numMoves; ++i) {
		int face;
		do {
			face = std::rand() % 6; // 0=L,1=R,2=U,3=D,4=F,5=B
		} while (face == lastFace);
		lastFace = face;

		int dir = (std::rand() % 2 == 0) ? 1 : -1;
		shuffleQueue.push({face, dir});
	}
	isShuffling = true;
}

// consome o próximo movimento da fila (chamado quando nenhum movimento está ativo)
void dispatchNextShuffleMove() {
	bool isBusy = (L != 0 || R != 0 || U != 0 || D != 0 || F != 0 || B != 0);
	if (isBusy) return; // ainda animando, espera terminar

	// Prioridade: solução > embaralhamento
	if (isPlayingSolution) {
		if (solutionQueue.empty()) {
			isPlayingSolution = false;
			return;
		}
		Move m = solutionQueue.front();
		solutionQueue.pop();
		switch (m.face) {
			case 0: L = m.dir; break;
			case 1: R = m.dir; break;
			case 2: U = m.dir; break;
			case 3: D = m.dir; break;
			case 4: F = m.dir; break;
			case 5: B = m.dir; break;
		}
		return;
	}

	if (!isShuffling) return;

	// só finaliza quando a fila está vazia E nenhum movimento está rodando
	if (shuffleQueue.empty()) {
		isShuffling = false;
		return;
	}

	Move m = shuffleQueue.front();
	shuffleQueue.pop();

	switch (m.face) {
		case 0: L = m.dir; break;
		case 1: R = m.dir; break;
		case 2: U = m.dir; break;
		case 3: D = m.dir; break;
		case 4: F = m.dir; break;
		case 5: B = m.dir; break;
	}
}

void drawShuffleGUI() {
	// ancora no canto inferior esquerdo
	ImVec2 displaySize = ImGui::GetIO().DisplaySize;
	ImGui::SetNextWindowPos(ImVec2(20, displaySize.y - 20), ImGuiCond_Always, ImVec2(0.0f, 1.0f));
	ImGui::SetNextWindowSize(ImVec2(300, 0), ImGuiCond_Always);
	ImGui::Begin("Embaralhar", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar);

	ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.3f, 1.0f), "=== Embaralhamento ===");
	ImGui::Separator();
	ImGui::SliderInt("Movimentos", &shuffleMoveCount, 1, 50);
	ImGui::SliderFloat("Velocidade", &rotationSpeed, 0.005f, 0.10f, "%.3f rad/f");

	if (isShuffling) {
		ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.0f, 1.0f),
			"Embaralhando... (%d restantes)", (int)shuffleQueue.size() + (L||R||U||D||F||B ? 1 : 0));
	} else {
		if (ImGui::Button("Embaralhar!", ImVec2(-1, 30))) {
			shuffleRubiks(shuffleMoveCount);
		}
		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();
		ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.6f, 0.1f, 0.1f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.8f, 0.2f, 0.2f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(1.0f, 0.3f, 0.3f, 1.0f));
		if (ImGui::Button("Resetar Cubo", ImVec2(-1, 30))) {
			resetRequested = true;
		}
		ImGui::PopStyleColor(3);
	}
	ImGui::End();
}
// FIM DE GERAR EMBARALHAMENTO


// ===================== PAINEL DO SOLVER (LADO DIREITO) =====================
// Referência ao cubo para os botões que precisam do estado visual
static std::array<CubeSection, 8>* g_cubeRef = nullptr;
static Coord3d*                     g_cubeOrigemRef = nullptr;

// Nome do movimento com 2 letras, baseado nas teclas de seta dos controles:
// L+Up=Lu, L+Down=Ld, R+Up=Ru, R+Down=Rd
// U+Left=Ul, U+Right=Ur, D+Right=Dr, D+Left=Dl
// F+Right=Fr, F+Left=Fl, B+Left=Bl, B+Right=Br
static const char* nomeMovimentoTecla(Movimento m) {
    switch (m) {
        case Movimento::L:       return "Lu";
        case Movimento::L_PRIME: return "Ld";
        case Movimento::R:       return "Ru";
        case Movimento::R_PRIME: return "Rd";
        case Movimento::U:       return "Ul";
        case Movimento::U_PRIME: return "Ur";
        case Movimento::D:       return "Dr";
        case Movimento::D_PRIME: return "Dl";
        case Movimento::F:       return "Fr";
        case Movimento::F_PRIME: return "Fl";
        case Movimento::B:       return "Bl";
        case Movimento::B_PRIME: return "Br";
        default:                 return "??";
    }
}

void drawSolverGUI() {
	ImVec2 displaySize = ImGui::GetIO().DisplaySize;
	// Painel fixo no lado esquerdo, centralizado verticalmente
	ImGui::SetNextWindowPos(ImVec2(displaySize.x - 320, 20), ImGuiCond_Always);
	ImGui::SetNextWindowSize(ImVec2(300, 0), ImGuiCond_Always);
	ImGui::Begin("Solver", nullptr,
		ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_AlwaysAutoResize |
		ImGuiWindowFlags_NoTitleBar);

	ImGui::TextColored(ImVec4(0.4f, 0.9f, 1.0f, 1.0f), "=== Algoritmos de Busca ===");
	ImGui::Separator();
	ImGui::Spacing();

	// --- Botão: Salvar Estado Atual ---
	bool isBusy = isShuffling || isPlayingSolution || solverRunning.load();

	ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.1f, 0.5f, 0.1f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.2f, 0.7f, 0.2f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(0.3f, 0.9f, 0.3f, 1.0f));
	if (!isBusy && ImGui::Button("Salvar estado atual", ImVec2(-1, 30))) {
		// Salva as cores atuais de cada peça
		if (g_cubeRef) {
			for (int i = 0; i < 8; i++) {
				savedCubeState.pieces[i] = {
					(*g_cubeRef)[i].left,
					(*g_cubeRef)[i].right,
					(*g_cubeRef)[i].top,
					(*g_cubeRef)[i].bottom,
					(*g_cubeRef)[i].front,
					(*g_cubeRef)[i].back
				};
			}
			savedCubeState.valid = true;
			// limpa resultados anteriores ao salvar novo estado
			solverResult = SolverResult{};
		}
	}
	ImGui::PopStyleColor(3);

	if (savedCubeState.valid) {
		ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.3f, 1.0f), "Estado salvo!");
	} else {
		ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Nenhum estado salvo.");
	}

	ImGui::Spacing();
	ImGui::Separator();
	ImGui::Spacing();

	// --- Botão: Voltar para estado salvo ---
	ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.5f, 0.3f, 0.0f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.7f, 0.45f, 0.0f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(1.0f, 0.65f, 0.0f, 1.0f));
	bool canRestore = savedCubeState.valid && !isBusy;
	if (!canRestore) ImGui::BeginDisabled();
	if (ImGui::Button("Voltar para estado salvo", ImVec2(-1, 30)) && canRestore) {
		if (g_cubeRef) {
			// Para qualquer movimento em andamento
			L = R = U = D = F = B = 0;
			amountOfRotation = 0.0f;
			while (!shuffleQueue.empty()) shuffleQueue.pop();
			while (!solutionQueue.empty()) solutionQueue.pop();
			isShuffling = false;
			isPlayingSolution = false;
			// Restaura geometria (vértices canônicos) e cores salvas
			Coord3d origin = {0.0f, 0.0f, 0.0f};
			auto centers = calcCubeVertex(origin, N);
			for (int i = 0; i < 8; i++) {
				auto v = calcCubeVertex(centers[i], N);
				(*g_cubeRef)[i].v0 = v[0]; (*g_cubeRef)[i].v1 = v[1];
				(*g_cubeRef)[i].v2 = v[2]; (*g_cubeRef)[i].v3 = v[3];
				(*g_cubeRef)[i].v4 = v[4]; (*g_cubeRef)[i].v5 = v[5];
				(*g_cubeRef)[i].v6 = v[6]; (*g_cubeRef)[i].v7 = v[7];
				(*g_cubeRef)[i].left   = savedCubeState.pieces[i].left;
				(*g_cubeRef)[i].right  = savedCubeState.pieces[i].right;
				(*g_cubeRef)[i].top    = savedCubeState.pieces[i].top;
				(*g_cubeRef)[i].bottom = savedCubeState.pieces[i].bottom;
				(*g_cubeRef)[i].front  = savedCubeState.pieces[i].front;
				(*g_cubeRef)[i].back   = savedCubeState.pieces[i].back;
			}
			// limpa resultado anterior
			solverResult = SolverResult{};
		}
	}
	if (!canRestore) ImGui::EndDisabled();
	ImGui::PopStyleColor(3);

	ImGui::Spacing();
	ImGui::Separator();
	ImGui::Spacing();

	// --- Botão: Resolver (BFS) ---
	ImGui::TextColored(ImVec4(0.4f, 0.9f, 1.0f, 1.0f), "Busca em Largura (BFS)");
	ImGui::Spacing();

	bool canSolve = savedCubeState.valid && !isBusy;
	ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.1f, 0.2f, 0.6f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.2f, 0.35f, 0.85f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(0.35f, 0.55f, 1.0f, 1.0f));
	if (!canSolve) ImGui::BeginDisabled();
	if (ImGui::Button("Resolver", ImVec2(-1, 30)) && canSolve) {
		if (g_cubeRef && !solverRunning.load()) {
			// Captura estado lógico do estado salvo
			// (recria um CubeSection temporário com as cores salvas para converter)
			std::array<CubeSection, 8> tempCube = *g_cubeRef;
			for (int i = 0; i < 8; i++) {
				tempCube[i].left   = savedCubeState.pieces[i].left;
				tempCube[i].right  = savedCubeState.pieces[i].right;
				tempCube[i].top    = savedCubeState.pieces[i].top;
				tempCube[i].bottom = savedCubeState.pieces[i].bottom;
				tempCube[i].front  = savedCubeState.pieces[i].front;
				tempCube[i].back   = savedCubeState.pieces[i].back;
			}
			EstadoCubo estadoInicial = cubeToEstado(tempCube);
			solverResult = SolverResult{};
			solverResult.ran = false;
			solverRunning.store(true);
			// Executa a BFS em uma thread separada para não travar a UI
			std::thread([estadoInicial]() {
				ResultadoBusca res = buscaEmLargura(estadoInicial);
				solverResult.found        = res.encontrou;
				solverResult.nodesVisited = (int)res.estadosVisitados;
				solverResult.moveCount    = (int)res.caminho.size();
				// Monta string do caminho com notação de 2 letras (baseado nas teclas)
				std::ostringstream oss;
				for (size_t i = 0; i < res.caminho.size(); i++) {
					if (i > 0) oss << " ";
					oss << nomeMovimentoTecla(res.caminho[i]);
				}
				solverResult.path         = oss.str();
				solverResult.ran          = true;
				// Nao anima: o usuario testa manualmente seguindo o caminho
				solverRunning.store(false);
			}).detach();
		}
	}
	if (!canSolve) ImGui::EndDisabled();
	ImGui::PopStyleColor(3);

	// --- Exibição do resultado ---
	ImGui::Spacing();
	ImGui::Separator();
	ImGui::Spacing();

	if (solverRunning.load()) {
		ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.0f, 1.0f), "Buscando solucao...");
	} else if (solverResult.ran) {
		if (solverResult.found) {
			ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.3f, 1.0f), "Solucao encontrada!");
			ImGui::Text("Nos visitados: %d", solverResult.nodesVisited);
			ImGui::Text("Movimentos: %d", solverResult.moveCount);
			ImGui::Spacing();
			ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.3f, 1.0f), "Caminho:");
			// Quebra o caminho em linhas de ~30 chars para não ultrapassar o painel
			const std::string& p = solverResult.path;
			const int lineLen = 35;
			for (size_t pos = 0; pos < p.size(); pos += lineLen) {
				ImGui::TextUnformatted(p.substr(pos, lineLen).c_str());
			}
			ImGui::Spacing();
			ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "(Ex: Lu = L+seta cima)");
		} else {
			ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "Sem solucao encontrada.");
			ImGui::Text("Nos visitados: %d", solverResult.nodesVisited);
		}
	} else {
		ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Aguardando...");
	}

	ImGui::End();
}
// ============================================================================


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

	float speed = rotationSpeed;

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

// inicializa (ou reinicializa) o cubo com as cores e vértices originais
void initCube(std::array<CubeSection, 8>& cube, Coord3d cubeOrigem) {
	Color red    = {1.0f, 0.0f, 0.0f};
	Color green  = {0.0f, 1.0f, 0.0f};
	Color blue   = {0.0f, 0.0f, 1.0f};
	Color white  = {0.8f, 0.8f, 0.8f};
	Color yellow = {1.0f, 1.0f, 0.0f};
	Color orange = {1.0f, 0.5f, 0.0f};
	Color gray   = {0.15f, 0.15f, 0.15f};

	auto sectionsCenter = calcCubeVertex(cubeOrigem, N);
	auto v = calcCubeVertex(sectionsCenter[0], N);

	cube[0] = {v[0],v[1],v[2],v[3],v[4],v[5],v[6],v[7], orange, gray,   white,  gray,   gray,  blue};
	v = calcCubeVertex(sectionsCenter[1], N);
	cube[1] = {v[0],v[1],v[2],v[3],v[4],v[5],v[6],v[7], gray,   red,    white,  gray,   gray,  blue};
	v = calcCubeVertex(sectionsCenter[2], N);
	cube[2] = {v[0],v[1],v[2],v[3],v[4],v[5],v[6],v[7], orange, gray,   white,  gray,   green, gray};
	v = calcCubeVertex(sectionsCenter[3], N);
	cube[3] = {v[0],v[1],v[2],v[3],v[4],v[5],v[6],v[7], gray,   red,    white,  gray,   green, gray};
	v = calcCubeVertex(sectionsCenter[4], N);
	cube[4] = {v[0],v[1],v[2],v[3],v[4],v[5],v[6],v[7], orange, gray,   gray,   yellow, gray,  blue};
	v = calcCubeVertex(sectionsCenter[5], N);
	cube[5] = {v[0],v[1],v[2],v[3],v[4],v[5],v[6],v[7], gray,   red,    gray,   yellow, gray,  blue};
	v = calcCubeVertex(sectionsCenter[6], N);
	cube[6] = {v[0],v[1],v[2],v[3],v[4],v[5],v[6],v[7], orange, gray,   gray,   yellow, green, gray};
	v = calcCubeVertex(sectionsCenter[7], N);
	cube[7] = {v[0],v[1],v[2],v[3],v[4],v[5],v[6],v[7], gray,   red,    gray,   yellow, green, gray};
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

	// inicializando e montando o cubo
	initCube(cube, cubeOrigem);

	// conecta a referência global para o painel do solver
	g_cubeRef = &cube;
	g_cubeOrigemRef = &cubeOrigem;

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

	// semente para geração de números aleatórios
	std::srand(static_cast<unsigned int>(std::time(nullptr)));

	// loop de renderização
	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();

		// despacha o próximo movimento do embaralhamento se houver
		dispatchNextShuffleMove();

		// reset do cubo (solicitado pelo botão da GUI)
		if (resetRequested) {
			L = R = U = D = F = B = 0;
			amountOfRotation = 0.0f;
			while (!shuffleQueue.empty()) shuffleQueue.pop();
			isShuffling = false;
			initCube(cube, cubeOrigem);
			resetRequested = false;
		}

		moveRubiks(uSection, dSection, rSection, lSection, fSection, bSection);

		// mexer com os inputs (bloqueado durante embaralhamento e solução)
		processInput(window);
		if (!isShuffling && !isPlayingSolution) rubiksInteractions(window, cube);

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
		drawShuffleGUI();
		drawSolverGUI();

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