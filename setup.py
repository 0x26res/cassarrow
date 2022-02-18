import pathlib
import pyarrow
from pybind11.setup_helpers import Pybind11Extension, build_ext
from setuptools import setup

__version__ = "0.0.1rc0"

ROOT = pathlib.Path(__file__).parent
README = (ROOT / "README.md").read_text()


def get_extension():
    pyarrow.create_library_symlinks()
    extension = Pybind11Extension(
        "_cassarrow",
        ["cpp/src/cassarrow/bindings.cpp"],
        define_macros=[("VERSION_INFO", __version__)],
        cxx_std=11,
    )
    extension.extra_compile_args.append("-D_GLIBCXX_USE_CXX11_")
    extension.extra_compile_args.append(f"-I{pyarrow.get_include()}")
    for library_dir in pyarrow.get_library_dirs():
        extension.extra_link_args.append(f"-L{library_dir}")
    for library in pyarrow.get_libraries():
        extension.extra_link_args.append(f"-l{library}")
    return extension


ext_modules = [get_extension()]
setup(
    name="cassarrow",
    version=__version__,
    description="An Apache Arrow adapter to Cassandra python driver",
    long_description=README,
    long_description_content_type="text/markdown",
    url="https://github.com/0x26res/cassarrow",
    author="0x26res",
    author_email="0x26res@gmail.com",
    classifiers=[
        "License :: OSI Approved :: MIT License",
        "Programming Language :: Python :: 3",
        "Programming Language :: Python :: 3.9",
    ],
    packages=["cassarrow"],
    ext_modules=ext_modules,
    extras_require={"test": ["pytest", "pandas", "tabulate"]},
    cmdclass={"build_ext": build_ext},
    zip_safe=False,
    python_requires=">=3.9",
)
