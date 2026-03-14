from conan import ConanFile
from conan.tools.cmake import CMakeToolchain, CMakeDeps, CMake

class KeplerWarsConan(ConanFile):
    settings = "os", "arch", "compiler", "build_type"

    default_options = {
        "fmt/*:header_only": True,
        "sdl/*:gpu": True,
        "sdl/*:x11": True,
        "sdl/*:alsa": False,
        "sdl/*:dbus": False,
        "sdl/*:tray": False,
        "sdl/*:xdbe": True,
        "sdl/*:audio": False,
        "sdl/*:power": True,
        "sdl/*:sndio": False,
        "sdl/*:video": True,
        "sdl/*:xsync": True,
        "sdl/*:camera": False,
        "sdl/*:dialog": True,
        "sdl/*:haptic": False,
        "sdl/*:hidapi": True,
        "sdl/*:libusb": False,
        "sdl/*:opengl": True,
        "sdl/*:render": False,
        "sdl/*:sensor": False,
        "sdl/*:shared": False,
        "sdl/*:vulkan": True,
        "sdl/*:xfixes": True,
        "sdl/*:xinput": True,
        "sdl/*:xrandr": True,
        "sdl/*:xshape": True,
        "sdl/*:libudev": True,
        "sdl/*:wayland": False,
        "sdl/*:xcursor": True,
        "sdl/*:joystick": True,
        "sdl/*:libiconv": False,
        "sdl/*:opengles": True,
        "sdl/*:pulseaudio": False,
        "sdl/*:xscrnsaver": False,
        "imgui/*:shared": False,
    }

    def requirements(self):
        self.requires("hedley/15")
        self.requires("fmt/12.1.0")
        self.requires("sfl/2.0.0")
        self.requires("glm/1.0.1")
        self.requires("cityhash/cci.20130801")
        self.requires("sdl/3.4.0")
        self.requires("stb/cci.20240531")
        self.requires("imgui/1.92.5")
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
