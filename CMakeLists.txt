cmake_minimum_required(VERSION 3.10)
project(SpeedTitans)

set(CMAKE_CXX_STANDARD 20)

# ----------------------------------------
# INCLUDES
# ----------------------------------------

include_directories(
        ${CMAKE_SOURCE_DIR}/Libs
        ${CMAKE_SOURCE_DIR}/Libs/assimp
        ${CMAKE_SOURCE_DIR}/Libs/GLFW
        ${CMAKE_SOURCE_DIR}/Libs/AL
        ${CMAKE_SOURCE_DIR}/Libs/glad
        ${CMAKE_SOURCE_DIR}/Libs/glm
        ${CMAKE_SOURCE_DIR}/Libs/stb
        ${CMAKE_SOURCE_DIR}/Libs/imgui
        ${CMAKE_SOURCE_DIR}/Libs/imgui/backends

)

link_directories(
        ${CMAKE_SOURCE_DIR}/Libs/assimp/lib
        ${CMAKE_SOURCE_DIR}/Libs/GLFW/lib
        ${CMAKE_SOURCE_DIR}/Libs/AL/lib
)

# ----------------------------------------
# FUENTES
# ----------------------------------------


add_executable(${PROJECT_NAME}
        main.cpp
        Libs/Mesh.cpp
        Libs/Model.cpp
        Libs/glad/glad.c
        Libs/stb/stb_image.cpp
        Libs/Sounds.cpp
        Libs/skybox.cpp

        Libs/imgui/imgui.cpp
        Libs/imgui/imgui_draw.cpp
        Libs/imgui/imgui_tables.cpp
        Libs/imgui/imgui_widgets.cpp
        Libs/imgui/imgui_demo.cpp

        Libs/imgui/backends/imgui_impl_glfw.cpp
        Libs/imgui/backends/imgui_impl_opengl3.cpp
)


# ----------------------------------------
# LIBRERÍAS A ENLAZAR
# ----------------------------------------

target_link_libraries(${PROJECT_NAME}
        assimp
        glfw3
        openal
        opengl32
        glu32
        gdi32
        winmm
)

# ----------------------------------------
# POST-BUILD: Copiar DLLs automáticamente
# ----------------------------------------

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/DLLs $<TARGET_FILE_DIR:${PROJECT_NAME}>
)

# Copiar automáticamente Modelos
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/Modelos $<TARGET_FILE_DIR:${PROJECT_NAME}>/Modelos
)

# Copiar automáticamente Shaders
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/Shaders $<TARGET_FILE_DIR:${PROJECT_NAME}>/Shaders
)

# Copiar automáticamente Sounds
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/Sounds $<TARGET_FILE_DIR:${PROJECT_NAME}>/Sounds
)

# Texturas del skybox
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/Modelos/Skybox $<TARGET_FILE_DIR:${PROJECT_NAME}>/Modelos/Skybox
)