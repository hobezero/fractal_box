from conan import ConanFile
from conan.tools.cmake import CMakeToolchain, CMakeDeps, CMake

class KeplerWarsConan(ConanFile):
    settings = "os", "arch", "compiler", "build_type"

    default_options = {
        "fmt/*:header_only": True,
        "sdl/*:shared": False,
        "sdl/*:fPIC": True,
        "sdl/*:alsa": False,
        "sdl/*:jack": False,
        "sdl/*:pulse": False,
        "sdl/*:sndio": False,
        "sdl/*:nas": False,
        "sdl/*:esd": False,
        "sdl/*:arts": False,
        "sdl/*:x11": True,
        "sdl/*:xcursor": True,
        "sdl/*:xinerama": True,
        "sdl/*:xinput": True,
        "sdl/*:xrandr": True,
        "sdl/*:xscrnsaver": False,
        "sdl/*:xshape": False,
        "sdl/*:xvm": True,
        "sdl/*:wayland": False,
        "sdl/*:directfb": False,
        "sdl/*:iconv": False,
        "sdl/*:video_rpi": False,
        "sdl/*:sdl2main": False,
        "sdl/*:opengl": True,
        "sdl/*:opengles": False,
        "sdl/*:vulkan": False,
        "sdl/*:libunwind": False,
        "imgui/*:shared": False,
    }

    def requirements(self):
        self.requires("hedley/15")
        self.requires("fmt/12.1.0")
        self.requires("glm/1.0.1")
        self.requires("cityhash/cci.20130801")
        self.requires("sdl/2.32.10")
        self.requires("stb/cci.20240531")
        self.requires("imgui/1.91.0")
        # TODO: make test dependencies optional
        self.requires("catch2/3.7.0")
        self.requires("nanobench/4.3.11")

    def generate(self):
        tc = CMakeToolchain(self)
        tc.generate()
        deps = CMakeDeps(self)
        deps.generate()

    def build(self):
        deps = CMake(self)
        cmake.configure()
        cmake.build()
