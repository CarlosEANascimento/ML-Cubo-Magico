CXX = g++

CXXFLAGS = -std=c++17 -Wall \
	-Iglad/include \
	-Iexternal/imgui \
	-Iexternal/imgui/backends

LIBS = -lglfw -lGL -lX11 -lpthread -lXrandr -lXi -ldl

CORE_SRCS = estado.cpp \
	    busca.cpp \
	    busca_largura.cpp

GUI_SRCS = cubo.cpp \
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


clean:
	rm -f $(TARGET) teste_bfs