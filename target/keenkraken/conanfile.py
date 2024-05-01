from os import path, environ
from conan import ConanFile
from conan.tools.files import copy
from conan.tools.cmake import CMake, CMakeDeps, CMakeToolchain, cmake_layout

class KeenKraken(ConanFile):
    # Optional metadata
    license = "<Put the package license here>"
    author = "Bryan Perdrizat bryan.perdrizat@epfl.ch"
    url = "github.com/parsa-epfl/qflex"
    description = "Quick & Flexible Rack-Scale Computer Architecture Simulator"

    name = "keenkraken"
    version = "2024.4"
    settings = "os", "compiler", "build_type", "arch"
    options = {
        "shared": [True, False],
        "fPIC": [True],
    }

    default_options = {
        "shared": True,
        "fPIC": True,
    }

    project_root = path.dirname(path.abspath(path.join(__file__, '../..')))

    def layout(self):
        cmake_layout(self, build_folder='.', src_folder=self.project_root)

    def requirements(self):
        self.requires("boost/1.83.0")

    def build_requirements(self):
        self.tool_requires("cmake/3.25.3")
        self.tool_requires("ninja/1.12.0")

    def generate(self):
        tc = CMakeToolchain(self, generator="Ninja")
        tc.cache_variables['FLEXUS_ROOT'] = self.project_root
        tc.cache_variables['SIMULATOR'] = self.name
        tc.preprocessor_definitions["SELECTED_DEBUG"] = "vverb"
        tc.generate()

        cmake = CMakeDeps(self)
        cmake.generate()

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    def package(self):
        export_path = path.join(self.export_sources_folder, "lib", self.settings.get_safe("build_type", default="Release"))
        self.output.success(f"Exporting library to {export_path}")
        copy(self, "*.so", self.build_folder, export_path, keep_path=False)