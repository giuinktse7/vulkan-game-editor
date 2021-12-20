# Vulkan Game Editor

A game editor written in C++20 using Vulkan and the QT framework

---

## Features

TODO

## Dependencies

-   [**Vulkan SDK**](https://vulkan.lunarg.com/) Low-level 3D graphics and computing API.
-   [**Qt6 (`Qt::Core`, `Qt::Widgets`, `Qt::Qml`, `Qt::Quick` and`Qt::Svg`)**](https://www.qt.io/download-open-source) Cross-plaftorm GUI framework.

    **NOTE**: The QT framework requires around 8 GB of disk space per target architecture (For example `msvc2019` or `msvc2019_64`).

-   [**vcpkg**](https://github.com/microsoft/vcpkg) C++ package manager.
-   The required libraries can be installed using vcpkg:

    ```sh
    vcpkg install liblzma protobuf nlohmann-json stb pugixml glm catch2 nano-signal-slot lua
    ```

    **Note**: This command varies depending on the triplet you want to compile against.
    For instance, for compiling against x64-windows, the command would be

    ```sh
    liblzma:x64-windows protobuf:x64-windows nlohmann-json:x64-windows stb:x64-windows pugixml:x64-windows glm:x64-windows catch2:x64-windows nano-signal-slot:x64-windows lua:x64-windows
    ```

## Building (Windows)

To build the project, first install the required [Dependencies](#dependencies).

### Building with Visual Studio 2019 (Recommended)

1. Download and install [Visual Studio 2019 (Community)](https://visualstudio.microsoft.com/vs/).
2. Go to `File->Open->Folder` and open the project root (The folder that contains `CMakeLists.txt`).
3. Set startup project to `main.exe`.
4. Run.

Build configurations can be changed under `Project -> CMake Settings for VulkanGameEditor`.

### Building with CMake

1. Download and install [CMake](https://cmake.org/download/).
2. Empty the `build/` folder.
3. **Environment Variables**

    - Add the Qt6 bin folder to the `PATH` environment variable:

    ```sh
      <your Qt6 install path>/6.1.0/msvc2019_64/bin
    ```

    - Set the environment variable `LIB` to the Windows libpath. Its default path is

        ```sh
        C:/Program Files (x86)/Windows Kits/10/Lib/10.0.19041.0/um/x64
        ```

        Select the last folder based on the architecture you are building for (for example x86 or x64).

    - Set the environment variable `Qt6_CMAKE_FOLDER` to

        ```sh
        <your Qt6 install path>/6.0.0/msvc2019_64/lib/cmake/Qt6
        ```

4. In `./build`, run (for example)

    ```sh
    # Use the Visual Studio 16 2019 makefile generator, targeting the x64 platform with the ClangCL compiler.
    cmake ../ -G "Visual Studio 17 2022" -A x64 [-T ClangCL]
    ```

    **Flags**:

    - -G specify a makefile generator.
    - -A specify platform name if supported by generator.
    - -T Toolset specification for the generator, if supported.

    to generate the build structure.

    **OPTIONAL**: Follow the instructions [here](https://docs.microsoft.com/en-us/cpp/build/clang-support-msbuild?view=vs-2019) to be able to use the `clang` compiler (`-T ClangCL`)

5. In `./build`, run `cmake --build .` to compile a debug build, or `cmake --build . --config release` to compile a release build.

6. Copy over the folders `shaders` and `data` into `./build`.

#### Run tests using CMake

To run the test suite `vme_tests`, run `./runtest` in the project root.

#### CMake Targets

There are four targets:

-   **main** (Executable): The `main` target is the executable of the application. This target links the `common` and `gui` libraries statically.
-   **common** (Library): The `common` target contains all code that **is not** related to GUI (i.e. everything except Qt6-reliant code).
-   **gui** (Library): The `gui` target contains all code that **is** related to GUI (i.e. all code that is Qt6-reliant).

-   **vme_tests** (Executable): Contains tests for the `common` and `gui` libraries (See [Run tests using CMake](#run-tests-using-cmake)).

The main purpose of having the `common` and `gui` library separate from the `main` target was to enable running unit tests against the code. It also ensures that there is no coupling introduced between core editor functionality and Qt6 (`common` does not have Qt6 as a dependency).

## Terminology

### Action

An action is an event that can occur and can be undone/redone.

## Investigate build performance

[VS Build Insights](https://devblogs.microsoft.com/cppblog/introducing-c-build-insights/)
