# MSYS2 ucrt64 Dependencies
Open the `MSYS2 UCRT64` prompt and then ensure you've installed the following packages.
- mingw-w64-ucrt-x86_64-glew 2.2.0-3
- mingw-w64-ucrt-x86_64-sdl3 3.2.20-1

```sh
pacman -S mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-ninja mingw-w64-ucrt-x86_64-cmake mingw-w64-ucrt-x86_64-sdl3 mingw-w64-ucrt-x86_64-glew
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

---

- https://glusoft.com/sdl3-tutorials/opengl-shaders-blur-sdl3/