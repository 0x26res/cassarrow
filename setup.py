import os
import pathlib

import pyarrow
from pybind11.setup_helpers import Pybind11Extension, build_ext
from setuptools import setup

__version__ = "0.1.2"

ROOT = pathlib.Path(__file__).parent
README = (ROOT / "README.md").read_text()

USE_CXX11_ABI = os.environ.get("USE_CXX11_ABI", "0")


def get_extension():
    pyarrow.create_library_symlinks()
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
        runtime_library_dirs=pyarrow.get_library_dirs(),
        include_dirs=[source_directory, pyarrow.get_include()],
    )
    if USE_CXX11_ABI is not None:
        extension.extra_compile_args.append(f"-D_GLIBCXX_USE_CXX11_ABI={USE_CXX11_ABI}")

    return extension


ext_modules = [get_extension()]
setup(
    name="cassarrow",
    description="Apache Arrow adapter for the Cassandra python driver",
    long_description=README,
    long_description_content_type="text/markdown",
    url="https://github.com/0x26res/cassarrow",
    author="0x26res",
    author_email="0x26res@gmail.com",
    version=__version__,
    classifiers=[
        "License :: OSI Approved :: Apache Software License",
        "Development Status :: 4 - Beta",
        "Programming Language :: Python :: 3",
        "Programming Language :: Python :: 3.9",
        "Programming Language :: Python :: 3.10",
        "Programming Language :: C++",
        "Topic :: Software Development :: Libraries :: Python Modules",
        "Natural Language :: English",
    ],
    packages=["cassarrow"],
    ext_modules=ext_modules,
    package_dir={"cassarrow": "cassarrow"},
    install_requires=[
        "setuptools>=42",
        "wheel",
        "pybind11>=2.9.0",
        "pyarrow>=7.0.0",
        "cassandra-driver",
    ],
    extras_require={"test": ["pytest", "pandas", "tabulate"]},
    cmdclass={"build_ext": build_ext},
    zip_safe=False,
    python_requires=">=3.9",
)
