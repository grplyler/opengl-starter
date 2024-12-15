
// OpenGL Stuff
#include <glad/glad.h>
#include <GLFW/glfw3.h>

// GLM Stuff
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// ImGui Stuff
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <ImGuizmo.h>

// Image Loading
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

// My Stuff
#include "camera.h"
#include "shader.h"
#include "mesh.h"

// Standard Library
#include <iostream>
#include <fstream>
#include <sstream>

int main() {
    RenderMesh mesh = RenderMesh::plane();
    mesh.compute_vertex_normals();
    mesh.to_obj("plane.obj");

    RenderMesh cube = RenderMesh::cube();
    mesh.compute_vertex_normals();
    cube.to_obj("cube_n.obj");

    RenderMesh uvsphere = RenderMesh::uvsphere(10, 10);
    mesh.compute_vertex_normals();
    uvsphere.to_obj("uvsphere.obj");

    RenderMesh cylinder = RenderMesh::cylinder(8);
    mesh.compute_vertex_normals();
    cylinder.to_obj("cylinder.obj");
    
    return 0;
}