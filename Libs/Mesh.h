#pragma once

#include <glad.h>
#include <glm.hpp>
#include <gtc/matrix_transform.hpp>

#include <vector>
#include <string>

// Estructuras para almacenar datos de vértices
struct Vertex {
    glm::vec3 Position;
    glm::vec3 Normal;
    glm::vec2 TexCoords;
    glm::vec3 Tangent;
    glm::vec3 Bitangent;
};

struct Texture {
    GLuint id;
    std::string type;
    std::string path;
};

class Mesh {
public:
    // Datos
    std::vector<Vertex> vertices;
    std::vector<GLuint> indices;
    std::vector<Texture> textures;
    GLuint VAO;

    // Constructor
    Mesh(std::vector<Vertex> vertices, std::vector<GLuint> indices, std::vector<Texture> textures);

    // Dibujar el mesh
    void Draw(GLuint shaderProgram);

private:
    // OpenGL buffers
    GLuint VBO, EBO;

    // Inicializar buffers y atributos
    void setupMesh();
};
