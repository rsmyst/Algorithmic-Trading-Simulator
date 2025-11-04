cmake_minimum_required(VERSION 3.11)
project(AlgorithmicTradingSimulator
  LANGUAGES CXX
  VERSION 1.0.0
)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find OpenMP
find_package(OpenMP REQUIRED)
if(OpenMP_CXX_FOUND)
    message(STATUS "OpenMP found: ${OpenMP_CXX_VERSION}")
endif()

# Find MPI (optional)
find_package(MPI)
if(MPI_CXX_FOUND)
    message(STATUS "MPI found: ${MPI_CXX_VERSION}")
    set(MPI_ENABLED TRUE)
else()
    message(STATUS "MPI not found - building without MPI support")
    set(MPI_ENABLED FALSE)
endif()

# Fetch FTXUI library
include(FetchContent)

FetchContent_Declare(ftxui
  GIT_REPOSITORY https://github.com/ArthurSonzogni/ftxui
  GIT_TAG v5.0.0
)

FetchContent_GetProperties(ftxui)
if(NOT ftxui_POPULATED)
  FetchContent_Populate(ftxui)
  add_subdirectory(${ftxui_SOURCE_DIR} ${ftxui_BINARY_DIR} EXCLUDE_FROM_ALL)
endif()

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/include)

# Create executable with all source files
add_executable(tradingSim 
  src/main.cpp
  src/trader.cpp
  src/market.cpp
  src/simulation.cpp
  src/order_book.cpp
  src/logger.cpp
)

# Link FTXUI libraries and OpenMP
target_link_libraries(tradingSim
  PRIVATE ftxui::screen
  PRIVATE ftxui::dom
  PRIVATE ftxui::component
  PRIVATE OpenMP::OpenMP_CXX
)

# Link MPI if available
if(MPI_ENABLED)
    target_link_libraries(tradingSim PRIVATE MPI::MPI_CXX)
    target_compile_definitions(tradingSim PRIVATE USE_MPI)
endif()

# Set output directory
set_target_properties(tradingSim PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)
