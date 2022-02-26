set(Python3_FIND_UNVERSIONED_NAMES NEVER)
find_package(Python3 3.9 EXACT REQUIRED COMPONENTS Interpreter Development)
find_package(pybind11 REQUIRED)
find_package(Arrow REQUIRED)

add_library(
        _cassarrow
        MODULE
        ${CMAKE_CURRENT_SOURCE_DIR}/cassarrow/bindings.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/cassarrow/cassarrow.cpp
)

target_include_directories(
        _cassarrow
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${Python3_INCLUDE_DIRS}
        ${Python3_NumPy_INCLUDE_DIRS}
        ${pybind11_INCLUDE_DIR}
)

target_link_libraries(
        _cassarrow
        pybind11::module
        arrow
        arrow_python
)

pybind11_extension(_cassarrow)
pybind11_strip(_cassarrow)

set_target_properties(
        _cassarrow
        PROPERTIES
        CXX_VISIBILITY_PRESET "hidden"
        CUDA_VISIBILITY_PRESET "hidden")
