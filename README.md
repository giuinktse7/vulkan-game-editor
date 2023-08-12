# Vulkan Game Editor

A game editor written in C++20 using Vulkan and the QT framework. Uses QML for the UI.

---

## Features

TODO

## Dependencies

-   [**Vulkan SDK**](https://vulkan.lunarg.com/) Low-level 3D graphics and computing API.
-   [**Qt6 (`Qt::Core`, `Qt::Qml`, `Qt::Quick` and`Qt::Svg`)**](https://www.qt.io/download-open-source) Cross-plaftorm GUI framework.

    **NOTE**: The QT framework requires around 8 GB of disk space per target architecture (For example `msvc2019` or `msvc2019_64`).

-   [**vcpkg**](https://github.com/microsoft/vcpkg) C++ package manager.
-   The required libraries can be installed using vcpkg:

    ```sh
    vcpkg install liblzma protobuf nlohmann-json stb pugixml glm catch2 nano-signal-slot lua date
    ```

    **Note**: This command varies depending on the triplet you want to compile against.
    For instance, for compiling against x64-windows, the command would be

    ```sh
    vcpkg install liblzma:x64-windows protobuf:x64-windows nlohmann-json:x64-windows stb:x64-windows pugixml:x64-windows glm:x64-windows catch2:x64-windows nano-signal-slot:x64-windows lua:x64-windows date:x64-windows
    ```

## Building (Windows)

1. To build the project, first install the required [Dependencies](#dependencies).

2. Set the environment variable `CMAKE_PREFIX_PATH` to the cmake path of your QT installation:

```
<your_installation_path>\Qt6.5\6.5.0\msvc2019_64\lib\cmake
```
3. Add the QT .dlls to the `PATH` environment variable:
```
<your_installation_path>\Qt6.5\6.5.0\msvc2019_64\bin
```


### Building with Visual Studio 2022 (Recommended)

1. Download and install [Visual Studio 2022 Preview (Community)](https://visualstudio.microsoft.com/vs/).
2. Follow steps 1.3 in [Building with CMake](#Building-with-CMake).
3. Open `./build/VulkanGameEditor.sln` with Visual Studio.
4. If necessary, update C++ version to `Preview - Features from the Latest C++ Working Draft (/std:c++latest)`. This can be done by right clicking a project in the solution explorer -> Properties -> Configuration Properties -> General. This must be done for each project in the solution.
5. Run.

Build configurations can be changed under `Project -> CMake Settings for VulkanGameEditor`.

## Generating c++ files from .proto files

-   Download [Protod](https://github.com/sysdream/Protod)
-   Get client.exe from tibia path `<tibia_path>\packages\Tibia\bin\client.exe`
-   Run `python27 ./protod.py client.exe` to generate .proto files and place them in `./in`.
-   Add `syntax = "proto2";` to top of each file .proto file.
-   Then:

    -   `protoc -I=./in --cpp_out=./out ./in/appearances.proto`
    -   `protoc -I=./in --cpp_out=./out ./in/map.proto`
    -   `protoc -I=./in --cpp_out=./out ./in/shared.proto`

    Or:

    -   `protoc -I=./in --cpp_out=./out ./in/appearances.proto && protoc -I=./in --cpp_out=./out ./in/map.proto && protoc -I=./in --cpp_out=./out ./in/shared.proto`

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
    # Use the Visual Studio 17 2022 makefile generator, targeting the x64 platform with the ClangCL compiler.
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

#### CMake Targets

There are four targets:
TODO

## Clang

If you want to use `clang` or `clang-format`, you can get it here: `https://releases.llvm.org/download.html`.

## Terminology

## Investigate build performance

[VS Build Insights](https://devblogs.microsoft.com/cppblog/introducing-c-build-insights/)
