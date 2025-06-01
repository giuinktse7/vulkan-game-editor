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
    `C:\ProgramData\Microsoft\Windows\Start Menu\Programs\Visual Studio 2022\Visual Studio Tools\VC`

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
