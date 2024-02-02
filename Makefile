# include paths
GLFWPATH_INCLUDE = "D:/STUDY/Libs/glfw-3.3.9.bin.WIN64/include"
GLFWPATHLIB = "D:/STUDY/Libs/glfw-3.3.9.bin.WIN64/lib-static-ucrt"
VULKANPATH = "D:/STUDY/Vulkan"

CXX = g++
CXXFLAGS = -std=c++20 -O2 -I$(GLFWPATH_INCLUDE) -I$(VULKANPATH)/Include
LDFLAGS = -L$(GLFWPATHLIB) -L$(VULKANPATH)/Lib -lglfw3dll -lvulkan-1


# 源文件列表
SOURCES = scene_viewer.cpp main.cpp \
			implement\instance.cpp \
			implement\validation_layer.cpp \
			implement\physical_device.cpp \
			implement\logical_device.cpp \
			implement\swap_chain.cpp \
			implement\surface.cpp	\
			implement\image_view.cpp \
			implement\graphics_pipeline.cpp \
			implement\framebuffer.cpp

# 生成目标文件列表
OBJECTS = $(SOURCES:.cpp=.o)

vulkan: $(OBJECTS)
	$(CXX) $(CXXFLAGS) -o VulkanTest.exe $(OBJECTS) $(LDFLAGS)

.PHONY: all test clean

all: VulkanTest.exe

test: VulkanTest.exe
	.\VulkanTest.exe

clean:
	del VulkanTest.exe $(OBJECTS)

# generating target files
main.o: main.cpp scene_viewer.hpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

scene_viewer.o: scene_viewer.cpp scene_viewer.hpp libs\cglm.hpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

instance.o: instance.cpp scene_viewer.hpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

validation_layer.o: validation_layer.cpp scene_viewer.hpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

physical_device.o: physical_device.cpp scene_viewer.hpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

logical_device.o: logical_device.cpp scene_viewer.hpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

surface.o: surface.cpp scene_viewer.hpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

swap_chain.o: swap_chain.cpp scene_viewer.hpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

image_view.o: image_view.cpp scene_viewer.hpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

graphics_pipeline.o: graphics_pipeline.cpp scene_viewer.hpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

framebuffer.o: framebuffer.cpp scene_viewer.hpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<
