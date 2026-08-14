PROJECT(dali-ui-components)
SET(PROJECT_LIB_NAME dali2-ui-components)
SET(COMPONENTS_ROOT_SRC_DIR ${ROOT_SRC_DIR}/dali-ui-components)

# Collect source files
FILE(GLOB_RECURSE SOURCES "${COMPONENTS_ROOT_SRC_DIR}/*.cpp")

# 공유 라이브러리 생성
ADD_LIBRARY(${PROJECT_LIB_NAME} SHARED ${SOURCES})

IF(WIN32)
  TARGET_COMPILE_DEFINITIONS(${PROJECT_LIB_NAME} PRIVATE BUILDING_DALI_UI_COMPONENTS)
ENDIF()

# Components-owned image dir for the default icon assets (installed under
# <dataReadOnlyInstallDir>/ui/images/components). Mirrors DALI_IMAGE_DIR's =0 env fallback so
# non-default-resource builds keep resolving via the DALI_IMAGE_DIR environment variable.
IF( USE_DEFAULT_RESOURCE_DIR )
  TARGET_COMPILE_DEFINITIONS(${PROJECT_LIB_NAME} PRIVATE DALI_UI_COMPONENTS_IMAGE_DIR="${dataReadOnlyDir}/ui/images/components/")
ELSE()
  TARGET_COMPILE_DEFINITIONS(${PROJECT_LIB_NAME} PRIVATE DALI_UI_COMPONENTS_IMAGE_DIR=0)
ENDIF()

# Components-owned message catalog, installed to the standard locale directory
# (<prefix>/share/locale) alongside dali-ui-foundation.mo. Mirrors
# DALI_UI_COMPONENTS_IMAGE_DIR's =0 env fallback so non-default-resource builds
# keep resolving via the DALI_UI_COMPONENTS_LOCALE_DIR environment variable.
SET(DALI_UI_COMPONENTS_LOCALE_DOMAIN "dali-ui-components")
# CMAKE_INSTALL_PREFIX may carry backslashes on Windows; normalize them the same
# way the top-level CMakeLists does for dataReadOnlyDir before baking the path.
SET(componentsLocaleDir "${CMAKE_INSTALL_PREFIX}/share/locale")
STRING(REPLACE "\\" "/" componentsLocaleDir "${componentsLocaleDir}")
TARGET_COMPILE_DEFINITIONS(${PROJECT_LIB_NAME} PRIVATE DALI_UI_COMPONENTS_LOCALE_DOMAIN="${DALI_UI_COMPONENTS_LOCALE_DOMAIN}")
IF( USE_DEFAULT_RESOURCE_DIR )
  TARGET_COMPILE_DEFINITIONS(${PROJECT_LIB_NAME} PRIVATE DALI_UI_COMPONENTS_LOCALE_DIR="${componentsLocaleDir}")
ELSE()
  TARGET_COMPILE_DEFINITIONS(${PROJECT_LIB_NAME} PRIVATE DALI_UI_COMPONENTS_LOCALE_DIR=0)
ENDIF()

TARGET_INCLUDE_DIRECTORIES(${PROJECT_LIB_NAME}
  PUBLIC
    $<BUILD_INTERFACE:${ROOT_SRC_DIR}>
    $<INSTALL_INTERFACE:include>
  PRIVATE
    ${DALICORE_INCLUDE_DIRS}
    ${DALIADAPTOR_INCLUDE_DIRS}
    ${DALIUI_INCLUDE_DIRS}
    ${DALI_UI_GETTEXT_INCLUDE_DIRS}
    ${VCPKG_INCLUDE_DIR}
    ${INCLUDE_DIR}
)

TARGET_LINK_LIBRARIES(${PROJECT_LIB_NAME}
  PUBLIC dali2-ui-foundation
  PRIVATE ${DALI_UI_GETTEXT_LIBRARIES} ${COVERAGE}
)

SET_TARGET_PROPERTIES(${PROJECT_LIB_NAME}
  PROPERTIES
  VERSION ${DALI_UI_VERSION}
  SOVERSION ${DALI_UI_VERSION_MAJOR}
  CLEAN_DIRECT_OUTPUT 1
)

# Install library and CMake package metadata.
IF(INSTALL_CMAKE_MODULES)
  INSTALL(TARGETS ${PROJECT_LIB_NAME}
    EXPORT ${PROJECT_LIB_NAME}-targets
    LIBRARY DESTINATION ${DALI_UI_INSTALL_LIB_DIR}
    ARCHIVE DESTINATION ${DALI_UI_INSTALL_LIB_DIR}
    RUNTIME DESTINATION ${DALI_UI_INSTALL_BIN_DIR}
  )

  INSTALL(EXPORT ${PROJECT_LIB_NAME}-targets
    NAMESPACE ${PROJECT_LIB_NAME}::
    FILE ${PROJECT_LIB_NAME}-targets.cmake
    DESTINATION share/${PROJECT_LIB_NAME}
  )

  FILE(WRITE ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_LIB_NAME}-config.cmake
"include(CMakeFindDependencyMacro)\n    find_dependency(dali2-ui-foundation CONFIG REQUIRED)\ninclude(\${CMAKE_CURRENT_LIST_DIR}/${PROJECT_LIB_NAME}-targets.cmake)\n")
  INSTALL(FILES ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_LIB_NAME}-config.cmake
          DESTINATION share/${PROJECT_LIB_NAME})

  IF(ENABLE_DEBUG AND WIN32)
    INSTALL(FILES $<TARGET_PDB_FILE:${PROJECT_LIB_NAME}>
            DESTINATION ${DALI_UI_INSTALL_BIN_DIR})
  ENDIF()
ELSE()
  INSTALL(TARGETS ${PROJECT_LIB_NAME}
    LIBRARY DESTINATION ${DALI_UI_INSTALL_LIB_DIR}
    ARCHIVE DESTINATION ${DALI_UI_INSTALL_LIB_DIR}
    RUNTIME DESTINATION ${DALI_UI_INSTALL_BIN_DIR}
  )
ENDIF()

# Install headers
SET(PROJECT_INCLUDE_DIR ${INCLUDE_DIR}/${PROJECT_NAME})
INSTALL(DIRECTORY ${COMPONENTS_ROOT_SRC_DIR}/public-api/
        DESTINATION ${PROJECT_INCLUDE_DIR}/public-api
        FILES_MATCHING PATTERN "*.h")

# Install dali-ui-components.h
INSTALL( FILES ${COMPONENTS_ROOT_SRC_DIR}/dali-ui-components.h DESTINATION ${PROJECT_INCLUDE_DIR} )

# Install pkg config
IF( ENABLE_PKG_CONFIGURE )
  SET( DEV_INCLUDE_PATH ${INCLUDE_DIR} )
  SET( PKG_CFG_FILE ${CMAKE_CURRENT_SOURCE_DIR}/${PROJECT_LIB_NAME}.pc )
  CONFIGURE_FILE( ${PKG_CFG_FILE}.in ${PKG_CFG_FILE} @ONLY )
  INSTALL( FILES ${PKG_CFG_FILE} DESTINATION ${LIB_DIR}/pkgconfig )
ENDIF()

# Install resources
INSTALL(DIRECTORY ${COMPONENTS_ROOT_SRC_DIR}/images/
        DESTINATION ${dataReadOnlyInstallDir}/ui/images/components)

# Compile the components message catalog (.po -> .mo) and install it under
# <prefix>/share/locale/<locale>/LC_MESSAGES/<domain>.mo, the layout
# gettext expects from bindtextdomain().
INCLUDE(${CMAKE_CURRENT_SOURCE_DIR}/../dali-ui-gettext.cmake)
DALI_UI_FIND_MSGFMT()

IF(MSGFMT_EXECUTABLE)
  MESSAGE(STATUS "Using gettext msgfmt: ${MSGFMT_EXECUTABLE}")

  SET(COMPONENTS_LOCALE_BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}/locale")
  FILE(GLOB COMPONENTS_PO_FILES "${COMPONENTS_ROOT_SRC_DIR}/po/*.po")

  SET(COMPONENTS_MO_FILES)
  FOREACH(PO_FILE ${COMPONENTS_PO_FILES})
    GET_FILENAME_COMPONENT(LOCALE ${PO_FILE} NAME_WE)
    SET(MO_DIR "${COMPONENTS_LOCALE_BINARY_DIR}/${LOCALE}/LC_MESSAGES")
    SET(MO_FILE "${MO_DIR}/${DALI_UI_COMPONENTS_LOCALE_DOMAIN}.mo")
    ADD_CUSTOM_COMMAND(
      OUTPUT ${MO_FILE}
      COMMAND ${CMAKE_COMMAND} -E make_directory ${MO_DIR}
      COMMAND ${MSGFMT_EXECUTABLE} -o ${MO_FILE} ${PO_FILE}
      DEPENDS ${PO_FILE}
      ${MSGFMT_JOB_POOL_ARGUMENT}
      COMMENT "Generating ${MO_FILE} from ${PO_FILE}"
      VERBATIM
    )
    INSTALL(FILES ${MO_FILE}
            DESTINATION ${componentsLocaleDir}/${LOCALE}/LC_MESSAGES)
    LIST(APPEND COMPONENTS_MO_FILES ${MO_FILE})
  ENDFOREACH()

  # An ALL target, so the catalogs are always built. The library does not consume
  # them, so it deliberately does not depend on this target: that would serialize
  # the link behind every msgfmt run.
  ADD_CUSTOM_TARGET(dali-ui-components-locale ALL DEPENDS ${COMPONENTS_MO_FILES})
ELSEIF(WIN32)
  MESSAGE(FATAL_ERROR
    "msgfmt is required to build the dali-ui-components message catalog. "
    "Run the windows-dependencies tools setup or set DALI_MSGFMT_EXECUTABLE."
  )
ELSE()
  MESSAGE(WARNING "msgfmt not found; the dali-ui-components message catalog will not be generated")
ENDIF()
