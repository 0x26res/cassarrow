set(Python3_FIND_UNVERSIONED_NAMES NEVER)
find_package(Python3 3.10 EXACT REQUIRED COMPONENTS Interpreter Development)
find_package(pybind11 REQUIRED)

add_library(
        _cassarrow
        MODULE
        ${CMAKE_CURRENT_SOURCE_DIR}/cassarrow/bindings.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/cassarrow/cassarrow.cpp
)

target_compile_features(_cassarrow PRIVATE cxx_std_17)

target_include_directories(
        _cassarrow
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${Python3_INCLUDE_DIRS}
        ${Python3_NumPy_INCLUDE_DIRS}
        ${pybind11_INCLUDE_DIR}
        ${pyarrow_INCLUDE_DIR}
        arrow
)

target_link_directories(_cassarrow PRIVATE ${pyarrow_LIBRARY_DIRS})

target_link_libraries(
        _cassarrow
        pybind11::module
        ${pyarrow_LIBRARIES}
)

pybind11_extension(_cassarrow)
pybind11_strip(_cassarrow)

set_target_properties(
        _cassarrow
        PROPERTIES
        CXX_VISIBILITY_PRESET "hidden"
        CUDA_VISIBILITY_PRESET "hidden")
