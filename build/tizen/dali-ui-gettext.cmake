# Locates the gettext "msgfmt" compiler used to turn .po sources into runtime
# .mo catalogs.
#
# This must stay a MACRO, not a FUNCTION: MSGFMT_JOB_POOL_ARGUMENT is a plain
# variable and has to reach the calling scope so that the caller can forward it
# to ADD_CUSTOM_COMMAND().
#
# After the call:
#   MSGFMT_EXECUTABLE        - path to msgfmt, or empty/NOTFOUND when missing
#   MSGFMT_JOB_POOL_ARGUMENT - "JOB_POOL <pool>" arguments, or empty
#
# The macro intentionally applies no missing-tool policy. Callers decide whether
# a missing msgfmt is fatal.
MACRO(DALI_UI_FIND_MSGFMT)
  IF(DEFINED ENV{DALI_MSGFMT_EXECUTABLE}
     AND EXISTS "$ENV{DALI_MSGFMT_EXECUTABLE}")
    SET(MSGFMT_EXECUTABLE "$ENV{DALI_MSGFMT_EXECUTABLE}" CACHE FILEPATH
        "Path to the gettext msgfmt executable" FORCE)
  ELSE()
    SET(MSGFMT_HINTS)
    IF(DEFINED ENV{DALI_GETTEXT_TOOLS_BIN})
      LIST(APPEND MSGFMT_HINTS "$ENV{DALI_GETTEXT_TOOLS_BIN}")
    ENDIF()
    IF(DEFINED _VCPKG_INSTALLED_DIR AND DEFINED VCPKG_TARGET_TRIPLET)
      LIST(APPEND MSGFMT_HINTS
        "${_VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/tools/gettext"
        "${_VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/tools/gettext/bin"
      )
    ENDIF()
    IF(DEFINED ENV{VCPKG_ROOT} AND DEFINED VCPKG_TARGET_TRIPLET)
      LIST(APPEND MSGFMT_HINTS
        "$ENV{VCPKG_ROOT}/installed/${VCPKG_TARGET_TRIPLET}/tools/gettext"
        "$ENV{VCPKG_ROOT}/installed/${VCPKG_TARGET_TRIPLET}/tools/gettext/bin"
      )
    ENDIF()
    IF(WIN32)
      LIST(APPEND MSGFMT_HINTS
        "C:/Tools/DALI_VCPKG_TOOLS/vcpkg/installed/x64-windows/tools/gettext"
        "C:/Tools/DALI_VCPKG_TOOLS/vcpkg/installed/x64-windows/tools/gettext/bin")
    ENDIF()
    FIND_PROGRAM(MSGFMT_EXECUTABLE NAMES msgfmt msgfmt.exe HINTS ${MSGFMT_HINTS})
  ENDIF()

  # MSYS2-based msgfmt can fail when multiple instances start simultaneously.
  # Serialize msgfmt while allowing the rest of the Ninja build to parallelize.
  SET(MSGFMT_JOB_POOL_ARGUMENT)
  IF(MSGFMT_EXECUTABLE)
    IF(CMAKE_GENERATOR MATCHES "Ninja" AND CMAKE_VERSION VERSION_GREATER_EQUAL "3.15")
      SET_PROPERTY(GLOBAL APPEND PROPERTY JOB_POOLS dali_msgfmt_pool=1)
      SET(MSGFMT_JOB_POOL_ARGUMENT JOB_POOL dali_msgfmt_pool)
    ENDIF()
  ENDIF()
ENDMACRO()
