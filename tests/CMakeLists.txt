add_executable(sanity_test sanity_test.cpp)
target_link_libraries(sanity_test PRIVATE scheduler_backend)
target_compile_options(sanity_test PRIVATE -Wall -Wextra)

add_executable(compare compare.cpp)
target_link_libraries(compare PRIVATE scheduler_backend)
target_compile_options(compare PRIVATE -Wall -Wextra)
