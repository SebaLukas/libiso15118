# add_subdirectory(fsm)
# add_subdirectory(states)

add_executable(evcc)

target_sources(evcc
    PRIVATE
        evcc.cpp
)

target_link_libraries(evcc
    PRIVATE
        iso15118::iso15118
)
