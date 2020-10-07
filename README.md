# Vulkan Game Editor - A game editor written in C++17 using Vulkan and the QT framework

## Features

TODO

## Terminology

### Action

An action is an event that can occur and can be undone/redone.

## Dependencies

- [Vulkan SDK](https://vulkan.lunarg.com/): Low-level 3D graphics and computing API.
- [QT5](https://www.qt.io/download-open-source) (`Qt::Core` and `Qt::Widgets`): Cross-plaftorm GUI framework.

  **NOTE**: The QT framework requires around 8 GB of disk space per target architecture (For example `msvc2019` or `msvc2019_64`).

- [vcpkg](https://github.com/microsoft/vcpkg) C++ package manager.
- The required libraries can be installed using vcpkg:

  ```sh
  vcpkg install liblzma protobuf nlohmann-json stb pugixml glm catch2
  ```

  **Note**: This command varies depending on the triplet you want to compile against.
  For instance, for compiling against x64-windows, the command would be

  ```sh
  vcpkg install liblzma:x64-windows :x64-windows nlohmann-json:x64-windows stb:x64-windows pugixml:x64-windows
  ```

## Building (Windows)

To build the project, first install the required [Dependencies](#dependencies).

### Building with Visual Studio 2019 (Recommended)

1. Download and install [Visual Studio 2019 (Community)](https://visualstudio.microsoft.com/vs/).
2. Go to `File->Open->Project/Solution` and open `./build/VulkanGameEditor.sln`.
3. Right click on the project `main` and select `Set as Startup Project`.
4. Right click on the project `main` and select `Properties` Navigate to `Configuration Properties->General` and select a Platform Toolset (probably `Visual Studio 2019 v142` or similar).
5. Click on `Local Windows Debugger`.

### Building with CMake

1. Download and install [CMake](https://cmake.org/download/).
2. Empty the `build/` folder.
3. Set the environment variable `LIB` to the Windows libpath. Its default path is

   ```sh
   C:\Program Files (x86)\Windows Kits\10\Lib\10.0.19041.0\um\x64
   ```

   Select the last folder based on the architecture you are building for (for example x86 or x64).

4. In `./build`, run (for example)

   ```sh
   # Use the Visual Studio 16 2019 makefile generator, targeting the x64 platform with the ClangCL compiler.
   cmake ../ -G "Visual Studio 16 2019" -A x64 [-T ClangCL]
   ```

   **Flags**:

   - -G specify a makefile generator.
   - -A specify platform name if supported by generator.
   - -T Toolset specification for the generator, if supported.

   to generate the build structure.

   **OPTIONAL**: Follow the instructions [here](https://docs.microsoft.com/en-us/cpp/build/clang-support-msbuild?view=vs-2019) to be able to use the `clang` compiler (`-T ClangCL`)

5. In `./build`, run `cmake --build .` to compile a debug build, or `cmake --build . --config release` to compile a release build.

#### Run tests using CMake

To run the test suite `common_tests`, run `./runtest` in the project root.

#### CMake Targets

There are three targets:

- **main** (Executable): The `main` target is the executable of the application. This target links the `common` library as a static dependency.
- **common** (Library): The `common` target contains all code that is not related to GUI (i.e. everything except QT5-reliant code).
- **common_tests** (Executable): Contains the tests for the `common` library (See [Run tests using CMake](#run-tests-using-cmake)).

The main purpose of having the `common` library separate from the `main` target was to enable running unit tests against it. It also ensures that there is no coupling introduced between core editor functionality and QT5 (Only `main` has QT5 as a dependency).
