cmake_minimum_required(VERSION 3.15)

# Project name and version
project(opengl-starter VERSION 1.0)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Define shared include directories
set(SHARED_INCLUDE_DIRS
    vendor/glfw/include
    vendor/stb
    vendor/glm
    vendor/imgui
    vendor/imgui/backends
    vendor/ImGuizmo
)

# Define shared libraries (without duplicates)
set(SHARED_LIBRARIES glfw glad ImGuizmo)

# Add main executable
add_executable(${PROJECT_NAME} src/main.cpp src/shader.h src/camera.h src/mesh.h)

# Add test executable
add_executable(test src/test.cpp src/shader.h src/camera.h src/mesh.h)

# Link shared libraries and imgui explicitly to both executables
target_link_libraries(${PROJECT_NAME} PRIVATE ${SHARED_LIBRARIES} imgui)
target_link_libraries(test PRIVATE ${SHARED_LIBRARIES} imgui)

# Add shared include directories to executables
target_include_directories(${PROJECT_NAME} PRIVATE ${SHARED_INCLUDE_DIRS})
target_include_directories(test PRIVATE ${SHARED_INCLUDE_DIRS})

# Set platform-specific options
if (WIN32)
    set(PLATFORM_LIBS opengl32)
elseif (APPLE)
    set(PLATFORM_LIBS "-framework OpenGL" "-framework Cocoa" "-framework IOKit" "-framework CoreVideo")
elseif (UNIX)
    find_package(OpenGL REQUIRED)
    set(PLATFORM_LIBS OpenGL::GL X11 pthread)
endif()

target_link_libraries(${PROJECT_NAME} PRIVATE ${PLATFORM_LIBS})
target_link_libraries(test PRIVATE ${PLATFORM_LIBS})

# Add GLFW as a subdirectory
add_subdirectory(vendor/glfw)

# Add GLAD as a static library
add_library(glad STATIC vendor/glad/src/glad.c)
target_include_directories(glad PUBLIC vendor/glad/include)

# Add ImGui as a static library
add_library(imgui STATIC
    vendor/imgui/imgui.cpp
    vendor/imgui/imgui_draw.cpp
    vendor/imgui/imgui_widgets.cpp
    vendor/imgui/imgui_tables.cpp
    vendor/imgui/backends/imgui_impl_glfw.cpp
    vendor/imgui/backends/imgui_impl_opengl3.cpp
)

# Ensure proper include directories for ImGui
target_include_directories(imgui PUBLIC
    vendor/imgui
    vendor/imgui/backends
    vendor/glfw/include  # Add GLFW include path explicitly
)

# Add ImGuizmo as a static library
add_library(ImGuizmo STATIC vendor/ImGuizmo/ImGuizmo.cpp)
target_include_directories(ImGuizmo PUBLIC
    vendor/ImGuizmo
    vendor/imgui
)

target_link_libraries(ImGuizmo PRIVATE imgui)
