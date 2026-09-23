CXX = g++

CXXFLAGS = -std=c++17 -Wall \
	-Iglad/include \
	-Iexternal/imgui \
	-Iexternal/imgui/backends

LIBS = -lglfw -lGL -lX11 -lpthread -lXrandr -lXi -ldl

CORE_SRCS = estado.cpp \
           simetria.cpp \
	    busca.cpp \
	    busca_largura.cpp \
	    busca_profundidade.cpp \
			a_estrela.cpp \
			heuristica.cpp

GUI_SRCS = cubo.cpp \
	   glad/src/glad.c \
	   external/imgui/imgui.cpp \
	   external/imgui/imgui_demo.cpp \
	   external/imgui/imgui_draw.cpp \
	   external/imgui/imgui_tables.cpp \
	   external/imgui/imgui_widgets.cpp \
	   external/imgui/backends/imgui_impl_glfw.cpp \
	   external/imgui/backends/imgui_impl_opengl3.cpp

SRCS = $(GUI_SRCS) $(CORE_SRCS)

TARGET = cubo

all: $(TARGET)

$(TARGET): $(SRCS)
	$(CXX) $(CXXFLAGS) $(SRCS) $(LIBS) -o $(TARGET)

run: $(TARGET)
	./$(TARGET)


teste_bfs: $(CORE_SRCS) teste_bfs.cpp
	$(CXX) -std=c++17 -Wall \
	$(CORE_SRCS) \
	teste_bfs.cpp \
	-o teste_bfs

test-bfs: teste_bfs
	./teste_bfs


# constroi (se preciso) e prova tabela_h.bin (~1 a 3 minutos, 29 MB)
.PHONY: tabela
tabela: $(CORE_SRCS) tabela.cpp
	$(CXX) -std=c++17 -Wall -O2 \
	$(CORE_SRCS) \
	tabela.cpp \
	-o tabela
	./tabela


clean:
	rm -f $(TARGET) teste_bfs tabela
