cmake_minimum_required(VERSION 3.10)
project(SpeedTitans)

set(CMAKE_CXX_STANDARD 20)

# Compilar assimp desde la carpeta ya configurada
add_subdirectory(Libs/assimp)

# Executable
add_executable(${PROJECT_NAME}
        main.cpp
        Libs/Mesh.cpp
        Libs/glad/glad.c
        Libs/Model.h
        Libs/Model.cpp
        Libs/stb/stb_image.cpp
)

target_include_directories(${PROJECT_NAME} PRIVATE
        ${CMAKE_SOURCE_DIR}/Libs
        ${CMAKE_SOURCE_DIR}/Libs/glad/include
        ${CMAKE_SOURCE_DIR}/Libs/GLFW
        ${CMAKE_SOURCE_DIR}/Libs/glm
        ${CMAKE_SOURCE_DIR}/Libs/stb
        ${CMAKE_SOURCE_DIR}/Libs/assimp/include
)

# Link libraries (GLFW, GLAD, ASSIMP, OpenGL)
target_link_libraries(${PROJECT_NAME}
        ${CMAKE_SOURCE_DIR}/Libs/GLFW/libglfw3.a
        assimp
        opengl32
        glu32
        gdi32
        winmm
)
