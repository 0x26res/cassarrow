set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}  -DBOOST_TEST_DYN_LINK")


find_package(Boost COMPONENTS system filesystem unit_test_framework REQUIRED)
find_package(Arrow REQUIRED)


add_executable(
        test_exe
        ${CMAKE_CURRENT_SOURCE_DIR}/cassarrow/test_cassarrow.cpp
        ${CMAKE_SOURCE_DIR}/cpp/src/cassarrow/cassarrow.cpp)

target_include_directories(test_exe PRIVATE ${CMAKE_SOURCE_DIR}/cpp/src)

target_link_libraries(
        test_exe
        arrow
        boost_unit_test_framework
)

enable_testing()
add_test(CassarrowTest test_exe)