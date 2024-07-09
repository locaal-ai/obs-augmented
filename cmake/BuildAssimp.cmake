include(ExternalProject)

set(ASSIMP_VERSION 5.4.1)
set(ASSIMP_URL https://github.com/assimp/assimp/archive/refs/tags/v${ASSIMP_VERSION}.tar.gz)
if(WIN32)
  set(ASSIMP_SHARE_LIB_NAME assimp-vc143-mt)
  set(ASSIMP_SHARE_LIB_FULL_NAME ${CMAKE_SHARED_LIBRARY_PREFIX}${ASSIMP_SHARE_LIB_NAME}${CMAKE_SHARED_LIBRARY_SUFFIX})
elseif(APPLE)
  set(ASSIMP_SHARE_LIB_NAME assimp)
  set(ASSIMP_SHARE_LIB_FULL_NAME
      ${CMAKE_SHARED_LIBRARY_PREFIX}${ASSIMP_SHARE_LIB_NAME}.${ASSIMP_VERSION}${CMAKE_SHARED_LIBRARY_SUFFIX})
else()
  set(ASSIMP_SHARE_LIB_NAME assimp)
  set(ASSIMP_SHARE_LIB_FULL_NAME
      ${CMAKE_SHARED_LIBRARY_PREFIX}${ASSIMP_SHARE_LIB_NAME}${CMAKE_SHARED_LIBRARY_SUFFIX}.${ASSIMP_VERSION})
endif()

ExternalProject_Add(
  assimp-build
  URL ${ASSIMP_URL} DOWNLOAD_EXTRACT_TIMESTAMP TRUE
  CMAKE_GENERATOR ${CMAKE_GENERATOR}
  BUILD_BYPRODUCTS <INSTALL_DIR>/bin/${ASSIMP_SHARE_LIB_FULL_NAME}
  CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
             -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
             -DBUILD_SHARED_LIBS=ON
             -DASSIMP_BUILD_ASSIMP_TOOLS=OFF
             -DASSIMP_BUILD_TESTS=OFF
             -DASSIMP_BUILD_SAMPLES=OFF
             -DASSIMP_BUILD_ALL_IMPORTERS_BY_DEFAULT=ON
             -DASSIMP_BUILD_ALL_EXPORTERS_BY_DEFAULT=OFF
             -DASSIMP_INSTALL_PDB=OFF
             -DASSIMP_NO_EXPORT=OFF)

ExternalProject_Get_Property(assimp-build INSTALL_DIR)

add_library(assimp::assimp SHARED IMPORTED)
add_dependencies(assimp::assimp assimp-build)
set_target_properties(assimp::assimp PROPERTIES IMPORTED_LOCATION ${INSTALL_DIR}/bin/${ASSIMP_SHARE_LIB_FULL_NAME})
if(WIN32)
  set_target_properties(
    assimp::assimp
    PROPERTIES IMPORTED_IMPLIB
               ${INSTALL_DIR}/lib/${CMAKE_IMPORT_LIBRARY_PREFIX}${ASSIMP_SHARE_LIB_NAME}${CMAKE_IMPORT_LIBRARY_SUFFIX})
endif()
target_include_directories(assimp::assimp INTERFACE ${INSTALL_DIR}/include)

add_library(assimp INTERFACE)
add_dependencies(assimp assimp-build)
target_link_libraries(assimp INTERFACE assimp::assimp)
target_include_directories(assimp INTERFACE ${INSTALL_DIR}/include)

if(WIN32)
  # install the DLL to the plugin output directory
  install(FILES ${INSTALL_DIR}/bin/${ASSIMP_SHARE_LIB_FULL_NAME}
          DESTINATION ${CMAKE_SOURCE_DIR}/release/${CMAKE_BUILD_TYPE}/obs-plugins/64bit)
endif()
