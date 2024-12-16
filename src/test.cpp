
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
#include "hemesh.h"

// Standard Library
#include <iostream>
#include <fstream>
#include <sstream>

void test_face_vertex_count(HalfEdgeMesh& mesh, Face* face, int expected_count) {
    auto vertices = mesh.get_vertices_around_face(face);
    assert(vertices.size() == expected_count);
}

void test_vertex_neighbor_count(HalfEdgeMesh& mesh, Vertex* vertex, int expected_count) {
    auto neighbors = mesh.get_vertices_around_vertex(vertex);
    assert(neighbors.size() == expected_count);
}

void test_cube_creation() {
    HalfEdgeMesh mesh;
    
    // Create 8 vertices of a cube
    Vertex* v000 = mesh.add_vertex(0, 0, 0);
    Vertex* v001 = mesh.add_vertex(0, 0, 1);
    Vertex* v010 = mesh.add_vertex(0, 1, 0);
    Vertex* v011 = mesh.add_vertex(0, 1, 1);
    Vertex* v100 = mesh.add_vertex(1, 0, 0);
    Vertex* v101 = mesh.add_vertex(1, 0, 1);
    Vertex* v110 = mesh.add_vertex(1, 1, 0);
    Vertex* v111 = mesh.add_vertex(1, 1, 1);
    
    // Create 6 faces (simplified - just creating front face as example)
    Face* front = mesh.add_face({v000, v100, v110, v010});
    Face* back = mesh.add_face({v001, v011, v111, v101});
    Face* top = mesh.add_face({v010, v110, v111, v011});
    Face* bottom = mesh.add_face({v000, v001, v101, v100});
    Face* left = mesh.add_face({v000, v010, v011, v001});
    Face* right = mesh.add_face({v100, v101, v111, v110});
    
    // Test the front face
    test_face_vertex_count(mesh, front, 4);
    test_vertex_neighbor_count(mesh, v000, 1);  // Will be 3 when all faces are added

    // Convert to RenderMesh
    RenderMesh render_mesh = mesh.to_rendermesh();

    // Save to file
    render_mesh.to_obj("cube.obj");
}

void test_cube_rmesh_to_hemesh() {
    RenderMesh mesh = RenderMesh::cube();
    mesh.to_obj("cube.obj");
    HalfEdgeMesh hemesh = HalfEdgeMesh::from_rendermesh(mesh);
    RenderMesh mesh2 = hemesh.to_rendermesh();
    mesh2.to_obj("cube2.obj");
}

void test_uvsphere_rmesh_to_hemesh() {
    RenderMesh mesh = RenderMesh::uvsphere(5, 6);
    mesh.to_obj("uvsphere.obj");
    HalfEdgeMesh hemesh = HalfEdgeMesh::from_rendermesh(mesh);
    RenderMesh mesh2 = hemesh.to_rendermesh();
    mesh2.to_obj("uvsphere2.obj");
}

void test_icosphere_rmesh_to_hemesh() {
    RenderMesh mesh = RenderMesh::icosphere(2);
    mesh.to_obj("icosphere.obj");
    HalfEdgeMesh hemesh = HalfEdgeMesh::from_rendermesh(mesh);
    RenderMesh mesh2 = hemesh.to_rendermesh();
    mesh2.to_obj("icosphere2.obj");
}

void test_ico_subdiv_loop() {
    RenderMesh mesh = RenderMesh::icosphere(0);
    HalfEdgeMesh hemesh = HalfEdgeMesh::from_rendermesh(mesh);
    HalfEdgeMesh hemesh2 = hemesh.subdivide_loop();
    RenderMesh mesh2 = hemesh2.to_rendermesh();
    mesh2.to_obj("icosphere_subdiv.obj");
}

int main() {
    // RenderMesh mesh = RenderMesh::plane();
    // mesh.compute_vertex_normals();
    // mesh.to_obj("plane.obj");

    // RenderMesh cube = RenderMesh::cube();
    // mesh.compute_vertex_normals();
    // cube.to_obj("cube_n.obj");

    // RenderMesh uvsphere = RenderMesh::uvsphere(10, 10);
    // mesh.compute_vertex_normals();
    // uvsphere.to_obj("uvsphere.obj");

    // RenderMesh cylinder = RenderMesh::cylinder(8);
    // mesh.compute_vertex_normals();
    // cylinder.to_obj("cylinder.obj");

    // RenderMesh icosphere = RenderMesh::icosphere(2);
    // mesh.compute_vertex_normals();
    // icosphere.to_obj("icosphere.obj");

    // ProcMesh pm = ProcMesh::from_rendermesh(icosphere);
    // pm.to_obj("icosphere_proc_sub1.obj");

    // test_cube_creation();
    test_cube_rmesh_to_hemesh();
    test_uvsphere_rmesh_to_hemesh();
    test_icosphere_rmesh_to_hemesh();
    test_ico_subdiv_loop();

    
    return 0;
}