#pragma once

#include <vector>
#include <string>

#include <glad/glad.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// Forward declaration
struct ProcMesh;

struct RenderMesh {
    std::vector<glm::vec3> positions;   // Vertex positions
    unsigned int num_vertices;          // Number of vertices
    std::vector<glm::vec3> normals;     // Normals
    std::vector<glm::vec2> tex_coords; // Texture coordinates
    std::vector<unsigned int> indices; // Index buffer for drawing
    unsigned int VAO, VBO, EBO;         // OpenGL handles
    bool has_shared_vertices = false;
    bool has_tex_coords = false;
    bool has_vertex_normals = false;


    void add_vertex(float x, float y, float z);
    void add_vertex(float x, float y, float z, float nx, float ny, float nz);
    void add_vertex(float x, float y, float z, float nx, float ny, float nz, float u, float v);
    void add_face(unsigned int i0, unsigned int i1, unsigned int i2);

    // Mesh generation methods
    static RenderMesh cube();
    static RenderMesh icosphere();
    static RenderMesh plane();
    static RenderMesh cylinder();

    // GPU methods
    void upload();
    void upload_elements();
    void draw();
    std::vector<float> get_vertex_data(); // Interleaved vertex data

    // Mesh processing methods
    void compute_vertex_normals();
    void flip_faces();
    ProcMesh to_procmesh();

    // Mesh IO
    void to_obj(std::string filename);
    static RenderMesh from_obj(std::string filename);
};



struct ProcMesh {
    struct HalfEdge {
        int vertex_index;          // Start vertex index
        int twin_index;            // Opposite half-edge index
        int next_index;            // Next half-edge in the face
        int face_index;            // Face this edge belongs to
    };

    struct Face {
        int edge_index;            // One of the half-edges in the face
    };

    struct Vertex {
        glm::vec3 position;        // Vertex position
        int edge_index;            // One of the half-edges originating from this vertex
    };

    std::vector<Vertex> vertices;
    std::vector<HalfEdge> half_edges;
    std::vector<Face> faces;

    // Method to compute adjacency or other mesh processing
    void compute_adjacency();
};

void RenderMesh::add_face(unsigned int i0, unsigned int i1, unsigned int i2) {
    indices.push_back(i0);
    indices.push_back(i1);
    indices.push_back(i2);
}

void RenderMesh::add_vertex(float x, float y, float z, float nx, float ny, float nz) {
    has_vertex_normals = true;
    positions.push_back(glm::vec3(x, y, z));
    normals.push_back(glm::vec3(nx, ny, nz));
}

void RenderMesh::add_vertex(float x, float y, float z, float nx, float ny, float nz, float u, float v) {
    has_vertex_normals = true;
    has_tex_coords = true;
    positions.push_back(glm::vec3(x, y, z));
    normals.push_back(glm::vec3(nx, ny, nz));
    tex_coords.push_back(glm::vec2(u, v));
}

void RenderMesh::add_vertex(float x, float y, float z) {
    num_vertices++;
    positions.push_back(glm::vec3(x, y, z));
}

void RenderMesh::flip_faces() {
    for (size_t i = 0; i < indices.size(); i += 3) {
        std::swap(indices[i], indices[i + 2]);
    }
}

std::vector<float> RenderMesh::get_vertex_data() {
    std::vector<float> data;
    for (size_t i = 0; i < positions.size(); i++) {

        data.push_back(positions[i].x);
        data.push_back(positions[i].y);
        data.push_back(positions[i].z);

        if (has_vertex_normals) {
            data.push_back(normals[i].x);
            data.push_back(normals[i].y);
            data.push_back(normals[i].z);
        }

        if (has_tex_coords) {
            data.push_back(tex_coords[i].x);
            data.push_back(tex_coords[i].y);
        }
    }
    return data;
}

void RenderMesh::compute_vertex_normals() {
    has_vertex_normals = true;
    
    // Compute vertex normals
    for (size_t i = 0; i < positions.size(); i++) {
        normals.push_back(glm::vec3(0.0f));
    }

    for (size_t i = 0; i < indices.size(); i += 3) {
        glm::vec3 v0 = positions[indices[i]];
        glm::vec3 v1 = positions[indices[i + 1]];
        glm::vec3 v2 = positions[indices[i + 2]];

        glm::vec3 normal = glm::normalize(glm::cross(v1 - v0, v2 - v0));

        normals[indices[i]] += normal;
        normals[indices[i + 1]] += normal;
        normals[indices[i + 2]] += normal;
    }

    for (size_t i = 0; i < normals.size(); i++) {
        normals[i] = glm::normalize(normals[i]);
    }
}

void RenderMesh::draw() {
    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

void RenderMesh::upload_elements() {
    // For geometric with shared vertices

    // Generate buffers
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    // Bind VAO
    glBindVertexArray(VAO);

    // Upload vertex data
    auto verts = get_vertex_data();
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(float), verts.data(), GL_STATIC_DRAW);

    // Upload index data
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

    // Set vertex attribute pointers
    // Position
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Normals
    if (has_vertex_normals) {
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);
    }

    // Tex Coords
    if (has_tex_coords) {
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
        glEnableVertexAttribArray(2);
    }


}


void RenderMesh::upload() {
    upload_elements();
}

void RenderMesh::to_obj(std::string filename) {
    std::ofstream file(filename);

    for (const auto& position : positions) {
        file << "v " << position.x << " " << position.y << " " << position.z << std::endl;
    }

    for (const auto& normal : normals) {
        file << "vn " << normal.x << " " << normal.y << " " << normal.z << std::endl;
    }

    for (const auto& tex_coord : tex_coords) {
        file << "vt " << tex_coord.x << " " << tex_coord.y << std::endl;
    }

    for (size_t i = 0; i < indices.size(); i += 3) {
        file << "f ";
        for (int j = 0; j < 3; ++j) {
            unsigned int idx = indices[i + j] + 1;
            file << idx;
            if (has_tex_coords || has_vertex_normals) {
                file << "/";
                if (has_tex_coords) file << idx;
                if (has_vertex_normals) file << "/" << idx;
            }
            file << " ";
        }
        file << std::endl;
    }

    std::cout << "Wrote " << filename << std::endl;
    file.close();
}

RenderMesh RenderMesh::from_obj(std::string filename) {
    RenderMesh mesh;

    std::ifstream file(filename);

    std::string line;

    while (std::getline(file, line)) {
        std::istringstream iss(line);
        std::string type;
        iss >> type;

        if (type == "v") {
            float x, y, z;
            iss >> x >> y >> z;
            mesh.add_vertex(x, y, z);
        } else if (type == "vn") {
            float nx, ny, nz;
            iss >> nx >> ny >> nz;
            mesh.normals.push_back(glm::vec3(nx, ny, nz));
        } else if (type == "vt") {
            float u, v;
            iss >> u >> v;
            mesh.tex_coords.push_back(glm::vec2(u, v));
        } else if (type == "f") {
            unsigned int i0, i1, i2;
            char slash;
            iss >> i0 >> slash >> slash >> i1 >> slash >> i2;
            mesh.add_face(i0 - 1, i1 - 1, i2 - 1);
        }
    }

    return mesh;

}

RenderMesh RenderMesh::plane() {
    RenderMesh mesh;
    mesh.has_shared_vertices = true;

    // Make a place facing up with two triangles and use no shared vertices and use indices
    mesh.add_vertex(-0.5f, 0.0f, -0.5f);
    mesh.add_vertex(0.5f, 0.0f, -0.5f);
    mesh.add_vertex(0.5f, 0.0f, 0.5f);
    mesh.add_vertex(-0.5f, 0.0f, 0.5f);

    mesh.add_face(0, 2, 1);
    mesh.add_face(0, 3, 2);

    return mesh;
}

RenderMesh RenderMesh::cube() {
    RenderMesh mesh;
    mesh.has_shared_vertices = true;

    // Front
    mesh.add_vertex(-0.5f, -0.5f,  0.5f);  // 0
    mesh.add_vertex( 0.5f, -0.5f,  0.5f);  // 1
    mesh.add_vertex( 0.5f,  0.5f,  0.5f);  // 2
    mesh.add_vertex(-0.5f,  0.5f,  0.5f);  // 3
    
    // Back
    mesh.add_vertex(-0.5f, -0.5f, -0.5f);  // 4
    mesh.add_vertex( 0.5f, -0.5f, -0.5f);  // 5 
    mesh.add_vertex( 0.5f,  0.5f, -0.5f);  // 6
    mesh.add_vertex(-0.5f,  0.5f, -0.5f);  // 7

    // Front
    mesh.add_face(0, 1, 2);
    mesh.add_face(0, 2, 3);
    
    // Back
    mesh.add_face(5, 4, 7);
    mesh.add_face(5, 7, 6);
    
    // Top
    mesh.add_face(3, 2, 6);
    mesh.add_face(3, 6, 7);
    
    // Bottom
    mesh.add_face(4, 5, 1);
    mesh.add_face(4, 1, 0);
    
    // Right
    mesh.add_face(1, 5, 6);
    mesh.add_face(1, 6, 2);
    
    // Left
    mesh.add_face(4, 0, 3);
    mesh.add_face(4, 3, 7);

    return mesh;
}
