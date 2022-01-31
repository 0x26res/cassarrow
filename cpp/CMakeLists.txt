
# find_package(Python3 COMPONENTS Interpreter Development NumPy REQUIRED)
# include(FindPython3)
set(Python3_FIND_UNVERSIONED_NAMES NEVER)
find_package(Python3 3.9 EXACT REQUIRED COMPONENTS Interpreter Development)
find_package(pybind11 REQUIRED)
find_package(Arrow REQUIRED)

add_library(
        bindings
        MODULE
        ${CMAKE_CURRENT_SOURCE_DIR}/src/cassarrow/bindings.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/cassarrow/cassarrow.cpp
)

target_include_directories(
        bindings
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${Python3_INCLUDE_DIRS}
        ${Python3_NumPy_INCLUDE_DIRS}
        ${pybind11_INCLUDE_DIR}
)

target_link_libraries(
        bindings
        pybind11::module
        arrow
        arrow_python
)

pybind11_extension(bindings)
pybind11_strip(bindings)

set_target_properties(
        bindings
        PROPERTIES
        CXX_VISIBILITY_PRESET "hidden"
        CUDA_VISIBILITY_PRESET "hidden")
