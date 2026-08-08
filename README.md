# FractalBox

## What Is It?

FractalBox is an experimental, code-first game engine for large-scale simulations. FractalBox seeks
to become a platform for future space, flight, and GIS applications for entertainment/gaming,
education, and visualization.

## DISCLAIMER

The project is in the early stages of development and is far from production-level completeness.
Be aware that there are no stability promises whatsoever: any component can be changed, replaced, or
removed at any time.

FractalBox is not a general-purpose game engine. After working with other engines, you might expect
many important features that were deliberately omitted from here.

## Platform Support

Currently tested targets:
 - x64 Linux GCC X11 OpenGL
 - x64 Linux Clang X11 OpenGL

Prospective targets: Vulkan, Wayland, Windows, web (WASM + WebGPU + Emscripten), Android, AArch64,
and MSVC.

FractalBox aims to support widely used targets that don't actively impede developers from doing so.
Desktop Linux with GCC/Clang toolchains is the current focus, as it is the platform on which the
development takes place. We are planning to properly set up CI/CD for Linux and Windows in the near
future.  MSVC, however, can't be supported right now due to Microsoft's unwillingness to keep up
with the Standard.

## Technology Stack
 - C++23
 - CMake
 - Conan
 - OpenGL

## Design Principles and Coding Practices

1. Complete set of features for building applications in the target domain.
Skip the features we don't need. Covering every use case is impossible
2. API that is easy to use, hard to misuse
3. High performance. No unreasonable overhead by default, highly optimized critical components.
4. Data-oriented design. Entity Component System as a core storage mechanism for data. Keep away
from OOP pitfalls, but use its features if needed
5. Modular components that can be reasonably customized or replaced
6. Prefer in-house solutions to third-party dependencies. Everything should be coherent and follow
the same design ideas. Adjacent APIs should fit together
7. Prefer compile-time errors to runtime errors
8. Correctness is important. Assume a fallible API can and will fail. Detect, process, log and
report errors to the user. Don't recover from contract violations. Prefer hard crashes to data
corruption
9. Modern C++23/CMake/other languages with the best practices applied
10. Use tools to write bug-free and performant code:
  - Compiler warnings
  - Code sanitizers
  - Profilers
  - Debuggers
  - Static analyzers

## Getting Started

### Prerequisites

 - C++23-compliant compiler
 - A graphics card with OpenGL 3.3 support
 - CMake 4.2 or newer
 - [Conan](https://conan.io/) package manager

### Dependencies

Component            | Required             | Location    | Installable by Conan
---------------------|----------------------|-------------|---------------------
OpenGL drivers       | Always               | System      | -
hedley               | Always               | External    | +
fmt                  | Always               | External    | +
glm                  | Always               | External    | +
SDL                  | Always               | External    | +
Dear ImGui           | Always               | External    | +
stb                  | Always               | External    | +
glad                 | Always               | In-tree     | -
Dear ImGui backends  | Always               | In-tree     | -
Catch2               | In tests/benchmarks  | External    | +
nanobench            | In benchmarks        | External    | +

On Linux, you can use the script `scripts/conan_install.sh` to install all dependencies at once for
common configurations of GCC and Clang in the specified build directory. *Note:* Conan profiles
named `gcc` and `clang` must be configured beforehand.

### CMake options

Option                         | Description                              | Possible Values                                 | Default Value
-------------------------------|------------------------------------------|-------------------------------------------------|---------------
FR_INSTALL                     | Enable install target                    | ON/OFF                                          | ON
FR_BUILD_TESTING               | Build unit test targets                  | ON/OFF                                          | ON if top-level
FR_BUILD_BENCH                 | Build benchmarking targets               | ON/OFF                                          | ON if top-level
FR_BUILD_EXAMPLES              | Build example targets                    | ON/OFF                                          | ON if top-level
FR_DEV_MODE                    | Build with developer defaults            | ON/OFF                                          | ON if top-level
FR_OVERRIDE_ASSERT_LEVEL       | Force a specific FR_ASSERT_LEVEL         | AUTO/NONE/FAST/DEFAULT/AUDIT/MAX                | AUTO
FR_OVERRIDE_LOG_LEVEL          | Force a specific FR_LOG_LEVEL            | AUTO/NONE/FATAL/ERROR/WARN/INFO/DEBUG/TRACE/MAX | AUTO
FR_ENABLE_COVERAGE             | Enable coverage instrumentation          | ON/OFF                                          | OFF
FR_ENABLE_GPROF                | Enable gprof instrumentation support     | ON/OFF                                          | OFF
FR_ENABLE_SANITIZER_ADDRESS    | Enable address sanitizer                 | ON/OFF                                          | OFF
FR_ENABLE_SANITIZER_LEAK       | Enable leak sanitizer                    | ON/OFF                                          | OFF
FR_ENABLE_SANITIZER_MEMORY     | Enable memory sanitizer                  | ON/OFF                                          | OFF
FR_ENABLE_SANITIZER_THREAD     | Enable thread sanitizer                  | ON/OFF                                          | OFF
FR_ENABLE_SANITIZER_UB         | Enable UB sanitizer                      | ON/OFF                                          | OFF

### Building

```bash
mkdir build
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=install -DFR_FORCE_COLORED_OUTPUT=ON
cmake --build build --target install
```

### Unit Tests

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
 - [P0792 reference implementation](https://github.com/zhihaoy/nontype_functional) licensed under
   the BSD 2-Clause License

## Licensing

All code of FractalBox (minus explicitly stated exceptions) is licensed under the permissive BSD
3-Clause License.
