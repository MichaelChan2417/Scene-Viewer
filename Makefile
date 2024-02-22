# include paths
GLFW_PATH_INCLUDE = "D:/STUDY/Libs/glfw-3.3.9.bin.WIN64/include"
GLFW_PATH_LIB = "D:/STUDY/Libs/glfw-3.3.9.bin.WIN64/lib-static-ucrt"
VULKAN_PATH = "D:/STUDY/Vulkan"
GLM_PATH = "D:/STUDY/Libs/glm"
STB_INCLUDE_PATH = "D:/STUDY/Libs/stb"

CXX = g++
CXXFLAGS = -std=c++20 -O2 -I$(GLFW_PATH_INCLUDE) -I$(VULKAN_PATH)/Include -I$(STB_INCLUDE_PATH) -I$(GLM_PATH)
LDFLAGS = -L$(GLFW_PATH_LIB) -L$(VULKAN_PATH)/Lib -lglfw3dll -lvulkan-1


# 源文件列表
SOURCES = scene_viewer.cpp main.cpp scene_config.cpp libs/mcjp.cpp \
			headless.cpp \
			implement/instance.cpp \
			implement/validation_layer.cpp \
			implement/physical_device.cpp \
			implement/logical_device.cpp \
			implement/swap_chain.cpp \
			implement/surface.cpp	\
			implement/image_view.cpp \
			implement/graphics_pipeline.cpp \
			implement/framebuffer.cpp	\
			implement/vertexbuffer.cpp \
			implement/depth_resources.cpp \
			implement/texture.cpp

# 生成目标文件列表
# OBJECTS = $(SOURCES:.cpp=.o)
OBJECTS = $(addprefix target/output/, $(SOURCES:.cpp=.o))


vulkan: $(OBJECTS)
	$(CXX) $(CXXFLAGS) -o viewer.exe $(OBJECTS) $(LDFLAGS)

.PHONY: all test clean

# clean:
# 	del viewer.exe $(OBJECTS)
clean:
	rd /s /q target\output 2>nul
	del viewer.exe


# create output directory
$(shell mkdir target 2>nul)
$(shell mkdir target\output 2>nul)
$(shell mkdir target\output\libs 2>nul)
$(shell mkdir target\output\implement 2>nul)


target/output/%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<