# Instructions for generating build files

## Windows

### VSCode & Clangd (Recommended)

1. Install the VSCode extensions `clangd` and `CMake`.
2. Uninstall/disable the `C/C++` extension by Microsoft.
3. Generate build files using CMake:
   3a. Export the compile commands & set the generator to Ninja:

    ```json
    {
        "cmake.exportCompileCommandsFile": true,
        "cmake.generator": "Ninja"
    }
    ```

    3b. From the command line, navigate to the `build/ninja` directory and run CMake to generate the build files:
    You MUST run this from a VS terminal, like
    `C:/ProgramData/Microsoft/Windows/Start Menu/Programs/Visual Studio 2022/Visual Studio Tools/VC/x64 Native Tools Command Prompt for VS 2022`

    ```
        cd build/ninja
        cmake -G "Ninja" -DCMAKE_EXPORT_COMPILE_COMMANDS=1 ../../
    ```

    4. Update the `vcpkg` include path in .clangd so that Clangd can find the libraries:

    ```yaml
    CompileFlags:
        Add:
            - -std=c++20
            - -isystem
            - C:/vcpkg/installed/x64-windows/include
    ```

### Visual Studio

From `./build/vs`:
`cmake -G 'Visual Studio 17 2022' -A x64 ../../`

## Build Protobuf

Hopefully we can go back to using the `vcpkg` version of protobuf soon, but for now we must compile it from source.

2025-06-07: The latest protobuf version from `vcpkg` is 29.3. This version has a bug where it cannot be used with `clang-cl`. For that reason, we must compile protobuf from source.
If it is still in the latest version of `protobuf`, apply this patch manually before building:
https://github.com/protocolbuffers/protobuf/issues/19975#issuecomment-2813972928

Instructions: https://github.com/protocolbuffers/protobuf/tree/main/cmake

Build generator commands:

```bash
cmake -B build/release -S . -G "Ninja" -DCMAKE_INSTALL_PREFIX=installed/release -DCMAKE_C_COMPILER=clang-cl -DCMAKE_CXX_COMPILER=clang-cl  -DCMAKE_CXX_STANDARD=20 -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake -Dprotobuf_BUILD_TESTS=OFF -Dprotobuf_BUILD_PROTOC_BINARIES=OFF -Dprotobuf_BUILD_SHARED_LIBS=ON
cmake -B build/debug -DCMAKE_BUILD_TYPE=Debug -S . -G "Ninja" -DCMAKE_INSTALL_PREFIX=installed/debug -DCMAKE_C_COMPILER=clang-cl -DCMAKE_CXX_COMPILER=clang-cl -DCMAKE_CXX_STANDARD=20 -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake -Dprotobuf_BUILD_TESTS=OFF -Dprotobuf_BUILD_PROTOC_BINARIES=OFF -Dprotobuf_BUILD_SHARED_LIBS=ON
```

Build commands:

```bash
cmake --build build/release --config Release
cmake --build build/debug --config Debug
```

Install commands:

```bash
cmake --install build/release
cmake --install build/debug
```

Set the environment variables for the protobuf root directories:

```
PROTOBUF_ROOT_DEBUG=[...]/protobuf/installed/debug
PROTOBUF_ROOT_RELEASE=[...]/protobuf/installed/release
```
