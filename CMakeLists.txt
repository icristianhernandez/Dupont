cmake_minimum_required(VERSION 3.10)

project(MyProject CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)

add_executable(MyProject
    src/main.cpp
    src/Pump.cpp
    src/Valve.cpp
    src/Tank.cpp
    src/Mixer.cpp
    src/Sensor.cpp
    src/System.cpp
)
target_include_directories(MyProject PRIVATE include)

# Testing
enable_testing()

# Add main application sources needed by tests directly
# This avoids creating a separate library just for tests if the project is small
# Adjust this list based on actual dependencies of Valve.cpp, Tank.cpp, Sensor.cpp etc.
# Enums.h is header-only.
set(APP_LIB_SOURCES
    src/Valve.cpp
    src/Tank.cpp
    src/Sensor.cpp
    # Add other cpp files if Valve/Tank tests depend on them indirectly
    # For example, if Tank uses Sensor, Sensor.cpp is needed.
)

add_executable(PaintSystemTests
    tests/main_test_runner.cpp
    tests/test_valve.cpp
    tests/test_tank.cpp
    ${APP_LIB_SOURCES} # Add the application sources here
)
# Link application's include directory to the test executable
target_include_directories(PaintSystemTests PRIVATE include)

add_test(NAME UnitTests COMMAND PaintSystemTests)
