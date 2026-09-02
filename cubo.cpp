#include <GLFW/glfw3.h>
#include <iostream>
#include <array>

#include "external/imgui/imgui.h"
#include "external/imgui/backends/imgui_impl_glfw.h"
#include "external/imgui/backends/imgui_impl_opengl3.h"

// constantes
constexpr float N = 1.0f;
constexpr unsigned int SCR_WIDTH = 800;
constexpr unsigned int SCR_HEIGTH = 600;

constexpr int L = 0;	// 0 para, -1 L', 1 L
constexpr int R = 0;	// equivalentes
constexpr int U = 0;
constexpr int D = 0;
constexpr int F = 0;
constexpr int B = 0;

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
	Coord3d center;
	Color left;
	Color right;
	Color top;
	Color bottom;
	Color front;
	Color back;
};

// callbacks
void framebufferSizeCallback(GLFWwindow* window, int width, int height) {
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
		glfwSetWindowShouldClose(window, true);
	}
}

void cursosPosCallback(GLFWwindow* window, double xPos, double yPos) {
    if (ImGui::GetIO().WantCaptureMouse) return;

    float rotationSpeed = 0.3f;
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

// IA fez -> basicamente projetado 1 ponto do plano 3d no plano 2d da tela
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

// interações com o teclado
void processInput (GLFWwindow* window) {
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
		glfwSetWindowShouldClose(window, true);
	}
}

void rubiksInteractions (GLFWwindow* window, const std::array<Coord3d, 8> &coords) {
    if (glfwGetKey(window, GLFW_KEY_L) == GLFW_PRESS) {
        if (glfwGetKey(window, GLFW_KEY_UP)) {
			std::cout << "teste";
        } else if (glfwGetKey(window, GLFW_KEY_DOWN)) {
			std::cout << "teste";
        }
    } else if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS) {
        if (glfwGetKey(window, GLFW_KEY_UP)) {
			std::cout << "teste";
        } else if (glfwGetKey(window, GLFW_KEY_DOWN)) {
			std::cout << "teste";
        }
    } else if (glfwGetKey(window, GLFW_KEY_U) == GLFW_PRESS) {
        if (glfwGetKey(window, GLFW_KEY_LEFT)) {
			std::cout << "teste";
        } else if (glfwGetKey(window, GLFW_KEY_RIGHT)) {
			std::cout << "teste";
        }
    } else if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
        if (glfwGetKey(window, GLFW_KEY_RIGHT)) {
			std::cout << "teste";
        } else if (glfwGetKey(window, GLFW_KEY_LEFT)) {
			std::cout << "teste";
        }
    } else if (glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS) {
        if (glfwGetKey(window, GLFW_KEY_RIGHT)) {
			std::cout << "teste";
        } else if (glfwGetKey(window, GLFW_KEY_LEFT)) {
			std::cout << "teste";
        }
    } else if (glfwGetKey(window, GLFW_KEY_B) == GLFW_PRESS) {
        if (glfwGetKey(window, GLFW_KEY_LEFT)) {
			std::cout << "teste";
        } else if (glfwGetKey(window, GLFW_KEY_RIGHT)) {
			std::cout << "teste";
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
	auto vertex = calcCubeVertex(cube.center, N);

	glBegin(GL_QUADS);
	// left
	glColor3f(cube.left.r, cube.left.g, cube.left.b);
		glVertex3f(vertex[4].x, vertex[4].y, vertex[4].z);
		glVertex3f(vertex[6].x, vertex[6].y, vertex[6].z);
		glVertex3f(vertex[2].x, vertex[2].y, vertex[2].z);
		glVertex3f(vertex[0].x, vertex[0].y, vertex[0].z);

	// right
	glColor3f(cube.right.r, cube.right.g, cube.right.b);
		glVertex3f(vertex[5].x, vertex[5].y, vertex[5].z);
		glVertex3f(vertex[7].x, vertex[7].y, vertex[7].z);
		glVertex3f(vertex[3].x, vertex[3].y, vertex[3].z);
		glVertex3f(vertex[1].x, vertex[1].y, vertex[1].z);

	// top
	glColor3f(cube.top.r, cube.top.g, cube.top.b);
		glVertex3f(vertex[0].x, vertex[0].y, vertex[0].z);
		glVertex3f(vertex[2].x, vertex[2].y, vertex[2].z);
		glVertex3f(vertex[3].x, vertex[3].y, vertex[3].z);
		glVertex3f(vertex[1].x, vertex[1].y, vertex[1].z);

	// bot
	glColor3f(cube.bottom.r, cube.bottom.g, cube.bottom.b);
		glVertex3f(vertex[4].x, vertex[4].y, vertex[4].z);
		glVertex3f(vertex[5].x, vertex[5].y, vertex[5].z);
		glVertex3f(vertex[7].x, vertex[7].y, vertex[7].z);
		glVertex3f(vertex[6].x, vertex[6].y, vertex[6].z);

	// front 
	glColor3f(cube.front.r, cube.front.g, cube.front.b);
		glVertex3f(vertex[2].x, vertex[2].y, vertex[2].z);
		glVertex3f(vertex[6].x, vertex[6].y, vertex[6].z);
		glVertex3f(vertex[7].x, vertex[7].y, vertex[7].z);
		glVertex3f(vertex[3].x, vertex[3].y, vertex[3].z);

	// back
	glColor3f(cube.back.r, cube.back.g, cube.back.b);
		glVertex3f(vertex[0].x, vertex[0].y, vertex[0].z);
		glVertex3f(vertex[4].x, vertex[4].y, vertex[4].z);
		glVertex3f(vertex[5].x, vertex[5].y, vertex[5].z);
		glVertex3f(vertex[1].x, vertex[1].y, vertex[1].z);
	glEnd();

	// contorno
	glLineWidth(3.0f);
	glBegin(GL_LINES);
	glColor3f(1.0f, 1.0f, 1.0f);
		//1
		glVertex3f(vertex[0].x, vertex[0].y, vertex[0].z);
		glVertex3f(vertex[2].x, vertex[2].y, vertex[2].z);

		glVertex3f(vertex[2].x, vertex[2].y, vertex[2].z);
		glVertex3f(vertex[6].x, vertex[6].y, vertex[6].z);

		glVertex3f(vertex[6].x, vertex[6].y, vertex[6].z);
		glVertex3f(vertex[4].x, vertex[4].y, vertex[4].z);

		glVertex3f(vertex[4].x, vertex[4].y, vertex[4].z);
		glVertex3f(vertex[0].x, vertex[0].y, vertex[0].z);

		//2
		glVertex3f(vertex[0].x, vertex[0].y, vertex[0].z);
		glVertex3f(vertex[1].x, vertex[1].y, vertex[1].z);

		glVertex3f(vertex[1].x, vertex[1].y, vertex[1].z);
		glVertex3f(vertex[5].x, vertex[5].y, vertex[5].z);

		glVertex3f(vertex[5].x, vertex[5].y, vertex[5].z);
		glVertex3f(vertex[4].x, vertex[4].y, vertex[4].z);

		glVertex3f(vertex[4].x, vertex[4].y, vertex[4].z);
		glVertex3f(vertex[0].x, vertex[0].y, vertex[0].z);

		//3
		glVertex3f(vertex[1].x, vertex[1].y, vertex[1].z);
		glVertex3f(vertex[5].x, vertex[5].y, vertex[5].z);

		glVertex3f(vertex[5].x, vertex[5].y, vertex[5].z);
		glVertex3f(vertex[7].x, vertex[7].y, vertex[7].z);

		glVertex3f(vertex[7].x, vertex[7].y, vertex[7].z);
		glVertex3f(vertex[3].x, vertex[3].y, vertex[3].z);

		glVertex3f(vertex[3].x, vertex[3].y, vertex[3].z);
		glVertex3f(vertex[1].x, vertex[1].y, vertex[1].z);

		//4
		glVertex3f(vertex[3].x, vertex[3].y, vertex[3].z);
		glVertex3f(vertex[7].x, vertex[7].y, vertex[7].z);

		glVertex3f(vertex[7].x, vertex[7].y, vertex[7].z);
		glVertex3f(vertex[6].x, vertex[6].y, vertex[6].z);

		glVertex3f(vertex[6].x, vertex[6].y, vertex[6].z);
		glVertex3f(vertex[2].x, vertex[2].y, vertex[2].z);

		glVertex3f(vertex[2].x, vertex[2].y, vertex[2].z);
		glVertex3f(vertex[3].x, vertex[3].y, vertex[3].z);
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

void drawInfosGUI () {
	ImGui::SetNextWindowPos(ImVec2(20, 20), ImGuiCond_Always);
	ImGui::Begin("TextoFixo", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground);
	ImGui::Text("Cubo Mágico (Arraste com o Mouse)");
	ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
	ImGui::End();
}

int main() {
	float countRotationIntern = 0.0f;
	float countRotationExtern = 0.0f;

	CamState cam;
	Coord3d cubeOrigem;
	std::array<CubeSection, 8> cube;

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

	cube[0] = {sectionsCenter[0], orange, gray, white, gray, gray, blue};
	cube[1] = {sectionsCenter[1], gray, red, white, gray, gray, blue};
	cube[2] = {sectionsCenter[2], orange, gray, white, gray, green, gray};
	cube[3] = {sectionsCenter[3], gray, red, white, gray, green, gray};
	cube[4] = {sectionsCenter[4], orange, gray, gray, yellow, gray, blue};
	cube[5] = {sectionsCenter[5], gray, red, gray, yellow, gray, blue};
	cube[6] = {sectionsCenter[6], orange, gray, gray, yellow, green, gray};
	cube[7] = {sectionsCenter[7], gray, red, gray, yellow, green, gray};

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
		drawInfosGUI();

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