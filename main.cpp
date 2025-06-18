#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include "Shader.h"
#include "Camera.h"
#include "Model.h"
#include "Sounds.h"
#include "stb_image.h"
#include <gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "Skybox.h"
#include <filesystem>
#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"


void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);

const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

// cámara
Camera camera(glm::vec3(0.0f, 5.0f, 15.0f));
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;
bool cursorEnabled = false;
bool showMainMenu = false;
float volumenCity = 1.0f; // 1.0 = volumen completo
ALuint sourceCity;        // fuente de audio reproducida
ALuint buffer;

// Interfaz
bool mostrarVolumen = false;
bool mostrarCreditos = false;





// tiempo
float deltaTime = 0.0f;
float lastFrame = 0.0f;

void processInput(GLFWwindow *window)
{

    // Dentro de la versión original de processInput()

    static bool tabPressedLastFrame = false;

    if (glfwGetKey(window, GLFW_KEY_TAB) == GLFW_PRESS) {
        if (!tabPressedLastFrame) {
            showMainMenu = !showMainMenu;
            cursorEnabled = showMainMenu;
            glfwSetInputMode(window, GLFW_CURSOR, showMainMenu ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_DISABLED);

            firstMouse = true;

        }
        tabPressedLastFrame = true;
    } else {
        tabPressedLastFrame = false;
    }

    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    float cameraSpeed = 6.0f * deltaTime;

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera.ProcessKeyboard(RIGHT, deltaTime);

}


glm::vec3 meteoroPos(0.0f, 2.0f, -5.0f);  // Posición inicial
float meteoroScale = 0.1f;               // Escala inicial
float meteoroRotY = 0.0f;                // Rotación en eje Y

int main() {
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef _APPLE_
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Model Viewer", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Error al crear ventana GLFW" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    if (!window) {
        std::cerr << "GLFW window is null!" << std::endl;
        return -1;
    }

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Error al inicializar GLAD" << std::endl;
        return -1;
    }

    // Inicializar Dear ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;

    ImGui::StyleColorsDark(); // Podés cambiar a Light si querés

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");

    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);




    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Error al inicializar GLAD" << std::endl;
        return -1;
    }

    // AÑADIDO: Activa el filtrado de cubemap sin fisuras para evitar líneas en el skybox
    glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

    std::cout << "Current working directory: " << std::filesystem::current_path() << std::endl;

    glEnable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);

    Shader shader("Shaders/model.vert", "Shaders/model.frag");

    // Skybox
    std::vector<std::string> faces = {
        "Modelos/skybox/valley_rt.jpg.jpg",
        "Modelos/skybox/valley_lf.jpg.jpg",
        "Modelos/skybox/valley_up.jpg.jpg",
        "Modelos/skybox/valley_dn.jpg.jpg",
        "Modelos/skybox/valley_ft.jpg.jpg",
        "Modelos/skybox/valley_bk.jpg.jpg"
    };
    Skybox skybox(faces);

    //cargar modelo
    Model ciudad("Modelos/ciudad/scene.gltf");

    Model Meteoro("Modelos/meteoro/scene.gltf");


    //Sonido
    if (LoadWavFile("Sounds/city.wav", buffer)) {
        alGenSources(1, &sourceCity);
        alSourcei(sourceCity, AL_BUFFER, buffer);
        alSourcef(sourceCity, AL_GAIN, volumenCity);
        alSourcei(sourceCity, AL_LOOPING, AL_TRUE); // opcional: repetir sonido
        alSourcePlay(sourceCity);
    }


    glm::vec3 lightPos(10.0f, 20.0f, 10.0f);
    glm::vec3 lightColor(1.0f, 1.0f, 1.0f);

    float escalaModelo = 1.0f;

    while (!glfwWindowShouldClose(window)) {
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        processInput(window);
        // Comenzar nuevo frame ImGui
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        if (showMainMenu) {
            ImGui::Begin("Menu Principal");

            // ----------- CARROS -------------
            if (ImGui::Button("Carros")) {
                std::cout << "Seleccionar carros\n";
            }

            // ----------- VOLUMEN ------------
            if (ImGui::Button("Volumen")) {
                mostrarVolumen = !mostrarVolumen;
            }

            if (mostrarVolumen) {
                if (ImGui::SliderFloat("Volumen", &volumenCity, 0.0f, 1.0f)) {
                    alSourcef(sourceCity, AL_GAIN, volumenCity);
                }
            }

            // ----------- CREDITOS -----------
            if (ImGui::Button("Créditos")) {
                mostrarCreditos = true;
            }

            // Mostrar popup después del frame actual
            if (mostrarCreditos) {
                ImGui::OpenPopup("Creditos");
                mostrarCreditos = false;
            }

            if (ImGui::BeginPopupModal("Creditos", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
                ImGui::Text("🎮 Speed Titans");
                ImGui::Text("Creado por ING.LOPEZ y ING.ROMERO");
                ImGui::Separator();
                if (ImGui::Button("Cerrar")) {
                    ImGui::CloseCurrentPopup();
                }
                ImGui::EndPopup();
            }

            // ----------- CERRAR MENÚ -----------
            if (ImGui::Button("Cerrar menú")) {
                showMainMenu = false;
                cursorEnabled = false;
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            }

            ImGui::End(); // <-- CIERRA ImGui::Begin("Menu Principal")
        }




        glClearColor(0.8f, 0.8f, 0.8f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glm::mat4 projection = glm::perspective(glm::radians(camera.GetZoom()), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
        glm::mat4 view = camera.GetViewMatrix();

        // SKYBOX primero
        skybox.Draw(view, projection);

        // MODELO
        shader.Use();

        glm::mat4 modelMatrix = glm::mat4(1.0f);

       //probar para centrar el mapa
        modelMatrix = glm::scale(modelMatrix, glm::vec3(escalaModelo));

        modelMatrix = glm::rotate(modelMatrix, glm::radians(75.0f), glm::vec3(30.0f, -5.0f, -2.0f)); // Rota 90 grados alrededor del eje Y
        modelMatrix = glm::translate(modelMatrix, glm::vec3(0.0f, -3.0f, 0.0f)); // Baja el modelo 3 unidades en Y

        glUniformMatrix4fv(glGetUniformLocation(shader.Program, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
        glUniformMatrix4fv(glGetUniformLocation(shader.Program, "view"), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(shader.Program, "model"), 1, GL_FALSE, glm::value_ptr(modelMatrix));

        glUniform3fv(glGetUniformLocation(shader.Program, "lightPos"), 1, glm::value_ptr(lightPos));
        glUniform3fv(glGetUniformLocation(shader.Program, "lightColor"), 1, glm::value_ptr(lightColor));
        glUniform3fv(glGetUniformLocation(shader.Program, "viewPos"), 1, glm::value_ptr(camera.GetPosition()));

        ciudad.Draw(shader.Program);

        // DIBUJAR METEORO
        glm::mat4 meteoroMatrix = glm::mat4(1.0f);
        meteoroMatrix = glm::translate(meteoroMatrix, glm::vec3(0.0f, 2.0f, -5.0f)); // cambia posición si querés
        meteoroMatrix = glm::scale(meteoroMatrix, glm::vec3(0.1f)); // ajusta escala

        glUniformMatrix4fv(glGetUniformLocation(shader.Program, "model"), 1, GL_FALSE, glm::value_ptr(meteoroMatrix));
        Meteoro.Draw(shader.Program);


        // Render de ImGui
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());


        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}


// redimensionamiento ventana
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
    if (cursorEnabled) {
        return; // Si el cursor está activo, no mover la cámara
    }

    if (firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos;

    lastX = xpos;
    lastY = ypos;

    camera.ProcessMouseMovement(xoffset, yoffset);
}
