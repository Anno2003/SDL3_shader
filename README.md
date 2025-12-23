# MSYS2 ucrt64 Dependencies
mingw-w64-ucrt-x86_64-glew 2.2.0-3
mingw-w64-ucrt-x86_64-sdl3 3.2.20-1
# MSYS2

Open the `MSYS2 UCRT64` prompt and then ensure you've installed the following packages. This will get you working toolchain, CMake, Ninja, and of course SDL3.

```sh
pacman -S mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-ninja mingw-w64-ucrt-x86_64-cmake mingw-w64-ucrt-x86_64-sdl3
```

## Create the file CMakeLists.txt
```cmake
cmake_minimum_required(VERSION 3.26)
project(hello C CXX)

find_package(SDL3 REQUIRED)

add_executable(hello)

target_sources(hello
PRIVATE
    hello.c
)

target_link_libraries(hello SDL3::SDL3)
```

## Configure and Build:
```sh
cmake -S . -B build
cmake --build build
```

## Run:

The executable is in the `build` directory:
```sh
cd build
./hello
```

## How do I copy a SDL3 dynamic library to another location?
Use CMake generator expressions. Generator expressions support multiple configurations, and are evaluated during build system generation time.

On Windows, the following example copies SDL3.dll to the directory where mygame.exe is built.

```cmake
if(WIN32)
    add_custom_command(
        TARGET mygame POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E copy $<TARGET_FILE:SDL3::SDL3-shared> $<TARGET_FILE_DIR:mygame>
        VERBATIM
    )
endif()
```

On Unix systems, `$<TARGET_FILE:...>` will refer to the dynamic library (or framework), and you might need to use `$<TARGET_SONAME_FILE:tgt>` instead.

Most often, you can avoid copying libraries by configuring your project with absolute `CMAKE_LIBRARY_OUTPUT_DIRECTORY` and `CMAKE_RUNTIME_OUTPUT_DIRECTORY` paths. When using a multi-config generator (such as Visual Studio or Ninja Multi-Config), eventually add `/$<CONFIG>` to both paths.