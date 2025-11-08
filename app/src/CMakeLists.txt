
# Main app sources
target_sources(app PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp
)

    # Submodules
add_subdirectory(Core)
add_subdirectory(SignalProcessing)
add_subdirectory(Threads)
add_subdirectory(Utils)
add_subdirectory(ADC)
add_subdirectory(Visualization)
