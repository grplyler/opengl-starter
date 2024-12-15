#pragma once

#include <vector>
#include <string>

#include <glad/glad.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// Forward declaration
struct ProcMesh;

struct DebugMeshLines {
    unsigned int VAO, VBO;
    unsigned int line_count;
};

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
    
    // Debug Visualization
    DebugMeshLines debug_normals = {0, 0, 0};
    DebugMeshLines debug_wireframe = {0, 0, 0};

    // Constructor
    void add_vertex(float x, float y, float z);
    void add_vertex(float x, float y, float z, float nx, float ny, float nz);
    void add_vertex(float x, float y, float z, float nx, float ny, float nz, float u, float v);
    void add_face(unsigned int i0, unsigned int i1, unsigned int i2);

    // Mesh generation methods
    static RenderMesh cube();
    static RenderMesh uvsphere(int rings, int sectors);
    static RenderMesh icosphere(int subdivisions);
    static RenderMesh plane();
    static RenderMesh cylinder(int sectors);

    // GPU methods
    void upload();
    void upload_elements();
    void draw();
    void draw_normals(float line_width = 1.0f, float length = 0.1f);
    void draw_wireframe(float line_width = 1.0f);
    std::vector<float> get_vertex_data(); // Interleaved vertex data

    // Mesh processing methods
    void compute_vertex_normals();
    void flip_faces();
    void create_debug_normals(float length);
    void create_debug_wireframe();
    ProcMesh to_procmesh();

    // Mesh IO
    void to_obj(std::string filename);
    static RenderMesh from_obj(std::string filename);
};

namespace std {
    template<>
    struct hash<pair<int,int>> {
        size_t operator()(const pair<int,int>& p) const {
            return hash<int>()(p.first) ^ (hash<int>()(p.second) << 1);
        }
    };
}

struct ProcMesh {
    // Core vertex data
    struct Vertex {
        glm::vec3 position;
        glm::vec3 normal;
        int edge;  // Index of one outgoing half-edge
    };

    // Half-edge connectivity
    struct HalfEdge {
        int vertex;     // Target vertex
        int pair;       // Opposite half-edge
        int next;       // Next in face loop
        int prev;       // Previous in face loop  
        int face;       // Face this belongs to
        int edge;       // Parent edge index
    };

    // Edge data
    struct Edge {
        int he1, he2;  // The two half-edges
        glm::vec3 midpoint; // For subdivision
    };

    // Face data  
    struct Face {
        int edge;      // One half-edge in face
        glm::vec3 normal;
    };

    // Main containers
    std::vector<Vertex> vertices;
    std::vector<HalfEdge> halfedges;
    std::vector<Edge> edges;
    std::vector<Face> faces;

    // Helper maps
    std::unordered_map<std::pair<int,int>, int> edge_map; // vertex pair -> edge index
    std::unordered_map<int, std::vector<int>> vertex_faces; // vertex -> faces
    std::unordered_map<int, std::vector<int>> vertex_edges; // vertex -> edges

    // Processing methods
    void compute_adjacency();
    void compute_vertex_normals();
    void subdivide_loop();

    // Construction methods
    static ProcMesh from_rendermesh(const RenderMesh& render_mesh);
    void add_vertex(const glm::vec3& pos);
    void add_face(int v0, int v1, int v2);
    void build_halfedge_structure();

    // IO
    void to_obj(std::string filename);
};

void ProcMesh::to_obj(std::string filename) {
    std::ofstream file(filename);
    if (!file) return;

    // Write vertex positions
    for (const auto& v : vertices) {
        file << "v " << v.position.x << " " 
                    << v.position.y << " " 
                    << v.position.z << "\n";
    }

    // Write vertex normals
    for (const auto& v : vertices) {
        file << "vn " << v.normal.x << " " 
                     << v.normal.y << " " 
                     << v.normal.z << "\n";
    }

    // Write faces
    for (size_t f = 0; f < faces.size(); f++) {
        file << "f ";
        HalfEdge* he = &halfedges[faces[f].edge];
        do {
            int v_idx = halfedges[he->prev].vertex + 1; // OBJ indices are 1-based
            file << v_idx << "//" << v_idx << " ";
            he = &halfedges[he->next];
        } while (he != &halfedges[faces[f].edge]);
        file << "\n";
    }

    file.close();
    std::cout << "Wrote mesh to " << filename << std::endl;
}

void ProcMesh::compute_adjacency() {
    // Clear existing maps
    vertex_faces.clear();
    vertex_edges.clear();
    edge_map.clear();
    
    // Build vertex->face adjacency
    for(size_t f = 0; f < faces.size(); f++) {
        HalfEdge* he = &halfedges[faces[f].edge];
        do {
            int v = halfedges[he->prev].vertex;
            vertex_faces[v].push_back(f);
            
            // Get edge vertices
            int v1 = halfedges[he->prev].vertex;
            int v2 = he->vertex;
            
            // Store edge mapping
            auto edge_key = std::make_pair(std::min(v1,v2), std::max(v1,v2));
            if(edge_map.find(edge_key) == edge_map.end()) {
                edge_map[edge_key] = he->edge;
                vertex_edges[v1].push_back(he->edge);
                vertex_edges[v2].push_back(he->edge);
            }
            
            he = &halfedges[he->next];
        } while(he != &halfedges[faces[f].edge]);
        
        // Compute face normal
        HalfEdge* he0 = he;
        glm::vec3 v0 = vertices[halfedges[he0->prev].vertex].position;
        glm::vec3 v1 = vertices[he0->vertex].position;
        he0 = &halfedges[he0->next];
        glm::vec3 v2 = vertices[he0->vertex].position;
        faces[f].normal = glm::normalize(glm::cross(v1 - v0, v2 - v0));
    }
}

void ProcMesh::add_face(int v0, int v1, int v2) {
    // Create new face
    Face f;
    f.edge = halfedges.size();
    faces.push_back(f);
    
    // Create three half-edges
    for (int i = 0; i < 3; i++) {
        HalfEdge he;
        he.face = faces.size() - 1;
        halfedges.push_back(he);
    }
    
    // Setup connectivity
    int base = halfedges.size() - 3;
    halfedges[base].vertex = v1;     // Points to next vertex
    halfedges[base+1].vertex = v2;
    halfedges[base+2].vertex = v0;
    
    halfedges[base].next = base + 1;
    halfedges[base+1].next = base + 2;
    halfedges[base+2].next = base;
    
    halfedges[base].prev = base + 2;
    halfedges[base+1].prev = base;
    halfedges[base+2].prev = base + 1;
}

void ProcMesh::build_halfedge_structure() {
    // Initialize maps
    edge_map.clear();
    vertex_faces.clear();
    vertex_edges.clear();

    // Find pairs and create edges
    for (int i = 0; i < halfedges.size(); i++) {
        HalfEdge& he = halfedges[i];
        int v1 = halfedges[he.prev].vertex;
        int v2 = he.vertex;

        // Store vertex->face connection
        vertex_faces[v1].push_back(he.face);

        // Look for matching half-edge
        for (int j = i + 1; j < halfedges.size(); j++) {
            HalfEdge& candidate = halfedges[j];
            int cv1 = halfedges[candidate.prev].vertex;
            int cv2 = candidate.vertex;

            if (v1 == cv2 && v2 == cv1) {
                // Found matching pair
                he.pair = j;
                candidate.pair = i;

                // Create new edge
                Edge e;
                e.he1 = i;
                e.he2 = j;
                edges.push_back(e);

                // Update edge indices
                he.edge = edges.size() - 1;
                candidate.edge = edges.size() - 1;

                // Store in edge map
                edge_map[{std::min(v1,v2), std::max(v1,v2)}] = edges.size() - 1;
                
                // Store vertex->edge connections
                vertex_edges[v1].push_back(edges.size() - 1);
                vertex_edges[v2].push_back(edges.size() - 1);
                break;
            }
        }
    }

    // Update vertex outgoing edges
    for (int i = 0; i < vertices.size(); i++) {
        if (!vertex_edges[i].empty()) {
            vertices[i].edge = halfedges[edges[vertex_edges[i][0]].he1].pair;
        }
    }
}

ProcMesh ProcMesh::from_rendermesh(const RenderMesh& render_mesh) {
    ProcMesh proc_mesh;
    
    // Transfer vertices
    for (const auto& pos : render_mesh.positions) {
        proc_mesh.add_vertex(pos);
    }
    
    // Create faces and initial topology
    for (size_t i = 0; i < render_mesh.indices.size(); i += 3) {
        proc_mesh.add_face(
            render_mesh.indices[i],
            render_mesh.indices[i + 1],
            render_mesh.indices[i + 2]
        );
    }
    
    // Build complete topology
    proc_mesh.build_halfedge_structure();
    proc_mesh.compute_adjacency();
    proc_mesh.compute_vertex_normals();
    
    return proc_mesh;
}

void ProcMesh::compute_vertex_normals() {
    // Reset normals
    for (auto& vertex : vertices) {
        vertex.normal = glm::vec3(0.0f);
    }

    // Accumulate face normals
    for (size_t i = 0; i < vertices.size(); i++) {
        const auto& adj_faces = vertex_faces[i];
        
        for (int face_idx : adj_faces) {
            const Face& face = faces[face_idx];
            
            // Get vertices of face
            HalfEdge* he = &halfedges[face.edge];
            glm::vec3 v0 = vertices[he->vertex].position;
            he = &halfedges[he->next];
            glm::vec3 v1 = vertices[he->vertex].position;
            he = &halfedges[he->next];
            glm::vec3 v2 = vertices[he->vertex].position;

            // Compute face angle at vertex
            glm::vec3 e1, e2;
            if (he->vertex == i) {
                e1 = v1 - v2;
                e2 = v0 - v2;
            } else if (halfedges[he->prev].vertex == i) {
                e1 = v2 - v1;
                e2 = v0 - v1;
            } else {
                e1 = v1 - v0;
                e2 = v2 - v0;
            }

            float angle = acos(glm::dot(glm::normalize(e1), glm::normalize(e2)));
            vertices[i].normal += face.normal * angle;
        }

        if (!adj_faces.empty()) {
            vertices[i].normal = glm::normalize(vertices[i].normal);
        }
    }
}

void ProcMesh::add_vertex(const glm::vec3& pos) {
    Vertex v;
    v.position = pos;
    v.normal = glm::vec3(0.0f);
    v.edge = -1;
    vertices.push_back(v);
}

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

void RenderMesh::create_debug_wireframe() {
    std::cout << "Creating debug wireframe" << std::endl;
    std::vector<float> lines;
    for (size_t i = 0; i < indices.size(); i += 3) {
        // Triangle vertices
        glm::vec3 v0 = positions[indices[i]];
        glm::vec3 v1 = positions[indices[i + 1]];
        glm::vec3 v2 = positions[indices[i + 2]];

        // Add lines for each edge
        lines.push_back(v0.x);
        lines.push_back(v0.y);
        lines.push_back(v0.z);

        lines.push_back(v1.x);
        lines.push_back(v1.y);
        lines.push_back(v1.z);

        lines.push_back(v1.x);
        lines.push_back(v1.y);
        lines.push_back(v1.z);

        lines.push_back(v2.x);
        lines.push_back(v2.y);
        lines.push_back(v2.z);

        lines.push_back(v2.x);
        lines.push_back(v2.y);
        lines.push_back(v2.z);

        lines.push_back(v0.x);
        lines.push_back(v0.y);
        lines.push_back(v0.z);
    }

    glGenVertexArrays(1, &debug_wireframe.VAO);
    glGenBuffers(1, &debug_wireframe.VBO);

    glBindVertexArray(debug_wireframe.VAO);
    glBindBuffer(GL_ARRAY_BUFFER, debug_wireframe.VBO);
    glBufferData(GL_ARRAY_BUFFER, lines.size() * sizeof(float), lines.data(), GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), 0);

    debug_wireframe.line_count = lines.size() / 3; // 3 floats per line (2 vertices * 3 coords)
    
}

void RenderMesh::create_debug_normals(float length) {
    if (!has_vertex_normals) return;
    
    std::cout << "Creating debug normals" << std::endl;

    std::vector<float> lines;
    // For each vertex, create a line from position to position + normal * length
    for (size_t i = 0; i < positions.size(); i++) {
        // Start point of line
        lines.push_back(positions[i].x);
        lines.push_back(positions[i].y);
        lines.push_back(positions[i].z);
        
        // End point of line
        glm::vec3 end = positions[i] + normals[i] * length;
        lines.push_back(end.x);
        lines.push_back(end.y);
        lines.push_back(end.z);
    }
    
    glGenVertexArrays(1, &debug_normals.VAO);
    glGenBuffers(1, &debug_normals.VBO);
    
    glBindVertexArray(debug_normals.VAO);
    glBindBuffer(GL_ARRAY_BUFFER, debug_normals.VBO);
    glBufferData(GL_ARRAY_BUFFER, lines.size() * sizeof(float), lines.data(), GL_STATIC_DRAW);
    
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), 0);
    
    debug_normals.line_count = lines.size() / 6; // 6 floats per line (2 vertices * 3 coords)
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

void RenderMesh::draw_normals(float line_width, float length) {
    // if (!has_vertex_normals) return;

    // Query max line width
    float max_line_width;
    glGetFloatv(GL_ALIASED_LINE_WIDTH_RANGE, &max_line_width);


    if (debug_normals.VAO == 0) {
        std::cout << "Max supported line width: " << max_line_width << std::endl;
        create_debug_normals(length);
    }

    float clamped_width = std::min(line_width, max_line_width);
    
    glLineWidth(clamped_width);
    glBindVertexArray(debug_normals.VAO);
    glDrawArrays(GL_LINES, 0, debug_normals.line_count * 2);
    glLineWidth(1.0f);
}

void RenderMesh::draw_wireframe(float line_width)
{
    if (debug_wireframe.VAO == 0) {
        create_debug_wireframe();
    }

    float max_line_width;
    glGetFloatv(GL_ALIASED_LINE_WIDTH_RANGE, &max_line_width);
    float clamped_width = std::min(line_width, max_line_width);

    glLineWidth(clamped_width);
    glBindVertexArray(debug_wireframe.VAO);
    glDrawArrays(GL_LINES, 0, debug_wireframe.line_count * 2);
    glLineWidth(1.0f);
}

void RenderMesh::upload_elements() {
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

    // Calculate stride - base position (3) + optional normals (3) + optional tex coords (2)
    int stride = 3; // Position always present
    if (has_vertex_normals) stride += 3;
    if (has_tex_coords) stride += 2;
    stride *= sizeof(float);

    size_t offset = 0;

    // Position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)offset);
    glEnableVertexAttribArray(0);
    offset += 3 * sizeof(float);

    // Normal attribute
    if (has_vertex_normals) {
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*)offset);
        glEnableVertexAttribArray(1);
        offset += 3 * sizeof(float);
    }

    // Texture coordinate attribute
    if (has_tex_coords) {
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, (void*)offset);
        glEnableVertexAttribArray(2);
    }

    // Unbind VAO
    glBindVertexArray(0);
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

RenderMesh RenderMesh::icosphere(int subdivisions) {
    RenderMesh mesh;
    mesh.has_shared_vertices = true;

    // Create icosahedron
    float t = (1.0f + glm::sqrt(5.0f)) / 2.0f;

    mesh.add_vertex(-1,  t,  0);
    mesh.add_vertex( 1,  t,  0);
    mesh.add_vertex(-1, -t,  0);
    mesh.add_vertex( 1, -t,  0);

    mesh.add_vertex( 0, -1,  t);
    mesh.add_vertex( 0,  1,  t);
    mesh.add_vertex( 0, -1, -t);
    mesh.add_vertex( 0,  1, -t);

    mesh.add_vertex( t,  0, -1);
    mesh.add_vertex( t,  0,  1);
    mesh.add_vertex(-t,  0, -1);
    mesh.add_vertex(-t,  0,  1);

    mesh.add_face(0, 11, 5);
    mesh.add_face(0, 5, 1);
    mesh.add_face(0, 1, 7);
    mesh.add_face(0, 7, 10);
    mesh.add_face(0, 10, 11);

    mesh.add_face(1, 5, 9);
    mesh.add_face(5, 11, 4);
    mesh.add_face(11, 10, 2);
    mesh.add_face(10, 7, 6);
    mesh.add_face(7, 1, 8);

    mesh.add_face(3, 9, 4);
    mesh.add_face(3, 4, 2);
    mesh.add_face(3, 2, 6);
    mesh.add_face(3, 6, 8);
    mesh.add_face(3, 8, 9);

    mesh.add_face(4, 9, 5);
    mesh.add_face(2, 4, 11);
    mesh.add_face(6, 2, 10);
    mesh.add_face(8, 6, 7);
    mesh.add_face(9, 8, 1);

    return mesh;
}

RenderMesh RenderMesh::uvsphere(int rings, int sectors) {
    RenderMesh mesh;
    mesh.has_shared_vertices = true;

    float R = 1.0f;
    float pi = glm::pi<float>();

    // Add north pole vertex
    mesh.add_vertex(0, R, 0, 0, 1, 0, 0.5f, 0.0f);

    // Generate vertices for rings (excluding poles)
    for (int i = 1; i < rings; i++) {
        float theta = i * pi / rings;
        float sinTheta = glm::sin(theta);
        float cosTheta = glm::cos(theta);

        for (int j = 0; j <= sectors; j++) {
            float phi = j * 2 * pi / sectors;
            float sinPhi = glm::sin(phi);
            float cosPhi = glm::cos(phi);

            float x = cosPhi * sinTheta;
            float y = cosTheta;
            float z = sinPhi * sinTheta;

            mesh.add_vertex(x * R, y * R, z * R, x, y, z, (float)j / sectors, (float)i / rings);
        }
    }

    // Add south pole vertex
    mesh.add_vertex(0, -R, 0, 0, -1, 0, 0.5f, 1.0f);

    // Create triangles for north pole (reversed winding)
    int northPoleIndex = 0;
    for (int j = 0; j < sectors; j++) {
        mesh.add_face(northPoleIndex, j + 2, j + 1);
    }

    // Create triangles for rings (reversed winding)
    for (int i = 1; i < rings - 1; i++) {
        int rowStart = 1 + (i - 1) * (sectors + 1);
        for (int j = 0; j < sectors; j++) {
            int p0 = rowStart + j;
            int p1 = p0 + sectors + 1;

            mesh.add_face(p0, p0 + 1, p1);
            mesh.add_face(p1, p0 + 1, p1 + 1);
        }
    }

    // Create triangles for south pole (reversed winding)
    int southPoleIndex = mesh.positions.size() - 1;
    int lastRingStart = southPoleIndex - (sectors + 1);
    for (int j = 0; j < sectors; j++) {
        mesh.add_face(southPoleIndex, lastRingStart + j, lastRingStart + j + 1);
    }

    return mesh;
}

RenderMesh RenderMesh::cylinder(int sectors) {
    RenderMesh mesh;
    mesh.has_shared_vertices = true;

    float H = 1.0f;  // Height of 1.0
    float R = 0.5f;  // Radius of 0.5 for diameter of 1.0
    float pi = glm::pi<float>();

    // Add center vertices for caps
    mesh.add_vertex(0, H/2, 0, 0, 1, 0, 0.5f, 0.5f);    // Top center
    mesh.add_vertex(0, -H/2, 0, 0, -1, 0, 0.5f, 0.5f);  // Bottom center

    // Generate vertices for caps
    for (int i = 0; i <= sectors; i++) {
        float theta = i * 2 * pi / sectors;
        float x = R * glm::cos(theta);
        float z = R * glm::sin(theta);
        float u = glm::cos(theta) * 0.5f + 0.5f;
        float v = glm::sin(theta) * 0.5f + 0.5f;

        // Top cap vertices
        mesh.add_vertex(x, H/2, z, 0, 1, 0, u, v);
        // Bottom cap vertices
        mesh.add_vertex(x, -H/2, z, 0, -1, 0, u, v);
    }

    // Generate vertices for sides
    for (int i = 0; i <= sectors; i++) {
        float theta = i * 2 * pi / sectors;
        float x = R * glm::cos(theta);
        float z = R * glm::sin(theta);
        float nx = glm::cos(theta);
        float nz = glm::sin(theta);

        mesh.add_vertex(x, H/2, z, nx, 0, nz, (float)i/sectors, 0.0f);
        mesh.add_vertex(x, -H/2, z, nx, 0, nz, (float)i/sectors, 1.0f);
    }

    // Create top circle (reversed winding)
    int topCenter = 0;
    for (int i = 0; i < sectors; i++) {
        int current = 2 + i * 2;
        int next = 2 + (i + 1) * 2;
        mesh.add_face(topCenter, next, current);
    }

    // Create bottom circle (reversed winding)
    int bottomCenter = 1;
    for (int i = 0; i < sectors; i++) {
        int current = 3 + i * 2;
        int next = 3 + (i + 1) * 2;
        mesh.add_face(bottomCenter, current, next);
    }

    // Create sides (reversed winding)
    int sideStart = 2 * (sectors + 1) + 2;
    for (int i = 0; i < sectors; i++) {
        mesh.add_face(sideStart + i * 2, sideStart + i * 2 + 3, sideStart + i * 2 + 1);
        mesh.add_face(sideStart + i * 2, sideStart + i * 2 + 2, sideStart + i * 2 + 3);
    }

    return mesh;
}