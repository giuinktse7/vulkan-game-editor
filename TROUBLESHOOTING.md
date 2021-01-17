# Compilation issues

## Lua-related

### unresolved external symbol in the Lua libraries

At one point, I could not get the Visual Studio program to link anything from lua.h.

**Possible Solution**:
Try generating the building and building from the command line instead. Note: Remove the current build folder before trying this.
The working build used `-T ClangCL` when compiling, as in

```sh
cmake ../ -G "Visual Studio 16 2019" -A x64 -T ClangCL
```

Might be related.
