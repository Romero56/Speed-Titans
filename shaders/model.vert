#version 330 core

// Atributos de vértices
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;
layout (location = 3) in vec3 aTangent;
layout (location = 4) in vec3 aBitangent;

// Salidas al fragment shader
out vec3 FragPos;
out vec2 TexCoords;
out mat3 TBN;

// Uniformes de transformación
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    // Posición del fragmento en espacio mundo
    vec3 fragWorldPos = vec3(model * vec4(aPos, 1.0));
    FragPos = fragWorldPos;

    // Coordenadas de textura
    TexCoords = aTexCoords;

    // Transformar las bases tangente/bitangente/normal al espacio mundo
    vec3 T = normalize(mat3(model) * aTangent);
    vec3 B = normalize(mat3(model) * aBitangent);
    vec3 N = normalize(mat3(model) * aNormal);
    TBN = mat3(T, B, N);

    // Transformación final a clip space
    gl_Position = projection * view * vec4(fragWorldPos, 1.0);
}
