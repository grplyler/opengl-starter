cmake_minimum_required(VERSION 3.15)

# Project name and version
project(opengl-starter VERSION 1.0)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add the main executable
add_executable(${PROJECT_NAME} src/main.cpp)

# Add GLFW as a subdirectory
add_subdirectory(vendor/glfw)

# Link GLFW to the main executable
target_link_libraries(${PROJECT_NAME} PRIVATE glfw)

# Ensure proper include directories for GLFW
target_include_directories(${PROJECT_NAME} PRIVATE vendor/glfw/include)

# Add stb image header
target_include_directories(${PROJECT_NAME} PRIVATE vendor/stb)

# Add GLM (header-only library)
target_include_directories(${PROJECT_NAME} PRIVATE vendor/glm)

# Optionally, enable support for the Vulkan backend in GLFW
option(GLFW_USE_VULKAN "Enable Vulkan support in GLFW" OFF)

# Add GLAD as a static library from vendor/glad
add_library(glad STATIC vendor/glad/src/glad.c)
target_include_directories(glad PUBLIC vendor/glad/include)

# Link GLAD to the main executable
target_link_libraries(${PROJECT_NAME} PRIVATE glad)

# Add ImGui as a static library
add_library(imgui STATIC
    vendor/imgui/imgui.cpp
    vendor/imgui/imgui_draw.cpp
    vendor/imgui/imgui_widgets.cpp
    vendor/imgui/imgui_tables.cpp
    vendor/imgui/backends/imgui_impl_glfw.cpp
    vendor/imgui/backends/imgui_impl_opengl3.cpp
)

# Ensure ImGui has proper include directories
target_include_directories(imgui PUBLIC
    vendor/imgui
    vendor/imgui/backends
    vendor/glfw/include # Add GLFW include path here
)

# Link ImGui to the main executable
target_link_libraries(${PROJECT_NAME} PRIVATE imgui)

# Set platform-specific options
if (WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE opengl32)
elseif (APPLE)
    target_link_libraries(${PROJECT_NAME} PRIVATE "-framework OpenGL" "-framework Cocoa" "-framework IOKit" "-framework CoreVideo")
elseif (UNIX)
    find_package(OpenGL REQUIRED)
    target_link_libraries(${PROJECT_NAME} PRIVATE OpenGL::GL X11 pthread)
endif()
