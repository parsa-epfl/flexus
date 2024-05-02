from os import path, environ
from conan import ConanFile
from conan.errors import ConanInvalidConfiguration
from conan.tools.files import copy
from conan.tools.cmake import CMake, CMakeDeps, CMakeToolchain, cmake_layout

class KeenKraken(ConanFile):
    # Optional metadata
    license = "<Put the package license here>"
    author = "Bryan Perdrizat bryan.perdrizat@epfl.ch"
    url = "github.com/parsa-epfl/qflex"
    description = "Quick & Flexible Rack-Scale Computer Architecture Simulator"

    name = None
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


    def set_name(self):
        self.name = self.name or None

    def configure(self):
        if self.name == None:
            raise ConanInvalidConfiguration("Need to set a name to compile [keenkraken/knottykraken]")
        if self.name == 'keenkraken' and int(self.settings_build.get_safe('compiler.version')) < 12:
            raise ConanInvalidConfiguration("Need GCC 12.3 at least to build keenkraken")
        if self.name == 'knottykraken' and int(self.settings_build.get_safe('compiler.version')) < 13:
            raise ConanInvalidConfiguration("Need GCC 13.1 at least to build knottykraken")


    def layout(self):
        cmake_layout(self, build_folder='.', src_folder=self.recipe_folder)

    def requirements(self):
        self.requires("boost/1.83.0", headers=True, libs=True)

    def build_requirements(self):
        self.tool_requires("cmake/3.25.3")
        self.tool_requires("ninja/1.12.0")

    def generate(self):
        tc = CMakeToolchain(self, generator="Ninja")
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
        export_path = path.join(
            self.export_sources_folder,
            "lib",
            self.settings.get_safe("build_type", default="Release"))
        self.output.highlight(f"Exporting library to {export_path}")
        copy(self, "*.so", self.build_folder, export_path, keep_path=False)