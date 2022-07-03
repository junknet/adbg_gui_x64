cmake_minimum_required(VERSION 3.5)

project(adbg VERSION 0.1 LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(PkgConfig REQUIRED)
pkg_check_modules(CAPSTONE REQUIRED capstone>=3.0.4)
include_directories(${CAPSTONE_INCLUDE_DIRS})
link_directories(${CAPSTONE_LIBRARY_DIRS})

find_package(Qt6 REQUIRED COMPONENTS Widgets)
find_package(Qt6 REQUIRED COMPONENTS Network)
find_package(fmt)


file(GLOB SRC ./src/*.cpp)
file(GLOB UI ./src/*.ui)
file(GLOB H ./src/*.h)
INCLUDE_DIRECTORIES(./src)


qt_wrap_ui(FORMS_UIC ${UI})
message("${FORMS_UIC}")
qt_add_executable(adbg
    MANUAL_FINALIZATION
    "resource.qrc"
    ${UI}
    ${SRC}
    ${H}
)

target_link_libraries(adbg PRIVATE Qt6::Widgets)
target_link_libraries(adbg PRIVATE Qt6::Network)
target_link_libraries(adbg PRIVATE ${CAPSTONE_LIBRARIES})
target_link_libraries(adbg PRIVATE fmt::fmt)