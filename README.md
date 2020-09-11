# Vulkan Game Editor

A game editor written in C++17 using the QT framework.

## Terminology

### Action

An action is an event that can occur and can be undone/redone.

## Dependencies

- [Vulkan SDK](https://vulkan.lunarg.com/) which is a low-level 3D graphics and computing API.
- The GUI framework [QT5](https://www.qt.io/download-open-source) (`Qt::Core` and `Qt::Widgets`).
- The c++ package manager [vcpkg](https://github.com/microsoft/vcpkg).
- Libraries from vcpkg: `vcpkg install liblzma protobuf nlohmann-json stb pugixml`. (Note: This command varies depending on the triplet you want to compile against.
  For instance, for compiling against x64-windows, the command would be `vcpkg install liblzma:x64-windows :x64-windows nlohmann-json:x64-windows stb:x64-windows pugixml:x64-windows`)

## Compiling

### Compiling with Visual Studio 2019 (Recommended)

- Download and install [Visual Studio 2019 (Community)](https://visualstudio.microsoft.com/vs/).
- Go to `File->Open->Project/Solution` and open `./build/VulkanGameEditor.sln`.
- Right clck on the project `main` and select `Set as Startup Project`.
- Right clck on the project `main` and select `Properties` Navigate to `Configuration Properties->General` and select a Platform Toolset (probably `Visual Studio 2019 v142` or similar).
- Click on `Local Windows Debugger`.

### Compiling with CMake

- Download and install [CMake](https://cmake.org/download/).
- Empty the `build/` folder.
- In `./build`, run

  ```bash
  cmake ../ -G "Visual Studio 16 2019" -A x64 -T ClangCL
  ```

  to generate the build structure.

  - -G specify a makefile generator.
  - -A specify platform name if supported by generator.
  - -T Toolset specification for the generator, if supported.

- In `./build`, run `cmake --build .` to compile a debug build, or `cmake --build . --config release` to compile a release build.
