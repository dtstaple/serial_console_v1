cmake_minimum_required(VERSION 3.16)
project(SerialDebugger VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Qt code-generation (moc/rcc/uic) is driven automatically.
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets SerialPort)

add_executable(SerialDebugger
    src/main.cpp
    src/gui/MainWindow.cpp
    src/gui/TerminalView.cpp
    src/serial/SerialManager.cpp
    src/logging/Logger.cpp
    src/models/LogModel.cpp
    src/models/LogFilterProxy.cpp
)

# Headers are included as "gui/MainWindow.h" etc., so add src/ to the path.
target_include_directories(SerialDebugger PRIVATE src)
target_link_libraries(SerialDebugger PRIVATE Qt6::Widgets Qt6::SerialPort)

# ---------------------------------------------------------------------------
# Optional unit tests (built only if Qt6::Test is available).
#   cmake -B build -DBUILD_TESTING=ON && cmake --build build && ctest --test-dir build
# ---------------------------------------------------------------------------
option(BUILD_TESTING "Build unit tests" ON)
if(BUILD_TESTING)
    find_package(Qt6 QUIET COMPONENTS Test)
    if(Qt6Test_FOUND)
        enable_testing()
        add_executable(logger_test
            tests/test_logger.cpp
            src/logging/Logger.cpp
        )
        target_include_directories(logger_test PRIVATE src)
        target_link_libraries(logger_test PRIVATE Qt6::Core Qt6::Test)
        add_test(NAME logger_test COMMAND logger_test)
    endif()
endif()
