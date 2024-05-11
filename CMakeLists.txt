cmake_minimum_required(VERSION 3.1)
project(binaryMesher)

set(SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src")

set(LIB_DIR "${CMAKE_CURRENT_SOURCE_DIR}/libraries")
set(LIB_INCLUDE_DIR "${LIB_DIR}/include/")
set(GLFW_DIR "${LIB_DIR}/glfw")
set(GLAD_DIR "${LIB_DIR}/glad")
set(GLM_DIR "${LIB_DIR}/glm")
set(FAST_NOISE_DIR "${LIB_DIR}/FastNoise")
set(CXXPOOL_DIR "${LIB_DIR}/cxxpool")

add_subdirectory(${GLM_DIR} EXCLUDE_FROM_ALL)
add_subdirectory(${GLFW_DIR})

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
set(CMAKE_BUILD_TYPE RelWithDebInfo)

add_executable(client
  ${SRC_DIR}/main.cpp
  ${FAST_NOISE_DIR}/FastNoise.cpp
)

target_link_libraries(client ${CMAKE_DL_LIBS})
target_link_libraries(client ${GLFW_LIBRARIES})
target_link_libraries(client "glm")
target_link_libraries(client "glfw" "${GLFW_LIBRARIES}")
target_link_libraries(client "glad" "${CMAKE_DL_LIBS}")

target_include_directories(client PRIVATE "${LIB_INCLUDE_DIR}")
target_include_directories(client PRIVATE ${SRC_DIR})
target_include_directories(client PRIVATE ${COMMON_DIR})
target_include_directories(client PRIVATE "${GLFW_DIR}/include")
target_include_directories(client PRIVATE "${GLAD_DIR}/include")
target_include_directories(client PRIVATE "${CXXPOOL_DIR}/include")

target_compile_definitions(client PRIVATE "GLFW_INCLUDE_NONE")

set_property(TARGET client PROPERTY CXX_STANDARD 17)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# GLFW
set(GLFW_BUILD_EXAMPLES OFF CACHE INTERNAL "Build the GLFW example programs" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE INTERNAL "Build the GLFW test programs" FORCE)
set(GLFW_BUILD_DOCS OFF CACHE INTERNAL "Build the GLFW documentation" FORCE)
set(GLFW_INSTALL OFF CACHE INTERNAL "Generate installation target")

# glad
add_library("glad" "${GLAD_DIR}/src/glad.c")
target_include_directories("glad" PRIVATE "${GLAD_DIR}/include")