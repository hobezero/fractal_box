# FractalBox

## What is it?

An experimental game engine for large-scale simulations.

## Project goals and design decisions

### Coding practices

 - The main goal is to learn new things in the process of writing code.
   Experiment with code style and programming paradigms
 - Modern C++23. Apply best practices as much as possible
 - Modern target-oriented CMake
 - Data-oriented design. Keep away from OOP pitfalls
 - Cross-platform. Build for desktop.
   No non-standard language extensions unless there is a default fallback.  
   Test on major compilers (gcc, clang. msvc) and emscripten
 - KISS. No unnecesary complexity unless we are testing a new idea
 - Use tools to write bug-free and performant code:
   - Unit tests
   - Benchmarks
   - Code sanitizers
   - Profilers
   - Package manager
   - TODO: Static analyzers
   - TODO: CI/CD

## How do I build and run it?

// TODO

### Prerequisites

 - C++23 compliant compiler
 - A graphics card with OpenGL 3.3 support
 - CMake 3.28 or newer
 - [Conan](https://conan.io/) package manager

### Dependencies

Libraries that can be installed using conan:
 - hedley
 - fmt
 - glm
 - SDL
 - Dear ImGui
 - stb
 - Catch2
 - nanobench

On Linux you can use the script `scripts/conan_install.sh` to install all dependencies at once for
common configurations of GCC and Clang in the specified build directory. *Note:* Conan profiles
named `gcc` and `clang` must be configured beforehand.

### Building

CMake options  
// TODO

```bash
mkdir build
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=install -DFR_FORCE_COLORED_OUTPUT=ON
cmake --build build --target install
```

### Unit tests

```bash
cmake -S . -B build -DFR_BUILD_TESTING=ON
cmake --build build --target fractal_box_tests
ctest --test-dir build -V -R test_
```

// TODO: How to run tests directly  

### Benchmarks

```bash
cmake -S . -B build -DFR_BUILD_BENCH=ON
cmake --build build --target fractal_box_bench
ctest --test-dir build -V -R bench_
```

// TODO: How to run benchmarks directly  

## Attribution

This project incorporates parts of the source code of several third-party projects:
 - [Boost](https://www.boost.org/) licensed under the Boost Software License - Version 1.0
 - [rapidhash](https://github.com/Nicoshev/rapidhash) licensed under the MIT License
 - [P0792 reference implementation](https://github.com/zhihaoy/nontype_functional) licensed under the BSD 2-Clause License
