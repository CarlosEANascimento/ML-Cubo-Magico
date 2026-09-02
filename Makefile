CXX = g++
CXXFLAGS = -std=c++17 -Wall -Iglad/include -Iexternal/imgui -Iexternal/imgui/backends
LIBS = -lglfw -lGL -lX11 -lpthread -lXrandr -lXi -ldl

SRCS = cubo.cpp \
       external/imgui/imgui.cpp \
       external/imgui/imgui_demo.cpp \
       external/imgui/imgui_draw.cpp \
       external/imgui/imgui_tables.cpp \
       external/imgui/imgui_widgets.cpp \
       external/imgui/backends/imgui_impl_glfw.cpp \
       external/imgui/backends/imgui_impl_opengl3.cpp

TARGET = cubo

all: $(TARGET)

$(TARGET): $(SRCS)
	$(CXX) $(CXXFLAGS) $(SRCS) $(LIBS) -o $(TARGET)

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET)