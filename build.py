import os
import pathlib
import sys

import pyarrow
from pybind11.setup_helpers import Pybind11Extension, build_ext

__version__ = "0.0.0"

ROOT = pathlib.Path(__file__).parent
README = (ROOT / "README.md").read_text()

USE_CXX11_ABI = os.environ.get("USE_CXX11_ABI", "0")


def build(setup_kwargs):
    try:
        pyarrow.create_library_symlinks()
    except FileExistsError:
        pass  # For some reason this started complaining
    source_directory = ROOT / "cpp" / "src"
    extension = Pybind11Extension(
        name="_cassarrow",
        sources=[
            str(source_directory / "cassarrow/bindings.cpp"),
            str(source_directory / "cassarrow/cassarrow.cpp"),
        ],
        define_macros=[("VERSION_INFO", __version__)],
        cxx_std=17,
        library_dirs=pyarrow.get_library_dirs(),
        libraries=pyarrow.get_libraries(),
        runtime_library_dirs=(
            [] if sys.platform == "win32" else pyarrow.get_library_dirs()
        ),
        include_dirs=[source_directory, pyarrow.get_include()],
        extra_compile_args=[f"-D_GLIBCXX_USE_CXX11_ABI={USE_CXX11_ABI}"],
    )
    setup_kwargs.update(
        {
            "ext_modules": [extension],
            "cmd_class": {"build_ext": build_ext},
            "zip_safe": False,
        }
    )
