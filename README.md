# Speed Titans:

## Summary
**Speed Titans** is a **3D virtual** tour peoyect that immerses users in a dynamic urban enviroment. Developed whith **OpenGl** and **C++**, the project includes interactive camera movement, realistic models and ambient sound, offering an exceptional Visual experience.
## Main Features

* The project presents a 3D environment of city that allows user interaction, offering an immersive exploration experience.
* It uses 3D model loading for the city and vehicle, a skybox to create an immersive visual environment, and integrate looping background audio to enrich the user experience.

## Keywords
OpenGL, c++, 3D Graphics, City Simulation, Visual Tour, GLFW, GLAD, OpenAL.

## Installation

1. **Install CMake**
 This project uses [CMake](https://cmake.org/) to manage the build process and dependencies. Core libraries (such as GLFW, GLAD, GLM, Assimp, Imgui and OpenAL)

 2. **Install C++ Compiler**
 Installs a modern C++ compiler. We recommend [MinGW-w64](https://www.mingw-w64.org/) (avaible through) [MSYS2](https://www.msys2.org/) for easy setup or the [Visual Studio](https://visualstudio.microsoft.com/) C++ toolset.

 ### Windows

 **Installation and construction steps:**

 1. **Clone the repository:** Open your terminal (CMD or Git Bash) and run:

 ```bash
 git clone https://github.com/Romero56/Speed-Titans.git
 ```
2. ** DLLs (Dynamic Libraries):** Download the DLLs folder from the link (place it in your project root folder) that contains all the dynamic libraries (.dll) needed for GLFW, Assimp, and OpenAL. These DLLs are essential for the executable to run on windows.

```bash
https://www.mediafire.com/file/3a71s9k6qrwa0hn/DLLs.rar/file
```

3. **Create a build directory and compile:**
```bash
mkdir build
cd build
cmake ..
cmake --build.
```
These steps will set up the project and compile it, generating the `SpeedTitans.exe` executable and copying the necessary files.


### Linux
**Installation and construction steps:**

1. **Clone the repository:** Open your terminal (CMD or Git Bash) and run:

 ```bash
 git clone https://github.com/Romero56/Speed-Titans.git
 ```

 2. **Navigate to the project directory:**
 ```bash
 cd Speed-Titans
 ```

 3. **Create a build directory and compile:**
```bash
mkdir build
cd build
cmake ..
cmake --build.
```
This will compile the `Speed Titans` executable into the build directory.

### Testing

After a successful build, the `SpeedTitans` executable (or `SpeedTitans.exe` on Windows) will be found in the build output directory (usually `build/Release` or `build`).

**Make sure the `shaders/`, `Models/` and `sounds/` folders (and `DLLs/` on windows) are located in the same directory as the executable. **The CMake build process should automatically copy these folders to the correct location.

When run, the program should:

* **Load a complete 3D city**.
* **Play background music in a loop**.
* **Allow switching between free camera mode (key `1`) and driving mode (key `2`)**.
* **Display a main menu on start your ride, access settings, or view the credits**.
* **Enter the menu with the `Tab` key, exit the menu with the same key, or press `Enter` to Start Adventure.**.

### Youtube video

[Test video](https://youtu.be/41Mx0lsdEUE?feature=shared) 

### License

Our project is licensed under the [MIT](https://github.com/Romero56/Speed-Titans/blob/main/LICENSE) read for more information.
