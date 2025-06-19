#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include "Shader.h"
#include "Camera.h"
#include "Model.h"
#include "Sounds.h"
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "Skybox.h"
#include <filesystem>
#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"


// Prototipos de Funciones
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void processInput(GLFWwindow *window);


const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

// Cámara
Camera camera(glm::vec3(0.0f, 5.0f, 15.0f));
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;
bool cursorEnabled = false;
bool showMainMenu = true; // aqui se muestra el menú principal al inicio

// Sonido
float volumenCity = 1.0f; // 1.0 = volumen de la musica
ALuint sourceCity;        // musica reproducida
ALuint buffer;

// Interfaz (ImGui)
bool mostrarConfiguracion = false; // para el submenú de configuración
bool mostrarCreditos = false;
float brilloJuego = 1.0f;   // Para controlar el brillo
int selectedMenuOption = 0;  // 0: Iniciar Viaje, 1: Configuración, 2: Créditos, 3: Salir

// Fuentes ImGui
ImFont* bigTitleFont = nullptr; // Puntero para la fuente del título

// Tiempo
float deltaTime = 0.0f;
float lastFrame = 0.0f;

// Variables de la escena
glm::vec3 lightPos(10.0f, 20.0f, 10.0f);
glm::vec3 lightColor(1.0f, 1.0f, 1.0f);
float escalaModelo = 1.0f;
glm::vec3 meteoroPos(0.0f, 2.0f, -5.0f);
float meteoroScale = 0.1f;
float meteoroRotY = 0.0f;


void processInput(GLFWwindow *window)
{
    ImGuiIO& io = ImGui::GetIO();

    static bool tabPressedLastFrame = false;

    // Lógica para abrir/cerrar el menú principal con TAB
    if (glfwGetKey(window, GLFW_KEY_TAB) == GLFW_PRESS) {
        if (!tabPressedLastFrame) {
            showMainMenu = !showMainMenu;
            cursorEnabled = showMainMenu;
            glfwSetInputMode(window, GLFW_CURSOR, showMainMenu ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_DISABLED);
            firstMouse = true;
            if (showMainMenu) {
                selectedMenuOption = 0;
                mostrarConfiguracion = false;
                mostrarCreditos = false;
            }
        }
        tabPressedLastFrame = true;
    } else {
        tabPressedLastFrame = false;
    }

    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);


    if (showMainMenu && !io.WantCaptureKeyboard) {
        static bool upPressedLastFrame = false;
        static bool downPressedLastFrame = false;
        static bool enterPressedLastFrame = false;

        // Flecha Arriba
        if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) {
            if (!upPressedLastFrame) {
                selectedMenuOption--;
                if (selectedMenuOption < 0) {
                    selectedMenuOption = 3;
                }
            }
            upPressedLastFrame = true;
        } else {
            upPressedLastFrame = false;
        }

        // Flecha Abajo
        if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) {
            if (!downPressedLastFrame) {
                selectedMenuOption++;

                if (selectedMenuOption > 3) {
                    selectedMenuOption = 0;
                }
            }
            downPressedLastFrame = true;
        } else {
            downPressedLastFrame = false;
        }

        // Tecla Enter
        if (glfwGetKey(window, GLFW_KEY_ENTER) == GLFW_PRESS) {
            if (!enterPressedLastFrame) {
                switch (selectedMenuOption) {
                    case 0: // Iniciar Viaje
                        std::cout << "¡Viaje iniciado desde teclado!\n";
                        showMainMenu = false;
                        cursorEnabled = false;
                        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
                        // Asegúrate de que los submenús estén cerrados al iniciar el juego
                        mostrarConfiguracion = false;
                        mostrarCreditos = false;
                        break;
                    case 1: // Configuración
                        mostrarConfiguracion = !mostrarConfiguracion;
                        mostrarCreditos = false;
                        break;
                    case 2: // Créditos
                        mostrarCreditos = !mostrarCreditos;
                        mostrarConfiguracion = false;
                        break;
                    case 3: // Salir
                        glfwSetWindowShouldClose(window, true);
                        break;
                }
            }
            enterPressedLastFrame = true;
        } else {
            enterPressedLastFrame = false;
        }
    }

    if (!showMainMenu && !io.WantCaptureMouse) {
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
            camera.ProcessKeyboard(FORWARD, deltaTime);
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
            camera.ProcessKeyboard(BACKWARD, deltaTime);
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
            camera.ProcessKeyboard(LEFT, deltaTime);
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
            camera.ProcessKeyboard(RIGHT, deltaTime);
    }
}


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

    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;


    ImFontConfig defaultFontConfig;
    defaultFontConfig.SizePixels = 20.0f;
    io.Fonts->AddFontDefault(&defaultFontConfig);


    ImFontConfig titleFontConfig;
    titleFontConfig.SizePixels = 60.0f;
    bigTitleFont = io.Fonts->AddFontDefault(&titleFontConfig);


    ImGui::StyleColorsDark();

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");


    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetInputMode(window, GLFW_CURSOR, showMainMenu ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_DISABLED);


    glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

    std::cout << "Current working directory: " << std::filesystem::current_path() << std::endl;

    glEnable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);

    Shader shader("Shaders/model.vert", "Shaders/model.frag");

    // Skybox
    std::vector<std::string> faces = {
        "Modelos/skybox/haze_rt.jpg",
        "Modelos/skybox/haze_lf.jpg",
        "Modelos/skybox/haze_up.jpg",
        "Modelos/skybox/haze_dn.jpg",
        "Modelos/skybox/haze_ft.jpg",
        "Modelos/skybox/haze_bk.jpg"
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
    } else {
        std::cerr << "Advertencia: No se pudo cargar Sounds/city.wav\n";
    }

    while (!glfwWindowShouldClose(window)) {
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // Comenzar nuevo frame ImGui
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        processInput(window); // Llama a processInput después de ImGui::NewFrame()

        // Si el menú principal está activo
        if (showMainMenu) {
            // Establecer la ventana de ImGui a pantalla completa para el menú principal
            ImGui::SetNextWindowPos(ImVec2(0, 0));
            ImGui::SetNextWindowSize(ImVec2(SCR_WIDTH, SCR_HEIGHT));

            ImGui::Begin("Menu Principal", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize |
                         ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar |
                         ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings |
                         ImGuiWindowFlags_NoBringToFrontOnFocus);

            // Mostrar el título del juego como texto grande
            const char* gameTitle = "SpeedTitans";
            ImVec2 textSize;


            if (bigTitleFont) {
                ImGui::PushFont(bigTitleFont);
                textSize = ImGui::CalcTextSize(gameTitle);
                ImGui::PopFont(); // Restaurar la fuente por defecto después de calcular el tamaño
            } else {
                // Fallback si la fuente grande no se cargó, usa la fuente por defecto actual
                textSize = ImGui::CalcTextSize(gameTitle);
            }

            // Cálculo del espaciado para centrar verticalmente el contenido (Título + Botones)
            float buttonHeight = 50.0f;
            float buttonSpacing = 20.0f;
            float totalButtonHeight = (buttonHeight + buttonSpacing) * 4.0f; // 4 botones
            float spacingBetweenTitleAndButtons = 50.0f; // Espacio deseado entre el título y el primer botón
            float totalContentHeight = textSize.y + spacingBetweenTitleAndButtons + totalButtonHeight;

            ImGui::Dummy(ImVec2(0, (SCR_HEIGHT - totalContentHeight) * 0.5f));

            // Centrar horizontalmente el texto del título
            ImGui::SetCursorPosX((SCR_WIDTH - textSize.x) * 0.5f);

            // Empujar la fuente grande solo para el texto "SpeedTitans"
            if (bigTitleFont) {
                ImGui::PushFont(bigTitleFont);
            }
            ImGui::Text("%s", gameTitle); // Aquí se dibuja el título con la fuente grande
            if (bigTitleFont) {
                ImGui::PopFont(); // Quitar la fuente grande para que el resto del menú use la fuente por defecto
            }
            ImGui::Spacing(); // Espacio después del título
            ImGui::Spacing(); // Más espacio para que no esté pegado a los botones

            // Tamaño y centrado de los botones
            float buttonWidth = 300.0f;
            float buttonXPos = (SCR_WIDTH - buttonWidth) * 0.5f;

            // --- Botón "Iniciar Viaje"
            ImGui::SetCursorPosX(buttonXPos);
            // Aplicar estilo de resaltado si está seleccionado
            if (selectedMenuOption == 0) {
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.5f, 0.8f, 1.0f)); // Color de resaltado
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.2f, 0.6f, 0.9f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.1f, 0.4f, 0.7f, 1.0f));
            }
            if (ImGui::Button("Iniciar Viaje", ImVec2(buttonWidth, buttonHeight))) {
                std::cout << "¡Viaje iniciado!\n";
                showMainMenu = false;
                cursorEnabled = false;
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
                mostrarConfiguracion = false; // Asegurar que submenús se cierren
                mostrarCreditos = false;      // Asegurar que el submenú de créditos se cierre
            }
            if (selectedMenuOption == 0) {
                ImGui::PopStyleColor(3); // Quitar los estilos aplicados
            }
            ImGui::Spacing(); // Espacio entre botones

            // --- Botón "Configuración" ---
            ImGui::SetCursorPosX(buttonXPos);
            if (selectedMenuOption == 1) {
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.5f, 0.8f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.2f, 0.6f, 0.9f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.1f, 0.4f, 0.7f, 1.0f));
            }
            if (ImGui::Button("Configuración", ImVec2(buttonWidth, buttonHeight))) {
                mostrarConfiguracion = !mostrarConfiguracion;
                mostrarCreditos = false; // Si abro config, cierro créditos
            }
            if (selectedMenuOption == 1) {
                ImGui::PopStyleColor(3);
            }

            // Submenú de Configuración (aparece debajo del botón si está activo)
            if (mostrarConfiguracion) {
                ImGui::Separator();
                ImGui::SetCursorPosX(buttonXPos + 20); // Un poco de indentación
                ImGui::Text("Ajustes de Configuración");
                ImGui::SetCursorPosX(buttonXPos + 20);
                if (ImGui::SliderFloat("Brillo", &brilloJuego, 0.1f, 2.0f, "Brillo: %.1f")) {
                    lightColor = glm::vec3(brilloJuego, brilloJuego, brilloJuego);
                }
                ImGui::SetCursorPosX(buttonXPos + 20);
                if (ImGui::SliderFloat("Volumen Música", &volumenCity, 0.0f, 1.0f, "Volumen: %.1f")) {
                    alSourcef(sourceCity, AL_GAIN, volumenCity);
                }
                ImGui::Separator();
            }
            ImGui::Spacing(); // Espacio después de config o su slider

            // --- Botón "Créditos" ---
            ImGui::SetCursorPosX(buttonXPos);
            if (selectedMenuOption == 2) {
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.5f, 0.8f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.2f, 0.6f, 0.9f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.1f, 0.4f, 0.7f, 1.0f));
            }
            if (ImGui::Button("Créditos", ImVec2(buttonWidth, buttonHeight))) {
                mostrarCreditos = !mostrarCreditos; // Alternar visibilidad del submenú de créditos
                mostrarConfiguracion = false; // Si abro créditos, cierro config
            }
            if (selectedMenuOption == 2) {
                ImGui::PopStyleColor(3);
            }

            // Contenido de Créditos como submenú, similar a Configuración
            if (mostrarCreditos) {
                ImGui::Separator();
                ImGui::SetCursorPosX(buttonXPos + 20); // Un poco de indentación
                ImGui::Text("--- Creditos del Juego ---");
                ImGui::SetCursorPosX(buttonXPos + 20);
                ImGui::Separator();
                ImGui::SetCursorPosX(buttonXPos + 20);
                ImGui::Spacing();

                ImGui::SetCursorPosX(buttonXPos + 20);
                ImGui::Text("Estudiante:");
                ImGui::SetCursorPosX(buttonXPos + 40); ImGui::BulletText("Nombre: Br.Romero Filipone German Antonio");
                ImGui::SetCursorPosX(buttonXPos + 40); ImGui::BulletText("Carnet: 2023-0684U");
                ImGui::Spacing();

                ImGui::SetCursorPosX(buttonXPos + 20);
                ImGui::Text("Estudiante:");
                ImGui::SetCursorPosX(buttonXPos + 40); ImGui::BulletText("Nombre: Br.Lopez Perez Cristian Eduardo");
                ImGui::SetCursorPosX(buttonXPos + 40); ImGui::BulletText("Carnet: 2023-0621U");
                ImGui::Spacing();

                ImGui::SetCursorPosX(buttonXPos + 20); ImGui::Text("Grupo: 3T4-COM");
                ImGui::SetCursorPosX(buttonXPos + 20); ImGui::Text("Clase: Programación Gráfica");


                ImGui::Spacing();
                ImGui::Separator();
            }
            ImGui::Spacing(); // Espacio después de créditos o su submenú

            // --- Botón "Salir" ---
            ImGui::SetCursorPosX(buttonXPos);
            if (selectedMenuOption == 3) {
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.5f, 0.8f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.2f, 0.6f, 0.9f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.1f, 0.4f, 0.7f, 1.0f));
            }
            if (ImGui::Button("Salir", ImVec2(buttonWidth, buttonHeight))) {
                glfwSetWindowShouldClose(window, true);
            }
            if (selectedMenuOption == 3) {
                ImGui::PopStyleColor(3);
            }

            ImGui::End();
        }


        // Si el menú está activo, limpiar el fondo con un color sólido para que no se vea la escena 3D
        if (showMainMenu) {
            glClearColor(0.0f, 0.0f, 0.0f, 1.0f); // Fondo negro cuando el menú está activo
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        } else {
            // Si el menú NO está activo, renderizar la escena 3D
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
        }


        // Render de ImGui
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());


        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // Limpieza de ImGui
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    // Limpieza de recursos de OpenAL
    if (sourceCity) alDeleteSources(1, &sourceCity);
    if (buffer) alDeleteBuffers(1, &buffer);
    // Nota: Si Sounds.h tiene una inicialización global de OpenAL, deberías llamar a alcDestroyContext y alcCloseDevice.
    // Esto dependerá de la implementación de tu librería Sounds.h.

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
    // Obtener el estado de ImGuiIO para ver si ImGui está capturando el ratón
    ImGuiIO& io = ImGui::GetIO();

    // Solo procesar el movimiento del ratón si el cursor no está habilitado
    if (cursorEnabled || io.WantCaptureMouse) {
        firstMouse = true; // Reiniciar para cuando la cámara se active de nuevo
        return; // No procesar movimiento de cámara
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