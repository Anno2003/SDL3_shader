# SDL3 Shader
a small sdl3 program that displays shader into a transparent window that always stays on top.
![example](example.png)
you can change the shader by editing `frag_shader.glsl` and `vertex_shader.glsl`

# MSYS2 ucrt64 Dependencies
Open the `MSYS2 UCRT64` prompt and then ensure you've installed the following packages.
- mingw-w64-ucrt-x86_64-python-jinja 3.1.6-1
- mingw-w64-ucrt-x86_64-sdl3 3.2.20-1

```sh
pacman -S mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-ninja mingw-w64-ucrt-x86_64-cmake mingw-w64-ucrt-x86_64-sdl3 mingw-w64-ucrt-x86_64-python-jinja
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
./main
```

## Install/distribute:
To make dist folder use:
```sh
cmake --install build --prefix ./dist
```

---

- https://glusoft.com/sdl3-tutorials/opengl-shaders-blur-sdl3/