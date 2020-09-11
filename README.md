Vulkan Game Editor - A game editor written in C++17 using Vulkan and the QT framework.
==================================================

## Features
TODO

## Terminology

### Action

An action is an event that can occur and can be undone/redone.

## Dependencies

- [Vulkan SDK](https://vulkan.lunarg.com/) which is a low-level 3D graphics and computing API.
- The GUI framework [QT5](https://www.qt.io/download-open-source) (`Qt::Core` and `Qt::Widgets`).

  **NOTE**: The QT framework requires more than 50 GB of disk space.
- The c++ package manager [vcpkg](https://github.com/microsoft/vcpkg).
- Libraries from vcpkg:
  ```sh
  vcpkg install liblzma protobuf nlohmann-json stb pugixml
  ```
  **Note**: This command varies depending on the triplet you want to compile against.
  For instance, for compiling against x64-windows, the command would be
  ```sh
  vcpkg install liblzma:x64-windows :x64-windows nlohmann-json:x64-windows stb:x64-windows pugixml:x64-windows
  ```

## Building

To build the project, first install the required [Dependencies](#dependencies).

### Building with Visual Studio 2019 (Recommended)

1. Download and install [Visual Studio 2019 (Community)](https://visualstudio.microsoft.com/vs/).
2. Go to `File->Open->Project/Solution` and open `./build/VulkanGameEditor.sln`.
3. Right clck on the project `main` and select `Set as Startup Project`.
4. Right clck on the project `main` and select `Properties` Navigate to `Configuration Properties->General` and select a Platform Toolset (probably `Visual Studio 2019 v142` or similar).
5. Click on `Local Windows Debugger`.

### Compiling with CMake

1. Download and install [CMake](https://cmake.org/download/).
2. Empty the `build/` folder.
3. In `./build`, run (for example)

  ```sh
  # Use the Visual Studio 16 2019 makefile generator, targeting the x64 platform with the ClangCL compiler.
  cmake ../ -G "Visual Studio 16 2019" -A x64 -T ClangCL
  ```

  to generate the build structure.

  - -G specify a makefile generator.
  - -A specify platform name if supported by generator.
  - -T Toolset specification for the generator, if supported.

4. In `./build`, run `cmake --build .` to compile a debug build, or `cmake --build . --config release` to compile a release build.
