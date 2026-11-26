from conan import ConanFile
from conan.tools.files import copy
from conan.tools.layout import basic_layout
import os


class TypeIDCConan(ConanFile):
    name = "typeid-c"
    version = "0.1.3"
    description = ("TypeID for C and C++: type-safe, K-sortable, globally unique "
                   "identifiers (UUIDv7) in a single header")
    license = "Apache-2.0"
    url = "https://github.com/Sharkeeeeeee/typeid-c"
    homepage = "https://github.com/Sharkeeeeeee/typeid-c"
    topics = ("typeid", "uuid", "uuidv7", "identifier", "header-only")
    package_type = "header-library"
    settings = "os", "arch", "compiler", "build_type"
    exports_sources = "typeid.h", "typeid.hpp", "LICENSE", "NOTICE"
    no_copy_source = True

    def layout(self):
        basic_layout(self, src_folder=".")

    def package_id(self):
        self.info.clear()

    def package(self):
        copy(self, "LICENSE", self.source_folder, os.path.join(self.package_folder, "licenses"))
        copy(self, "NOTICE", self.source_folder, os.path.join(self.package_folder, "licenses"))
        for header in ("typeid.h", "typeid.hpp"):
            copy(self, header, self.source_folder, os.path.join(self.package_folder, "include"))

    def package_info(self):
        self.cpp_info.set_property("cmake_file_name", "typeid")
        self.cpp_info.set_property("cmake_target_name", "typeid::typeid")
        self.cpp_info.set_property("pkg_config_name", "typeid")
        self.cpp_info.bindirs = []
        self.cpp_info.libdirs = []
        if self.settings.os == "Windows":
            self.cpp_info.system_libs = ["bcrypt"]
