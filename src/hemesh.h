#pragma once
#include <vector>
#include <set>
#include <unordered_map>
#include "mesh.h"

class Vertex;
class HalfEdge;
class Face;

class Vertex
{
public:
    float x, y, z;
    HalfEdge *outgoing_halfedge;

    Vertex(float x = 0, float y = 0, float z = 0)
        : x(x), y(y), z(z), outgoing_halfedge(nullptr) {}
};

class HalfEdge
{
public:
    Vertex *vertex;
    HalfEdge *pair;
    HalfEdge *next;
    Face *face;

    HalfEdge()
        : vertex(nullptr), pair(nullptr), next(nullptr), face(nullptr) {}
};

class Face
{
public:
    HalfEdge *halfedge; // One of the halfedges bordering this face

    Face() : halfedge(nullptr) {}
};

class HalfEdgeMesh
{
public:

    std::vector<std::unique_ptr<Vertex>> vertices;
    std::vector<std::unique_ptr<HalfEdge>> halfedges;
    std::vector<std::unique_ptr<Face>> faces;

    Vertex *add_vertex(float x, float y, float z)
    {
        vertices.push_back(std::make_unique<Vertex>(x, y, z));
        return vertices.back().get();
    };

    Face *add_face(const std::vector<Vertex *> &input_vertices)
    {
        if (input_vertices.size() < 3)
        {
            throw std::invalid_argument("Face must have at least 3 vertices");
        }

        // Reverse vertices for counter-clockwise winding
        std::vector<Vertex *> face_vertices(input_vertices.rbegin(), input_vertices.rend());

        // Create a new face
        auto face = std::make_unique<Face>();
        Face *face_ptr = face.get();

        // Create halfedges for this face
        std::vector<HalfEdge *> new_halfedges;
        for (size_t i = 0; i < face_vertices.size(); ++i)
        {
            auto he = std::make_unique<HalfEdge>();
            he->vertex = face_vertices[(i + 1) % face_vertices.size()];
            he->face = face_ptr;

            if (!new_halfedges.empty())
            {
                new_halfedges.back()->next = he.get();
            }

            new_halfedges.push_back(he.get());
            halfedges.push_back(std::move(he));
        }

        // Connect the last halfedge to the first
        new_halfedges.back()->next = new_halfedges.front();

        // Set face's halfedge
        face->halfedge = new_halfedges.front();

        // Set vertices' outgoing halfedges
        for (size_t i = 0; i < face_vertices.size(); ++i)
        {
            face_vertices[i]->outgoing_halfedge = new_halfedges[i];
        }

        faces.push_back(std::move(face));
        return face_ptr;
    }

    void connect_opposites(HalfEdge *he1, HalfEdge *he2)
    {
        he1->pair = he2;
        he2->pair = he1;
    }

    // Calculate edge length
    float get_edge_length(HalfEdge *edge)
    {
        glm::vec3 v1(edge->vertex->x, edge->vertex->y, edge->vertex->z);
        glm::vec3 v2(edge->next->vertex->x, edge->next->vertex->y, edge->next->vertex->z);
        return glm::length(v2 - v1);
    }

    // Calculate face area
    float get_face_area(Face *face)
    {
        HalfEdge *edge = face->halfedge;
        glm::vec3 v1(edge->vertex->x, edge->vertex->y, edge->vertex->z);
        glm::vec3 v2(edge->next->vertex->x, edge->next->vertex->y, edge->next->vertex->z);
        glm::vec3 v3(edge->next->next->vertex->x, edge->next->next->vertex->y, edge->next->next->vertex->z);
        return 0.5f * glm::length(glm::cross(v2 - v1, v3 - v1));
    }

    // Calculate vertex normal
    glm::vec3 get_vertex_normal(Vertex *vertex)
    {
        glm::vec3 normal(0.0f);
        auto faces = get_faces_around_vertex(vertex);
        for (Face *face : faces)
        {
            normal += get_face_normal(face);
        }
        return glm::normalize(normal);
    }

    // Calculate face normal
    glm::vec3 get_face_normal(Face *face)
    {
        HalfEdge *edge = face->halfedge;
        glm::vec3 v1(edge->vertex->x, edge->vertex->y, edge->vertex->z);
        glm::vec3 v2(edge->next->vertex->x, edge->next->vertex->y, edge->next->vertex->z);
        glm::vec3 v3(edge->next->next->vertex->x, edge->next->next->vertex->y, edge->next->next->vertex->z);
        return glm::normalize(glm::cross(v2 - v1, v3 - v1));
    }

    // Validate mesh integrity
    bool validate_mesh() const
    {
        for (const auto &edge : halfedges)
        {
            if (!edge->next || !edge->face || !edge->vertex)
                return false;
            if (edge->pair && (!edge->pair->pair || edge->pair->pair != edge.get()))
                return false;
        }
        return true;
    }

    // Get all faces adjacent to a vertex
    std::vector<Face *> get_faces_around_vertex(Vertex *vertex)
    {
        std::vector<Face *> faces;
        HalfEdge *start = vertex->outgoing_halfedge;
        HalfEdge *current = start;
        do
        {
            faces.push_back(current->face);
            current = current->pair->next;
        } while (current != start);
        return faces;
    }

    std::vector<Vertex *> get_vertices_around_face(Face *face)
    {
        std::vector<Vertex *> result;
        if (!face || !face->halfedge)
            return result;

        HalfEdge *start = face->halfedge;
        HalfEdge *current = start;
        do
        {
            result.push_back(current->vertex);
            current = current->next;
        } while (current != start);

        return result;
    }

    std::vector<Vertex *> get_vertices_around_vertex(Vertex *vertex)
    {
        std::vector<Vertex *> result;
        if (!vertex || !vertex->outgoing_halfedge)
            return result;

        HalfEdge *start = vertex->outgoing_halfedge;
        HalfEdge *current = start;
        do
        {
            result.push_back(current->vertex);
            if (!current->pair)
                break;
            current = current->pair->next;
        } while (current != start);

        return result;
    }

    RenderMesh to_rendermesh()
    {
        RenderMesh mesh;

        // First create mapping from Vertex* to index
        std::unordered_map<Vertex *, size_t> vertex_indices;
        for (size_t i = 0; i < vertices.size(); i++)
        {
            vertex_indices[vertices[i].get()] = i;
            mesh.add_vertex(vertices[i]->x, vertices[i]->y, vertices[i]->z);
        }

        // Add faces using the vertex indices
        for (const auto &face : faces)
        {
            std::vector<Vertex *> face_vertices = get_vertices_around_face(face.get());

            // Triangulate face using fan triangulation
            for (size_t i = 1; i < face_vertices.size() - 1; i++)
            {
                mesh.add_face(
                    vertex_indices[face_vertices[0]],
                    vertex_indices[face_vertices[i]],
                    vertex_indices[face_vertices[i + 1]]);
            }
        }

        return mesh;
    }

    static HalfEdgeMesh from_rendermesh(const RenderMesh &render_mesh)
    {
        HalfEdgeMesh hemesh;

        // Create vertices
        std::vector<Vertex *> vertex_ptrs;
        for (const auto &pos : render_mesh.positions)
        {
            vertex_ptrs.push_back(hemesh.add_vertex(pos.x, pos.y, pos.z));
        }

        // Create faces from triangles with reversed winding
        for (size_t i = 0; i < render_mesh.indices.size(); i += 3)
        {
            std::vector<Vertex *> face_vertices = {
                vertex_ptrs[render_mesh.indices[i + 2]], // Reversed order
                vertex_ptrs[render_mesh.indices[i + 1]],
                vertex_ptrs[render_mesh.indices[i]]};
            hemesh.add_face(face_vertices);
        }

        return hemesh;
    }

    // Add pair_hash struct at class scope
    struct pair_hash
    {
        template <class T1, class T2>
        std::size_t operator()(const std::pair<T1, T2> &p) const
        {
            auto h1 = std::hash<T1>{}(p.first);
            auto h2 = std::hash<T2>{}(p.second);
            return h1 ^ (h2 << 1);
        }
    };

    HalfEdgeMesh subdivide_loop()
    {
        HalfEdgeMesh new_mesh;
        std::unordered_map<Vertex *, Vertex *> old_to_new_vertices;
        std::unordered_map<std::pair<Vertex *, Vertex *>, Vertex *, pair_hash> edge_to_vertex;

        // Step 1: Calculate new vertex positions (3/8 * old + 5/8 * neighbor average)
        for (const auto &v : vertices)
        {
            auto neighbors = get_vertices_around_vertex(v.get());
            int n = neighbors.size();

            float beta = (n == 3) ? 3.0f / 16.0f : 3.0f / (8.0f * n);
            glm::vec3 sum(0.0f);
            for (auto neighbor : neighbors)
            {
                sum += glm::vec3(neighbor->x, neighbor->y, neighbor->z);
            }

            glm::vec3 new_pos = glm::vec3(v->x, v->y, v->z) * (1.0f - n * beta) + sum * beta;
            old_to_new_vertices[v.get()] = new_mesh.add_vertex(new_pos.x, new_pos.y, new_pos.z);
        }

        // Step 2: Create edge vertices using the 3/8 - 3/8 - 1/8 - 1/8 rule
        for (const auto &face : faces)
        {
            HalfEdge *start = face->halfedge;
            HalfEdge *current = start;
            do
            {
                Vertex *v0 = current->vertex;
                Vertex *v1 = current->next->vertex;
                auto edge = v0 < v1 ? std::make_pair(v0, v1) : std::make_pair(v1, v0);

                if (edge_to_vertex.find(edge) == edge_to_vertex.end())
                {
                    glm::vec3 edge_point;
                    if (current->pair)
                    {
                        // Interior edge: use 3/8 - 3/8 - 1/8 - 1/8 rule
                        edge_point =
                            glm::vec3(v0->x, v0->y, v0->z) * 0.375f +
                            glm::vec3(v1->x, v1->y, v1->z) * 0.375f +
                            glm::vec3(current->next->next->vertex->x,
                                      current->next->next->vertex->y,
                                      current->next->next->vertex->z) *
                                0.125f +
                            glm::vec3(current->pair->next->next->vertex->x,
                                      current->pair->next->next->vertex->y,
                                      current->pair->next->next->vertex->z) *
                                0.125f;
                    }
                    else
                    {
                        // Boundary edge: simple average
                        edge_point = (glm::vec3(v0->x, v0->y, v0->z) +
                                      glm::vec3(v1->x, v1->y, v1->z)) *
                                     0.5f;
                    }
                    edge_to_vertex[edge] = new_mesh.add_vertex(
                        edge_point.x, edge_point.y, edge_point.z);
                }
                current = current->next;
            } while (current != start);
        }

        // Step 3: Create new faces with correct winding
        for (const auto &face : faces)
        {
            HalfEdge *start = face->halfedge;

            Vertex *v0 = old_to_new_vertices[start->vertex];
            Vertex *v1 = old_to_new_vertices[start->next->vertex];
            Vertex *v2 = old_to_new_vertices[start->next->next->vertex];

            auto e0 = edge_to_vertex[std::make_pair(
                std::min(start->vertex, start->next->vertex),
                std::max(start->vertex, start->next->vertex))];
            auto e1 = edge_to_vertex[std::make_pair(
                std::min(start->next->vertex, start->next->next->vertex),
                std::max(start->next->vertex, start->next->next->vertex))];
            auto e2 = edge_to_vertex[std::make_pair(
                std::min(start->next->next->vertex, start->vertex),
                std::max(start->next->next->vertex, start->vertex))];

            // Create four triangles with consistent winding
            new_mesh.add_face({v0, e0, e2});
            new_mesh.add_face({e0, v1, e1});
            new_mesh.add_face({e2, e1, v2});
            new_mesh.add_face({e0, e1, e2});
        }

        return new_mesh;
    }
};

std::vector<std::pair<glm::vec3, glm::vec3>> get_unique_edges(HalfEdgeMesh& mesh) {
    std::vector<std::pair<glm::vec3, glm::vec3>> edges;
    std::set<std::pair<Vertex*, Vertex*>> visited;
    
    for (const auto& edge : mesh.halfedges) {
        Vertex* v1 = edge->vertex;
        Vertex* v2 = edge->next->vertex;
        
        // Create ordered pair to avoid duplicates
        auto edge_pair = std::minmax(v1, v2);
        if (visited.insert(edge_pair).second) {
            glm::vec3 p1(v1->x, v1->y, v1->z);
            glm::vec3 p2(v2->x, v2->y, v2->z);
            edges.push_back({p1, p2});
        }
    }
    return edges;
}

RenderMesh create_edge_cylinder(const glm::vec3& start, const glm::vec3& end, float radius, int segments) {

    // // Calculate transformation
    // glm::vec3 dir = end - start;
    // float length = glm::length(dir);
    // glm::vec3 mid = 0.5f * (start + end);
    // RenderMesh cylinder = RenderMesh::cylinder(segments);

    // // Move cylinder edge start.
    // glm::mat4 transform;

    // // Get direction we need to move the veritices to.
    // glm::vec3 move_dir = glm::normalize(dir);

    // transform = glm::translate


    // return cylinder;
}


RenderMesh create_pipe_wireframe(HalfEdgeMesh& mesh, float radius = 0.02f, int segments = 8) {
    RenderMesh result;
    auto edges = get_unique_edges(mesh);
    
    for (const auto& edge : edges) {
        RenderMesh pipe = create_edge_cylinder(edge.first, edge.second, radius, segments);
        
        // Merge pipe into result mesh
        size_t base_idx = result.positions.size();
        result.positions.insert(result.positions.end(), 
                              pipe.positions.begin(), 
                              pipe.positions.end());
                              
        for (size_t i = 0; i < pipe.indices.size(); i += 3) {
            result.add_face(base_idx + pipe.indices[i],
                          base_idx + pipe.indices[i+1],
                          base_idx + pipe.indices[i+2]);
        }
    }
    
    return result;
}