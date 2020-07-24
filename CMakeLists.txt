project(mave)

include_directories(
        dep/argvparse/include
)

add_subdirectory(dep/argvparse/)

add_executable(mave
        src/main.cpp
        )

target_link_libraries(mave
        argvparse
        )
