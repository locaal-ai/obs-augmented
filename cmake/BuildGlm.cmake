
include(ExternalProject)

ExternalProject_Add(
    glm_build
    URL https://github.com/g-truc/glm/archive/refs/tags/1.0.1.tar.gz
    CMAKE_GENERATOR ${CMAKE_GENERATOR}
    CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
                -DBUILD_SHARED_LIBS=OFF
               -DGLM_BUILD_TESTS=OFF -DGLM_BUILD_INSTALL=ON
  )

ExternalProject_Get_Property(glm_build INSTALL_DIR)

add_library(glm INTERFACE)
target_include_directories(glm INTERFACE ${INSTALL_DIR}/include)
add_dependencies(glm glm_build)
