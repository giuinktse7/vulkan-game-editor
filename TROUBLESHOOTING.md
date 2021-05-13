# Compilation issues

## QT Issues

### QT tries to use `experimental/source_location` in `qproperty.h`

In qproperty.h:

- Line 53, replace `#include <experimental/source_location>` with `#include <source_location>`
- Line 100, replace `std::experimental::source_location` with `std::source_location`

## Lua-related issues

### unresolved external symbol in the Lua libraries

At one point, I could not get the Visual Studio program to link anything from lua.h.

**Possible Solution**:
Try generating the building and building from the command line instead. Note: Remove the current build folder before trying this.
The working build used `-T ClangCL` when compiling, as in

```sh
cmake ../ -G "Visual Studio 16 2019" -A x64 -T ClangCL
```

Might be related.

## VSCode-related issues

### IntelliSense issues with concepts (or other libraries)

Make sure VSCode is using the correct compiler. If you have older versions of Visual Studio installed, VSCode might be using the wrong headers. I have tested the intellisense for Visual Studio version >= 16.10.0.

> **_Possible fix:_** Uninstall all other versions of Visual Studio, except for the latest one.
