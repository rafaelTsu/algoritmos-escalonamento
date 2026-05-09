add_library(scheduler_backend STATIC
    src/simulator.cpp
)

target_include_directories(scheduler_backend PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_options(scheduler_backend PRIVATE -Wall -Wextra)
