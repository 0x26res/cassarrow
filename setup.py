import pyarrow

from pybind11.setup_helpers import Pybind11Extension, build_ext
from setuptools import setup

__version__ = "0.0.1"


def get_extension():
    pyarrow.create_library_symlinks()
    extension = Pybind11Extension(
        "_cassarrow",
        ["cpp/src/cassarrow/bindings.cpp"],
        define_macros=[("VERSION_INFO", __version__)],
        cxx_std=11,
    )
    extension.extra_compile_args.append("-D_GLIBCXX_USE_CXX11_")

    for library_dir in pyarrow.get_library_dirs():
        extension.extra_link_args.append(f"-L{library_dir}")
    for library in pyarrow.get_libraries():
        extension.extra_link_args.append(f"-l{library}")
    return extension


ext_modules = [get_extension()]

setup(
    name="cassarrow",
    version=__version__,
    author="0x26res",
    author_email="0x26res@gmail.com",
    url="https://github.com/0x26res/cassarrow",
    description="An Apache Arrow adapter to Cassandra python driver",
    long_description="",
    ext_modules=ext_modules,
    extras_require={"test": ["pytest", "pandas", "tabulate"]},
    cmdclass={"build_ext": build_ext},
    zip_safe=False,
    python_requires=">=3.6",
)
