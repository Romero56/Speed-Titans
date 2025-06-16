#pragma once

#include <vector>
#include <string>
#include <glad/glad.h>
#include <glm/glm.hpp>
#include "Shader.h"

class Skybox
{
public:
    Skybox(const std::vector<std::string>& faces);
    ~Skybox();

    void Draw(const glm::mat4& view, const glm::mat4& projection);

private:
    GLuint loadCubemap(const std::vector<std::string>& faces);
    GLuint VAO, VBO;
    GLuint cubemapTexture;
    Shader shader;

    void setupSkybox();
};