#define GLM_ENABLE_EXPERIMENTAL

#include "stb_image.h"
#include <iostream>
#include "Model.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/matrix_major_storage.hpp>


Model::Model(const std::string& path) {
    loadModel(path);
}

void Model::Draw(GLuint shaderProgram) {
    for (auto& mesh : meshes) {
        mesh.Draw(shaderProgram);
    }
}
void Model::loadModel(const std::string& path) {
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(path,
        aiProcess_Triangulate |
        aiProcess_FlipUVs |
        aiProcess_CalcTangentSpace |
        aiProcess_GenSmoothNormals |
        aiProcess_JoinIdenticalVertices |
        aiProcess_ImproveCacheLocality |
        aiProcess_RemoveRedundantMaterials |
        aiProcess_FindInvalidData |
        aiProcess_OptimizeMeshes);


    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
        std::cerr << "ERROR::ASSIMP::" << importer.GetErrorString() << std::endl;
        return;
    }

    directory = path.substr(0, path.find_last_of('/'));

    // ✅ Llama a processNode con matriz identidad como transformación raíz
    processNode(scene->mRootNode, scene, glm::mat4(1.0f));
}




void Model::processNode(aiNode* node, const aiScene* scene, glm::mat4 parentTransform)
{
    // 🔹 1. Obtener la transformación local del nodo
    glm::mat4 localTransform = glm::transpose(glm::make_mat4(&node->mTransformation.a1));

    // 🔹 2. Combinar con la transformación acumulada (del padre)
    glm::mat4 globalTransform = parentTransform * localTransform;

    // 🔹 3. Procesar todas las mallas del nodo
    for (unsigned int i = 0; i < node->mNumMeshes; i++) {
        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
        meshes.push_back(processMesh(mesh, scene)); // ← aquí podrías pasar globalTransform más adelante si haces skinning
    }

    // 🔹 4. Recursivamente procesar los nodos hijos
    for (unsigned int i = 0; i < node->mNumChildren; i++) {
        processNode(node->mChildren[i], scene, globalTransform);
    }
}


Mesh Model::processMesh(aiMesh* mesh, const aiScene* scene) {
    std::vector<Vertex> vertices;
    std::vector<GLuint> indices;
    std::vector<Texture> textures;

    for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
        Vertex vertex;
        vertex.Position = glm::vec3(
            mesh->mVertices[i].x,
            mesh->mVertices[i].y,
            mesh->mVertices[i].z
        );

        vertex.Normal = glm::vec3(
            mesh->mNormals[i].x,
            mesh->mNormals[i].y,
            mesh->mNormals[i].z
        );

        if (mesh->mTextureCoords[0]) {
            vertex.TexCoords = glm::vec2(
                mesh->mTextureCoords[0][i].x,
                mesh->mTextureCoords[0][i].y
            );
        } else {
            vertex.TexCoords = glm::vec2(0.0f, 0.0f);
        }

        if (mesh->HasTangentsAndBitangents()) {
            vertex.Tangent = glm::vec3(
                mesh->mTangents[i].x,
                mesh->mTangents[i].y,
                mesh->mTangents[i].z
            );
            vertex.Bitangent = glm::vec3(
                mesh->mBitangents[i].x,
                mesh->mBitangents[i].y,
                mesh->mBitangents[i].z
            );
        } else {
            vertex.Tangent = glm::vec3(0.0f);
            vertex.Bitangent = glm::vec3(0.0f);
        }


        vertices.push_back(vertex);
    }

    for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
        aiFace face = mesh->mFaces[i];
        for (unsigned int j = 0; j < face.mNumIndices; j++)
            indices.push_back(face.mIndices[j]);
    }
    if (mesh->mMaterialIndex >= 0) {
        aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];

        // Diffuse/BaseColor
        std::vector<Texture> diffuseMaps = loadMaterialTextures(material, aiTextureType_DIFFUSE, "texture_diffuse");
        for (const auto& tex : diffuseMaps)
            std::cout << "Malla usa diffuse: " << tex.path << std::endl;
        textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());

        // Specular
        std::vector<Texture> specularMaps = loadMaterialTextures(material, aiTextureType_SPECULAR, "texture_specular");
        for (const auto& tex : specularMaps)
            std::cout << "Malla usa specular: " << tex.path << std::endl;
        textures.insert(textures.end(), specularMaps.begin(), specularMaps.end());

        // Normal Maps
        std::vector<Texture> normalMaps = loadMaterialTextures(material, aiTextureType_NORMALS, "texture_normal");
        for (const auto& tex : normalMaps)
            std::cout << "Malla usa normal map: " << tex.path << std::endl;
        textures.insert(textures.end(), normalMaps.begin(), normalMaps.end());

        // Metallic-Roughness
        std::vector<Texture> metallicMaps = loadMaterialTextures(material, aiTextureType_METALNESS, "texture_metallicRoughness");
        for (const auto& tex : metallicMaps)
            std::cout << "Malla usa metallicRoughness: " << tex.path << std::endl;
        textures.insert(textures.end(), metallicMaps.begin(), metallicMaps.end());

        // Emissive (opcional)
        std::vector<Texture> emissiveMaps = loadMaterialTextures(material, aiTextureType_EMISSIVE, "texture_emissive");
        for (const auto& tex : emissiveMaps)
            std::cout << "Malla usa emissive: " << tex.path << std::endl;
        textures.insert(textures.end(), emissiveMaps.begin(), emissiveMaps.end());
    }



    return Mesh(vertices, indices, textures);
}

std::vector<Texture> Model::loadMaterialTextures(aiMaterial* mat, aiTextureType type, const std::string& typeName) {
    std::vector<Texture> textures;

    for (unsigned int i = 0; i < mat->GetTextureCount(type); i++) {
        aiString str;
        mat->GetTexture(type, i, &str);

        std::string filename = directory + "/" + std::string(str.C_Str());

        GLuint textureID;
        glGenTextures(1, &textureID);

        int width, height, nrComponents;
        unsigned char* data = stbi_load(filename.c_str(), &width, &height, &nrComponents, 0);
        std::cout << "Cargando textura: " << filename << " (" << (data ? "Éxito" : "FALLÓ") << ")" << std::endl;

        if (data) {
            GLenum format;
            if (nrComponents == 1)
                format = GL_RED;
            else if (nrComponents == 3)
                format = GL_RGB;
            else if (nrComponents == 4)
                format = GL_RGBA;

            glBindTexture(GL_TEXTURE_2D, textureID);
            glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
            glGenerateMipmap(GL_TEXTURE_2D);

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

            stbi_image_free(data);

            Texture texture;
            texture.id = textureID;
            texture.type = typeName;
            texture.path = str.C_Str();
            textures.push_back(texture);
        } else {
            std::cerr << "Failed to load texture at path: " << filename << std::endl;
            stbi_image_free(data);
        }
    }

    return textures;
}

