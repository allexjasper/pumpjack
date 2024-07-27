cmake_minimum_required(VERSION 3.10)
project(oiljack VERSION 1.0 LANGUAGES CXX)

# Specify the C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Add the path to custom modules (if any)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

find_package(OpenGL REQUIRED)

# Find Boost
find_package(Boost REQUIRED COMPONENTS system filesystem log_setup log program_options regex)

# If Boost is not found automatically, you can hint the paths
if (NOT Boost_FOUND)
    set(BOOST_ROOT "C:/msys64/mingw64")
    find_package(Boost REQUIRED)
endif()

# Find SDL2
if (NOT SDL2_FOUND)
    set(SDL2_DIR "C:/msys64/mingw64")
    find_package(SDL2 REQUIRED)
endif()

# Find libusb
find_path(LIBUSB_INCLUDE_DIR NAMES libusb-1.0/libusb.h )
find_library(LIBUSB_LIBRARY NAMES usb-1.0 )
if (LIBUSB_INCLUDE_DIR AND LIBUSB_LIBRARY)
    set(LIBUSB_FOUND TRUE)
    set(LIBUSB_INCLUDE_DIRS ${LIBUSB_INCLUDE_DIR})
    set(LIBUSB_LIBRARIES ${LIBUSB_LIBRARY})
else ()
    message(FATAL_ERROR "libusb not found")
endif ()

# Find GLM
find_path(GLM_INCLUDE_DIR NAMES glm/glm.hpp)
if (GLM_INCLUDE_DIR)
    set(GLM_FOUND TRUE)
    set(GLM_INCLUDE_DIRS ${GLM_INCLUDE_DIR})
else ()
    message(FATAL_ERROR "GLM not found")
endif ()

# Set path for manually downloaded GLI
set(GLI_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/external/gli")
if (EXISTS "${GLI_INCLUDE_DIR}/gli.hpp")
    set(GLI_FOUND TRUE)
    set(GLI_INCLUDE_DIRS ${GLI_INCLUDE_DIR})
else ()
    message(FATAL_ERROR "GLI not found in ${GLI_INCLUDE_DIR}")
endif ()

# Find GLEW
find_package(GLEW REQUIRED)

# Include directories
include_directories(
    ${Boost_INCLUDE_DIRS}
    ${SDL2_INCLUDE_DIRS}
    ${LIBUSB_INCLUDE_DIRS}
    ${GLM_INCLUDE_DIRS}
    ${GLI_INCLUDE_DIRS}
    ${GLEW_INCLUDE_DIRS}
    ${CMAKE_SOURCE_DIR}/src/include
)

# Add source files
file(GLOB_RECURSE SOURCES "${CMAKE_SOURCE_DIR}/src/*.cpp")

# Add executable
add_executable(${PROJECT_NAME} ${SOURCES})

# Link libraries
target_link_libraries(${PROJECT_NAME}
    Boost::boost
    Boost::log_setup
    Boost::log
    Boost::system
    Boost::filesystem
    Boost::program_options
    Boost::regex
    SDL2::SDL2
    ${LIBUSB_LIBRARIES}
    ${OPENGL_LIBRARIES}
    GLEW::GLEW
    # GLM and GLI are header-only libraries, no linking needed
)

# Installation rules
install(TARGETS ${PROJECT_NAME} DESTINATION bin)
install(DIRECTORY ${CMAKE_SOURCE_DIR}/src/include/ DESTINATION include)
