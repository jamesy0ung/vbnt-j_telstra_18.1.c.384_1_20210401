#
# - Find libxtables
# Find the native LIBXTABLES includes and library
#
#  LIBXTABLES_INCLUDE_DIRS - where to find libiptc.h, etc.
#  LIBXTABLES_LIBRARIES    - List of libraries when using libxtables.
#  LIBXTABLES_FOUND        - True if libxtables found.


IF (LIBXTABLES_INCLUDE_DIRS)
  # Already in cache, be silent
  SET(LIBXTABLES_FIND_QUIETLY TRUE)
ENDIF (LIBXTABLES_INCLUDE_DIRS)

FIND_PATH(LIBXTABLES_INCLUDE_DIR xtables.h)
SET(LIBXTABLES_NAMES libxtables.so)
FIND_LIBRARY(LIBXTABLES_LIBRARY NAMES ${LIBXTABLES_NAMES})

# handle the QUIETLY and REQUIRED arguments and set LIBXTABLES_FOUND to TRUE if
# all listed variables are TRUE
INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(LIBXTABLES DEFAULT_MSG LIBXTABLES_LIBRARY LIBXTABLES_INCLUDE_DIR)

IF(LIBXTABLES_FOUND)
  SET( LIBXTABLES_LIBRARIES ${LIBXTABLES_LIBRARY} )
  SET( LIBXTABLES_INCLUDE_DIRS ${LIBXTABLES_INCLUDE_DIR} )
ELSE(LIBXTABLES_FOUND)
  SET( LIBXTABLES_LIBRARIES )
  SET( LIBXTABLES_INCLUDE_DIRS )
ENDIF(LIBXTABLES_FOUND)

MARK_AS_ADVANCED( LIBXTABLES_LIBRARIES LIBXTABLES_INCLUDE_DIRS )
