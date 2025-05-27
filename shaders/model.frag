#version 330 core

in vec3 FragPos;
in vec2 TexCoords;
in vec3 Normal;


out vec4 FragColor;

uniform sampler2D texture_diffuse1;

uniform vec3 lightPos;
uniform vec3 viewPos;
uniform vec3 lightColor;

void main()
{
    vec3 albedo = texture(texture_diffuse1, TexCoords).rgb;

    // Iluminación principal
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);

    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * albedo * lightColor;

    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 16.0);
    vec3 specular = 0.3 * spec * lightColor;

    vec3 ambient = 0.4 * albedo + vec3(0.05); // luz ambiente fuerte

    // 🔹 Luz secundaria desde arriba (opcional)
    vec3 lightPos2 = vec3(0.0, 10.0, 0.0);
    vec3 lightDir2 = normalize(lightPos2 - FragPos);
    float diff2 = max(dot(norm, lightDir2), 0.0);
    vec3 diffuse2 = diff2 * albedo * vec3(1.0); // luz blanca auxiliar

    // Resultado total
    vec3 color = ambient + diffuse + diffuse2 + specular;
    FragColor = vec4(color, 1.0);
}
